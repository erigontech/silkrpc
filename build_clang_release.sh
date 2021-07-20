#!/bin/bash

PROJECT_DIR=$PWD
cd build_clang_release
cmake --build . --parallel
cmd/unit_test
