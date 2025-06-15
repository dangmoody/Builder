#!/bin/bash

args="clang -lstdc++ -luuid -std=c++20 -fexceptions -ferror-limit=0 -o BuilderTests.exe tests_main.cpp -DCORE_SUC -DHLML_NAMESPACE -DCORE_USE_SUBPROCESS -I ../src/core/include"
echo $args
$args