# JieLi Bluetooth migration investigation (not flashable)

Goal: replace the experimental Optimist WL82 radio path with the JieLi SDK Bluetooth host path used by Hortator, to support the M-VAVE SMC-Mixer.

## Confirmed source comparisons
- Hortator `DeadActive/hortator`: `firmware/src/ble/{ble_port.c,ble_central.c,ble_os.c,ble_rf_tables.c,ble_vm.c,ble_midi.c}`, `firmware/hal/fm1_ble_hal.h`, `firmware/src/smc.c`, `tools/ble_libs.py`.
- Hortator's Bluetooth path links proprietary/prebuilt **JieLi AC79** SDK bitcode archives (`btctrler.a`, `btstack.a`, `wl_rf_common.a`, crypto and support archives); it is **not** a drop-in C source replacement for Optimist.
- `tools/ble_libs.py` pins SDK **V1.2.0** archive hashes and builds SDK configuration bitcode.
- Current SLOOP laboratory CI downloads JieLi SDK **V1.2.1_2023-12-13**, then builds the Optimist stack. This mismatch must be resolved before linking Hortator's code.
- Hortator's `docs/superpowers/plans/2026-10-10-midi-bt-smc.md` requires manual user activation of BLE, no flash writes, audio IRQ priority preservation, and target-build safety gates.
- `charlesvestal/fm1-base` `device/radio.c` is a second reference for JieLi-stack initialization and crash-safe startup.

## Required integration before hardware release
1. Obtain and hash-check the exact compatible SDK libraries (including matching bitcode/toolchain); do not copy binary libraries without checking their licence and provenance.
2. Build a separate SLOOP JieLi adapter and map radio HAL, IRQs, OS shims, timing, BLE-MIDI central and SMC control into SLOOP's UI and audio runtime.
3. Ensure mutually exclusive Optimist/JieLi linkage; keep the current firmware/flasher untouched until replacement passes.
4. Run host tests, target link/build and independent package checks; confirm no flash writes or startup auto-enable.
5. Only then publish an explicitly experimental hardware test with recovery path.

**Status:** investigation only. No JieLi Bluetooth implementation has been linked or hardware-tested in SLOOP.
