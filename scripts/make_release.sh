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

source ${builder_dir}/scripts/build.sh release

releases_folder=${builder_dir}/releases
temp_folder=${builder_dir}/releases/temp

mkdir -p ${releases_folder}
mkdir -p ${temp_folder}

cp -R ./bin/linux/release ${temp_folder}/bin
cp -R ./clang             ${temp_folder}/clang

7za a -tzip releases/builder_${version}_linux.zip ${temp_folder}/bin ${temp_folder}/clang include doc README.md LICENSE

rm -r $temp_folder
