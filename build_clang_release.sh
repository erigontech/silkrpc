#!/bin/bash

rm -fr build_clang_release
mkdir build_clang_release
cd build_clang_release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$PWD/cmake/clang-libcxx20-fpic.cmake ..
cmake --build .
cmd/unit_test
