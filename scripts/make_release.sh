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

builder_dir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/..
releases_folder=${builder_dir}/releases

# build tests (also builds builder)
source ${builder_dir}/scripts/build_tests.sh release

# run the tests and make sure they pass
${builder_dir}/bin/builder_tests_release

mkdir -p ${releases_folder}

cp ./bin/builder_release ./bin/builder

tar cvf releases/builder_${version}_linux.tar.xz -I 'xz -9e --lzma2=dict=256M' ./bin/builder clang include doc README.md LICENSE
