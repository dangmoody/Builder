#!/bin/bash

args="clang -std=c++20 -ferror-limit=0 -o BuilderTests.exe tests_main.cpp -DCORE_SUC -DHLML_NAMESPACE -I ../src/core/include"
echo $args
$args