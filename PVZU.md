# PVZU: fork notes

Fork of [PvZ-Portable](https://github.com/wszqkzqk/PvZ-Portable) (LGPL-3.0) for the S26 Ultra test phone:
full-screen 19.5:9, touch fit, a mod-pack system, and an "Unnamed" mode (data lives in the separate repo
`pvz-unnamed-pack`).

Rules
- Never commit game files: no `main.pak`, images, sounds, exe/dll (EA or Unnamed). See `.gitignore`.
- Vanilla mode must play exactly like upstream.
- Our code lives in `src/Mod/`; engine edits are marked `// PVZU:`.

Build
- Mac (testing): `scripts/pvzu-build-mac.sh` → `SDL_AUDIO_DRIVER=dummy build-mac/pvz-portable -resdir <folder with main.pak>`
  **Always launch test runs silent (`SDL_AUDIO_DRIVER=dummy`)**.
  **Test runs are invisible**: any run with -pvzu-shot/-input/-quit-frame/-level/-quit-after/-dump-tables/-autoplay,
  `-pvzu-hidden`, or env `PVZU_HIDDEN=1` opens a hidden, windowed window (never full screen, never a new desktop).
- Tests: `ctest --test-dir build-mac --output-on-failure`
- HD art (private, never in git): `python3 -I scripts/pvzu-hd-prepare.py [out]` builds a pack
  (default `pvz-game-files/hd-pack-2x`) from the 4x upscales; Mac `-pvzu-hd <pack>`, Android `<files dir>/hd/` with
  hd.json; `"hd": false` in pvzu-settings.json turns it off. Checks: `-pvzu-log-hd` (texture MB per group, live/peak),
  `-pvzu-window-px 2340x1080` (phone-size hidden capture). Engine side: `src/Mod/HdArt.h`, `src/Mod/Engine/HdTextures.h`.
  Unnamed mode adds a second pack for the art the overlay serves: `scripts/pvzu-hd-prepare.py --unnamed` builds
  `pvz-game-files/hd-unnamed-2x` (layout + U2's SeedVR2 hand-off in its README.md); Mac `-pvzu-hd-unnamed <pack>`, Android
  `<files dir>/hd-unnamed/`. Its hd.json `samePc` lists Unnamed images identical to PC (they reuse the PC pack's twin).
- Android: `scripts/pvzu-build-android.sh` → `dist/pvz-unnamed-arm64.apk` (app id `io.github.pvzunnamed.port`)
