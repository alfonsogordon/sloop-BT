# SLOOP-BT Web Flasher

This directory is the **standalone SLOOP-BT browser flasher**, with the
original SLOOP Web MIDI updater reused (no USB flashing protocol rewrite).

## What works in the source

- Choose a `.fwsc` file **from your computer**.
- Check its SLOOP/Felucca family marker (`FM-1_9xx`), file length and SHA-256.
- Show filename, checksum and product ID before proceeding.
- Require a backup/experimental-use confirmation and a second install prompt.
- Use the upstream `web/fm1pkg.js` parser and `web/fm1ota.js` updater.
- Resume a compatible interrupted SLOOP-family update and reject a
  foreign update-mode loader.
- Progress indicator, status, error log, links to original stable SLOOP
  installer, editor and hardware recovery information.
- Files remain in the browser and are not uploaded.

**Safety:** A valid family marker is NOT a cryptographic authenticity or
hardware compatibility guarantee. Flash only trusted, tested packages.
There is no SLOOP-BT Bluetooth release package yet.

## Hosting

Workflow: `.github/workflows/publish-flasher.yml`, publishing the
`feature/smc-ble` development branch's flasher to GitHub Pages.
It stages `flasher/index.html` and both original SLOOP Web MIDI
libraries into the static site artifact (no firmware package).
The deployment may require the repository owner to configure
**Settings → Pages → Build and deployment → Source: GitHub Actions**.
GitHub Pages deploy is independent of the firmware build.

If Pages is configured, the expected project URL is:

https://alfonsogordon.github.io/sloop-BT/

Do not claim it is live until the deployment finishes and the URL
has been checked.

## Browser compatibility

Chrome or Edge with Web MIDI/SysEx permission, from HTTPS.
Physical FM1 and a USB data cable required. No Web Bluetooth
permission is needed: SMC-Mixer pairing happens inside the
future FM1 firmware, **not** through the browser.

## Tests / milestones

- [x] Flasher page source and packaging workflow
- [x] Existing updater imported without protocol changes
- [ ] Successful GitHub Pages deployment and live URL verified
- [ ] Browser UI smoke test on HTTPS
- [ ] Real FM1 flash of a known-good stock SLOOP-family test package
- [ ] Working SLOOP-BT BLE build; package added only after hardware validation

SLOOP and Hortator GPL-3.0-only source licence notices are retained.
