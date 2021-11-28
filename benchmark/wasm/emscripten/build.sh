#!/bin/bash

set -xe

cd /emsdk
. ./emsdk_env.sh

# halide generator
pushd /wasm/halide
  g++ -o /tmp/gen preprocess.cpp preprocess_lut.cpp postprocess.cpp /Halide/share/Halide/tools/GenGen.cpp -I /Halide/include/ -lHalide -L /Halide/lib/ -lm -lpthread -ldl -std=c++17 -Wl,-rpath /Halide/lib
  mkdir -p ../lib ../lib_simd
  /tmp/gen -g preprocess_task target=wasm-32-wasmrt-wasm_bulk_memory -o ../lib
  /tmp/gen -g preprocess_task target=wasm-32-wasmrt-wasm_simd128-wasm_threads-wasm_bulk_memory -o ../lib_simd
  /tmp/gen -g preprocess_task_lut target=wasm-32-wasmrt-wasm_bulk_memory -o ../lib
  /tmp/gen -g preprocess_task_lut target=wasm-32-wasmrt-wasm_simd128-wasm_threads-wasm_bulk_memory -o ../lib_simd
  /tmp/gen -g postprocess_task target=wasm-32-wasmrt-wasm_bulk_memory -o ../lib
  /tmp/gen -g postprocess_task target=wasm-32-wasmrt-wasm_simd128-wasm_threads-wasm_bulk_memory -o ../lib_simd
popd

pushd /wasm
  [ -e build ] && rm -r build
  mkdir -p build
  pushd build
    emmake cmake -DCMAKE_BUILD_TYPE=Release ..
    emmake cmake --build . --config Release
    mv *.wasm *.js /wasm/bin
  popd
  rm -r build
popd
