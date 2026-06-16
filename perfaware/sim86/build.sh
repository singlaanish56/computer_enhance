#!/usr/bin/env bash
set -e

mkdir -p build
pushd build >/dev/null

clang++ -g ../sim86.cpp -o sim86_clang_debug
clang++ -O3 -g ../sim86.cpp -o sim86_clang_release

popd >/dev/null