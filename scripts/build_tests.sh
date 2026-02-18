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

builderDir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/..
clangDir="${builderDir}/clang"

echo Building tests config "$config"...

binFolder="${builderDir}/bin"

mkdir -p $binFolder

symbols="-g"
optimisation=""
if [[ "$config" != "release" ]]; then
	optimisation="-O3"
fi

sourceFiles="tests/tests_main.cpp src/builder.cpp src/visual_studio.cpp src/backend_clang.cpp src/backend_msvc.cpp src/core/src/core.suc.cpp"

defines="-DCORE_SUC -DHLML_NAMESPACE -DCORE_USE_SUBPROCESS -DHASHMAP_HIDE_MISSING_KEY_WARNING"
if [[ "$config" == "debug" ]]; then
	programName=\"builder_$config\"
	defines="$defines -D_DEBUG -DBUILDER_PROGRAM_NAME=$programName"
elif [[ "$config" == "release" ]]; then
	programName=\"builder\"
	defines="$defines -DNDEBUG -DBUILDER_PROGRAM_NAME=$programName"
fi

args="${clangDir}/bin/clang ${symbols} ${optimization} -lstdc++ -luuid -std=c++20 -fexceptions -ferror-limit=0 -o ${binFolder}/builder_tests_${config} ${sourceFiles} ${defines} -Isrc/core/include"
echo ${args}
${args}
