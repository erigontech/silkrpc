#!/bin/bash

cd build_gcc_release
cmake --build .
cmd/unit_test "$*"
