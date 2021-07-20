#!/bin/bash

rm -fr build_gcc_release
mkdir build_gcc_release
cd build_gcc_release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-10 -DCMAKE_CXX_COMPILER=g++-10 ..
cmake --build . --parallel
cmd/unit_test
