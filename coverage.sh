#!/bin/sh
# Simple script to do an lcov report based on make check
make clean
make CFLAGS="-static -O0 -g -Wall -fprofile-arcs -ftest-coverage" check
lcov --directory src/ --capture --output-file coverage/carc.info
genhtml -o coverage coverage/carc.info
make clean
