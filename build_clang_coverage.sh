#!/bin/bash

PROJECT_DIR=$PWD
cd build_clang_coverage
cmake --build .
cmd/unit_test "$*"
mv default.profraw unit_test.profraw
llvm-profdata merge *.profraw -o profdata
llvm-cov export -instr-profile profdata -format=lcov cmd/unit_test > silkrpc.lcov
