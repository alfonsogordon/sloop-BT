# SLOOP-BT Optimist LAB: FM1 hardware test checklist

**EXPERIMENTAL / UNTESTED RADIO CODE. NOT A NORMAL RELEASE.**
Do not flash until you have a tested recovery method and a complete
backup of your FM1's projects and presets. Radio startup and Bluetooth
connection have not been tested on the physical FM1 by the maintainers.

## Build and availability

- Source branch: `feature/smc-ble`.
- GitHub Actions workflow: `UNTESTED SLOOP-BT hardware lab firmware`
  (Actions run started at 2026-11-06T21:xx UTC).
- The build must pass the **physical**, NOT expanded diagnostic,
  linker, flash-image, memory-pool, ROM-reference and source HAL checks
  before it attaches `sloop-bt-optimist-lab-UNTESTED.fwsc`.
- CI also tests the SMC protocol and Optimist central code, and
  performs separate GCC and Clang `-Oz` golden-audio host checks.
- The file is an Actions artifact with a seven-day retention period,
  **not** a GitHub release or automatic web-flasher download.
- If the job fails there will be no firmware file, only logs.

## Before even considering flashing

1. Verify the SWD/RP recovery programmer really works with your
   hardware and retain a known-good original firmware backup.
2. Export project/preset data through the existing SLOOP web editor.
3. Keep a working USB cable and desktop browser (Chrome/Edge) ready
   for known-good SLOOP OTA recovery.
4. Never install the package on a device needed for a live performance.
5. Be prepared to restore the known-good firmware if it stops booting.
   This is **not** verified on real hardware; you assume recovery risk.

## Controlled first boot

- On power-up Bluetooth is OFF by default. Play normal synths and
  drums with BT OFF before entering the Bluetooth menu.
- Open the SLOOP settings/menu, navigate to **BT**:
  BLUETOOTH / SCAN / DEVICE / STATUS / RECONNECT.
- Turn BLUETOOTH ON manually. The radio will attempt the experimental
  V15 RF calibration and then initiate BLE MIDI scanning.
- Put the M-VAVE SMC-Mixer into its compatible BLE DAW/MIDI mode.
  The device must advertise BLE-MIDI as expected by Optimist's stack.
- Start SCAN or RESCAN, turn DEVICE to the appropriate entry and
  press to connect. Only one peripheral at a time is supported.
- Read STATUS. If it reports PAIR REQUIRED, this minimal central
  firmware lacks pairing and cannot be made to work by repeatedly
  trying: stop and report the result.
- If connected, test all eight faders, mute/solo/record/select,
  PLAY/STOP, LED feedback and all 16 drum lane assignments.
- Test audio stress (full chord polyphony + drums + mixer moves),
  reconnect after a mixer power cycle, and BT OFF/ON once.
- After testing, export a backup and restore stable SLOOP if the
  audio, project data, LED response or connection is unreliable.

## What to report

A photo or short video of the Bluetooth STATUS/menu, the SMC-Mixer
operating mode, whether it appears in SCAN, whether it reaches
CONNECTED, and any audio glitches or control issues. Include the
GitHub Actions lab build run and SHA if possible.

## Known limits in this first lab edition

- The BLE radio driver was successfully compiled for the FM1 target,
  but startup calibration, IRQ timing and sound under real radio
  traffic have not been hardware verified.
- Bluetooth is OFF at boot to preserve basic usability after a
  reboot. There is no radio auto-start on reboot.
- `BLE_CENTRAL=3` is a *no pairing* central. A controller requiring
  pairing/encryption cannot connect until we implement full central.
- The test uses a temporary static random Bluetooth address rather
  than a saved device-unique identity: this **must** be replaced
  before any production release.
- Reconnect remembers the selected peripheral only while powered;
  settings are not persistent across FM1 restarts.
- The lab uses `-Oz`, which passed host audio golden tests but is
  not yet performance-verified on the actual FM1.
- No firmware package is automatically placed on the web flasher.

This test is for recovery-equipped development hardware only.
