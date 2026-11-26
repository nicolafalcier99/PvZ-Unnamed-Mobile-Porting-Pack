#!/bin/bash
# PVZU: desktop build for testing on the Mac (Homebrew deps, like upstream's macOS CI job).
set -euo pipefail
cd "$(dirname "$0")/.."
export HOMEBREW_NO_AUTO_UPDATE=1
for p in cmake jpeg-turbo libopenmpt libpng ninja sdl3 sdl3_mixer; do brew list "$p" >/dev/null 2>&1 || brew install "$p"; done
cmake -G Ninja -B build-mac -DCMAKE_BUILD_TYPE=RelWithDebInfo "$@"
cmake --build build-mac
echo "BUILT $(pwd)/build-mac/pvz-portable"
