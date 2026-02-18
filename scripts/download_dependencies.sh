#!/bin/bash

set -e

builderDir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/..

# ----------------------------------------------------------------

# download clang
clangVersion=20.1.5
clangDir="${builderDir}/clang"

if [[ ! -d "${clangDir}" ]]; then
	mkdir ${clangDir}
fi

echo "Downloadinging Clang archive..."
curl -o ${clangDir}/clang.tar.xz -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-${clangVersion}/LLVM-${clangVersion}-Linux-X64.tar.xz"
echo "Done."
echo ""

# unarchive clang
echo "Extracting Clang archive..."
tar xf "${clangDir}/clang.tar.xz" -C "${clangDir}"
cp -r "${clangDir}/LLVM-${clangVersion}-Linux-X64/." "${clangDir}"
rm -rf "${clangDir}/LLVM-${clangVersion}-Linux-X64"
rm "${clangDir}/clang.tar.xz"
echo "Done."
echo ""

# ----------------------------------------------------------------

# DM: unless we build from source I'm not sure there's really a way to get this to work
# so I'm disabling this until I can figure it out

# download gcc
#gccVersion=15.1.0
#gccDir="${builderDir}/tools/gcc"
#
#if [[ ! -d "${gccDir}" ]]; then
#	mkdir ${gccDir}
#fi
#
#echo "Downloadinging GCC archive..."
#curl -o "${gccDir}/gcc.tar.xz" -L "https://mirrorservice.org/sites/sourceware.org/pub/gcc/releases/gcc-${gccVersion}/gcc-${gccVersion}.tar.xz"
#echo "Done."
#echo ""
#
## unarchive gcc
#echo "Extracting GCC archive..."
#tar xf "${gccDir}/gcc.tar.xz" -C "${gccDir}"
#cp -r "${gccDir}/gcc-${gccVersion}/." "${gccDir}"
#rm -rf "${gccDir}/gcc-${gccVersion}"
#rm "${gccDir}/gcc.tar.xz"
#echo "Done."
#echo ""

echo "Done!"
