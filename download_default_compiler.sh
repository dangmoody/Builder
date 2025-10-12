#!/bin/bash

set -e

ShowUsage() {
    echo "Usage: download_default_compiler.sh <version>"
	echo "Where <version> is the version of Clang you want to download (so something like 20.1.5)"
	exit 1
}

# get clang version and check its not empty
clang_version=$1
if [[ -z "$clang_version" ]]; then
	echo "ERROR: No clang version specified."
	ShowUsage
fi

# download versions of clang that we care about
if [[ ! -d "clang_linux" ]]; then
	mkdir clang_linux
fi

if [[ ! -d "clang_win64" ]]; then
	mkdir clang_win64
fi

echo "Downloading Clang linux archive..."
curl -o clang_linux/clang_${clang_version}_linux.tar.xz -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-${clang_version}/LLVM-${clang_version}-Linux-X64.tar.xz"
echo "Done."
echo ""

echo "Downloading Clang win64 archive..."
curl -o clang_win64/clang_${clang_version}_win64.tar.xz -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-${clang_version}/clang+llvm-${clang_version}-x86_64-pc-windows-msvc.tar.xz"
echo "Done."
echo ""

# unarchive curl downloads
echo "Unarchiving Clang linux archive..."
./7zip/7za.exe x "clang_linux/clang_${clang_version}_linux.tar.xz" -o"clang_linux"
./7zip/7za.exe x "clang_linux/clang_${clang_version}_linux.tar" -o"clang_linux"
robocopy /nfl /ndl /e "clang_linux/LLVM-20.1.5-Linux-X64" clang_linux
rmdir /s /q "clang_linux/LLVM-20.1.5-Linux-X64"
echo "Done."
echo ""

echo "Unarchiving Clang win64 archive..."
./7zip/7za.exe x "clang_win64/clang_${clang_version}_win64.tar.xz" -o"clang_win64"
./7zip/7za.exe x "clang_win64/clang_${clang_version}_win64.tar" -o"clang_win64"
robocopy /nfl /ndl /e "clang_win64/clang+llvm-${clang_version}-x86_64-pc-windows-msvc" clang_win64
rmdir /s /q "clang_win64/clang+llvm-${clang_version}-x86_64-pc-windows-msvc"
echo "Done."
echo ""

# cleanup - delete the temp files
echo "Cleaning up..."
del /q "clang_linux/clang_${clang_version}_linux.tar"
del /q "clang_linux/clang_${clang_version}_linux.tar.xz"
del /q "clang_win64/clang_${clang_version}_win64.tar"
del /q "clang_win64/clang_${clang_version}_win64.tar.xz"

echo "Done!"
