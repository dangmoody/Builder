#!/bin/bash

set -e

builder_dir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/..

# ----------------------------------------------------------------

# download clang
clang_version=20.1.5
clang_dir="${builder_dir}/clang"

if [[ ! -d "${clang_dir}" ]]; then
	mkdir ${clang_dir}
fi

echo "Downloadinging Clang archive..."
curl -o ${clang_dir}/clang.tar.xz -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-${clang_version}/LLVM-${clang_version}-Linux-X64.tar.xz"
echo "Done."
echo ""

# unarchive clang
echo "Extracting Clang archive..."
tar xf "${clang_dir}/clang.tar.xz" -C "${clang_dir}"
cp -r "${clang_dir}/LLVM-${clang_version}-Linux-X64/." "${clang_dir}"
rm -rf "${clang_dir}/LLVM-${clang_version}-Linux-X64"
rm "${clang_dir}/clang.tar.xz"
echo "Done."
echo ""

# ----------------------------------------------------------------

# DM: unless we build from source I'm not sure there's really a way to get this to work
# so I'm disabling this until I can figure it out

# download gcc
#gcc_version=15.1.0
#gcc_dir="${builder_dir}/tools/gcc"
#
#if [[ ! -d "${gcc_dir}" ]]; then
#	mkdir ${gcc_dir}
#fi
#
#echo "Downloadinging GCC archive..."
#curl -o "${gcc_dir}/gcc.tar.xz" -L "https://mirrorservice.org/sites/sourceware.org/pub/gcc/releases/gcc-${gcc_version}/gcc-${gcc_version}.tar.xz"
#echo "Done."
#echo ""
#
## unarchive gcc
#echo "Extracting GCC archive..."
#tar xf "${gcc_dir}/gcc.tar.xz" -C "${gcc_dir}"
#cp -r "${gcc_dir}/gcc-${gcc_version}/." "${gcc_dir}"
#rm -rf "${gcc_dir}/gcc-${gcc_version}"
#rm "${gcc_dir}/gcc.tar.xz"
#echo "Done."
#echo ""

echo "Done!"
