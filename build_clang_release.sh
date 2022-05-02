#!/bin/bash

PROJECT_DIR=$PWD
cd build_clang_release
cmake --build .
cmd/unit_test "$*"
