#!/bin/sh

set -e

echo Building Core in release mode...
~/builder/bin/builder ~/dev/Core/build.cpp --config=core --release --force-rebuild

echo Copying core src folder...
cp -r ~/dev/Core/src ~/dev/builder/src/core

echo Copying core include folder...
cp -r ~/dev/Core/include ~/dev/builder/src/core

echo Copying core binary files...
cp ~/dev/Core/bin/release/core.so ~/dev/builder/3rdparty/core/core.so

echo ""
echo Done.
echo ""