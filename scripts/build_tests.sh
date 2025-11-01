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

builder_dir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/..
clang_dir="${builder_dir}/clang"

bin_folder="${builder_dir}/bin/linux/$config"

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

args="${clang_dir}/bin/clang ${symbols} ${optimization} -lstdc++ -luuid -std=c++20 -fexceptions -ferror-limit=0 -o bin/linux/${config}/builder_tests tests/tests_main.cpp ${defines} -Isrc/core/include"
echo ${args}
${args}
