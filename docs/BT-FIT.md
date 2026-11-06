# SLOOP-BT firmware fit investigation

This is a **feasibility gate**, not proof of a completed Bluetooth build.
We must not distribute a .fwsc until a compiled SLOOP + BLE image
passes these checks and physical FM1 tests.

## Device limits and prior measured figures

* SLOOP `tools/build.py` uses the FM1 application slot
  `APP_SLOT = 581,564 bytes` (original SLOOP package builder).
* SLOOP `RAM.md` cites a **historical** dual-core image of
  `576,564 bytes`, just **5,000 bytes free in XIP**, measured before
  subsequent 2.8.x changes. This is NOT a current branch build.
* The same RAM investigation cites `.data+.bss = 58,220/98,304`
  (40,084 free) and `.pool = 333,900/344,064`
  (**10,164 free**). These are historical, not measured for our branch.
* Hortator `docs/ble/FIT.md` reports a prior optimised BLE library
  link (`LTO`) reducing the BT library contribution from ~418 KB
  to about **104 KB**, plus BLE glue. This is a different firmware
  base and cannot simply be subtracted or copied as an exact SLOOP cost.
* Hortator's current `firmware/src/ble/ble.c` reserves a
  **23,552-byte** Bluetooth heap (for its more capable two-link setup);
  there are also cooperative task stacks, queue buffers and radio
  state. Historic Hortator estimates of RAM fit do not prove SLOOP fit.

**Conclusion:** copying Hortator's full BLE option directly into
current SLOOP is very unlikely to fit the application slot and would
also exceed historical free pool memory. An optimised build variant,
measured size report and potential removal/restructuring of *optional*
non-audio facilities are required. Do not alter the sound or silently
discard the current synth/drum engines.

## Implementation path

1. Establish the exact **current** SLOOP XIP, RAM, pool, lowram and
   CPU budgets with an unmodified build from this fork, using the
   pinned JieLi toolchain and SDK. Preserve artifacts and map file.
2. Create a SLOOP-specific **host-only, one-controller** BLE variant
   using Hortator's central MIDI stack as reference, without FM-Link
   and without the FM1-as-peripheral role. Pin the SDK ABI and hashes.
3. Measure with an enlarged *diagnostic-only* linker map as Hortator
   did. It MUST NOT emit a flashable package when over budget.
4. Compare footprint of 1-link BLE stack with app, audio, editor,
   10-channel USB audio and optional diagnostics separately.
   Reclaim bytes conservatively with opt-in variant flags; no sound
   changes, no change to rescue/OTA or saved project formats.
5. Bring up the radio only after an explicit menu action, never by
   default, and ensure radio calibration and flash layout are safe.
6. Scan, connect and subscribe to the SMC-Mixer MIDI characteristic
   (UUID 7772E5DB-3868-4112-A1A9-F2669D106BF3).
7. Deliver decoded MIDI packets to
   `sloop_bt_receive_midi(status,data1,data2)` **on the main loop**.
8. Route LED feedback through `smcbt_led_next`, and acknowledge
   with `smcbt_led_sent` only after BLE accepts the outgoing message.
9. Implement HOME-held > BLUETOOTH controls, saved-device settings
   and tests of scan failures, reconnect and forced radio stop.
10. Build, regression-test, bench audio under full polyphony and
    mixer movements, then flash on actual FM1 with recovery equipment.

## Non-negotiable release checks

- image <= APP_SLOT and no region overflows
- no missing library ABI or SDK pins
- unchanged OTA / rescue path, verified loader
- Bluetooth disabled by default, safe after failed radio startup
- no audio underruns and no excessive BT IRQ masking under load
- preserved projects, presets and samples after a downgrade
- hardware pairing and reconnection with an actual SMC-Mixer

The current GitHub Pages flasher allows user-selected known-good
packages but contains **no Bluetooth firmware release**.

## Actual GitHub Actions target builds, 6 November 2026

The `Experimental FM1 firmware compile` job now builds with a fresh
JieLi compiler and AC79 SDK on GitHub Actions. This is a **real target
compiler/linker pass** (not the host tests); the resulting flash
packages are deliberately not published.

| Variant | XIP image | APP_SLOT free | .data+.bss | Pool |
| --- | ---: | ---: | ---: | ---: |
| Original SLOOP, BT flags 0 | 571,820 B | 9,744 B | 59,340 B | 333,900 B |
| SMC controls + menu + drum mix, NO BLE radio | 573,096 B | 8,468 B | 59,628 B | 333,900 B |

The memory figures are from GitHub Actions run 42241395925. Both
configurations passed the slot, RAM, lowram and pool linker checks.
**These builds do not include the JieLi Bluetooth stack or establish a
wireless connection.** Do not label or distribute them as SLOOP-BT
firmware.

The symbol report shows the largest functions include `ui_draw`
(~35 KB), `ui_input` (~14.5 KB), `events_block` (~11 KB),
`mix_block` (~11 KB), `ed_handle` (~10.7 KB). This is a first
direction for investigating footprint, not approval to remove
anything: audio DSP, OTA/recovery and projects must remain functional.

Hortator's previous BLE-optimised builds add approximately 125 KB
relative to its own baseline, far more than SLOOP's current 8.5 KB of
XIP headroom. The radio integration therefore needs a substantial
size/architecture redesign. A mere copy of the Hortator radio files
is not a safe or successful FM1 build.
