#!/bin/sh

set -eu

[ ! -d "./build_packages" ] && echo "Run this script from the project root directory" && exit
[ -z "$RELEASE_VER" ] && echo "RELEASE_VER isn't specified" && exit

mkdir -p ./build_packages/linux64_AppImage

IMAGE="dockbuild/ubuntu2004-gcc9:latest@sha256:16a314de0b5fbfd3d135ed9f850e95e138d9d0eedede171d0a6fe6bd464e3c58"
SCRIPT="build_packages/build_AppImage.sh"

USER_IDS="-e BUILDER_UID=$(id -u) -e BUILDER_GID=$(id -g) -e BUILDER_USER=$(id -un) -e BUILDER_GROUP=$(id -gn)"
APP_ARGS="-e APP_VERSION=$RELEASE_VER"

tty -s && TTY_ARGS="-ti" || TTY_ARGS=""

docker run --rm \
  -v "$(pwd)":/work \
  $TTY_ARGS \
  $USER_IDS \
  $APP_ARGS \
  "$IMAGE" "/work/$SCRIPT"

rm ./build_packages/linux64_AppImage/linuxdeploy-*.AppImage
mv ./build_packages/linux64_AppImage/*.AppImage "./dist/QSP_Legacy_Classic-$RELEASE_VER-x86_64.AppImage"
