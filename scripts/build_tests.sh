#!/bin/bash

source "$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/build_common.sh"

echo Building tests config "$config"...

binFolder="${builderDir}/bin"

mkdir -p $binFolder

symbols="-g"
optimisation=""
if [[ "$config" != "release" ]]; then
	optimisation="-O3"
fi

sourceFiles="tests/tests_main.cpp src/builder.cpp src/visual_studio.cpp src/vs_code.cpp src/zed_editor.cpp src/backend_clang.cpp src/backend_msvc.cpp src/win_support.cpp\
	src/debug.cpp src/file.cpp src/hash.cpp src/hashmap.cpp src/linear_allocator.cpp src/math.cpp src/paths.cpp src/stb_impl.cpp src/string.cpp src/string_builder.cpp src/temp_storage.cpp\
	src/linux/*.cpp"

args="${clangDir}/bin/clang ${symbols} ${optimisation} -std=c++20 -fexceptions -ferror-limit=0 -o ${binFolder}/builder_tests_${config} ${sourceFiles} ${defines} ${includes} ${libPaths} ${libraries} ${warningLevels} ${ignoreWarnings} -Wl,-rpath=$binFolder"
echo ${args}
${args}
