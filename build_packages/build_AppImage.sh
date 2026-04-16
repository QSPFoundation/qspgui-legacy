#!/bin/sh

set -eu

export DEBIAN_FRONTEND=noninteractive

sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  ninja-build \
  libgtk-3-dev \
  wget \
  ca-certificates
sudo rm -rf /var/lib/apt/lists/*

REL_BUILD_DIR="./build_packages/linux64_AppImage"

cmake -S . -B "$REL_BUILD_DIR" -GNinja \
  -DAPP_VERSION="$APP_VERSION" \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DCMAKE_BUILD_TYPE=Release

cmake --build "$REL_BUILD_DIR" --parallel
DESTDIR="$REL_BUILD_DIR/AppDir" cmake --install "$REL_BUILD_DIR" --component Main --strip

cd "$REL_BUILD_DIR" || { echo "Can't switch to $REL_BUILD_DIR"; exit 1; }

wget -q --show-progress -c "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh"
wget -q --show-progress -c "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
chmod +x linuxdeploy-x86_64.AppImage linuxdeploy-plugin-gtk.sh

LIB_PATH=$(find ./AppDir -type d -exec printf ":%s" {} +)
export APPIMAGE_EXTRACT_AND_RUN=1
LD_LIBRARY_PATH="${LIB_PATH#:}" ./linuxdeploy-x86_64.AppImage --appdir ./AppDir --plugin gtk --output appimage
