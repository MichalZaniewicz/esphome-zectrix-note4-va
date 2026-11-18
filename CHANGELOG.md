# Changelog

## [Unreleased]

First version. **Untested on hardware**: it validates (`esphome config
ci/local.yaml`) and nothing more is claimed yet.

### Added
- **Screen languages** German (`de`), Dutch (`nl`), French (`fr`) and Spanish (`es`), besides English and Polish; also in the blueprints. Written without native speakers; review welcome.
- **Home Assistant blueprints** (`blueprints/automation/`): the "Now" card with its priority rules and an AI day brief, tomorrow's forecast, today's events and the doorbell.
- The device skips `set_now`, `set_forecast` and `set_events` calls that change nothing, so scheduled resends cost no e-paper refresh.
- **Core** (`base/core.yaml`), ported from the ESP32-S3-BOX-3 voice assistant:
  the same esp_audio_stack + esp_aec full-duplex audio, media player, three
  wake words plus the internal "Stop" word, the Assist pipeline with barge-in,
  TTS routing to an external speaker, voice timers with their alarm, the
  deaf-device watchdog and the live tuning entities (mic gain, wake word
  cutoff and gain, AEC mode, wake word engine, TTS output).
- **ES8311 digital-feedback echo reference**: one codec as microphone and
  speaker, the DAC copy on the right channel of a stereo bus.
- **E-paper UI wiring**: the `zectrix_epd` display and the `note4_ui` model,
  driven from the pipeline - listening with a live waveform, the heard
  question, the answer typed while it is spoken, back to the page of the day a
  minute later. Timers, a tomorrow-forecast screen and a doorbell snapshot.
  UP/DOWN step through the screens, OK talks or stops, a 2 s hold on OK
  silences the alarm.
- **Home Assistant actions**: `set_now`, `set_forecast`, `set_events`,
  `show_screen`, `doorbell`.
- **Board support**: power latch and audio rail at boot, PCF8563 RTC kept in
  sync with Home Assistant, battery voltage and percentage (vendor curve),
  charging state, status LED, a Power off button.
- **Skin** select (eight skins) and a **16 greys** switch.
- **Screen language**: English by default, Polish with `language: pl`. Every
  phrase the skins and the core draw comes from one table per language
  (`components/note4_ui/strings.cpp`).
- CI: `.github/workflows/build.yml` validates and compiles `ci/local.yaml`.

### Not carried over from the Box-3
- LVGL, the touchscreen, characters, screens, night mode, the backlight and
  the hello screen: none of it applies to an e-paper panel. The language
  packages are replaced by the C++ string table above.
