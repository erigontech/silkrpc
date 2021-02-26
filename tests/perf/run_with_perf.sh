#!/bin/bash

sudo perf stat --topdown -a -- taskset -c 0 build_gcc_release/silkrpc/silkrpcdaemon --target localhost:9090
