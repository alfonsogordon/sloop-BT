#!/usr/bin/env python3
"""Verify the exact AC79 V1.2.0 Bluetooth SDK inputs used by Hortator.

No firmware is built and no device is accessed.
Hashes are from DeadActive/hortator tools/ble_libs.py, inspected 2026-11-07.
"""
import hashlib
import os
from pathlib import Path

PINS = {
    "cpu/wl82/liba/btctrler.a": "885cd53747753488b380f761318bf30d691f75541dd28adc72457690aa51506a",
    "cpu/wl82/liba/btstack.a": "d4c02f42e1be97a5288ecbcb5d42e6d0727e2ca4ffa2e0e2e6bb2ae61fede4bc",
    "cpu/wl82/liba/wl_rf_common.a": "1e6f7cdf0f0956282d251cdf23dffc2dae39c4ca498cb314801dde35b00e1769",
    "cpu/wl82/liba/crypto_toolbox_Osize.a": "373251c39f8de0851204b623601b348254023e98b3d0b062ecab743b0c3aa35e",
    "cpu/wl82/liba/system.a": "a71bdc75e5afbcffaf7d79bc2b27f2b65fec63d94d2261b584744d3e9b9098ea",
    "cpu/wl82/liba/cpu.a": "fe18129bfee7758074c486b2d33b2ed59a8c365600362d982992677c47821b2a",
    "cpu/wl82/liba/lib_ccm_aes.a": "bd3dd110197ae0084826538bfb5e432a6fadb3a001acadfdbd5f3a409fb5da2c",
    "include_lib/newlib/pi32v2-lib/libcompiler_rt.a": "141997ab6977c0c4abfb857bc32a897eb345f8d593e27e74c33bdebcf3fd5009",
    "apps/common/config/log_config/lib_btctrler_config.c": "7323d979d70af02110ada6d3a6eaf76ad5a1292ccbbb7ff30a51e9c667d28fe8",
    "apps/common/config/bt_profile_config.c": "1d385cbd6b07e1b8d87d7336debe51b34b414c1feae4366973b18468fac6f7f2",
}

def main():
    root = Path(os.environ.get("AC79_BT_SDK", os.environ.get("AC79_SDK", "")))
    if not str(root) or not root.is_dir():
        raise SystemExit("FAIL: set AC79_BT_SDK to an existing AC79 V1.2.0 checkout")
    failures = 0
    for name, expected in PINS.items():
        path = root / name
        actual = hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else "MISSING"
        good = actual == expected
        print(("PASS" if good else "FAIL") + " " + name + (" (hash mismatch or missing)" if not good else ""))
        failures += not good
    if failures:
        raise SystemExit(f"FAIL: {failures} SDK inputs mismatch Hortator's pinned V1.2.0")
    print("PASS: all ten pinned JieLi SDK inputs match Hortator")

if __name__ == "__main__":
    main()
