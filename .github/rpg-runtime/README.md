# Caprice32 EmulatorJS candidate

This local fork starts at EmulatorJS/libretro-cap32 commit
310cc579b79b6051b378b192224b325a73437c9b. The Emscripten header declares
`fill_pathname` with the size_t return type of the pinned RetroArch linker.
The upstream void declaration makes the static Wasm link insert a signature
trap when a DSK image is added. Native shared-library builds retain the older
bundled libretro-common declaration.

Run `python3 .github/rpg-runtime/test-path-abi.py` before the explicit PFB core
build. It needs no ROM or BIOS and fails with the unpatched declaration.
`build-candidate.sh` snapshots the current worktree, uses the fixed emsdk image
and RetroArch commit, detects source edits during compilation and emits the
core, notices, complete core sources and an identity/digest descriptor.

Local candidates remain unpublished until explicitly promoted. Before any release, verify ordinary
Retrom import, review preview, gameplay, nonempty snapshot, fresh-Launch
restore, input after restore, and exit. Do not publish test games or firmware.
The source repository URL is the intended downstream identity; no remote fork,
maintenance branch or release is created by the candidate builder.

Formal publication uses `build-release.py --output <absolute-empty-directory> --tag <tag>`.
The workflow runs the same native regression and pinned Web build for PRs and tags.
Only annotated tags reachable from the declared maintenance branch may publish;
release assets include the complete core source archive, license and integrity report.


GX4000 / CPC Plus libretro snapshots append a fixed-size `CPPLUS02` extension
containing explicit little-endian ASIC fields, sprite and register memory,
DMA progress, palette, cartridge mapping and split-screen state. Loading a
legacy Plus SNA without this extension is rejected before resetting the core:
its missing ASIC state cannot be recovered. Ordinary CPC snapshots and SNA
file import/export keep their existing format. The source snapshot includes
`test-plus-snapshot.py`, which exercises real native libretro save/restore,
truncation/unknown-format rejection before mutation, and ordinary CPC
compatibility without external ROM or BIOS files. The pinned web candidate
build runs both this regression and the path ABI regression.
