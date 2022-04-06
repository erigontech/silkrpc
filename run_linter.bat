@echo off

REM Run cpplint
REM py cpplint.py \
REM     cmd/*.cpp \
REM     examples/*.cpp \
REM     silkrpc/*.hpp silkrpc/*.cpp silkrpc/**/*.hpp silkrpc/**/*.cpp silkrpc/**/**/*.hpp silkrpc/**/**/*.cpp
py cpplint.py --version

REM Run cpplint
REM pylint tests
pylint --version
