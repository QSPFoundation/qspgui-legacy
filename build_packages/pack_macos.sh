#!/bin/sh

set -eu

# Validation
[ ! -d "./build_packages" ] && echo "Run this script from the project root directory" && exit
[ -z "$RELEASE_VER" ] && echo "RELEASE_VER isn't specified" && exit

# Build
REL_BUILD_DIR="./build_packages/macos"
mkdir -p "$REL_BUILD_DIR"

cmake -S . -B "$REL_BUILD_DIR" -GNinja \
  -DAPP_VERSION="$RELEASE_VER" \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11 \
  -DCPACK_OUTPUT_FILE_PREFIX="$REL_BUILD_DIR/packages" \
  -DCPACK_GENERATOR="DragNDrop" \
  -DCMAKE_INSTALL_PREFIX="$REL_BUILD_DIR/out" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build "$REL_BUILD_DIR" --parallel $(sysctl -n hw.ncpu)
cmake --install "$REL_BUILD_DIR" --component Main --strip
cpack -B "$REL_BUILD_DIR" --config "$REL_BUILD_DIR/CPackConfig.cmake"

# Move to dist
mv "$REL_BUILD_DIR"/packages/*.dmg "./dist/qspgui-legacy-$RELEASE_VER-universal.dmg"
