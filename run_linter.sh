#!/bin/bash

cpplint \
    cmd/*.cpp \
    examples/*.cpp \
    silkrpc/*.hpp silkrpc/*.cpp silkrpc/**/*.hpp silkrpc/**/*.cpp silkrpc/**/**/*.hpp silkrpc/**/**/*.cpp

pylint tests
