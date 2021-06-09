#!/bin/bash

cpplint \
    cmd/*.hpp cmd/*.cpp \
    examples/*.cpp \
    silkrpc/*.hpp silkrpc/*.cpp silkrpc/**/*.hpp silkrpc/**/*.cpp silkrpc/**/**/*.hpp silkrpc/**/**/*.cpp

pylint tests
