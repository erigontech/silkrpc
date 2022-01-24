#!/bin/bash

PROJECT_DIR=$PWD
rm -fr build_gcc_debug
mkdir build_gcc_debug
cd build_gcc_debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..
cd $PROJECT_DIR
./build_gcc_debug.sh
