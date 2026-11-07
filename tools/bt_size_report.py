#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Inspect largest target functions and symbols without changing the firmware."""
import os
import pathlib
import re
import subprocess

root = pathlib.Path(__file__).resolve().parent.parent
elf = root / "build" / "felucca.elf"
toolchain = pathlib.Path(os.environ.get("JIELI_TOOLCHAIN", "~/.jieli/toolchain")).expanduser()
objdump = toolchain / "common" / "bin" / "objdump"
if not elf.is_file():
    raise SystemExit(f"Missing {elf}")
output = subprocess.check_output([str(objdump), "-t", str(elf)], text=True, errors="replace")
symbols = []
sections = {}
for line in output.splitlines():
    # GNU objdump: ADDRESS [g/l/w] [F/O] SECTION LENGTH SYMBOL
    tokens = line.split()
    if len(tokens) < 6 or not re.fullmatch(r"[0-9a-fA-F]{8,16}", tokens[0]):
        continue
    section = next((x for x in tokens[1:-2] if x.startswith(".text") or
                    x.startswith(".rodata") or x.startswith(".data") or x.startswith(".bss")), "")
    if not section:
        continue
    try:
        size = int(tokens[-2], 16)
    except ValueError:
        continue
    if size <= 0:
        continue
    symbols.append((size, section, tokens[-1]))
    sections[section] = sections.get(section, 0) + size

print("SLOOP-BT target symbol footprint (symbol sizes, not section alignment/padding):")
print("Largest sections, counted by symbols:")
for section, size in sorted(sections.items(), key=lambda p: -p[1])[:16]:
    print(f"  {size:>8} B  {section}")
print("Largest 70 individual symbols:")
for size, section, name in sorted(symbols, reverse=True)[:70]:
    print(f"  {size:>8} B  {section:<18} {name}")
