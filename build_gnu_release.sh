#!/bin/bash

rm -fr build_gnu_release
mkdir build_gnu_release
cd build_gnu_release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cmd/unit_test
