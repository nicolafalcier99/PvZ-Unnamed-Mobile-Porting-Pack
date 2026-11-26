# PvZ Unnamed for phones

Play the Chinese *Plants vs. Zombies* mod **"Unnamed"** (植物大战僵尸无名版) 1.3beta2 on Android, with a layout made for
touch screens. Built on [PvZ-Portable](https://github.com/wszqkzqk/PvZ-Portable). You bring your own copy of the game.

## Install on Android

**You need:** an Android phone (64-bit, Android 7 or newer) and the game files from your own copy of
*Plants vs. Zombies: Game of the Year Edition* (Steam or EA).

1. **Get your game files.** On your computer, open the game's folder (Steam: right-click the game, then
   **Manage → Browse local files**). Put `main.pak` and the `properties` folder into one ZIP file and copy it to
   your phone.
2. **Install the app.** On your phone, download `PvZ-Unnamed-<version>-arm64.apk` from
   [Releases](https://github.com/nicolafalcier99/PvZ-Unnamed-Mobile-Porting-Pack/releases/latest) and open it.
   If Android asks, allow installing apps from your browser or file manager.
3. **Add the game files.** Start the app. It asks for your game files: pick the ZIP from step 1.
4. **Turn on the mod.** In the game, open **Options**, turn on **Unnamed Mode**, and restart the app.

That's it: you play the mod's rules (its plants, zombies, levels and modes) with the normal game's look.

**Optional: the mod's own art and music.** If you have the Unnamed 1.3beta2 ZIP from the mod's authors, open
**Options** from the main menu, tap **Import Unnamed ZIP** and pick it, then close and reopen the app. Only its pictures, sounds and music are copied; nothing
from it is run. Turn on **Chinese Text** in Options if you want the mod's Chinese text too.

The app installs next to upstream PvZ-Portable and doesn't touch its saves.

## Credits

- **PvZ-Portable** by wszqkzqk (Zhou Qiankang) and its contributors, licensed under LGPL-3.0-or-later. This fork
  keeps that license (`LICENSE`, `COPYING`) and all upstream copyright notices.
- **The Unnamed mod** is the work of its authors. Its official channel is the Tencent Channel
  "植物大战僵尸无名版" by JFY. The mod's in-game credits name HwequentBrony as mod author and 伴久九技高一筹 as an
  Almanac contributor. This port re-implements the mod's rules from the released 1.3beta2 game; it contains none of
  the mod's files.
- *Plants vs. Zombies* and all original game assets are the property of PopCap Games and Electronic Arts.

This project is **not affiliated with or endorsed by** PopCap Games, Electronic Arts, the PvZ-Portable project or
the authors of the Unnamed mod.

## What is included, and what isn't

This repository and the app hold code and the mod's rule numbers only. There are no images, sounds, music, fonts,
reanimations or `main.pak`, from EA or from the mod, which is why you add your own game files.

None of the mod's own text is included either (Almanac descriptions, tooltips, Crazy Dave's lines, level messages,
button jokes), nor our English translations of it, until the mod's authors agree. So:

- With **Chinese Text** on, the game reads the mod's text from your imported ZIP, so it looks like the mod.
- In English, the normal PopCap text is shown where the mod changed it, and a few messages in the mod's levels and
  modes use short English lines built into the code. Plant, zombie and mode names are kept, and the 26 mod-only
  plants have Almanac pages and hints written for this port.

The game has two modes: **Vanilla** plays exactly like upstream PvZ-Portable; **Unnamed** loads the mod's rules from
the data pack in [PvZ-Unnamed-Mobile-Porting-Pack-Data](https://github.com/nicolafalcier99/PvZ-Unnamed-Mobile-Porting-Pack-Data),
which is already inside the APK.

## Where the app keeps its files (Android)

- Game files: `Android/data/io.github.pvzunnamed.port/files/`
- The mod's art and music: `files/mods/unnamed/assets/`; the data pack: `files/mods/unnamed/`
- Optional HD art packs you make yourself: `files/hd/` and `files/hd-unnamed/`

The ZIP import is also reachable by long-pressing the app icon and opening **Manage Data**.

## For developers: build it yourself

Clone both repositories side by side; the build scripts look for the data pack in `../pvz-unnamed-pack`:

```sh
git clone https://github.com/nicolafalcier99/PvZ-Unnamed-Mobile-Porting-Pack.git pvz-unnamed
git clone https://github.com/nicolafalcier99/PvZ-Unnamed-Mobile-Porting-Pack-Data.git pvz-unnamed-pack
cd pvz-unnamed
```

**macOS** (Homebrew; installs `cmake jpeg-turbo libopenmpt libpng ninja sdl3 sdl3_mixer` if missing):

```sh
scripts/pvzu-build-mac.sh
SDL_AUDIO_DRIVER=dummy build-mac/pvz-portable -resdir <folder with main.pak> -pvzu-pack ../pvz-unnamed-pack -pvzu-mode unnamed
```

On the Mac, put `main.pak` and `properties/` next to the binary or pass `-resdir <folder>`. The pack goes in with
`-pvzu-pack <path>` or in `~/Library/Application Support/io.github.wszqkzqk/PvZPortable/mods/unnamed/`. Import the
mod's art and music with `python3 -I tools/pvzu_import_unnamed.py <unnamed.zip> <pack folder>/assets` (asset folders
only, nothing executable). Switch modes in **Options → Unnamed Mode**, or start with `-pvzu-mode unnamed`.

**Android** (arm64 APK): needs a JDK 21 (`JAVA_HOME`, default Homebrew `openjdk@21`), the Android SDK
(`ANDROID_HOME`, default `~/Library/Android/sdk`) with an NDK, and [vcpkg](https://github.com/microsoft/vcpkg)
(`VCPKG_ROOT`, default `~/dev/vcpkg`). The script bundles the data pack found at `PVZU_PACK_DIR` (default:
`../pvz-unnamed-pack`) into the APK.

```sh
PVZU_PACK_DIR=../pvz-unnamed-pack scripts/pvzu-build-android.sh   # -> dist/pvz-unnamed-arm64.apk (debug-signed)
```

Other platforms build as described in [README-PvZ-Portable.md](README-PvZ-Portable.md) (the original PvZ-Portable
README, which also covers saves and command-line options); only macOS and Android are tested for this fork.

### Tests

The port's rules are pure functions with [doctest](https://github.com/doctest/doctest) unit tests in `tests/`. After
the Mac build:

```sh
PVZU_PACK_DIR=../pvz-unnamed-pack ./build-mac/tests/pvzu_tests
python3 -m unittest discover -s tools/tests -t .
```

Tests that read the pack look for `PVZU_PACK_DIR` (default `~/pvz-unnamed-pack`).

### Layout

- `src/Mod/`: the port. `src/Mod/Unnamed/` holds the mod's rules as pure functions, `src/Mod/Engine/` the bridges
  into the engine. Engine edits outside `src/Mod/` are small and marked `// PVZU:`, each behind a pack feature, so
  Vanilla mode stays the same as upstream.
- `PVZU.md`: developer notes (test flags, hidden test runs, HD packs).
- `scripts/pvzu-*.sh`: Mac and Android builds; `tools/pvzu_import_unnamed.py`: the desktop importer.

## License

LGPL-3.0-or-later, as upstream (see `LICENSE` and `COPYING`). Vendored: doctest (MIT), nlohmann/json (MIT), the
Gradle wrapper (Apache-2.0).
