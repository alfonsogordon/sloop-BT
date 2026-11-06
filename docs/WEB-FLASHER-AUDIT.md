# SLOOP-BT web flasher: release gate and verification

**Status: hardware radio UNTESTED; developer-only SWD recovery required.**
Do not describe this as verified safe for arbitrary FM1 hardware.

## Dedicated web route

Source `flasher/lab.html`, hosted as `/lab/` by
`.github/workflows/publish-flasher.yml`.

The lab route uses SLOOP's known updater protocol
`web/fm1ota.js`, but it adds important restrictions:

1. The **exact firmware filename**, 610076-byte package size,
   **FM-1_900** product marker, and SHA-256
   `96b192869ca45abdcb364ca1f6fbaf89819776553a0df2b330a5c238f6fe2ab1`
   are all checked. No foreign, modified or arbitrary FM1 package can
   be selected through the lab page. A matching SHA-256 confirms
   package integrity, not hardware safety.
2. A **read-only Web MIDI identity probe** must find a running
   `FM-1_9xx` device; stock, foreign or OTA-loader identities are
   blocked. The page will not blindly resume an interrupted write.
3. The user must confirm verified SWD recovery and backups,
   accept unverified Bluetooth RF risk, approve a confirmation dialog,
   and type **FLASH UNTESTED** before a write command is dispatched.

Client guards are verified by `tests/lab_flasher_test.mjs`. These
simulations cannot prove that a real FM1 will boot or transmit BLE.

## Firmware integrity and simulated OTA checks

`.github/workflows/lab-firmware.yml` refuses to publish a new
candidate unless all checks pass:

- Target JieLi application and OTA loader compiled with physical
  flash map, RAM and pool limits, ROM references and HAL/MMIO checks.
- An independent Python parser checks the exact built file digest,
  bytes, product identity, embedded loader and logical layout.
- A second JavaScript parser checks the product, bytes, digest and
  logical image independently in browser-compatible code.
- `tests/install_test.py` simulates updates, disconnect and resume
  conditions; `web/test_web.mjs` runs browser updater and UI tests.
- Existing host tests cover the SMC protocol and the Optimist BLE
  central driver. Engine output reference tests under GCC and Clang
  `-Oz` have passed.

The lab package remains labelled UNTESTED and stays in a GitHub Actions
artifact; it is never pushed as a normal release or auto-flashed.

## Hosting blocker

The repository's GitHub Actions token has Pages deployment permission
but **cannot create a Pages site for the repo**. The owner must enable
GitHub Pages once:

1. Open https://github.com/alfonsogordon/sloop-BT/settings/pages
2. Set **Build and deployment → Source → GitHub Actions** and save.
3. Re-run **Publish SLOOP-BT Web Flasher** under the Actions tab.
4. Confirm the deployment succeeded; only then use
   `https://alfonsogordon.github.io/sloop-BT/lab/`.

The workflow uploads the site as an ordinary Actions artifact even
when Pages activation fails. That artifact is an HTML bundle, **not a
working HTTPS Web MIDI site** if opened using `file://`.

## Hardware safety limits

No access to the user's physical FM1, original SWD dump, verified
recovery setup or real SMC-Mixer connection is currently available
here. Radio startup, RF calibration, MIDI latency and live audio
performance remain unverified; the candidate could hang or require
SWD recovery. Only the user, with their recovery equipment and
hardware, can validate those properties. Never promise otherwise.

The full test plan is in `docs/LAB-TEST.md`.
