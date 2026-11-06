# SLOOP-BT: SMC-Mixer integration

Work-in-progress branch for adding M-VAVE SMC-Mixer BLE MIDI control to SLOOP.

## Source and attribution

- SLOOP base: https://github.com/isod89/sloop-fm1 (GPL-3.0-only)
- Hortator BLE-MIDI host, SMC DAW map and soft pickup:
  https://github.com/DeadActive/hortator (GPL-3.0-only)
- `firmware/src/smcbt.h` is a small, standalone SLOOP-specific
  implementation derived from the SMC message mapping and pickup in
  Hortator's `firmware/src/smc.c`.
- Retain GPL-3.0-only notices and check third-party SDK distribution
  constraints before distributing firmware or copying binary libraries.

## Implemented, NOT wired into firmware

- `smcbt_reset`: initialize four logical strips from SLOOP's levels.
- `smcbt_level_changed`: invalidate pickup after a firmware-side level change.
- `smcbt_midi`: decode an already-received 3-byte MIDI message into
  track, transport, button and knob actions.
- The SMC's 8 DAW strips remain distinguishable; tracks 5-8 and master
  fader are intentionally ignored until we choose meaningful mappings.
- Standalone host regression test: `tests/smcbt_test.c`.

This branch does **not** yet enable the radio, connect a controller, modify
the SLOOP audio code, or produce a Bluetooth `.fwsc` package. Do not flash
this as a Bluetooth build.

## Next integration steps

1. **Establish the memory budget.** SLOOP's `firmware/src/felucca.c`
   currently defaults `FELUCCA_CDC=0` to save space. SLOOP's pool and
   lowram map also differ from Hortator's. Measure XIP, RAM, pool,
   stacks and audio timing with an actual BLE link; reject builds
   that exceed the available regions.
2. **Port BLE host as a separate build option.** Hortator's independent
   `firmware/src/ble/` unit, `ble_core.h`, `ble_port.c`,
   `ble_central.c`, radio calibration, BLE MIDI and JieLi SDK
   dependency/linker integration are the reference. Keep the radio OFF
   by default and the existing non-BLE build unchanged.
3. **Route BLE MIDI on the main loop.** Call `smcbt_midi` for
   SMC DAW packets and translate events into SLOOP's existing
   `trk[].p[P_LEVEL]`, `P_MUTE`, `song.solo`, `song.sel`,
   `transport_req`, and MIDI Learn facilities. For the drum strip,
   confirm whether to control `G_DRLVL` rather than `P_LEVEL`;
   its normal UI uses `G_DRLVL`.
4. **Feedback.** Send Mackie LED states (and pickup indication)
   without blocking the audio path, and reinitialise on reconnect.
5. **Build and hardware test.** Verify scanning `SMC-Mixer` in DAW
   mode, reconnect, audio under load, project saving, and stock
   firmware recovery. Run SLOOP's host regression/golden DSP tests.

## SMC DAW messages

- Faders 1..8: pitch bend, channels 1..8, 14-bit value mapped to 0..127
- Knobs 1..8: CC 16..23 on channel 1, sign-magnitude relative steps
- Buttons REC/SOLO/MUTE/SELECT: notes 0..31 (8 per row)
- PLAY/STOP/REC: notes 0x5e / 0x5d / 0x5f
- Buttons respond only to note-on with nonzero velocity

## Host tests

```sh
cc -std=c99 -Wall -Wextra -Werror tests/smcbt_test.c -o /tmp/smcbt_test
/tmp/smcbt_test
```

No physical FM1 pairing, full firmware compile or audio test has been
performed for this branch yet.
