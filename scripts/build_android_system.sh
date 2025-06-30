#!/bin/bash

set -e


git submodule init
git submodule update


echo "Building GMP for Android..."



ANDROID_NDK=/Users/apik/Library/Android/sdk/ndk/23.1.7779620 ./build_gmp.sh android


ANDROID_NDK=/Users/apik/Library/Android/sdk/ndk/23.1.7779620 make android -j4

