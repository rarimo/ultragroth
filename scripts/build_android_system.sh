#!/bin/bash

set -euo pipefail

NDK_PATH="***/Library/Android/sdk/ndk/23.1.7779620"

if [ ! -d "$NDK_PATH" ]; then
  echo "NDK path not found: $NDK_PATH"
  exit 1
fi


echo "Initializing git submodules..."
git submodule update --init --recursive


echo "Building GMP for Android..."
ANDROID_NDK="$NDK_PATH" ./build_gmp.sh android


echo "Building UltraGroth for Android..."
ANDROID_NDK="$NDK_PATH" make android -j$(nproc)