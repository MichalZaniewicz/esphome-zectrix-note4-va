# zectrix_epd

ESPHome (2026.10, ESP32-S3, ESP-IDF) display platform for the Zectrix Note 4
4.2" e-paper panel (SSD2683 controller, 400x300).

The panel driver is a port of the Zectrix ESP-IDF component `zectrix_epd`
(Copyright (c) 2026 Zectrix Lab, MIT, see `LICENSE-zectrix-epd`). Command
sequences, waveform tables (`ssd2683_waveform.h`, copied verbatim) and timing
are unchanged. The panel is driven over ESP-IDF `spi_master` on its own bus
(SPI3 by default, like the reference), not through ESPHome's `spi:` component.

## What it does

- One 400x300 draw buffer in PSRAM, 1 byte per pixel: low nibble = grey 0..15
  (0 black, 15 white), bit 7 = "tone" flag (set when `Color.w >= 128`).
- Mono refreshes: tone pixels are ordered-dithered (8x8 Bayer), ink pixels are
  thresholded (grey >= 8 is white). Grey refreshes use the nibble directly.
- Panel transfers and BUSY waits run in a separate FreeRTOS task, so the main
  loop (API, audio) does not stall. If a refresh is still running, only the
  latest frame is queued.
- `clear()` (and so auto-clear) fills white. Note that `COLOR_ON` is white and
  `COLOR_OFF` is black under the luminance mapping, so pass explicit colours:
  ink `Color(0, 0, 0, 0)`, tone grey `Color(128, 128, 128, 255)`.

## YAML

```yaml
psram:
  mode: octal

external_components:
  - source: {type: local, path: components}
    components: [zectrix_epd]

display:
  - platform: zectrix_epd
    id: epd
    # every key below is optional; the values are the defaults
    cs_pin: GPIO11
    dc_pin: GPIO10
    reset_pin: GPIO9
    busy_pin: GPIO8
    power_pin: GPIO6
    clk_pin: GPIO12
    mosi_pin: GPIO13
    spi_host: SPI3
    spi_frequency: 20MHz
    busy_timeout: 2000ms        # every BUSY wait (reference busy_timeout_ms)
    gray_busy_timeout: 5000ms   # per gray pass (reference kExternalRefreshTimeoutUs)
    full_refresh_every: 30      # partials before a forced FULL_MONO (0 = never)
    power_off_between_refreshes: false
    update_interval: never
    rotation: 0
    lambda: |-
      it.filled_rectangle(0, 0, 400, 60, Color(160, 160, 160, 255));  // tone
      it.print(10, 10, id(font_big), Color(0, 0, 0, 0), "Hello");     // ink
      id(epd).set_refresh_mode(zectrix_epd::RefreshMode::PARTIAL);
      id(epd).add_partial_rect(0, 0, 400, 60);
```

Trigger a redraw with `component.update: epd` or `id(epd).update();`.

## Refresh API (C++, from lambdas)

```cpp
enum class RefreshMode { FULL_MONO, FULL_GRAY, PARTIAL };
void set_refresh_mode(RefreshMode m);          // sticky until changed
void add_partial_rect(int x, int y, int w, int h);  // display coords, used once
void set_partial_rects(const std::vector<PartialRect> &rects);
void clear_partial_rects();
void set_full_refresh_every(uint32_t n);
bool is_busy() const;                          // refresh running or queued
uint32_t last_full_mono_ms() const;
uint32_t last_full_gray_ms() const;
uint32_t last_partial_ms() const;
```

`update()` runs the lambda/pages, converts the frame, then queues the refresh.
You can set the mode and rects inside the display lambda itself. PARTIAL
refreshes the union of the given rects, widened to multiples of 8 px in x. With
no rects it refreshes the bounding box of the pixels that changed since the last
refresh, and skips the refresh if nothing changed. Every refresh is logged at
INFO with its mode, rect count and duration.

## Limitations

- After a FULL_GRAY (4bpp) refresh the controller has no mono base. A PARTIAL
  requested then logs a warning and runs as FULL_MONO instead. The first refresh
  after boot and the first one after a failed refresh are also FULL_MONO.
- Partial refresh is mono only. Tone areas come out dithered and ink is
  thresholded, so anti-aliased edges become hard.
- `power_off_between_refreshes: true` is untested here. The reference demo does
  power-cycle the panel around partial refreshes, because the old image is
  kept in a host-side shadow buffer, but this port has not been checked on
  hardware.
