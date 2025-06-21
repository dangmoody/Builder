#!/bin/bash

args="clang -O0 -g -lstdc++ -luuid -std=c++20 -fexceptions -ferror-limit=0 -o BuilderTests.exe tests/tests_main.cpp -DCORE_SUC -DHLML_NAMESPACE -DCORE_USE_SUBPROCESS -I src/core/include"
echo $args
$args
