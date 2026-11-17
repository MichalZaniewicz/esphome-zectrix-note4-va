// ESPHome display platform for the Zectrix Note 4 e-paper panel (SSD2683, 400x300).
//
// The Ssd2683Driver below is a port of the Zectrix ESP-IDF component "zectrix_epd"
// (zectrix_epd.cc), Copyright (c) 2026 Zectrix Lab, MIT License (see LICENSE-zectrix-epd).
// Command sequences, waveform loading, BUSY handling and timing are kept as in the
// reference; only the C API wrapper and its mutex were replaced, because the driver is
// owned by a single refresh task here.

#ifdef USE_ESP32

#include "zectrix_epd.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <new>
#include <utility>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include "ssd2683_waveform.h"

namespace esphome::zectrix_epd {

static const char *const TAG = "zectrix_epd";

// ===========================================================================
// Reference driver port
// ===========================================================================

namespace {

constexpr int kWidth = PANEL_WIDTH;
constexpr int kHeight = PANEL_HEIGHT;
constexpr int kBwStride = kWidth / 8;
constexpr int kNativeStride = kWidth / 4;
constexpr size_t kMaxTransferSize = 1024;
constexpr size_t k1bppFrameBytes = 15000;  // ZECTRIX_EPD_1BPP_FRAME_BYTES
constexpr size_t k4bppFrameBytes = 60000;  // ZECTRIX_EPD_4BPP_FRAME_BYTES



uint8_t ReadPackedBit(const uint8_t *data, size_t stride, int x, int y) {
  return static_cast<uint8_t>((data[static_cast<size_t>(y) * stride + x / 8] >> (7 - (x & 7))) & 1U);
}

void WritePackedBit(uint8_t *data, size_t stride, int x, int y, uint8_t bit) {
  uint8_t &value = data[static_cast<size_t>(y) * stride + x / 8];
  const uint8_t mask = static_cast<uint8_t>(1U << (7 - (x & 7)));
  if (bit != 0) {
    value |= mask;
  } else {
    value &= static_cast<uint8_t>(~mask);
  }
}

}  // namespace

struct DriverConfig {
  spi_host_device_t spi_host;
  gpio_num_t pin_cs;
  gpio_num_t pin_dc;
  gpio_num_t pin_reset;
  gpio_num_t pin_busy;
  gpio_num_t pin_mosi;
  gpio_num_t pin_sclk;
  gpio_num_t pin_power;
  int spi_clock_hz;
  uint32_t busy_timeout_ms;
  int64_t external_refresh_timeout_us;  // reference: kExternalRefreshTimeoutUs (5 s)
  bool initialize_spi_bus;
};

struct Ssd2683Driver {
  explicit Ssd2683Driver(const DriverConfig &value) : config(value) {}

  DriverConfig config = {};
  spi_device_handle_t spi = nullptr;
  uint8_t *shadow = nullptr;
  uint8_t *dma_buffer = nullptr;
  bool owns_bus = false;
  bool bus_initialized = false;
  bool receive_mode = false;
  bool powered = false;
  bool controller_ready = false;
  bool internal_power_on = false;
  bool shadow_valid = false;

  void SetCs(int level) { gpio_set_level(config.pin_cs, level); }
  void SetDc(int level) { gpio_set_level(config.pin_dc, level); }
  void SetReset(int level) { gpio_set_level(config.pin_reset, level); }

  esp_err_t ConfigureSpi(bool receive) {
    if (spi != nullptr) {
      esp_err_t err = spi_bus_remove_device(spi);
      if (err != ESP_OK) {
        return err;
      }
      spi = nullptr;
    }
    if (bus_initialized && owns_bus) {
      esp_err_t err = spi_bus_free(config.spi_host);
      if (err != ESP_OK) {
        return err;
      }
      bus_initialized = false;
    }

    if (!config.initialize_spi_bus && receive) {
      return ESP_ERR_NOT_SUPPORTED;
    }

    if (config.initialize_spi_bus) {
      spi_bus_config_t bus_config = {};
      bus_config.miso_io_num = receive ? config.pin_mosi : -1;
      bus_config.mosi_io_num = receive ? -1 : config.pin_mosi;
      bus_config.sclk_io_num = config.pin_sclk;
      bus_config.quadwp_io_num = -1;
      bus_config.quadhd_io_num = -1;
      bus_config.max_transfer_sz = kMaxTransferSize;
      esp_err_t err = spi_bus_initialize(config.spi_host, &bus_config, SPI_DMA_CH_AUTO);
      if (err != ESP_OK) {
        return err;
      }
      owns_bus = true;
      bus_initialized = true;
    }

    spi_device_interface_config_t device_config = {};
    device_config.spics_io_num = -1;
    device_config.clock_speed_hz = receive ? std::min(config.spi_clock_hz, 8000000) : config.spi_clock_hz;
    device_config.mode = 0;
    device_config.queue_size = 1;
    esp_err_t err = spi_bus_add_device(config.spi_host, &device_config, &spi);
    if (err != ESP_OK) {
      if (owns_bus && bus_initialized) {
        spi_bus_free(config.spi_host);
        bus_initialized = false;
      }
      return err;
    }
    receive_mode = receive;
    return ESP_OK;
  }

  esp_err_t Transmit(const uint8_t *data, size_t size) {
    if (spi == nullptr || data == nullptr || size == 0) {
      return ESP_ERR_INVALID_STATE;
    }
    size_t offset = 0;
    while (offset < size) {
      const size_t chunk = std::min(kMaxTransferSize, size - offset);
      spi_transaction_t transaction = {};
      transaction.length = chunk * 8;
      if (chunk <= sizeof(transaction.tx_data)) {
        transaction.flags = SPI_TRANS_USE_TXDATA;
        std::memcpy(transaction.tx_data, data + offset, chunk);
      } else {
        if (dma_buffer == nullptr) {
          return ESP_ERR_INVALID_STATE;
        }
        std::memcpy(dma_buffer, data + offset, chunk);
        transaction.tx_buffer = dma_buffer;
      }
      const esp_err_t err = spi_device_polling_transmit(spi, &transaction);
      if (err != ESP_OK) {
        return err;
      }
      offset += chunk;
    }
    return ESP_OK;
  }

  esp_err_t SendCommand(uint8_t command) {
    SetDc(0);
    SetCs(0);
    const esp_err_t err = Transmit(&command, 1);
    SetCs(1);
    return err;
  }

  esp_err_t SendData(uint8_t data) {
    SetDc(1);
    SetCs(0);
    const esp_err_t err = Transmit(&data, 1);
    SetCs(1);
    return err;
  }

  esp_err_t SendBytes(const uint8_t *data, size_t size) {
    SetDc(1);
    SetCs(0);
    const esp_err_t err = Transmit(data, size);
    SetCs(1);
    return err;
  }

  esp_err_t ReadData(uint8_t *value) {
    if (value == nullptr || !owns_bus) {
      return ESP_ERR_NOT_SUPPORTED;
    }
    esp_err_t err = ConfigureSpi(true);
    if (err != ESP_OK) {
      return err;
    }
    SetDc(1);
    SetCs(0);
    spi_transaction_t transaction = {};
    transaction.length = 8;
    transaction.flags = SPI_TRANS_USE_RXDATA;
    err = spi_device_polling_transmit(spi, &transaction);
    if (err == ESP_OK) {
      *value = transaction.rx_data[0];
    }
    SetCs(1);
    const esp_err_t restore_err = ConfigureSpi(false);
    return err != ESP_OK ? err : restore_err;
  }

  esp_err_t WaitBusy(const char *operation, int64_t timeout_us = -1) {
    if (timeout_us < 0) {
      timeout_us = static_cast<int64_t>(config.busy_timeout_ms) * 1000;
    }
    const int64_t start = esp_timer_get_time();
    while (gpio_get_level(config.pin_busy) == 0) {
      if (esp_timer_get_time() - start >= timeout_us) {
        ESP_LOGE(TAG, "BUSY timeout during %s", operation);
        return ESP_ERR_TIMEOUT;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    return ESP_OK;
  }

  esp_err_t HardwareReset() {
    SetReset(1);
    vTaskDelay(pdMS_TO_TICKS(10));
    SetReset(0);
    vTaskDelay(pdMS_TO_TICKS(20));
    SetReset(1);
    vTaskDelay(pdMS_TO_TICKS(10));
    return WaitBusy("reset");
  }

  esp_err_t SelectOtp() {
    esp_err_t err = SendCommand(0x00);
    if (err == ESP_OK)
      err = SendData(0x2F);
    if (err == ESP_OK)
      err = SendData(0x0E);
    if (err == ESP_OK)
      err = SendCommand(0xE9);
    if (err == ESP_OK)
      err = SendData(0x01);
    if (err == ESP_OK)
      err = WaitBusy("OTP init");
    controller_ready = err == ESP_OK;
    return err;
  }

  esp_err_t PrepareOtpRefresh() {
    if (!powered) {
      return ESP_ERR_INVALID_STATE;
    }
    // SSD2683 does not reliably accept a second refresh after its internal
    // power-off command without a fresh reset/OTP selection. The physical
    // image is retained, so this is also safe before a transition-based
    // partial refresh.
    internal_power_on = false;
    controller_ready = false;
    esp_err_t err = HardwareReset();
    if (err == ESP_OK) {
      err = SelectOtp();
    }
    return err;
  }

  esp_err_t PowerOnLocked() {
    if (powered && controller_ready) {
      return ESP_OK;
    }
    gpio_hold_dis(config.pin_power);
    esp_err_t err = gpio_set_level(config.pin_power, 1);
    gpio_hold_en(config.pin_power);
    if (err != ESP_OK) {
      return err;
    }
    powered = true;
    vTaskDelay(pdMS_TO_TICKS(10));
    err = HardwareReset();
    if (err == ESP_OK) {
      err = SelectOtp();
    }
    if (err != ESP_OK) {
      PowerOffLocked();
    }
    return err;
  }

  esp_err_t PowerOffLocked() {
    esp_err_t result = ESP_OK;
    if (powered && internal_power_on && spi != nullptr) {
      result = SendCommand(0x02);
      if (result == ESP_OK)
        result = SendData(0x00);
      if (result == ESP_OK)
        result = WaitBusy("power off");
    }
    gpio_hold_dis(config.pin_power);
    const esp_err_t gpio_err = gpio_set_level(config.pin_power, 0);
    gpio_hold_en(config.pin_power);
    if (result == ESP_OK) {
      result = gpio_err;
    }
    powered = false;
    controller_ready = false;
    internal_power_on = false;
    return result;
  }

  esp_err_t SetTemperatureForOtp() {
    esp_err_t err = SendCommand(0x40);
    if (err == ESP_OK)
      err = WaitBusy("temperature read");
    uint8_t temperature = 25;
    if (err == ESP_OK && owns_bus) {
      err = ReadData(&temperature);
    }
    if (err == ESP_ERR_NOT_SUPPORTED) {
      temperature = 25;
      err = ESP_OK;
    }
    uint8_t encoded = 244;
    if (temperature <= 5)
      encoded = 232;
    else if (temperature <= 10)
      encoded = 235;
    else if (temperature <= 20)
      encoded = 238;
    else if (temperature <= 30)
      encoded = 241;
    else if (temperature <= 127)
      encoded = 244;
    if (err == ESP_OK)
      err = SendCommand(0xE0);
    if (err == ESP_OK)
      err = SendData(0x02);
    if (err == ESP_OK)
      err = SendCommand(0xE6);
    if (err == ESP_OK)
      err = SendData(encoded);
    if (err == ESP_OK)
      err = SendCommand(0xA5);
    if (err == ESP_OK)
      err = WaitBusy("temperature activate");
    if (err == ESP_OK)
      vTaskDelay(pdMS_TO_TICKS(10));
    return err;
  }

  esp_err_t TriggerOtpRefresh() {
    esp_err_t err = SendCommand(0x04);
    if (err == ESP_OK)
      err = WaitBusy("internal power on");
    if (err == ESP_OK)
      internal_power_on = true;
    if (err != ESP_OK) {
      controller_ready = false;
      internal_power_on = false;
      return err;
    }
    if (err == ESP_OK)
      err = SendCommand(0x12);
    if (err == ESP_OK)
      err = SendData(0x00);
    if (err == ESP_OK)
      err = WaitBusy("display refresh");
    if (err != ESP_OK) {
      // Do not issue 0x02 while BUSY is still asserted. Mark the
      // controller unusable so power_off() cuts the external rail.
      controller_ready = false;
      internal_power_on = false;
      return err;
    }
    err = SendCommand(0x02);
    if (err == ESP_OK)
      err = SendData(0x00);
    if (err == ESP_OK)
      err = WaitBusy("internal power off");
    internal_power_on = false;
    if (err != ESP_OK) {
      controller_ready = false;
    }
    return err;
  }

  esp_err_t WriteOtpFrame(const uint8_t *framebuffer) {
    esp_err_t err = SetTemperatureForOtp();
    if (err == ESP_OK)
      err = SendCommand(0x10);
    std::array<uint8_t, kNativeStride> line = {};
    for (int y = 0; err == ESP_OK && y < kHeight; ++y) {
      for (int source_byte = 0; source_byte < kBwStride; ++source_byte) {
        const uint8_t input = framebuffer[static_cast<size_t>(y) * kBwStride + source_byte];
        uint8_t first = 0;
        uint8_t second = 0;
        for (int bit = 0; bit < 8; ++bit) {
          const uint8_t pixel = static_cast<uint8_t>((input >> (7 - bit)) & 1U);
          if (bit < 4) {
            first |= static_cast<uint8_t>(pixel << (6 - bit * 2));
          } else {
            second |= static_cast<uint8_t>(pixel << (14 - bit * 2));
          }
        }
        line[static_cast<size_t>(source_byte) * 2] = first;
        line[static_cast<size_t>(source_byte) * 2 + 1] = second;
      }
      err = SendBytes(line.data(), line.size());
    }
    if (err == ESP_OK)
      err = TriggerOtpRefresh();
    return err;
  }

  esp_err_t DisplayOtpWhiteBase() {
    std::array<uint8_t, kNativeStride> white_line = {};
    white_line.fill(0x55);
    esp_err_t err = SetTemperatureForOtp();
    if (err == ESP_OK)
      err = SendCommand(0x10);
    for (int y = 0; err == ESP_OK && y < kHeight; ++y) {
      err = SendBytes(white_line.data(), white_line.size());
    }
    if (err == ESP_OK)
      err = TriggerOtpRefresh();
    return err;
  }

  esp_err_t SetPartialWindow(int x0, int y0, int x1, int y1) {
    esp_err_t err = SendCommand(0x83);
    const uint8_t values[] = {static_cast<uint8_t>((x0 >> 8) & 0x03), static_cast<uint8_t>(x0),
                              static_cast<uint8_t>((x1 >> 8) & 0x03), static_cast<uint8_t>(x1),
                              static_cast<uint8_t>((y0 >> 8) & 0x03), static_cast<uint8_t>(y0),
                              static_cast<uint8_t>((y1 >> 8) & 0x03), static_cast<uint8_t>(y1),
                              0x01};
    for (uint8_t value : values) {
      if (err == ESP_OK)
        err = SendData(value);
    }
    return err;
  }

  esp_err_t InitExternalWaveform(const uint8_t *waveform, size_t size) {
    if (waveform == nullptr || size != ssd2683_waveform::kWaveformSize) {
      return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = HardwareReset();
    if (err == ESP_OK)
      err = SendCommand(0x00);
    if (err == ESP_OK)
      err = SendData(0x2F);
    if (err == ESP_OK)
      err = SendData(0x8E);
    if (err == ESP_OK)
      err = SendCommand(0x01);
    const uint8_t power_values[] = {0x07, waveform[0], waveform[1], waveform[2], waveform[3], waveform[4]};
    for (uint8_t value : power_values) {
      if (err == ESP_OK)
        err = SendData(value);
    }
    if (err == ESP_OK)
      err = SendCommand(0x30);
    if (err == ESP_OK)
      err = SendData(waveform[6]);
    if (err == ESP_OK)
      err = SendCommand(0x82);
    if (err == ESP_OK)
      err = SendData(waveform[5]);
    if (err == ESP_OK)
      err = SendCommand(0x06);
    const uint8_t booster[] = {0x0F, 0x8B, 0x9C, 0xAA};
    for (uint8_t value : booster) {
      if (err == ESP_OK)
        err = SendData(value);
    }
    if (err == ESP_OK)
      err = SendCommand(0xE7);
    if (err == ESP_OK)
      err = SendData(0x98);
    if (err == ESP_OK)
      err = SendCommand(0x50);
    if (err == ESP_OK)
      err = SendData(0x37);
    if (err == ESP_OK)
      err = SendCommand(0x61);
    const uint8_t resolution[] = {0x01, 0x90, 0x01, 0x2C};
    for (uint8_t value : resolution) {
      if (err == ESP_OK)
        err = SendData(value);
    }
    if (err == ESP_OK)
      err = SendCommand(0x62);
    if (err == ESP_OK)
      err = SendData(0x64);
    if (err == ESP_OK)
      err = SendData(0x53);
    if (err == ESP_OK)
      err = SendCommand(0x65);
    for (int i = 0; err == ESP_OK && i < 4; ++i)
      err = SendData(0x00);
    if (err == ESP_OK)
      err = SendCommand(0xE9);
    if (err == ESP_OK)
      err = SendData(0x01);
    if (err == ESP_OK)
      err = WaitBusy("external waveform init");
    if (err == ESP_OK)
      err = SendCommand(0x20);
    if (err == ESP_OK)
      err = SendBytes(waveform, size);
    controller_ready = err == ESP_OK;
    return err;
  }

  esp_err_t LoadExternalWaveform(const uint8_t *waveform, size_t size) {
    if (waveform == nullptr || size != ssd2683_waveform::kWaveformSize) {
      return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = WaitBusy("external waveform reload");
    if (err == ESP_OK)
      err = SendCommand(0x30);
    if (err == ESP_OK)
      err = SendData(waveform[6]);
    if (err == ESP_OK)
      err = SendCommand(0x20);
    if (err == ESP_OK)
      err = SendBytes(waveform, size);
    return err;
  }

  esp_err_t WriteGrayPass(int pass, const uint8_t *framebuffer) {
    esp_err_t err = SendCommand(0x10);
    if (err == ESP_OK)
      err = WaitBusy("4bpp RAM write");
    std::array<uint8_t, kNativeStride> line = {};
    for (int y = 0; err == ESP_OK && y < kHeight; ++y) {
      for (int byte_x = 0; byte_x < kNativeStride; ++byte_x) {
        uint8_t packed = 0;
        for (int index = 0; index < 4; ++index) {
          const int x = byte_x * 4 + index;
          const size_t pixel = static_cast<size_t>(y) * kWidth + x;
          const uint8_t pair = framebuffer[pixel / 2];
          const uint8_t level = (pixel & 1U) ? pair & 0x0F : pair >> 4;
          const uint8_t code = ssd2683_waveform::VendorGray16RenderPassOfLevel(level) == static_cast<size_t>(pass)
                                   ? ssd2683_waveform::VendorGray16RenderCodeOfLevel(level)
                                   : 0;
          packed |= static_cast<uint8_t>(code << (6 - index * 2));
        }
        line[byte_x] = packed;
      }
      err = SendBytes(line.data(), line.size());
    }
    return err;
  }

  esp_err_t TriggerExternalBatch(bool power_on) {
    esp_err_t err = ESP_OK;
    if (power_on) {
      err = SendCommand(0x04);
      if (err == ESP_OK)
        err = WaitBusy("external power on");
      if (err == ESP_OK)
        internal_power_on = true;
      if (err == ESP_OK)
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (err == ESP_OK)
      err = SendCommand(0x12);
    if (err == ESP_OK)
      err = SendData(0x00);
    if (err == ESP_OK)
      vTaskDelay(pdMS_TO_TICKS(10));
    if (err == ESP_OK) {
      err = WaitBusy("external display refresh", config.external_refresh_timeout_us);
    }
    return err;
  }

  esp_err_t RestoreOtp() {
    internal_power_on = false;
    esp_err_t err = HardwareReset();
    if (err == ESP_OK)
      err = SelectOtp();
    return err;
  }

  // ---- Former C API (zectrix_epd_new / _refresh_*), minus the mutex ----

  esp_err_t Init() {
    const DriverConfig &c = config;
    if (c.spi_clock_hz <= 0 || c.busy_timeout_ms == 0 || !GPIO_IS_VALID_OUTPUT_GPIO(c.pin_cs) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(c.pin_dc) || !GPIO_IS_VALID_OUTPUT_GPIO(c.pin_reset) ||
        !GPIO_IS_VALID_GPIO(c.pin_busy) || !GPIO_IS_VALID_OUTPUT_GPIO(c.pin_mosi) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(c.pin_sclk) || !GPIO_IS_VALID_OUTPUT_GPIO(c.pin_power)) {
      return ESP_ERR_INVALID_ARG;
    }
    // Reference: shadow with MALLOC_CAP_8BIT (PSRAM is fine), DMA bounce buffer internal.
    shadow = static_cast<uint8_t *>(heap_caps_malloc(k1bppFrameBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (shadow == nullptr)
      shadow = static_cast<uint8_t *>(heap_caps_malloc(k1bppFrameBytes, MALLOC_CAP_8BIT));
    dma_buffer = static_cast<uint8_t *>(heap_caps_malloc(kMaxTransferSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (shadow == nullptr || dma_buffer == nullptr) {
      return ESP_ERR_NO_MEM;
    }
    std::memset(shadow, 0xFF, k1bppFrameBytes);

    gpio_config_t output = {};
    output.pin_bit_mask =
        (1ULL << c.pin_cs) | (1ULL << c.pin_dc) | (1ULL << c.pin_reset) | (1ULL << c.pin_power);
    output.mode = GPIO_MODE_OUTPUT;
    output.pull_up_en = GPIO_PULLUP_ENABLE;
    esp_err_t err = gpio_config(&output);
    gpio_config_t input = {};
    input.pin_bit_mask = 1ULL << c.pin_busy;
    input.mode = GPIO_MODE_INPUT;
    input.pull_up_en = GPIO_PULLUP_ENABLE;
    if (err == ESP_OK)
      err = gpio_config(&input);
    if (err == ESP_OK) {
      SetCs(1);
      SetDc(1);
      SetReset(1);
      gpio_hold_dis(c.pin_power);
      err = gpio_set_level(c.pin_power, 0);
      gpio_hold_en(c.pin_power);
    }
    if (err == ESP_OK)
      err = ConfigureSpi(false);
    return err;
  }

  esp_err_t RefreshFull1bpp(const uint8_t *framebuffer) {
    if (!powered || !controller_ready)
      return ESP_ERR_INVALID_STATE;
    esp_err_t err = PrepareOtpRefresh();
    if (err == ESP_OK) {
      err = WriteOtpFrame(framebuffer);
    }
    if (err == ESP_OK) {
      std::memcpy(shadow, framebuffer, k1bppFrameBytes);
      shadow_valid = true;
    } else {
      shadow_valid = false;
    }
    return err;
  }

  // rect: x, y, width, height in native coordinates; pixels tightly packed,
  // ceil(width / 8) bytes per row, MSB first, 1 = white.
  esp_err_t RefreshPartial1bpp(int rx, int ry, int rw, int rh, const uint8_t *pixels) {
    if (pixels == nullptr || rx < 0 || ry < 0 || rw <= 0 || rh <= 0 || rx + rw > kWidth || ry + rh > kHeight) {
      return ESP_ERR_INVALID_ARG;
    }
    const size_t source_stride = static_cast<size_t>((rw + 7) / 8);
    if (!powered || !controller_ready || !shadow_valid) {
      return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = PrepareOtpRefresh();
    if (err != ESP_OK) {
      shadow_valid = false;
      return err;
    }

    const int x0 = rx & ~7;
    const int aligned_x_end = std::min(kWidth, (rx + rw + 7) & ~7);
    const int x1 = aligned_x_end - 1;
    const int y0 = ry;
    const int y1 = ry + rh - 1;
    const int output_stride = ((x1 - x0 + 1) / 8) * 2;
    std::array<uint8_t, kNativeStride> line = {};

    err = SendCommand(0x50);
    if (err == ESP_OK)
      err = SendData(0x77);
    if (err == ESP_OK)
      err = SendCommand(0xE0);
    if (err == ESP_OK)
      err = SendData(0x00);
    if (err == ESP_OK)
      err = SendCommand(0xA5);
    if (err == ESP_OK)
      err = WaitBusy("partial temperature");
    if (err == ESP_OK)
      vTaskDelay(pdMS_TO_TICKS(10));
    if (err == ESP_OK)
      err = SetPartialWindow(x0, y0, x1, y1);
    if (err == ESP_OK)
      err = SendCommand(0x10);
    if (err == ESP_OK)
      err = WaitBusy("partial RAM write");

    for (int y = y0; err == ESP_OK && y <= y1; ++y) {
      std::fill_n(line.begin(), output_stride, 0);
      for (int x = x0; x <= x1; ++x) {
        const uint8_t old_pixel = ReadPackedBit(shadow, kBwStride, x, y);
        uint8_t new_pixel = old_pixel;
        if (x >= rx && x < rx + rw) {
          new_pixel = ReadPackedBit(pixels, source_stride, x - rx, y - ry);
        }
        const uint8_t transition = static_cast<uint8_t>((old_pixel << 1) | new_pixel);
        const int relative_x = x - x0;
        line[static_cast<size_t>(relative_x / 4)] |= static_cast<uint8_t>(transition << (6 - (relative_x & 3) * 2));
      }
      err = SendBytes(line.data(), static_cast<size_t>(output_stride));
    }
    if (err == ESP_OK)
      err = TriggerOtpRefresh();
    if (err == ESP_OK) {
      for (int y = 0; y < rh; ++y) {
        for (int x = 0; x < rw; ++x) {
          WritePackedBit(shadow, kBwStride, rx + x, ry + y, ReadPackedBit(pixels, source_stride, x, y));
        }
      }
    } else {
      // A timeout can leave part of the physical window updated. Refuse any
      // further transition refresh until a full B/W frame re-establishes a
      // trustworthy old-image baseline.
      shadow_valid = false;
    }
    return err;
  }

  esp_err_t RefreshFull4bpp(const uint8_t *framebuffer) {
    if (!powered || !controller_ready)
      return ESP_ERR_INVALID_STATE;

    esp_err_t err = PrepareOtpRefresh();
    if (err == ESP_OK) {
      err = DisplayOtpWhiteBase();
    }
    bool session_open = false;
    for (size_t pass = 0; err == ESP_OK && pass < ssd2683_waveform::kVendorGray16RenderPassCount; ++pass) {
      const auto &waveform = ssd2683_waveform::kVendorGray16RenderWaveforms[pass];
      if (!session_open) {
        err = InitExternalWaveform(waveform.data(), waveform.size());
        session_open = err == ESP_OK;
      } else {
        err = LoadExternalWaveform(waveform.data(), waveform.size());
      }
      if (err == ESP_OK)
        err = WriteGrayPass(static_cast<int>(pass), framebuffer);
      if (err == ESP_OK)
        err = TriggerExternalBatch(pass == 0);
    }
    if (session_open && internal_power_on) {
      esp_err_t off_err = SendCommand(0x02);
      if (off_err == ESP_OK)
        off_err = SendData(0x00);
      if (off_err == ESP_OK)
        off_err = WaitBusy("4bpp internal power off");
      internal_power_on = false;
      if (err == ESP_OK)
        err = off_err;
    }
    const esp_err_t restore_err = RestoreOtp();
    if (err == ESP_OK)
      err = restore_err;
    shadow_valid = false;
    return err;
  }
};

// ===========================================================================
// ESPHome component
// ===========================================================================

namespace {

// 8x8 ordered-dither (Bayer) thresholds, 0..63.
constexpr uint8_t BAYER8[8][8] = {
    {0, 32, 8, 40, 2, 34, 10, 42},  {48, 16, 56, 24, 50, 18, 58, 26}, {12, 44, 4, 36, 14, 46, 6, 38},
    {60, 28, 52, 20, 62, 30, 54, 22}, {3, 35, 11, 43, 1, 33, 9, 41},  {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37},  {63, 31, 55, 23, 61, 29, 53, 21},
};

constexpr uint8_t TONE_FLAG = 0x80;
constexpr uint8_t GREY_MASK = 0x0F;
constexpr uint8_t WHITE_INK = 0x0F;  // grey 15, no tone flag

const char *mode_name(RefreshMode m) {
  switch (m) {
    case RefreshMode::FULL_MONO:
      return "FULL_MONO";
    case RefreshMode::FULL_GRAY:
      return "FULL_GRAY";
    case RefreshMode::PARTIAL:
      return "PARTIAL";
  }
  return "?";
}

}  // namespace

void ZectrixEPD::setup() {
  RAMAllocator<uint8_t> psram(RAMAllocator<uint8_t>::ALLOC_EXTERNAL);
  this->buffer_ = psram.allocate(PANEL_WIDTH * PANEL_HEIGHT);
  this->tx_pending_ = psram.allocate(k4bppFrameBytes);
  this->tx_active_ = psram.allocate(k4bppFrameBytes);
  this->patch_ = psram.allocate(k1bppFrameBytes);
  if (this->buffer_ == nullptr || this->tx_pending_ == nullptr || this->tx_active_ == nullptr ||
      this->patch_ == nullptr) {
    ESP_LOGE(TAG, "Could not allocate frame buffers in PSRAM");
    this->mark_failed();
    return;
  }
  this->clear();

  DriverConfig cfg = {};
  cfg.spi_host = this->spi_host_ == 2 ? SPI2_HOST : SPI3_HOST;
  cfg.pin_cs = static_cast<gpio_num_t>(this->pin_cs_);
  cfg.pin_dc = static_cast<gpio_num_t>(this->pin_dc_);
  cfg.pin_reset = static_cast<gpio_num_t>(this->pin_reset_);
  cfg.pin_busy = static_cast<gpio_num_t>(this->pin_busy_);
  cfg.pin_mosi = static_cast<gpio_num_t>(this->pin_mosi_);
  cfg.pin_sclk = static_cast<gpio_num_t>(this->pin_sclk_);
  cfg.pin_power = static_cast<gpio_num_t>(this->pin_power_);
  cfg.spi_clock_hz = static_cast<int>(this->spi_frequency_);
  cfg.busy_timeout_ms = this->busy_timeout_ms_;
  cfg.external_refresh_timeout_us = static_cast<int64_t>(this->gray_busy_timeout_ms_) * 1000;
  cfg.initialize_spi_bus = true;

  this->drv_ = new (std::nothrow) Ssd2683Driver(cfg);  // NOLINT(cppcoreguidelines-owning-memory)
  if (this->drv_ == nullptr) {
    ESP_LOGE(TAG, "Could not allocate driver");
    this->mark_failed();
    return;
  }
  esp_err_t err = this->drv_->Init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Driver init failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  this->lock_ = xSemaphoreCreateMutex();
  if (this->lock_ == nullptr) {
    this->mark_failed();
    return;
  }
#if CONFIG_FREERTOS_UNICORE
  const BaseType_t core = 0;
#else
  const BaseType_t core = 1;
#endif
  if (xTaskCreatePinnedToCore(ZectrixEPD::task_entry_, "zectrix_epd", 6144, this, 1, &this->task_, core) != pdPASS) {
    ESP_LOGE(TAG, "Could not create refresh task");
    this->mark_failed();
    return;
  }
}

void ZectrixEPD::dump_config() {
  LOG_DISPLAY("", "Zectrix EPD (SSD2683)", this);
  ESP_LOGCONFIG(TAG,
                "  CS: GPIO%d, DC: GPIO%d, RESET: GPIO%d, BUSY: GPIO%d, POWER: GPIO%d\n"
                "  SCK: GPIO%d, MOSI: GPIO%d, SPI host: SPI%d @ %u Hz\n"
                "  BUSY timeout: %u ms, gray BUSY timeout: %u ms\n"
                "  Full refresh every: %u partials, power off between refreshes: %s",
                this->pin_cs_, this->pin_dc_, this->pin_reset_, this->pin_busy_, this->pin_power_, this->pin_sclk_,
                this->pin_mosi_, this->spi_host_, (unsigned) this->spi_frequency_, (unsigned) this->busy_timeout_ms_,
                (unsigned) this->gray_busy_timeout_ms_, (unsigned) this->full_refresh_every_,
                YESNO(this->power_off_between_refreshes_));
  LOG_UPDATE_INTERVAL(this);
}

void ZectrixEPD::clear() {
  if (this->buffer_ != nullptr)
    std::memset(this->buffer_, WHITE_INK, PANEL_WIDTH * PANEL_HEIGHT);
}

void HOT ZectrixEPD::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || y < 0 || x >= PANEL_WIDTH || y >= PANEL_HEIGHT || this->buffer_ == nullptr)
    return;
  // Rec. 601 luma, 0..255 (77 + 150 + 29 = 256)
  const uint32_t lum = (77u * color.r + 150u * color.g + 29u * color.b) >> 8;
  const uint8_t grey = static_cast<uint8_t>((lum * 15u + 127u) / 255u);
  this->buffer_[y * PANEL_WIDTH + x] = static_cast<uint8_t>(grey | (color.w >= 128 ? TONE_FLAG : 0));
}

void ZectrixEPD::add_partial_rect(int x, int y, int w, int h) { this->partial_rects_.push_back({x, y, w, h}); }

void ZectrixEPD::set_partial_rects(const std::vector<PartialRect> &rects) { this->partial_rects_ = rects; }

bool ZectrixEPD::is_busy() const {
  if (this->lock_ == nullptr)
    return false;
  xSemaphoreTake(this->lock_, portMAX_DELAY);
  const bool busy = this->running_ || this->pending_valid_;
  xSemaphoreGive(this->lock_);
  return busy;
}

// Map a rect in display (rotated) coordinates to native panel coordinates and clamp.
bool ZectrixEPD::map_rect_to_native_(const PartialRect &in, PartialRect &out) const {
  const int x = in.x, y = in.y, w = in.w, h = in.h;
  if (w <= 0 || h <= 0)
    return false;
  switch (this->rotation_) {
    case display::DISPLAY_ROTATION_90_DEGREES:  // native = (W-1-uy, ux)
      out = {PANEL_WIDTH - y - h, x, h, w};
      break;
    case display::DISPLAY_ROTATION_180_DEGREES:  // native = (W-1-ux, H-1-uy)
      out = {PANEL_WIDTH - x - w, PANEL_HEIGHT - y - h, w, h};
      break;
    case display::DISPLAY_ROTATION_270_DEGREES:  // native = (uy, H-1-ux)
      out = {y, PANEL_HEIGHT - x - w, h, w};
      break;
    case display::DISPLAY_ROTATION_0_DEGREES:
    default:
      out = {x, y, w, h};
      break;
  }
  int x0 = std::max(0, out.x), y0 = std::max(0, out.y);
  int x1 = std::min(PANEL_WIDTH, out.x + out.w), y1 = std::min(PANEL_HEIGHT, out.y + out.h);
  if (x1 <= x0 || y1 <= y0)
    return false;
  out = {x0, y0, x1 - x0, y1 - y0};
  return true;
}

// 1 byte/pixel draw buffer -> panel 1bpp (MSB first, 1 = white).
// Tone pixels: 8x8 Bayer dither, grey 0..15 scaled to 0..64. Ink pixels: threshold at 8.
void ZectrixEPD::convert_1bpp_(uint8_t *out) const {
  const uint8_t *src = this->buffer_;
  for (int y = 0; y < PANEL_HEIGHT; y++) {
    const uint8_t *row = BAYER8[y & 7];
    for (int bx = 0; bx < PANEL_WIDTH / 8; bx++) {
      uint8_t byte = 0;
      for (int i = 0; i < 8; i++) {
        const int x = bx * 8 + i;
        const uint8_t p = *src++;
        const uint32_t grey = p & GREY_MASK;
        bool white;
        if (p & TONE_FLAG) {
          // grey*64/15 > bayer + 0.5  <=>  128*grey > 30*bayer + 15
          white = 128u * grey > 30u * row[x & 7] + 15u;
        } else {
          white = grey >= 8;
        }
        if (white)
          byte |= static_cast<uint8_t>(0x80 >> i);
      }
      out[y * (PANEL_WIDTH / 8) + bx] = byte;
    }
  }
}

// 1 byte/pixel draw buffer -> panel 4bpp (left pixel high nibble, 0 black, 15 white).
void ZectrixEPD::convert_4bpp_(uint8_t *out) const {
  const uint8_t *src = this->buffer_;
  for (size_t i = 0; i < k4bppFrameBytes; i++) {
    out[i] = static_cast<uint8_t>(((src[0] & GREY_MASK) << 4) | (src[1] & GREY_MASK));
    src += 2;
  }
}

void ZectrixEPD::update() {
  if (this->is_failed() || this->buffer_ == nullptr || this->lock_ == nullptr)
    return;

  this->do_update_();  // lambda / pages -> draw buffer (may call set_refresh_mode / add_partial_rect)

  RefreshMode mode = this->refresh_mode_;
  std::vector<PartialRect> rects;
  if (mode == RefreshMode::PARTIAL) {
    for (const auto &r : this->partial_rects_) {
      PartialRect n;
      if (this->map_rect_to_native_(r, n))
        rects.push_back(n);
    }
    if (!this->partial_rects_.empty() && rects.empty()) {
      ESP_LOGD(TAG, "All partial rects are off-screen; using changed-area detection");
    }
  }
  this->partial_rects_.clear();

  xSemaphoreTake(this->lock_, portMAX_DELAY);
  if (this->pending_valid_) {
    // Coalesce with the job still waiting: the new frame replaces it.
    if (mode == RefreshMode::PARTIAL) {
      if (this->pending_mode_ != RefreshMode::PARTIAL) {
        mode = this->pending_mode_;  // never downgrade a queued full refresh
        rects.clear();
      } else if (rects.empty() || this->pending_rects_.empty()) {
        rects.clear();  // either side wants auto-detection; that covers both
      } else {
        rects.insert(rects.end(), this->pending_rects_.begin(), this->pending_rects_.end());
      }
    }
    ESP_LOGD(TAG, "Refresh still running; replaced queued %s job with %s", mode_name(this->pending_mode_),
             mode_name(mode));
  }
  if (mode == RefreshMode::FULL_GRAY) {
    this->convert_4bpp_(this->tx_pending_);
  } else {
    this->convert_1bpp_(this->tx_pending_);
  }
  this->pending_mode_ = mode;
  this->pending_rects_ = std::move(rects);
  this->pending_valid_ = true;
  xSemaphoreGive(this->lock_);

  xTaskNotifyGive(this->task_);
}

void ZectrixEPD::task_entry_(void *arg) { static_cast<ZectrixEPD *>(arg)->task_loop_(); }

void ZectrixEPD::task_loop_() {
  // Power the panel on once at boot (the reference does reset + OTP select here).
  if (!this->power_off_between_refreshes_) {
    esp_err_t err = this->drv_->PowerOnLocked();
    if (err != ESP_OK)
      ESP_LOGE(TAG, "Panel power on failed: %s", esp_err_to_name(err));
  }
  std::vector<PartialRect> rects;
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    for (;;) {
      RefreshMode mode;
      xSemaphoreTake(this->lock_, portMAX_DELAY);
      if (!this->pending_valid_) {
        this->running_ = false;
        xSemaphoreGive(this->lock_);
        break;
      }
      std::swap(this->tx_active_, this->tx_pending_);
      mode = this->pending_mode_;
      rects = std::move(this->pending_rects_);
      this->pending_rects_.clear();
      this->pending_valid_ = false;
      this->running_ = true;
      xSemaphoreGive(this->lock_);

      this->run_job_(mode, rects, this->tx_active_);
    }
  }
}

void ZectrixEPD::run_job_(RefreshMode mode, std::vector<PartialRect> &rects, const uint8_t *frame) {
  Ssd2683Driver *d = this->drv_;
  const RefreshMode requested = mode;

  if (mode == RefreshMode::PARTIAL) {
    if (this->gray_base_) {
      ESP_LOGW(TAG, "PARTIAL requested after a gray refresh; doing FULL_MONO first");
      mode = RefreshMode::FULL_MONO;
    } else if (!d->shadow_valid) {
      ESP_LOGI(TAG, "No valid mono base yet (boot or previous error); PARTIAL -> FULL_MONO");
      mode = RefreshMode::FULL_MONO;
    } else if (this->full_refresh_every_ > 0 && this->partial_count_ >= this->full_refresh_every_) {
      ESP_LOGI(TAG, "%u partial refreshes done; ghosting guard -> FULL_MONO", (unsigned) this->partial_count_);
      mode = RefreshMode::FULL_MONO;
    }
  }

  // Resolve the partial window: union of rects, or the changed area vs. the panel shadow.
  int rx = 0, ry = 0, rw = 0, rh = 0;
  const size_t rect_count = rects.size();
  if (mode == RefreshMode::PARTIAL) {
    int x0 = PANEL_WIDTH, y0 = PANEL_HEIGHT, x1 = 0, y1 = 0;  // x1/y1 exclusive
    if (!rects.empty()) {
      for (const auto &r : rects) {
        x0 = std::min(x0, r.x);
        y0 = std::min(y0, r.y);
        x1 = std::max(x1, r.x + r.w);
        y1 = std::max(y1, r.y + r.h);
      }
    } else {
      const int stride = PANEL_WIDTH / 8;
      for (int y = 0; y < PANEL_HEIGHT; y++) {
        const uint8_t *a = frame + y * stride;
        const uint8_t *b = d->shadow + y * stride;
        for (int bx = 0; bx < stride; bx++) {
          if (a[bx] != b[bx]) {
            x0 = std::min(x0, bx * 8);
            x1 = std::max(x1, bx * 8 + 8);
            y0 = std::min(y0, y);
            y1 = std::max(y1, y + 1);
          }
        }
      }
      if (x1 <= x0) {
        ESP_LOGD(TAG, "PARTIAL: nothing changed, skipping refresh");
        return;
      }
    }
    // The reference partial window is byte (8 px) aligned in x; expand outward.
    x0 &= ~7;
    x1 = std::min(PANEL_WIDTH, (x1 + 7) & ~7);
    rx = x0;
    ry = y0;
    rw = x1 - x0;
    rh = y1 - y0;
    const int src_stride = PANEL_WIDTH / 8, dst_stride = rw / 8;
    for (int r = 0; r < rh; r++) {
      std::memcpy(this->patch_ + r * dst_stride, frame + (ry + r) * src_stride + rx / 8, dst_stride);
    }
  }

  const int64_t start = esp_timer_get_time();
  esp_err_t err = d->PowerOnLocked();  // no-op while powered and ready
  if (err == ESP_OK) {
    switch (mode) {
      case RefreshMode::FULL_MONO:
        err = d->RefreshFull1bpp(frame);
        if (err == ESP_OK) {
          this->gray_base_ = false;
          this->partial_count_ = 0;
        }
        break;
      case RefreshMode::FULL_GRAY:
        err = d->RefreshFull4bpp(frame);
        this->gray_base_ = true;  // reference: no partial until the next full 1bpp
        this->partial_count_ = 0;
        break;
      case RefreshMode::PARTIAL:
        err = d->RefreshPartial1bpp(rx, ry, rw, rh, this->patch_);
        if (err == ESP_OK)
          this->partial_count_++;
        break;
    }
  }
  if (this->power_off_between_refreshes_) {
    esp_err_t off = d->PowerOffLocked();
    if (err == ESP_OK)
      err = off;
  }
  const uint32_t ms = static_cast<uint32_t>((esp_timer_get_time() - start) / 1000);

  switch (mode) {
    case RefreshMode::FULL_MONO:
      this->last_full_mono_ms_.store(ms);
      break;
    case RefreshMode::FULL_GRAY:
      this->last_full_gray_ms_.store(ms);
      break;
    case RefreshMode::PARTIAL:
      this->last_partial_ms_.store(ms);
      break;
  }

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Refresh %s failed after %u ms: %s", mode_name(mode), (unsigned) ms, esp_err_to_name(err));
  } else if (mode == RefreshMode::PARTIAL) {
    ESP_LOGI(TAG, "Refresh PARTIAL: %u rect(s)%s -> window x=%d y=%d w=%d h=%d, %u ms", (unsigned) rect_count,
             rect_count == 0 ? " (auto)" : "", rx, ry, rw, rh, (unsigned) ms);
  } else {
    ESP_LOGI(TAG, "Refresh %s%s%s: %u ms", mode_name(mode), requested != mode ? " (requested " : "",
             requested != mode ? "PARTIAL)" : "", (unsigned) ms);
  }
}

}  // namespace esphome::zectrix_epd

#endif  // USE_ESP32
