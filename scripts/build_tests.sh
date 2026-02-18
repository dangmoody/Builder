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
	programName=builder_$config
	defines="$defines -D_DEBUG -DBUILDER_PROGRAM_NAME=\"$programName\""
elif [[ "$config" == "release" ]]; then
	programName=builder
	defines="$defines -DNDEBUG -DBUILDER_PROGRAM_NAME=\"$programName\""
fi

includes="-I${builderDir}/src/core/include -I${builderDir}/clang/include"

libPaths="-L${builderDir}/clang/lib"
libs="-lstdc++ -luuid -lclang"

warningLevels="-Werror -Wall -Wextra -Weverything -Wpedantic"
ignoreWarnings="-Wno-newline-eof -Wno-format-nonliteral -Wno-gnu-zero-variadic-macro-arguments -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -Wno-zero-as-null-pointer-constant -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-missing-field-initializers -Wno-switch-default -Wno-covered-switch-default -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-cast-align -Wno-double-promotion -Wno-alloca -Wno-padded -Wno-documentation-unknown-command"

args="${clangDir}/bin/clang ${symbols} ${optimization} -std=c++20 -fexceptions -ferror-limit=0 -o ${binFolder}/builder_tests_${config} ${sourceFiles} ${defines} ${includes} ${libPaths} ${libs} ${warningLevels} ${ignoreWarnings} -Wl,-rpath=$binFolder"
echo ${args}
${args}
