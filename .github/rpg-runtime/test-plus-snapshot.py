#!/usr/bin/env python3
"""Exercise actual native core serialization without external ROM/BIOS files."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
subprocess.run(["make", "-j4", "platform=unix"], cwd=root, check=True)
objects = sorted(str(p) for folder in ("cap32", "libretro", "libretro-common")
                 for p in (root / folder).rglob("*.o"))
with tempfile.TemporaryDirectory(prefix="cap32-plus-state-") as directory:
    executable = Path(directory) / "snapshot"
    subprocess.run(["cc", "-std=c11", "-I", str(root / "cap32"),
                    "-I", str(root / "libretro-common/include"),
                    str(root / ".github/rpg-runtime/test-plus-snapshot.c"),
                    *objects, "-lm", "-o", str(executable)], check=True)
    subprocess.run([str(executable)], cwd=directory, check=True)
