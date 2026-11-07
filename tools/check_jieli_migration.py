#!/usr/bin/env python3
"""Fail closed on missing prerequisites for the experimental JieLi BLE port.

Static checks only: this is NOT a firmware build or a hardware safety certification.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = (ROOT / "tools/build.py").read_text()
LINK = (ROOT / "firmware/app.ld").read_text()
LAB = (ROOT / ".github/workflows/lab-firmware.yml").read_text()
DOC = (ROOT / "docs/JIELI-BT-MIGRATION.md").read_text()

def check(ok, message):
    print(("PASS" if ok else "FAIL") + ": " + message)
    return bool(ok)

checks = [
    check("SLOOP_BT_OPEN_MEASURE" in BUILD and "optimist_ble" in BUILD,
          "current experimental Optimist build path identified"),
    check("felucca.o" in BUILD and "pi32v2/bin/ld" in BUILD,
          "current single-unit compilation/link architecture identified"),
    check('FELUCCA_BLE' not in BUILD,
          "JieLi Bluetooth build option is NOT yet integrated"),
    check(re.search(r"POOL\s*\(rw\).*LENGTH\s*=\s*0x54000", LINK),
          "pool linker region is present (capacity only; free space unverified)"),
    check("AC79NN_SDK_V1.2.1_2023-12-13" in LAB,
          "lab workflow still pins SDK v1.2.1"),
    check("V1.2.0" in DOC and "23,552" in DOC,
          "migration records SDK compatibility and BLE heap requirements"),
]
print("JieLi BLE migration: NOT FLASHABLE; port and hardware checks outstanding.")
sys.exit(0 if all(checks) else 1)
