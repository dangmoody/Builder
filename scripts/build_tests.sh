#!/bin/bash

source "$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/build_common.sh"

echo Building tests config "$config"...

sourceFiles="tests/tests_main.cpp src/builder.cpp src/visual_studio.cpp src/backend_clang.cpp src/backend_msvc.cpp src/core/src/core.suc.cpp"

args="${clangDir}/bin/clang ${symbols} ${optimization} -std=c++20 -fexceptions -ferror-limit=0 -o ${binFolder}/builder_tests_${config} ${sourceFiles} ${defines} ${includes} ${libPaths} ${libraries} ${warningLevels} ${ignoreWarnings} -Wl,-rpath=$binFolder"
echo ${args}
${args}
