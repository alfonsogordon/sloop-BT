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
tracks). The event decoder emits `SMCBT_DRUM_LEVEL` and the firmware adapter now routes it to the dedicated lane-mixer state when compiled with `SLOOP_BT_CONTROL=1`.

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

## Hortator-style button and LED layout (firmware adapter behind feature gate)

The original SMC DAW mode sends four eight-button rows. We keep their
physical meaning rather than inventing a new controller mode:

| Control | Strips 1-3 | Strip 4 | Strips 5-8 |
| --- | --- | --- | --- |
| Fader | Synth level | Drum bus level | Individual selected drum-lane gain |
| MUTE | Mute synth track | Mute drum bus | Mute that drum lane |
| SOLO | Solo synth track | Solo drum bus | Solo that drum lane (within drums) |
| SELECT | Select synth track | Select drum track | Select drum track and focus that sound lane |
| REC | Arm synth track | Arm drum track | Arm drum track; focus sound lane (not four independent recorders) |

Transport PLAY, STOP and REC map to SLOOP's normal transport,
and BANK left/right may move the focused strip/lane. Knobs 1-4
will edit the currently focused SLOOP parameters; knobs 5-8
will use the existing MIDI Learn support once integrated.
Unused buttons (rewind, fast-forward, arrow controls) remain unbound.

Hortator's LED note layout is implemented independently in
`firmware/src/smcbt_leds.h` (35 status notes: REC/SOLO/MUTE/SELECT
per strip, plus STOP/PLAY/REC). It only sends changes and resends all
states after a new connection. The transport adapter must confirm
a message was queued before marking it sent.

**Drum solo policy:** an individual lane solo must affect other
drum lanes without accidentally muting all synth tracks. Define
the precedence of the drum bus mute/solo and per-lane mute/solo
in the dedicated mixer bridge and make project restore safe.
The optional drum mixer implements runtime levels, mute and solo. Drum-lane settings are not yet saved in projects, and no BLE connection has been established on hardware.

**Important:** Even though `smcbt_midi` decodes all eight
button columns, its event's `track` field represents the *physical
strip*, not an index into `trk[]`. The integration bridge must
translate strips >=4 into drum-lane targets and not dereference
`trk[4]` or beyond.

## 2026-11-06 implementation checkpoint

Committed independent, host-tested control pieces:

- `smcbt.h`: SMC Mackie DAW message decoder, 8 faders and soft takeover.
- `smcbt_leds.h`: 35 changed-note LED messages with retry on full MIDI output.
- `sloop_bt_drums.h`: 16 sound-lane levels/mute/solo, 4 assignable strips.
- `sloop_bt_dispatch.h`: translation of physical strips into tracks 1-4
  and individual drum lanes, transport, select, REC and knob callbacks.
- `drums.c`: an optional `SLOOP_BT_DRUM_MIX=1` DSP hook after render
  for both sampled and synthesised drum voices, default OFF in all
  firmware builds. At default unity the hook returns exactly the same
  sample. Validate CPU and smoothing before enabling in hardware.

The dispatcher is now connected to SLOOP's live synth/drum track state through `sloop_bt_control.c`, guarded by `SLOOP_BT_CONTROL=1` and `SLOOP_BT_DRUM_MIX=1` (both default OFF). Hortator's BLE-MIDI wire codec is imported into `firmware/src/sloop_bt_ble/` and its notification callback is wired to that controller adapter. The radio driver, scanning, on-device pairing menu, LED transport, project persistence and full JieLi build are not yet present.

No verified FM1 BLE build exists at this checkpoint. The web flasher
is separate and can use known-good user-supplied SLOOP packages after
GitHub Pages is enabled; it does not ship unfinished Bluetooth firmware.

The drum DSP hook has since been changed to a Q15 smoothed per-voice gain with block-level target calculation, avoiding a per-sample 64-bit division. The host tests cover mixer control semantics, but this audio path still requires JieLi target compilation and hardware CPU/audio tests. See `docs/BT-FIT.md` for the larger BLE firmware-size and pool-memory obstacle.

## Latest host-tested pipeline

Hortator BLE-MIDI bytes -> `ble_midi_parse` -> decoded MIDI event ->
`sloop_bt_ble_notification` / `sloop_bt_receive_midi` ->
`sloop_bt_dispatch` -> SLOOP's actual tracks or drum-lane state.

The `tests/sloop_bt_control_test.c` test includes the **real firmware
adapter** with lightweight SLOOP state stubs. It tests a BLE packet
reaching the real track mute state, track selection, fader pickup,
transport and preexisting solo restoration on disconnect. The codec
also has independent BLE running-status and outbound packet tests.

Build flags (explicitly forwarded by `tools/build.py`):
```sh
SLOOP_BT_CONTROL=1 SLOOP_BT_DRUM_MIX=1 ./build.sh
```
This is **NOT a Bluetooth-capable firmware** yet. No BLE central
hardware driver or link-time BLE libraries are included by these flags,
and compilation on the actual JieLi target has not been verified.
The flags merely compile the controller/notification and optional drum
DSP glue for sizing and integration work.

## BLE link state and outbound feedback (latest)

- `firmware/src/sloop_bt_link.h` is a pure C single-controller
  connect model with radio OFF at boot, eight discovered devices,
  address-based selection, connection/error states and paced retry.
- A link loss retries at 2, 4, 8, then 16 seconds, and will attempt
  connection **only** when the remembered BLE address reappears.
- The model is not yet connected to the hardware BLE central driver
  or on-device menu; those must drive the intent API on the main loop
  and report real startup, scan and connection events.
- `sloop_bt_led_packet()` in the actual firmware adapter builds an
  outbound BLE-MIDI packet from the pending SMC LED state.
  `sloop_bt_led_queued()` marks the change sent only after the future
  radio output queue accepts it. No radio writes are currently present.
- The controller adapter and BLE-MIDI codec have host tests covering
  notifications, live mute/solo, faders, transport and LED packet
  retry. The GitHub workflow is a host simulation, **not** a target
  toolchain build or a hardware radio test.

### Required for an end-to-end hardware test

1. Adapt Hortator's `ble_os.c`, `ble_port.c`, `ble_central.c`,
   `ble_scan.c`, `ble_gatt.c` and relevant HAL/SDK configuration
   for a **single BLE central only**, pinned to its correct AC79 SDK.
2. Wire scan results and ATT/GATT link events into
   `sloop_bt_link.h`; execute scan/connect/disconnect intents.
3. Call `sloop_bt_control_connect` on MIDI subscription and
   `sloop_bt_control_disconnect` on link loss.
4. Pass BLE-MIDI GATT notifications to
   `sloop_bt_ble_notification`; enqueue
   `sloop_bt_led_packet` and only then mark `sloop_bt_led_queued`.
5. Connect on-device Bluetooth menus to the actual link state.
6. Confirm XIP/RAM/pool capacity and test audio under stress.
7. Release a browser-installable firmware package only after those
   gates are passed.

The web flasher's GitHub Pages workflow currently requires the repo
owner to enable GitHub Actions as the Pages source.
