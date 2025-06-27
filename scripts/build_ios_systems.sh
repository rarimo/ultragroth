#!/bin/bash

set -e

echo "Building GMP for iOS and iOS Simulator..."

./build_gmp.sh ios > /dev/null 2>&1

echo "Building GMP for iOS Simulator..."

./build_gmp.sh ios_simulator > /dev/null 2>&1

echo "Building UltraGroth as a static library for iOS..."

make ios > /dev/null 2>&1

echo "Building UltraGroth as a static library for iOS Simulator..."
make ios_simulator > /dev/null 2>&1
