mkdir -p build/romulus.app/Contents/MacOS
mkdir temp

pushd build
echo "Building Executable"
clang -fdiagnostics-absolute-paths -g -framework AppKit -o romulus ../source/macosx/osx-main.mm
popd

# TODO: Packaging into proper location, multi-arch build if needed, copying plist, building iconset.... or just move all this to xcode
