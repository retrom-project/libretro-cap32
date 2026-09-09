# Retrom Caprice32 local fork

Keep `master` as the upstream mirror and develop on `fix/*`, `feat/*` or
`build/*` branches from the baseline in `retrom-fork.json`. The intended
maintenance branch and repository are not published by local builds.

Core source and build recipes belong here. Never add Retrom host API calls,
private games or firmware. Run the path ABI regression before building through
Retrom's explicit `pfb-core-build`; preserve the pinned toolchain/linker and
source-digest checks. Verify gameplay and cross-Launch state restoration in the
actual Retrom PFB before proposing publication. Do not push, create releases or
move tags without user authorization.
