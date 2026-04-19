#!/bin/bash

source "$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")/build_common.sh"

echo Building Builder, config "$config"...

sourceFolder="${builderDir}/src"
sourceFiles="$sourceFolder/main.cpp $sourceFolder/builder.cpp $sourceFolder/visual_studio.cpp $sourceFolder/backend_clang.cpp $sourceFolder/core/src/core.suc.cpp"

args="${clangDir}/bin/clang -std=c++20 -ferror-limit=0 -o $binFolder/$programName $symbols $optimisation $sourceFiles $defines $includes $libPaths $libraries $warningLevels $ignoreWarnings -Wl,-rpath=$binFolder"
echo $args
$args

cp ${clangDir}/lib/libclang.so ${binFolder}
cp ${clangDir}/lib/libclang.so.20.1 ${binFolder}
