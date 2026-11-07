#!/bin/sh

set -eu

sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  ninja-build \
  libgtk-3-dev \
  wget \
  ca-certificates \
  file
sudo rm -rf /var/lib/apt/lists/*

REL_BUILD_DIR="./build_packages/linux64_AppImage"

cmake -S . -B "$REL_BUILD_DIR" -GNinja \
  -DAPP_VERSION="$APP_VERSION" \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DCMAKE_BUILD_TYPE=Release \
  -DwxUSE_LIBPNG=builtin

cmake --build "$REL_BUILD_DIR" --parallel
DESTDIR="$REL_BUILD_DIR/AppDir" cmake --install "$REL_BUILD_DIR" --component Main --strip

cd "$REL_BUILD_DIR" || { echo "Can't switch to $REL_BUILD_DIR"; exit 1; }

LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_SHA256="c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d"
PLUGIN_GTK_URL="https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/7a3fbc31a9e5075073ff8790f26effbac5f84453/linuxdeploy-plugin-gtk.sh"
PLUGIN_GTK_SHA256="b0f4cbc684a0103a9651f0955b635eaea0096b3a66c0f5a2c2aa337960375171"

wget -q --show-progress -O linuxdeploy-x86_64.AppImage "$LINUXDEPLOY_URL"
wget -q --show-progress -O linuxdeploy-plugin-gtk.sh "$PLUGIN_GTK_URL"
printf '%s  %s\n%s  %s\n' \
  "$LINUXDEPLOY_SHA256" linuxdeploy-x86_64.AppImage \
  "$PLUGIN_GTK_SHA256" linuxdeploy-plugin-gtk.sh | sha256sum -c -
chmod +x linuxdeploy-x86_64.AppImage linuxdeploy-plugin-gtk.sh

export APPIMAGE_EXTRACT_AND_RUN=1

LIB_PATH=$(find ./AppDir -type d -exec printf ":%s" {} +)
export VERSION="$APP_VERSION"
LD_LIBRARY_PATH="${LIB_PATH#:}" ./linuxdeploy-x86_64.AppImage --appdir ./AppDir --plugin gtk --output appimage
