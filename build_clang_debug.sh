#!/bin/bash

rm -fr build_clang_debug
mkdir build_clang_debug
cd build_clang_debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$PWD/cmake/clang-libcxx20-fpic.cmake ..
cmake --build .
cmd/unit_test
