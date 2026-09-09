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

This is an unpublished local candidate. Before any release, verify ordinary
Retrom import, review preview, gameplay, nonempty snapshot, fresh-Launch
restore, input after restore, and exit. Do not publish test games or firmware.
The source repository URL is the intended downstream identity; no remote fork,
maintenance branch or release is created by the candidate builder.
