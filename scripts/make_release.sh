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

# couldn't find a way to just change the working director real quick - so i'll just do this for now and push absolute paths
# tbf - I could probably stash pwd and then cd into the absolute path and then cd out but not right now, let's get this running first
# https://stackoverflow.com/questions/207959/equivalent-of-dp0-retrieving-source-file-name-in-sh
called_path=${0%/*}
stripped=${called_path#[^/]*}
project_folder=`pwd`$stripped

if [[ -z "$version" ]]; then
	echo ERROR: Release version was not set!  Please specify a release version
	ShowUsage
fi

source build.sh release

releases_folder=$project_folder/releases
temp_folder=$project_folder/releases/temp

mkdir -p $releases_folder
mkdir -p $temp_folder

cp -R ./bin/linux/release $temp_folder/bin
cp -R ./clang_linux       $temp_folder/clang

7za a -tzip releases/builder_linux_$version.zip $temp_folder/bin $temp_folder/clang include doc README.md LICENSE

rm -r $temp_folder
