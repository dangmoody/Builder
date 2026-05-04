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

# Remove unused Clang files to reduce size
echo "Removing unused Clang files..."

# Entire top-level directories not needed at runtime
rm -rf "${clangDir}/include"
rm -rf "${clangDir}/share"
rm -rf "${clangDir}/libexec"
rm -rf "${clangDir}/local"

# Subdirectories of lib/ not needed at runtime
# (lib/clang/ is the compiler resource dir and must be kept)
rm -rf "${clangDir}/lib/cmake"
rm -rf "${clangDir}/lib/libscanbuild"
rm -rf "${clangDir}/lib/libear"
rm -rf "${clangDir}/lib/objects-RELEASE"
rm -rf "${clangDir}/lib/x86_64-unknown-linux-gnu"

# From lib/: delete all files except libclang.so*
# (subdirectories like lib/clang/ are untouched by -maxdepth 1)
find "${clangDir}/lib" -maxdepth 1 \( -type f -o -type l \) \
    ! -name 'libclang.so*'   \
    ! -name 'libLLVM.so*'    \
    ! -name 'libLTO.so*'     \
    ! -name 'libRemarks.so*' \
    -delete

# From bin/: delete everything except the tools Builder needs
find "${clangDir}/bin" -maxdepth 1 \( -type f -o -type l \) \
    ! -name 'clang'         \
    ! -name 'clang++'       \
    ! -name 'clang-20'      \
    ! -name 'clang-cl'      \
    ! -name 'clang-cpp'     \
    ! -name 'lld'           \
    ! -name 'lld-link'      \
    ! -name 'ld.lld'        \
    ! -name 'ld64.lld'      \
    ! -name 'llvm-ar'       \
    ! -name 'llvm-ranlib'   \
    ! -name 'llvm-lib'      \
    ! -name 'llvm-dlltool'  \
    -delete

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
