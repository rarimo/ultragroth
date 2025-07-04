#!/bin/bash

set -e

ORGNAME="Rarilabs" 
XCFWNAME="UltraGroth"
FWNAME="UltraGrothLib"

### Functions declarations

function create_framework() {
    fw_paths=()
    for fw in "$@"; do
        fw_paths+=("-framework" "Frameworks/${fw}/$FWNAME.framework")
    done

    for fw in "$@"; do
        {
        echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        echo "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
        echo "<plist version=\"1.0\">"
        echo "<dict>"
        echo "    <key>CFBundlePackageType</key>"
        echo "    <string>FMWK</string>"
        echo "    <key>CFBundleIdentifier</key>"
        echo "    <string>$ORGNAME.$FWNAME</string>"
        echo "    <key>CFBundleExecutable</key>"
        echo "    <string>$FWNAME</string>"
        echo "    <key>CFBundleShortVersionString</key>"
        echo "    <string>1.0.0</string>"
        echo "    <key>CFBundleVersion</key>"
        echo "    <string>3</string>"
        echo "    <key>MinimumOSVersion</key>"
        echo "    <string>100</string>"
        echo "</dict>"
        echo "</plist>"
        } > "Frameworks/$fw/$FWNAME.framework/Info.plist"
    done

    rm -rf "Frameworks/$XCFWNAME.xcframework"
    xcrun xcodebuild -create-xcframework \
        "${fw_paths[@]}" \
        -output "Frameworks/$XCFWNAME.xcframework"

    
    if [ -n "$CODE_SIGNER" ]; then
        codesign --timestamp -s "$CODE_SIGNER" "Frameworks/$XCFWNAME.xcframework"
    fi
}

function copy_headers_files() {
    for fw in "$@"; do
        FRAMEWORK_PATH="Frameworks/$fw/$FWNAME.framework"

        mkdir -p "$FRAMEWORK_PATH/Headers"

        cp src/prover.h "$FRAMEWORK_PATH/Headers/$FWNAME.h"

        mkdir -p $FRAMEWORK_PATH/Modules
        {
        echo "framework module $FWNAME {"
        echo "    umbrella header \"$FWNAME.h\""
        echo "    export *"
        echo "    module * { export * }"
        echo "}"
        } > $FRAMEWORK_PATH/Modules/module.modulemap
    done
}

function strip_debug_symbols() {
    for fw in "$@"; do
        strip -x artifacts/$fw/src/Release/libfq.a \
            artifacts/$fw/src/Release/libfr.a \
            artifacts/$fw/src/Release/libgmp.a \
            artifacts/$fw/src/Release/libultragroth.a
    done
}

function merge_static_libraries() {
    for fw in "$@"; do
        mkdir -p "Frameworks/$fw/$FWNAME.framework/Headers"

        libtool -static -o Frameworks/$fw/$FWNAME.framework/$FWNAME \
            artifacts/$fw/src/Release/libfq.a \
            artifacts/$fw/src/Release/libfr.a \
            artifacts/$fw/src/Release/libgmp.a \
            artifacts/$fw/src/Release/libultragroth.a
        
        lipo Frameworks/$fw/$FWNAME.framework/$FWNAME  -remove x86_64 -output Frameworks/$fw/$FWNAME.framework/$FWNAME || true
        lipo Frameworks/$fw/$FWNAME.framework/$FWNAME  -remove arm64e -output Frameworks/$fw/$FWNAME.framework/$FWNAME || true
    done
}

### Execution starts here

rm -rf Frameworks

frameworks=("aarch64-apple-ios" "aarch64-apple-ios-sim")

echo "Stripping debug symbols from static libraries..."

strip_debug_symbols "${frameworks[@]}"

echo "Merging static libraries..."

merge_static_libraries "${frameworks[@]}"

echo "Copying headers and modulemap files..."

copy_headers_files "${frameworks[@]}"

echo "Creating xcframework..."

create_framework "${frameworks[@]}"

pushd "Frameworks"
zip -X -9 -r "$XCFWNAME.xcframework.zip" "$XCFWNAME.xcframework" -i */$FWNAME -i *.plist -i *.h -i *.modulemap
popd

echo "XCFramework checksum $(swift package compute-checksum Frameworks/$XCFWNAME.xcframework.zip)"
