#!/usr/bin/env python3
"""Check the Caprice32/EmulatorJS static-link ABI without ROM or BIOS bytes."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = r'''#include <stddef.h>
#include <file/file_path.h>
typedef size_t (*frontend_fill_pathname)(char *, const char *, const char *, size_t);
_Static_assert(__builtin_types_compatible_p(__typeof__(&fill_pathname), frontend_fill_pathname),
               "EmulatorJS RetroArch fill_pathname must return size_t");
int main(void) { return 0; }
'''
with tempfile.TemporaryDirectory(prefix="cap32-path-abi-") as directory:
    test = Path(directory) / "signature.c"
    test.write_text(source)
    subprocess.run(["cc", "-std=c11", "-Werror", "-D__EMSCRIPTEN__",
                    "-I", str(root / "libretro-common/include"), "-fsyntax-only", str(test)], check=True)
print("Caprice32 static path ABI: PASS")
