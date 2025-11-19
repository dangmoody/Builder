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

echo Building Builder, config "$config"...

builder_dir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/..
clang_dir="${builder_dir}/clang"

bin_folder="${builder_dir}/bin"
intermediate_folder="${builder_dir}/intermediate"
source_folder="${builder_dir}/src"

mkdir -p $bin_folder
mkdir -p $intermediate_folder

symbols=""
if [[ "$config" == "debug" ]]; then
	symbols="-g"
fi

optimisation=""
if [[ "$config" != "release" ]]; then
	optimisation="-O3"
fi

source_files="$source_folder/builder.cpp $source_folder/visual_studio.cpp $source_folder/backend_clang.cpp $source_folder/core/src/core.suc.cpp"

defines="-D_CRT_SECURE_NO_WARNINGS -DCORE_USE_XXHASH -DCORE_SUC -DCORE_USE_SUBPROCESS -DHASHMAP_HIDE_MISSING_KEY_WARNING -DHLML_NAMESPACE"
if [[ "$config" == "debug" ]]; then
	defines="$defines -D_DEBUG"
fi

if [[ "$config" == "release" ]]; then
	defines="$defines -DNDEBUG"
fi

includes="-I${builder_dir}/src/core/include"

libraries="-lstdc++ -luuid"

warning_levels="-Werror -Wall -Wextra -Weverything -Wpedantic"
ignore_warnings="-Wno-newline-eof -Wno-format-nonliteral -Wno-gnu-zero-variadic-macro-arguments -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -Wno-zero-as-null-pointer-constant -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-missing-field-initializers -Wno-switch-default -Wno-covered-switch-default -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-cast-align -Wno-double-promotion -Wno-alloca -Wno-padded"

args="${clang_dir}/bin/clang -std=c++20 -ferror-limit=0 -o $bin_folder/builder_${config} $symbols $optimisation $source_files $defines $includes $libraries $warning_levels $ignore_warnings"
echo $args
$args