#!/bin/bash

set -e

ShowUsage() {
	echo "Usage:"
	echo "make_release.sh <version>"
	echo
	echo "Arguments:"
	echo "    <version> (required):"
	echo "        The version of the release that you are making (example: v1.2.3 - 1 would be the major version, 2 would be the minor version, and 3 would be the patch version)"

	exit 1
}

version=$1

if [[ -z "$version" ]]; then
	echo "ERROR: Release version was not set!  Please specify a release version"
	ShowUsage
fi

builderDir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/..

pushd ${builderDir}

rm -f releases/builder_${version}.zip

7za a -tzip releases/builder_${version}.zip builder.h builder_visual_studio.h builder_vs_code.h builder_zed.h builder_compilation_database.h doc/CHANGELOG.txt doc/CHANGELOG_OLD.txt doc/Contributing.md README.md LICENSE

popd
