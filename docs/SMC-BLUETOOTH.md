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

- `smcbt_reset`: initialize all eight physical strips from SLOOP's current levels.
- `smcbt_level_changed`: invalidate pickup after a firmware-side level change.
- `smcbt_midi`: decode an already-received 3-byte MIDI message into
  track, transport, button and knob actions.
- Strips 1-3 target SLOOP synth tracks 1-3; strip 4 targets the drum-bus `G_DRLVL`; strips 5-8 target individual drum lane gains. The physical master fader is ignored.
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
   `transport_req`, and MIDI Learn facilities. For physical strip 4, use `G_DRLVL` rather than `P_LEVEL`; SLOOP's existing drum UI already uses `G_DRLVL`. Add separate lane gains for strips 5-8, applied by voice to sampled AND synthesised drums, initially unity (127) and with a short gain ramp to avoid clicks. Keep the original DSP path unchanged when Bluetooth is disabled.
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

## SMC-Mixer eight-fader layout

| Fader | Target | Default |
| --- | --- | --- |
| 1 | Synth track 1 level | SLOOP track 1 |
| 2 | Synth track 2 level | SLOOP track 2 |
| 3 | Synth track 3 level | SLOOP track 3 |
| 4 | Drum bus level | SLOOP `G_DRLVL` |
| 5 | Drum lane A level | KICK, lane 0, GM note 36 |
| 6 | Drum lane B level | SNARE, lane 2, GM note 38 |
| 7 | Drum lane C level | HAT, lane 4, GM note 42 |
| 8 | Drum lane D level | CLAP, lane 3, GM note 39 |

The four drum lanes are *sound lanes within SLOOP's existing fourth
track*, not four new sequencer tracks. Eventually each of the four may
be reassigned to any of SLOOP's sixteen named drum sounds. Mute, solo,
select and record buttons for these four strips need specifically
designed semantics; DO NOT index `trk[4..7]` (it contains only four
tracks). Initial fader decoding emits `SMCBT_DRUM_LEVEL` and awaits a
drum-specific handler.

Fader pickup is always reset when a link establishes or a value is
changed using the FM1's own controls, so connecting the physical
controller does not abruptly move the mix.

## Bluetooth menu (planned, not implemented)

Entry: **HOME held > BLUETOOTH**. Keep the radio off by default until
enabled, with safe recovery from a failed start. Proposed page:

- **BLUETOOTH**: OFF / ON (OFF at boot unless AUTOSTART enabled)
- **AUTOSTART**: OFF / ON
- **SCAN**: discover BLE MIDI controllers, prioritising SMC devices
- **DEVICE**: select/pair SMC-Mixer or forget saved device
- **STATUS**: OFF / SEARCHING / CONNECTED / LOST / FAILED
- **RECONNECT**: AUTO / SLOW / FAST (avoid scan spikes while audio plays)
- **DRUM A/B/C/D**: assign one of the 16 SLOOP drum lanes to each fader
- **KNOBS**: parameter preset / MIDI LEARN (once basic controls work)

After pairing, display a small BT indicator in the existing top bar.
The Bluetooth page must never block real-time audio processing.

No menu entries exist in the current firmware yet. The menu is gated
on a successful BLE port and will be implemented separately.
