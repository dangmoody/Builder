#!/bin/bash

ShowUsage()
{
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

echo Building config "$config"...

#couldn't find a way to just change the working director real quick - so i'll just do this for now and push absolute paths
# tbf - I could probably stash pwd and then cd into the absolute path and then cd out but not right now, let's get this running first
# https://stackoverflow.com/questions/207959/equivalent-of-dp0-retrieving-source-file-name-in-sh
called_path=${0%/*}
stripped=${called_path#[^/]*}
project_folder=`pwd`$stripped

bin_folder="$project_folder/bin/linux/$config"
intermediate_folder="$project_folder/intermediate"
source_folder="$project_folder/src"

mkdir -p $bin_folder
mkdir -p $intermediate_folder

symbols="-g"
optimisation=""
if [[ "$config" != "release" ]]; then
	optimisation="-O3"
fi

source_files="$source_folder/builder.cpp $source_folder/visual_studio.cpp $source_folder/core/src/core.suc.cpp"

defines="-D_CRT_SECURE_NO_WARNINGS -DCORE_USE_XXHASH -DCORE_SUC -DCORE_USE_SUBPROCESS -DHASHMAP_HIDE_MISSING_KEY_WARNING -DHLML_NAMESPACE"
if [[ "$config" != "debug" ]]; then
	defines="$define -D_DEBUG"
fi

if [[ "$config" != "release" ]]; then
	defines="$defines -DNDEBUG"
fi

includes="-I$project_folder/src/core/include"

libraries="-lstdc++ -luuid"

warning_levels="-Werror -Wall -Wextra -Weverything -Wpedantic"
ignore_warnings="-Wno-newline-eof -Wno-format-nonliteral -Wno-gnu-zero-variadic-macro-arguments -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -Wno-zero-as-null-pointer-constant -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-missing-field-initializers -Wno-switch-default -Wno-covered-switch-default -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-cast-align -Wno-double-promotion -Wno-alloca"

args="clang -std=c++20 -ferror-limit=0 -o $bin_folder/builder.exe $symbols $optimisation $source_files $defines $includes $libraries $warning_levels $ignore_warnings"
echo $args
#echo -n -e $args
$args


exit 0