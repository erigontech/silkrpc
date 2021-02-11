#!/bin/bash

rm -fr build_gcc_debug
mkdir build_gcc_debug
cd build_gcc_debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc-10 -DCMAKE_CXX_COMPILER=g++-10 ..
cmake --build .
cmd/unit_test
