#!/bin/bash
# Public domain / CC0. Use freely for any purpose. RoyR 2026
# run_tests.sh - Quick test runner script

set -e

echo "Building Common compiler..."
gcc -o common common.c

echo "Building test runner..."
gcc -std=c99 -o test_runner test_runner.c

echo ""
echo "Running tests..."
echo "================"
./test_runner

exit $?
