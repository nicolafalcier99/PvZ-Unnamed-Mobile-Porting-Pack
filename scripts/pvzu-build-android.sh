#!/bin/bash
# PVZU: arm64 APK build (debug-signed), like upstream's Android CI job. Output: dist/pvz-unnamed-arm64.apk
set -euo pipefail
cd "$(dirname "$0")/.."
export JAVA_HOME="${JAVA_HOME:-/opt/homebrew/opt/openjdk@21/libexec/openjdk.jdk/Contents/Home}"
export ANDROID_HOME="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
export ANDROID_NDK="${ANDROID_NDK:-$(ls -d "$ANDROID_HOME"/ndk/*/ | sort -V | tail -1)}"; ANDROID_NDK="${ANDROID_NDK%/}"
export ANDROID_NDK_HOME="$ANDROID_NDK" ANDROID_NDK_ROOT="$ANDROID_NDK"
export VCPKG_ROOT="${VCPKG_ROOT:-$HOME/dev/vcpkg}"
# PVZU: bundle the private Unnamed pack (numbers + English text only; never assets/) into the APK
PACK="${PVZU_PACK_DIR:-$(cd .. && pwd)/pvz-unnamed-pack}"
DEST=android/app/src/main/assets/pvzu-pack
rm -rf "$DEST"
if [ -f "$PACK/mod.json" ]; then
	mkdir -p "$DEST"
	cp "$PACK/mod.json" "$DEST/"
	cp -R "$PACK/data" "$PACK/strings" "$DEST/"
	rm -f "$DEST/strings/en-report.md"
	echo "bundled pack from $PACK"
else
	echo "no Unnamed pack at $PACK (Unnamed mode will be unavailable)"
fi
cd android
echo "sdk.dir=$ANDROID_HOME" > local.properties
[ -x gradlew ] || gradle wrapper -q
./gradlew assembleDebug --no-daemon -q "$@"
cd ..
mkdir -p dist
APK=$(find android -name "*.apk" -path "*/debug/*" | head -1)
cp "$APK" dist/pvz-unnamed-arm64.apk
echo "BUILT $(pwd)/dist/pvz-unnamed-arm64.apk"
