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

builder_dir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/..
clang_dir="${builder_dir}/clang"

source ${builder_dir}/scripts/build.sh ${config}
echo ""

echo Building tests config "$config"...

bin_folder="${builder_dir}/bin"

mkdir -p $bin_folder

symbols="-g"
optimisation=""
if [[ "$config" != "release" ]]; then
	optimisation="-O3"
fi

source_files="tests/tests_main.cpp src/builder.cpp src/visual_studio.cpp src/backend_clang.cpp src/backend_msvc.cpp"

defines="-DCORE_SUC -DHLML_NAMESPACE -DCORE_USE_SUBPROCESS"
if [[ "$config" == "debug" ]]; then
	defines="$defines -D_DEBUG"
elif [[ "$config" == "release" ]]; then
	defines="$defines -DNDEBUG"
fi

args="${clang_dir}/bin/clang ${symbols} ${optimization} -lstdc++ -luuid -std=c++20 -fexceptions -ferror-limit=0 -o ${bin_folder}/builder_tests_${config} ${source_files} ${defines} -Isrc/core/include"
echo ${args}
${args}
