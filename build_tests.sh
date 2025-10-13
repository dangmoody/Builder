#!/bin/bash

ShowUsage() {
	echo Usage: build.bat \[debug\|release\]
	exit 1
}

config="$1"

if [[ -z "$config" ]]; then
	echo ERROR: build config not specified.
	ShowUsage
fi

if [[ "$config" != "debug" && "$config" != "release" ]]; then
	echo ERROR: build config MUST be either "debug" or "release".
	ShowUsage
fi

source build.sh ${config}
echo ""

echo Building tests config "$config"...

# couldn't find a way to just change the working director real quick - so i'll just do this for now and push absolute paths
# tbf - I could probably stash pwd and then cd into the absolute path and then cd out but not right now, let's get this running first
# https://stackoverflow.com/questions/207959/equivalent-of-dp0-retrieving-source-file-name-in-sh
called_path=${0%/*}
stripped=${called_path#[^/]*}
project_folder=`pwd`$stripped

bin_folder="$project_folder/bin/linux/$config"

mkdir -p $bin_folder

symbols="-g"
optimisation=""
if [[ "$config" != "release" ]]; then
	optimisation="-O3"
fi

defines="-DCORE_SUC -DHLML_NAMESPACE -DCORE_USE_SUBPROCESS"
if [[ "$config" == "debug" ]]; then
	defines="$defines -D_DEBUG"
fi

args="clang ${symbols} ${optimization} -lstdc++ -luuid -std=c++20 -fexceptions -ferror-limit=0 -o bin/linux/${config}/builder_tests tests/tests_main.cpp ${defines} -Isrc/core/include"
echo ${args}
${args}
