# ESPHome Voice Assistant for the Zectrix Note 4

A **Home Assistant voice satellite** for the Zectrix Note 4 (ESP32-S3 N16R8,
4.2" 400x300 e-paper, ES8311 codec, one microphone and a speaker), with an
e-paper "page of the day": clock, weather, today's events, the one thing that
matters right now, and the conversation itself - the question you asked and the
answer, typed out while it is spoken.

Pure ESPHome plus two custom components: an always-on core you pull as a
package, eight skins, and one thin config file you actually edit. The voice
side is a port of
[esphome-esp32-s3-box-3-va](https://github.com/MichalZaniewicz/esphome-esp32-s3-box-3-va).

> **🚧 Work in progress - not ready to flash.** The device has not arrived yet:
> nothing here has run on a Note 4. It compiles in CI (ESPHome 2026.10.2, all
> eight skins, 2.97 MB image), but compiling is all that is proven. Follow
> along, but do not install it expecting a working assistant.
>
> **Status: untested on hardware.** The configuration validates with ESPHome
> 2026.10.2 and that is all that is claimed. The pin map comes from Zectrix's
> reference firmware; the audio settings, the wake word thresholds and the
> volume limits are carried over from the Box-3 and need confirming on a real
> Note 4. [docs/HARDWARE.md](docs/HARDWARE.md) has the bring-up checklist.

## What it does

- **Voice assistant**: on-device wake word (`alexa`, `okay nabu`, `hey jarvis`,
  pick one in Home Assistant) via `micro_wake_word`, the full Assist pipeline,
  and "Stop" on its own to silence an alarm or cut a reply short.
- **Full duplex** through
  [esp_audio_stack](https://github.com/MichalZaniewicz/esphome-audio-stack) and
  esp_aec, with the ES8311's own DAC feedback as the echo reference, so the
  wake word works while the device is talking (barge-in).
- **Replies on another speaker** if the room has one (`TTS output` select).
- **Voice timers** with an alarm that always rings on the device.
- **E-paper UI**: listening (live waveform), thinking (the heard question),
  answering (typed word by word), the page of the day, tomorrow's forecast, a
  running timer, a doorbell snapshot. Partial refreshes wherever the layout
  allows, so the clock and the typing do not flash the panel.
- **Buttons**: OK talks (or stops whatever is going on), a 2 s hold silences
  the alarm; UP/DOWN step through page of the day, forecast and timer.
- **Battery**: voltage, percentage, charging state; an RTC that keeps the clock
  right before Wi-Fi is up.

## Skins

Switch live with the **Skin** select in Home Assistant; `skin:` in the thin
config is the boot default. Only skins whose package is in the build can be
picked.

`paper`, `swiss`, `instrument`, `tiles`, `poster`, `dial`, `pixel`,
`quiet`

## Screen language

Everything the panel draws is English by default. Set `language: pl` in the
thin config's `substitutions:` for Polish:

```yaml
substitutions:
  language: pl    # en (default) or pl
```

This is only the on-screen text (labels, weekday and month names, weather
words, hints). Home Assistant entity names stay English, and what the assistant
hears and says is the language of the Assist pipeline you pick in Home
Assistant. The text you push with the actions below (event titles, the "Now"
card, the forecast summary) is shown as sent, so send it in the same language.
All phrases live in one table per language in
[components/note4_ui/strings.cpp](components/note4_ui/strings.cpp); a new
language is one more table there.

The **16 greys** switch lets screens without live regions (forecast,
doorbell) use a 16-grey refresh; everything with a clock or a countdown is
1-bit (see [docs/HARDWARE.md](docs/HARDWARE.md) for why).

## Quick start

> Requires **ESPHome 2026.7.0+**.

1. Copy `secrets.example.yaml` to `secrets.yaml` and fill in your Wi-Fi.
2. Copy **`note4-va.yaml`** next to it and edit the `substitutions:` at the top
   (name, skin, room, language, weather entity, timezone). Delete the skin lines you will
   never use: each one costs flash and build time.
3. First flash over USB, then updates go wireless:
   ```
   esphome run note4-va.yaml
   ```
4. In Home Assistant: the new ESPHome device appears; open **Configure** and
   assign an Assist pipeline.
5. Say "Alexa", or press OK.

After changing anything in the core, run `esphome clean` before the next build -
otherwise ESPHome reuses the cached copy of the remote package.

## Repository layout

```
note4-va.yaml            # YOUR config: copy + edit this (pulls the rest from GitHub)
secrets.example.yaml     # copy to secrets.yaml
base/
  core.yaml              # the always-on core, pulled as a remote package
  skins/<name>.yaml      # one package per skin
  sounds/                # wake beep, boot chime, timer alarm
components/
  zectrix_epd/           # e-paper display driver (SSD2683, ported from Zectrix)
  note4_ui/              # UI model, refresh policy, skin framework
ci/local.yaml            # validates/compiles the working tree (CI uses this)
docs/HARDWARE.md         # pin map, power latch, battery, refresh limits
```

## Home Assistant actions

Home Assistant pushes what the page of the day shows. The actions appear as
`esphome.<name>_<action>`, e.g. `esphome.note4_va_set_now`.

| Action | Variables | What it does |
|---|---|---|
| `set_now` | `level` int (0 brief, 1 to do, 2 alarm), `title`, `body`, `detail`, `icon` | The "Now" card: what matters right now. Level 2 is drawn in negative and brings the page of the day up. Icons: `window`, `wind`, `plant`, `vacuum`, `school`, `flood`, `smoke`, `brief` |
| `set_forecast` | `day`, `date`, `hi` float, `lo` float, `summary`, `detail`, `temps` float[19], `rain` float[19], `rain_start` float, `wind`, `humidity`, `sunrise`, `sunset` | Tomorrow's forecast. `temps` / `rain` (mm) are hourly for 6:00-24:00; `rain_start` is an hour (15.5 = 15:30), negative for none |
| `set_events` | `starts` float[], `ends` float[], `titles` string[], `next_time`, `next_title`, `next_in` | Today's timeline (hours as floats) and the "next" line |
| `show_screen` | `name`: `idle`, `weather`, `timer`, `doorbell` | Switch screen. Ignored during a conversation (except `doorbell`) |
| `doorbell` | `url`, `caption`, `place` | Fetches a JPEG snapshot (greyscale, 264x300), shows it with the caption, back to the page of the day after a minute |

Example:

```yaml
action: esphome.note4_va_set_now
data:
  level: 1
  title: "Window open"
  body: "Bathroom · 12 min"
  detail: "heating paused"
  icon: window
```

```yaml
action: esphome.note4_va_doorbell
data:
  url: "http://homeassistant.local:8123{{ state_attr('camera.front_gate', 'entity_picture') }}"
  caption: "Courier with a parcel"
  place: "Gate"
```

Weather now comes from `weather_entity` on its own (its state and its
`temperature` attribute); today's high/low from the optional
`weather_high_entity` / `weather_low_entity` sensors.

## Credits

The e-paper panel driver is ported from Zectrix's ESP-IDF `zectrix_epd`
component (MIT, see `components/zectrix_epd/LICENSE-zectrix-epd`). Audio:
[n-IA-hane/esphome-audio-stack](https://github.com/n-IA-hane/esphome-audio-stack).
The timer sound is Home Assistant Voice PE's.

## License

MIT, see [LICENSE](LICENSE).
