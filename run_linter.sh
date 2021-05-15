#!/bin/bash

cpplint silkrpc/**/*.hpp silkrpc/**/*.cpp silkrpc/**/**/*.hpp silkrpc/**/**/*.cpp

pylint tests
