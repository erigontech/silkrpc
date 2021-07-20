#!/bin/bash

PROJECT_DIR=$PWD
cd build_clang_debug
cmake --build . --parallel
cmd/unit_test
