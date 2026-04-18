#!/bin/sh

set -eu

sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  ninja-build \
  libgtk-3-dev \
  rpm
sudo rm -rf /var/lib/apt/lists/*

REL_BUILD_DIR=./build_packages/linux64

cmake -S . -B "$REL_BUILD_DIR" -GNinja \
  -DAPP_VERSION="$APP_VERSION" \
  -DCPACK_OUTPUT_FILE_PREFIX="$REL_BUILD_DIR/packages" \
  -DCPACK_GENERATOR="TGZ;DEB;RPM" \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DCMAKE_BUILD_TYPE=Release

cmake --build "$REL_BUILD_DIR" --parallel
cpack -B "$REL_BUILD_DIR" --config "$REL_BUILD_DIR/CPackConfig.cmake"
