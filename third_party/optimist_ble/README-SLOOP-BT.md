# Optimist open BLE stack — SLOOP-BT evaluation

The source files in this folder were copied from
[fm1-base](https://github.com/charlesvestal/fm1-base/tree/317cb86cc60d6286fa80a02743e94e6a60ff842d/bt/ble)
on 2026-11-06 (upstream snapshot SHA
`317cb86cc60d6286fa80a02743e94e6a60ff842d`).

The same radio/stack work originated with Henri David (Optimist,
https://github.com/w0ts/optimist). The original SPDX headers are
retained: `GPL-3.0-only OR MPL-2.0` for these BLE sources.
SLOOP-BT is GPL-3.0-only, so the BLE source is used under its GPL
option. Do not remove licence/copyright notices.

The radio hardware reference files in `hal/` and `rf_share.h`
were copied separately from `w0ts/optimist` with their original
dual-licensing headers. A final implementation must pin an exact
upstream revision and verify all HAL API differences.

## Status and experimental compile

- Host tests compile and test the imported BLE-MIDI codec, advertisement
  parser, and central GATT/LL state machine (no actual radio).
- `firmware/src/sloop_bt_open_measure.c` currently uses a **fake**
  baseband driver and `BLE_CENTRAL=3` (minimal, no BLE pairing).
- `SLOOP_BT_OPEN_MEASURE=1` causes `tools/build.py` to use an
  intentionally overlarge *diagnostic-only* XIP region. It deletes
  `felucca.bin` and every `.fwsc` and never produces an installer.
  Do not install or distribute this diagnostic image.
- SLOOP's default build remains unchanged, and the real SMC mixer
  decoder remains separate. Once Optimist BLE is selected, the old
  Hortator wire codec should be removed to avoid duplicating code.

## Remaining hard work

1. Hook in FM1's physical BLE radio peripheral, IRQ 29 (RX) and 45
   (connection event) with priorities below audio.
2. Port actual RF calibration and the radio's baseband SRAM window
   safely to SLOOP's existing memory layout.
3. Connect a bounded interrupt-to-main MIDI queue and retryable
   outbound SMC LED queue.
4. Integrate scan list, connection and disconnect events into the
   existing SLOOP-BT menu and controller state.
5. Confirm SMC-Mixer BLE DAW mode does **not** require encryption:
   the `BLE_CENTRAL=3` minimal stack cannot pair. Use full central
   level 1 if pairing is required (adds roughly 8.5 KB over minimal).
6. Measure full target image including real driver/RF data tables;
   only release a flashable package if the existing OTA, audio,
   RAM/pool and recovery checks pass.

The open protocol/central code alone is NOT wireless Bluetooth support.
