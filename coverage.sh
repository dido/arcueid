#!/bin/sh
# Simple script to do an lcov report based on make check
make clean
make CFLAGS="-static -O0 -g -Wall -fprofile-arcs -ftest-coverage" check -j4
lcov --directory src/ --capture --output-file coverage/arcueid.info
genhtml -o coverage coverage/arcueid.info
make clean
