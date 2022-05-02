#!/bin/bash

cd build_gcc_debug
cmake --build .
cmd/unit_test "$*"
