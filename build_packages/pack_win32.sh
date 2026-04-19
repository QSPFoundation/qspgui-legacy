#!/bin/sh

set -eu

# Validation
[ ! -d "./build_packages" ] && echo "Run this script from the project root directory" && exit
[ -z "$RELEASE_VER" ] && echo "RELEASE_VER isn't specified" && exit

# Build
REL_BUILD_DIR="./build_packages/win32"
mkdir -p "$REL_BUILD_DIR"

DOCKCROSS_IMG="dockcross/windows-static-x86:20240812-60fa1b0"
DOCKCROSS_SCRIPT="./build_packages/dockcross-windows-static-x86"

if [ ! -x "$DOCKCROSS_SCRIPT" ]; then
    echo "Generating dockcross wrapper script..."
    docker run --rm "$DOCKCROSS_IMG" > "$DOCKCROSS_SCRIPT"
    chmod +x "$DOCKCROSS_SCRIPT"
fi

"$DOCKCROSS_SCRIPT" cmake -S . -B "$REL_BUILD_DIR" -GNinja \
  -DAPP_VERSION="$RELEASE_VER" \
  -DCPACK_OUTPUT_FILE_PREFIX="$REL_BUILD_DIR/packages" \
  -DCPACK_GENERATOR="ZIP;NSIS" \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DCMAKE_BUILD_TYPE=Release

"$DOCKCROSS_SCRIPT" cmake --build "$REL_BUILD_DIR" --parallel
"$DOCKCROSS_SCRIPT" cpack -B "$REL_BUILD_DIR" --config "$REL_BUILD_DIR/CPackConfig.cmake"

# Move to dist
mv "$REL_BUILD_DIR"/packages/*.zip "./dist/qspgui-legacy-$RELEASE_VER-win32.zip"
mv "$REL_BUILD_DIR"/packages/*.exe "./dist/qspgui-legacy-$RELEASE_VER-win32.exe"
