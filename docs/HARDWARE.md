# Zectrix Note 4 - hardware notes

Everything here is what `base/core.yaml` assumes. The pin map is taken from
Zectrix's own reference firmware (`components/zectrix_board/include/zectrix_board_config.h`
and `zectrix_board.cc`), not from guesswork. **None of it has been checked on a
device by this project yet**; the bring-up list at the end is what to confirm
first.

## Board

| | |
|---|---|
| SoC | ESP32-S3 (N16R8), 240 MHz, dual core |
| Flash | 16 MB |
| PSRAM | octal, 80 MHz - **mandatory**: esp_audio_stack and the e-paper driver require it |
| Display | 4.2" e-paper, 400x300, SSD2683 controller, own SPI bus |
| Audio | ES8311 codec, one chip for both the microphone ADC and the speaker DAC |
| Microphone | one |
| RTC | PCF8563, battery backed |
| NFC | NT3H (not used) |
| Power | Li-ion cell with a software power latch |
| ESPHome board | `esp32-s3-devkitc-1` with `flash_size: 16MB` |

## Pin map

| Function | GPIO | Notes |
|---|---|---|
| I2C SDA | 47 | Shared bus: ES8311, PCF8563, NT3H |
| I2C SCL | 48 | |
| I2S MCLK | 14 | |
| I2S BCLK | 15 | |
| I2S WS / LRCLK | 38 | |
| I2S DIN | 16 | ES8311 → ESP (mic, plus the DAC feedback channel) |
| I2S DOUT | 45 | ESP → ES8311. Strapping pin; the build warns about it (the audio stack's pin schema has no `ignore_strapping_warning`) |
| Amplifier enable | 46 | Active high, strapping pin |
| Audio power rail | 42 | Must be HIGH before the codec or the I2C bus is touched |
| Power latch | 17 | HIGH keeps the board on when on battery; LOW = power off |
| OK / voice button | 0 | Active low, strapping pin |
| UP button | 39 | Active low |
| DOWN button | 18 | Active low |
| Status LED | 3 | Green, active LOW, strapping pin |
| Battery ADC | 4 | ADC1 channel 3, through a 1:2 divider |
| Charging | 2 | LOW while charging |
| Charge complete | 1 | HIGH when full |
| RTC interrupt | 5 | Not used |
| NFC field detect / power | 7 / 21 | Not used |
| E-paper CS / DC / RST / BUSY | 11 / 10 / 9 / 8 | Driver defaults (`components/zectrix_epd`) |
| E-paper power | 6 | Driver default |
| E-paper CLK / MOSI | 12 / 13 | Driver default, SPI3 |

## I2C map

| Address | Device | Used here |
|---|---|---|
| 0x18 | ES8311 audio codec | yes, as input and output |
| 0x51 | PCF8563 RTC | yes, `time: platform: pcf8563` |
| 0x55 | NT3H NFC tag | no |

## Power latch

On battery the board only stays powered while GPIO17 is driven high; release
it and the board switches itself off. The vendor firmware sets it in the first
lines of board init, and so does this one: an `on_boot` lambda at priority
1100 - above the I2C bus's 1000, so it runs before any component that could
touch the audio rail - releases any GPIO hold, sets the level before switching
the pin to output (so it never glitches low), and then holds it with
`gpio_hold_en`, which keeps the board powered through a soft reset (OTA, crash).
GPIO42, the codec/I2C power rail, gets the same treatment in the same lambda.

The **Power off** button entity drives the latch low. On USB power the board
keeps running regardless.

## Battery

`Battery voltage` is the ADC reading (12 dB attenuation) times two, for the
divider. `Battery` (%) uses the vendor's curve, with `v` the cell voltage in mV:

```
percent = (-v*v + 9016*v - 19189000) / 10000,   clamped to 0..100
```

That gives about 12% at 3.50 V and saturates just under 4.2 V. The charger
status lines are read as plain inputs, no pulls, exactly like the vendor's
`ChargeStatus::Init`.

## Audio: ES8311 digital feedback

The Box-3 gets its echo reference by looping the ES8311's output into a second
ADC (ES7210) on a TDM slot. The Note 4 has one codec, so the reference comes
from the ES8311 itself: with `no_dac_ref: false` the codec returns a stereo
frame, left = microphone ADC, right = what the DAC is playing, sample-aligned.
esp_audio_stack calls this "ES8311 digital feedback" and recommends it for this
codec. The core configures it as:

```yaml
esp_audio_stack:
  num_channels: 2
  use_stereo_aec_reference: true
  reference_channel: right
  codec:
    input:  {type: es8311, address: 0x18, no_dac_ref: false}
    output: {type: es8311, address: 0x18, no_dac_ref: false}
  processor_id: aec_processor   # esp_aec
```

**Bus rate.** The vendor runs the codec at 16 kHz. The core runs the bus at
48 kHz and lets the stack decimate the mic to 16 kHz, because on the Box-3 a
16 kHz bus "initialised fine and played total silence" on the same DAC, and the
audio stack's own ES8311 example runs 48 kHz. If the speaker is silent, or the
echo canceller does nothing, set `audio_bus_rate: '16000'` - one substitution,
the announcement pipeline follows it.

## E-paper: grey versus partial refresh

The panel can refresh in three ways: a full 1-bit refresh, a full 16-grey
refresh, and a partial 1-bit refresh of a few rectangles (no flash, much
faster). The vendor driver **forbids a partial refresh after a 16-grey
refresh**: the grey waveform leaves the panel in a state the partial waveform
cannot start from.

So any screen with live regions - the clock on the page of the day, the timer
countdown, the listening waveform, the answer being typed - is drawn and
refreshed 1-bit, and its flat grey fills are rendered as an 8x8 Bayer raster.
Only screens without live regions (the forecast) may use a
16-grey full refresh, and only while the **16 greys** switch is on. Live
regions must sit on pure white or pure black for the same reason: a partial
refresh is 1-bit, and grey under it turns into dots. The driver also forces a
full 1-bit refresh after every 30 partial ones, to clear ghosting.

## Bring-up checklist

1. **Power latch**: on battery only, the board stays on after releasing the
   button. If it dies after a second, the latch lambda is too late or wrong.
2. **Power key**: DOWN (GPIO18) is the hardware power-on key, not OK, so
   powering on does not hit the GPIO0 strapping pin. A 3 s DOWN hold powers off
   (battery only; on USB the latch does nothing).
3. **I2C scan** in the boot log shows 0x18, 0x51, 0x55. Missing 0x18 means the
   audio rail (GPIO42) is not up yet when the bus scans.
4. **Speaker**: the boot chime and the wake beep play. If not, try
   `audio_bus_rate: '48000'` (the Box-3 needed it through the same stack).
5. **Microphone and AEC**: STT works; the wake word is heard while a reply is
   playing (barge-in). If barge-in fails but the wake word works at rest, the
   reference channel is wrong - try `reference_channel: left`.
6. **Volume**: does `volume_max: 1.0` distort on this speaker?
7. **Wake word**: the cutoffs are Box-3 measurements. Lower `Wake word cutoff`
   live and read the probabilities from the log.
8. **Refresh times**: the three `Diag: ... refresh` sensors.

## What the community firmware already found

Collected from 22 open-source NOTE4 projects (shallow clones, read only). The
references point at `repo:path`.

- **Audio**: everyone runs the ES8311 at 16 kHz in and out (xiaozhi-zectrix
  `boards/zectrix/zectrix-s3-epaper-4.2/config.h`, slate `firmware/README.md`).
  Mic gain 30 dB in xiaozhi and slate; kerray/zectrix-epaper-rs measured peaks of
  -13.6 dBFS at 30 dB and moved to 36 dB (`docs/AUDIO.md`). No project uses the
  DAC feedback for echo cancellation yet, but epaper-rs measured it: mic in the
  left slot, DAC reference in the right, reading zero while recording.
- **Amplifier**: GPIO46 is active high and a strapping pin; slate drives it low
  before GPIO42 rises and enables it ~100 ms after the codec opens, to avoid a
  pop (`bsp/board_power.cc`).
- **GPIO42** powers the I2C pull-ups as well as the codec. Turn it off and every
  I2C device disappears. wqn keeps it on permanently (switching it popped).
- **Refresh times measured by others**: full mono 1.2-3 s, partial 0.3-0.8 s
  (slate forces a full one after 8 partials, chalk after 12), 16-grey 7.3 s
  (weather-glass). That is why a finished answer stays 1-bit: a 7 s grey flash
  right after the reply is worse than no grey. csloz/macau-bus-note4 has an
  optional fast black/white waveform (~0.36 s full, ~0.45 s partial); epaper-rs
  avoids it because of ghosting. Worth trying later.
- **No project** does a partial refresh after a grey one. Do not resend the
  0xE0/0xE6 voltage-boost settings before a partial, or it does nothing (slate).
- **Power**: DOWN (GPIO18) powers on. Others latch GPIO17 first thing and wait
  for DOWN to be released. Wake sources used for deep sleep: EXT1 on GPIO0,
  GPIO18, GPIO2 (USB in) and GPIO5 (RTC alarm, open drain, needs a pull-up).
  GPIO39 (UP) cannot wake the chip. No measured battery life anywhere; hearth
  estimates ~1.5 days at 60 mA.
- **Flash**: back up all 16 MB first. The factory layout is xiaozhi's (ota_0 at
  0x20000); epaper-rs advises never writing the bootloader. Factory bootloader
  header decodes to DIO 80 MHz.
- **ESPHome precedent**: grappeq/note4c-ha-display is an ESPHome + HA config,
  but for the colour NOTE4C (different panel, JD79660). Useful for its power and
  sleep handling, not its display.
