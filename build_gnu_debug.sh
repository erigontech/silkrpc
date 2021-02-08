#!/bin/bash

rm -fr build_gnu_debug
mkdir build_gnu_debug
cd build_gnu_debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
cmd/unit_test
