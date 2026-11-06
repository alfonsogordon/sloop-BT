# SLOOP-BT lean build: real FM1 target measurements

Measured 2026-11-06 using GitHub Actions, pinned JieLi compiler and AC79 SDK.

- [Individual feature builds](https://github.com/alfonsogordon/sloop-BT/actions/runs/42267774981)
- [Visualiser builds](https://github.com/alfonsogordon/sloop-BT/actions/runs/42267955536)

All figures are from the linker-checked application image, not guesses or host native builds.
Application slot: **581,564 bytes**. No BLE controller/radio stack is linked in these builds.

| Variant | FM1 image | Flash headroom | RAM .data+.bss | .pool |
|---|---:|---:|---:|---:|
| Baseline SLOOP | 571,820 | 9,744 | 59,340 | 333,900 |
| SLOOP-BT controls & menu, all UI | 573,096 | 8,468 | 59,628 | 333,900 |
| No USB Audio | 569,500 | 12,064 | 56,140 | 333,900 |
| No icons | 568,344 | 13,220 | 59,628 | 333,900 |
| Text boot logo | 568,504 | 13,060 | 59,628 | 333,900 |
| No TRS MIDI | 573,864 | 7,700 | 59,420 | 333,900 |
| No full-screen visualiser | 565,780 | 15,784 | 56,732 | 328,396 |
| Lean: no USB Audio/icons, text logo | 559,452 | 22,112 | 56,140 | 333,900 |
| **Lean plus no visualiser** | **551,176** | **30,388** | **53,260** | **328,396** |

All sizes in bytes. The combined savings against controller build are
**21,920 flash bytes**, **6,368 main RAM bytes**, **5,504 pool bytes**.
Some deltas are not additive due to compiler/linker optimisation.

## Configuration for measurement (NOT BT firmware)

```sh
SLOOP_BT_CONTROL=1 SLOOP_BT_DRUM_MIX=1 \
FELUCCA_UAC=0 FELUCCA_ICONS=0 FELUCCA_LOGO_TEXT=1 \
SLOOP_BT_VISUALIZER=0 FELUCCA_UART=1 sh build.sh
```

The baseline build is unchanged unless explicit flags are supplied.
The above builds have compiled and linked for the FM1, but the
visualiser/USB removal variants are NOT tested on physical hardware,
have not been audio-regression compared and do not provide Bluetooth.

## Decision

- **Keep** TRS MIDI: turning it off grew the binary by 768 B.
- **Likely easy sacrifices**: graphical boot logo, decorative icons.
- **Possible sacrifice**: USB audio streaming; preserve USB MIDI.
- **Largest optional UI saving**: full-screen visualiser. The audio
  engine is not changed; the UI visualiser opens as a no-op.
- **Hard lower bound**: 551,176 B remains before adding any BLE radio
  libraries. Only 30,388 bytes of XIP are free, still far short of
  Hortator's reference BLE radio footprint (~100 KB+).
- **Pool limitation**: lean build's pool uses 328,396 / 344,064 B;
  15,668 B free; Hortator's one-controller BLE stack needs substantially
  more for the heap, task stacks and queue. Some large SLOOP buffers
  must be relocated or substantially reduced, but without breaking
  the audio or update/recovery path.

The correct next step is a separate **measurement-only BLE link**
(with a diagnostic linker map, no flash package), using the pinned
Hortator AC79 BLE archives, to get its precise size on the SLOOP base.
Only after that can we judge the additional changes needed without
cutting essential synths, drums or sequencing.
