// ESPHome display platform for the Zectrix Note 4 e-paper panel (SSD2683, 400x300).
//
// The panel driver (command sequences, waveform handling, timing) is ported from the
// Zectrix ESP-IDF component "zectrix_epd", Copyright (c) 2026 Zectrix Lab, MIT License.
// See LICENSE-zectrix-epd in this directory.

#pragma once

#ifdef USE_ESP32

#include <atomic>
#include <cstdint>
#include <vector>

#include "esphome/components/display/display_buffer.h"
#include "esphome/core/component.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

namespace esphome::zectrix_epd {

static constexpr int PANEL_WIDTH = 400;
static constexpr int PANEL_HEIGHT = 300;

enum class RefreshMode : uint8_t { FULL_MONO = 0, FULL_GRAY = 1, PARTIAL = 2 };

/// Rectangle in display (rotated, user) coordinates.
struct PartialRect {
  int x;
  int y;
  int w;
  int h;
};

struct Ssd2683Driver;  // Ported reference driver, defined in zectrix_epd.cpp.

class ZectrixEPD : public display::DisplayBuffer {
 public:
  // ---- Codegen setters ----
  void set_pins(int cs, int dc, int reset, int busy, int power, int sclk, int mosi) {
    this->pin_cs_ = cs;
    this->pin_dc_ = dc;
    this->pin_reset_ = reset;
    this->pin_busy_ = busy;
    this->pin_power_ = power;
    this->pin_sclk_ = sclk;
    this->pin_mosi_ = mosi;
  }
  void set_spi_host(int host) { this->spi_host_ = host; }  // 2 = SPI2_HOST, 3 = SPI3_HOST
  void set_spi_frequency(uint32_t hz) { this->spi_frequency_ = hz; }
  void set_busy_timeout(uint32_t ms) { this->busy_timeout_ms_ = ms; }
  void set_gray_busy_timeout(uint32_t ms) { this->gray_busy_timeout_ms_ = ms; }
  void set_power_off_between_refreshes(bool v) { this->power_off_between_refreshes_ = v; }

  // ---- Refresh API (callable from lambdas) ----
  /// Mode used by the next update(). Sticky until changed.
  void set_refresh_mode(RefreshMode m) { this->refresh_mode_ = m; }
  RefreshMode get_refresh_mode() const { return this->refresh_mode_; }
  /// Rects for the next PARTIAL refresh, in display coordinates. Consumed by update().
  /// With no rects, PARTIAL refreshes the bounding box of all pixels that changed.
  void add_partial_rect(int x, int y, int w, int h);
  void set_partial_rects(const std::vector<PartialRect> &rects);
  void clear_partial_rects() { this->partial_rects_.clear(); }
  /// After N partial refreshes the next PARTIAL becomes FULL_MONO (0 = never).
  void set_full_refresh_every(uint32_t n) { this->full_refresh_every_ = n; }
  /// True while a refresh is running or queued.
  bool is_busy() const;
  uint32_t last_full_mono_ms() const { return this->last_full_mono_ms_.load(); }
  uint32_t last_full_gray_ms() const { return this->last_full_gray_ms_.load(); }
  uint32_t last_partial_ms() const { return this->last_partial_ms_.load(); }

  // ---- Component / Display ----
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }
  display::DisplayType get_display_type() override { return display::DISPLAY_TYPE_GRAYSCALE; }
  /// Clears to white (e-paper background), not COLOR_OFF/black.
  void clear() override;

 protected:
  int get_width_internal() override { return PANEL_WIDTH; }
  int get_height_internal() override { return PANEL_HEIGHT; }
  void draw_absolute_pixel_internal(int x, int y, Color color) override;

  static void task_entry_(void *arg);
  void task_loop_();
  void run_job_(RefreshMode mode, std::vector<PartialRect> &rects, const uint8_t *frame);
  bool map_rect_to_native_(const PartialRect &in, PartialRect &out) const;
  void convert_1bpp_(uint8_t *out) const;
  void convert_4bpp_(uint8_t *out) const;

  // Config
  int pin_cs_{11}, pin_dc_{10}, pin_reset_{9}, pin_busy_{8}, pin_power_{6}, pin_sclk_{12}, pin_mosi_{13};
  int spi_host_{3};
  uint32_t spi_frequency_{20000000};
  uint32_t busy_timeout_ms_{2000};
  uint32_t gray_busy_timeout_ms_{5000};
  bool power_off_between_refreshes_{false};
  uint32_t full_refresh_every_{30};

  // Main-loop state
  RefreshMode refresh_mode_{RefreshMode::FULL_MONO};
  std::vector<PartialRect> partial_rects_;

  // Shared with the refresh task (guarded by lock_)
  SemaphoreHandle_t lock_{nullptr};
  TaskHandle_t task_{nullptr};
  uint8_t *tx_pending_{nullptr};
  uint8_t *tx_active_{nullptr};
  RefreshMode pending_mode_{RefreshMode::FULL_MONO};
  std::vector<PartialRect> pending_rects_;  // native coords; empty = auto (changed area)
  bool pending_valid_{false};
  bool running_{false};

  // Refresh task state (touched only by the task after setup)
  Ssd2683Driver *drv_{nullptr};
  uint8_t *patch_{nullptr};
  bool gray_base_{false};
  uint32_t partial_count_{0};

  std::atomic<uint32_t> last_full_mono_ms_{0};
  std::atomic<uint32_t> last_full_gray_ms_{0};
  std::atomic<uint32_t> last_partial_ms_{0};
};

}  // namespace esphome::zectrix_epd

#endif  // USE_ESP32
