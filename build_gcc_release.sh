#!/bin/bash

cd build_gcc_release
cmake --build . --parallel
cmd/unit_test
