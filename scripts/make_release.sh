#!/bin/bash

set -e

ShowUsage() {
	echo "Usage:"
	echo "make_release.bat \<version\>"
	echo
	echo "Arguments:"
	echo     "\<version\> (required):"
	echo         "The version of the release that you are making (example: v1.2.3 - 1 would be the major version, 2 would be the minor version, and 3 would be the patch version)"

	exit 1
}

version=$1

if [[ -z "$version" ]]; then
	echo ERROR: Release version was not set!  Please specify a release version
	ShowUsage
fi

builderDir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/..

pushd ${builderDir}

# build builder and tests
source ${builderDir}/scripts/build.sh release
echo ""

source ${builderDir}/scripts/build_tests.sh release

# run the tests and make sure they pass
pushd ${builderDir}/tests
../bin/builder_tests_release
popd

mkdir -p releases

echo "Smoke-testing trimmed Clang..."
printf 'int main(void) { return 0; }\n' > releases/smoke.c
"${builderDir}/clang/bin/clang" -o releases/smoke_test releases/smoke.c
rm releases/smoke.c releases/smoke_test
echo "Done."
echo ""

echo "Making release archive..."
tar cvf releases/builder_${version}_linux.tar.xz -I 'xz -9e --lzma2=dict=256M' ./bin/builder ./bin/libclang.so ./bin/libclang.so.20.1 ./bin/core.so clang include doc/CHANGELOG.txt doc/Contributing.md README.md LICENSE

popd
