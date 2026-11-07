# SLOOP-BT: Optimist own-stack port

## Goal

Connect M-VAVE SMC-Mixer directly to the FM1 over its built-in Bluetooth
radio, without any PC, phone or external host device. Preserve the
existing SLOOP synths, drums, sequencer, projects, effects and OTA.

This work is isolated on `feature/smc-ble` and **not ready to flash**.

## Imported components

- `third_party/optimist_ble/`: Henri David's dual GPL-3.0-only/MPL-2.0
  own BLE stack, via fm1-base (pinned import snapshot recorded in
  `README-SLOOP-BT.md`).
- The BLE central, GATT discovery, host, scanner, MIDI packet codec,
  WL82 radio driver, calibration and RF tables are present.
- `BLE_CENTRAL=3`: minimalist central for devices that do not require
  pairing (BLE MIDI both ways, no bonding or encryption as master).
- Upstream imported MIDI, scanner and central host tests are running
  in `.github/workflows/optimist-ble-tests.yml`.
- `sloop_bt_midi_usb_packet` accepts Optimist-style USB-MIDI event
  words and routes them to the existing eight-channel SMC controller.
  Tested using host SLOOP stubs.
- `sloop_bt_midi_ring.h` handles up to 64 USB-MIDI packets from an
  eventual BLE interrupt producer, drained on the main loop.
  Host tests cover ordering, overflow and wrapping.
- Existing `sloop_bt_link.h` manages the user-facing connection
  state; no radio events are actually feeding it yet.

## Size: two deliberately nonflashable variants

`.github/workflows/optimist-target-measure.yml` uses the real JieLi
target compiler with SLOOP's measured lean build. It compares:

1. `SLOOP_BT_RADIO_MEASURE=0`: protocol with **fake** baseband.
   The compiler may eliminate radio-dependent paths, so this is
   only a lower bound, never a reliable firmware size.
2. `SLOOP_BT_RADIO_MEASURE=1`: the genuine Optimist WL82 driver,
   calibration code and V15 RF table source with the protocol.

Both set `SLOOP_BT_OPEN_MEASURE=1`, which uses an artificially
expanded XIP linker region. The build script deletes any `.fwsc`
and `felucca.bin` and explicitly refuses to create a flashable
package. Logs and symbol reports alone can be uploaded.

The physical application slot holds 581,564 bytes. The last lean
SLOOP-BT controller-only build measured 551,176 B (30,388 free).
**Do not treat successful linking with this expanded region as
proof the image fits on the device.**

## Current integration blockers

1. Actual WL82 IRQ 29 RX and IRQ 45 event handlers need to be
   registered using SLOOP's vector/interrupt HAL; a diagnostic
   source file importing the driver is not an operational port.
2. Tune radio calibration to the FM1's flash and RF trims safely.
3. Ensure all radio callbacks publish only into the bounded MIDI
   queue. SLOOP's track/UI dispatch runs on the main loop only.
4. Implement FM1 BLE scan/pair menu operations and saved-controller
   selection. Minimal central cannot pair; validate actual SMC-Mixer
   security requirements, upgrade to `BLE_CENTRAL=1` if necessary.
5. Integrate the existing SMC LED output encoder with the Optimist
   outgoing USB-MIDI event queue; retain retry and resync.
6. Measure under full audio load, confirm no audio glitches.
7. Verify memory, flash, OTA, recovery, presets and project formats
   before any .fwsc release.

## Safety

No diagnostic image, fake radio build, unvalidated loader or
over-budget firmware must be distributed through the web flasher.
Keep the original SLOOP `main` build undisturbed.

## Actual target-link measurements (6 November 2026)

A nonflashable, expanded-XIP JieLi target link of SLOOP's lean
controller build plus the Optimist **real WL82 driver**, IRQ 29/45
call trees, RF calibration startup and V15 RF tables completed in
GitHub Actions run
[42287856803](https://github.com/alfonsogordon/sloop-BT/actions/runs/42287856803):

| Variant | Image size | FM1 physical app slot | Nominal space left |
| --- | ---: | ---: | ---: |
| Lean SLOOP + SMC controller, no radio | 551,176 B | 581,564 B | 30,388 B |
| Optimist minimal central, fake baseband | 552,248 B | 581,564 B | 29,316 B |
| Optimist central + real radio driver, IRQ handlers + RF startup | **579,756 B** | 581,564 B | **1,808 B** |

The third value has much better retention of real Bluetooth code than
the initial stub-only experiment. It is still a **diagnostic**: it is
not a production radio initialization path; no tested IRQ installation,
real scanning, physical SMC pairing, latency/load or hardware test
exists. It deliberately did not produce a .fwsc file. The very narrow
1,808-byte margin will likely need more space once UI/persistence
and production state-management code is in place.

Since that experiment the development branch has been updated to
**exclude the redundant Hortator BLE-MIDI packet codec** when
`SLOOP_BT_OPEN_MEASURE=1`. Optimist already supplies its own codec.
The latest measurement is rerun separately and must be verified
before quoting new image sizes.

## Native Optimist MIDI bridge and later link measurement

The Optimist measurement variant now excludes the redundant legacy
Hortator BLE-MIDI codec, and has an IRQ-only input path through the
bounded queue. The full Optimist BLE codec provides MIDI packet
encoding; `ble_app_midi_peek` and `ble_app_midi_pop` feed SMC LEDs
back to the controller, with acknowledgement deferred to the main
loop. The UI/track functions are never intentionally invoked directly
from Bluetooth interrupt context.

A new real-driver/IRQ/RF-startup build with this more complete output
path measured **583,628 B** against the **581,564 B** physical app
slot: **2,064 B too large**. This is a nonflashable, expanded-linker
diagnostic. The radio is not yet initialized or connected on real
hardware. Earlier 579,756 B measurement was an incomplete diagnostic,
not a release candidate.

Two further nonflashable alternatives are configured for measurement:

- `SLOOP_BT_SPLASH=0`: remove only the decorative boot splash and
  boot pause; no audio functionality removed.
- `SLOOP_BT_SIZE_OPT=1`: compile the diagnostic with Clang `-Oz`
  instead of `-Os`. This changes optimization of all firmware
  functions, so before any use in a product build it must pass the
  full DSP golden-output and CPU performance tests. It is not
  automatically safe merely because the image is smaller.

No optimized-build size claim is valid until its GitHub Actions job
has completed successfully.
