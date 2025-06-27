#!/bin/bash

set -e

echo "Building GMP for iOS and iOS Simulator..."

./build_gmp.sh ios

echo "Building GMP for iOS Simulator..."

./build_gmp.sh ios_simulator

echo "Building UltraGroth as a static library for iOS..."

make ios

echo "Building UltraGroth as a static library for iOS Simulator..."
make ios_simulator
