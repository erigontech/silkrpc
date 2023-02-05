#!/bin/bash

cpplint \
    cmd/*.cpp \
    examples/*.cpp \
    silkworm/silkrpc/*.hpp silkworm/silkrpc/*.cpp silkworm/silkrpc/**/*.hpp silkworm/silkrpc/**/*.cpp silkworm/silkrpc/**/**/*.hpp silkworm/silkrpc/**/**/*.cpp

if [ $? -ne 0 ];
then
    echo "cpplint failed"
    exit 1
fi

pylint tests

if [ $? -ne 0 ];
then
    echo "pylint failed"
    exit 1
fi

exit 0
