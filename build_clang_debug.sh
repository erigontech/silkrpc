#!/bin/bash

PROJECT_DIR=$PWD
cd build_clang_debug
cmake --build .
cmd/unit_test "$*"
