#!/bin/bash

set -e

clang_version=20.1.5
clang_dir="$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/clang"

# download versions of clang that we care about
if [[ ! -d "${clang_dir}" ]]; then
	mkdir ${clang_dir}
fi

echo "Downloading Clang archive..."
curl -o ${clang_dir}/clang_${clang_version}_linux.tar.xz -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-${clang_version}/LLVM-${clang_version}-Linux-X64.tar.xz"
echo "Done."
echo ""

# unarchive curl downloads
echo "Unarchiving Clang archive..."
tar xf "${clang_dir}/clang_${clang_version}_linux.tar.xz" -C "${clang_dir}"
# tar xf "clang/clang_${clang_version}_linux.tar" -C "clang" Not sure what this is doing!
cp -r "${clang_dir}/LLVM-${clang_version}-Linux-X64/." "${clang_dir}"
rm -rf "${clang_dir}/LLVM-20.1.5-Linux-X64"
echo "Done."
echo ""

# cleanup - delete the temp files
echo "Cleaning up..."
rm -f "${clang_dir}/clang_${clang_version}_linux.tar"
rm -f "${clang_dir}/clang_${clang_version}_linux.tar.xz"

echo "Done!"
