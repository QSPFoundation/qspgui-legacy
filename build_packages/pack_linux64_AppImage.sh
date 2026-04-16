#!/bin/sh

set -euo pipefail

# Validation
[ ! -d "./build_packages" ] && echo "Run this script from the project root directory" && exit
[ -z "$RELEASE_VER" ] && echo "RELEASE_VER isn't specified" && exit

# Build
mkdir -p ./build_packages/linux64_AppImage
mkdir -p ./dist

IMAGE="dockbuild/ubuntu2004-gcc9"
SCRIPT="build_packages/build_AppImage.sh"
SSH_DIR="$HOME/.ssh"

HOST_VOLUMES="-v $SSH_DIR:/home/$(id -un)/.ssh"
USER_IDS="-e BUILDER_UID=$(id -u) -e BUILDER_GID=$(id -g) -e BUILDER_USER=$(id -un) -e BUILDER_GROUP=$(id -gn)"
APP_ARGS="-e APP_VERSION=$RELEASE_VER"

TTY_ARGS=""
[ -t 0 ] && TTY_ARGS="-ti"

docker run --rm \
  -v "$(pwd)":/work \
  $TTY_ARGS \
  $HOST_VOLUMES \
  $USER_IDS \
  $APP_ARGS \
  "$IMAGE" "/work/$SCRIPT"

# Cleanup & Move to dist
rm -f ./build_packages/linux64_AppImage/linuxdeploy-*.AppImage
mv ./build_packages/linux64_AppImage/*.AppImage "./dist/QSP_Classic-$RELEASE_VER-x86_64.AppImage"
