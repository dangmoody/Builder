#!/bin/bash

set -e

clang_version=20.1.5

# download versions of clang that we care about
if [[ ! -d "clang" ]]; then
	mkdir clang
fi

echo "Downloading Clang archive..."
curl -o clang/clang_${clang_version}_linux.tar.xz -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-${clang_version}/LLVM-${clang_version}-Linux-X64.tar.xz"
echo "Done."
echo ""

# unarchive curl downloads
echo "Unarchiving Clang archive..."
./7zip/7za.exe x "clang/clang_${clang_version}_linux.tar.xz" -o"clang"
./7zip/7za.exe x "clang/clang_${clang_version}_linux.tar" -o"clang"
robocopy /nfl /ndl /e "clang/LLVM-20.1.5-Linux-X64" clang
rmdir /s /q "clang/LLVM-20.1.5-Linux-X64"
echo "Done."
echo ""

# cleanup - delete the temp files
echo "Cleaning up..."
del /q "clang/clang_${clang_version}_linux.tar"
del /q "clang/clang_${clang_version}_linux.tar.xz"

echo "Done!"
