#!/bin/bash

PROJECT_DIR=$PWD
rm -fr build_gcc_release
mkdir build_gcc_release
cd build_gcc_release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-10 -DCMAKE_CXX_COMPILER=g++-10 ..
cd $PROJECT_DIR
./build_gcc_release.sh
