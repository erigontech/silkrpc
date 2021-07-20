#!/bin/bash

cd build_gcc_debug
cmake --build . --parallel
cmd/unit_test
