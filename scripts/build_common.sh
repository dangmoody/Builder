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

binFolder="${builderDir}/bin"
mkdir -p $binFolder

intermediateFolder="${builderDir}/intermediate"
mkdir -p $intermediateFolder

defines="-D_CRT_SECURE_NO_WARNINGS -DCORE_USE_XXHASH -DCORE_SUC -DCORE_USE_SUBPROCESS -DHASHMAP_HIDE_MISSING_KEY_WARNING -DHLML_NAMESPACE"
if [[ "$config" == "debug" ]]; then
	symbols="-g"
	optimisation=""
	programName=builder_$config
	defines="$defines -D_DEBUG -DBUILDER_PROGRAM_NAME=\"$programName\""
elif [[ "$config" == "release" ]]; then
	optimisation="-O3"
	programName=builder
	defines="$defines -DNDEBUG -DBUILDER_PROGRAM_NAME=\"$programName\""
fi

includes="-I${builderDir}/src/core/include -I${builderDir}/clang/include"

libPaths="-L${builderDir}/clang/lib"

libraries="-lstdc++ -luuid -lclang"

warningLevels="-Werror -Wall -Wextra -Weverything -Wpedantic"
ignoreWarnings="-Wno-newline-eof -Wno-format-nonliteral -Wno-gnu-zero-variadic-macro-arguments -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -Wno-zero-as-null-pointer-constant -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-missing-field-initializers -Wno-switch-default -Wno-covered-switch-default -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-cast-align -Wno-double-promotion -Wno-alloca -Wno-padded -Wno-documentation-unknown-command"
