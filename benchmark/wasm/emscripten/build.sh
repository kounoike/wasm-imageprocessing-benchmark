#!/bin/bash

set -xe

cd /emsdk
. ./emsdk_env.sh

# halide generator
AUTOSCHEDULER=Adams2019
AUTOSCHEDULER_SO=/Halide/lib/libautoschedule_adams2019.so
# AUTOSCHEDULER=Li2018
# AUTOSCHEDULER_SO=/Halide/lib/libautoschedule_li2018.so
# AUTOSCHEDULER=Mullapudi2016
# AUTOSCHEDULER_SO=/Halide/lib/libautoschedule_mullapudi2016.so
MACHINE_PARAMS="machine_params=8,16777216,10 -e schedule"
pushd /wasm/halide
  g++ -o /tmp/gen preprocess.cpp preprocess_lut.cpp postprocess.cpp /Halide/share/Halide/tools/GenGen.cpp -I /Halide/include/ -lHalide -L /Halide/lib/ -lm -lpthread -ldl -std=c++17 -Wl,-rpath /Halide/lib
  mkdir -p ../lib ../lib_simd ../lib_threads ../lib_simd_threads
  /tmp/gen -g preprocess_task target=wasm-32-wasmrt-wasm_bulk_memory -o ../lib -p $AUTOSCHEDULER_SO -s $AUTOSCHEDULER auto_schedule=true $MACHINE_PARAMS
  /tmp/gen -g preprocess_task target=wasm-32-wasmrt-wasm_simd128-wasm_bulk_memory -o ../lib_simd -p $AUTOSCHEDULER_SO -s $AUTOSCHEDULER auto_schedule=true $MACHINE_PARAMS
  /tmp/gen -g preprocess_task target=wasm-32-wasmrt-wasm_bulk_memory-wasm_threads -o ../lib_threads -p $AUTOSCHEDULER_SO -s $AUTOSCHEDULER auto_schedule=true $MACHINE_PARAMS
  /tmp/gen -g preprocess_task target=wasm-32-wasmrt-wasm_simd128-wasm_bulk_memory-wasm_threads -o ../lib_simd_threads -p $AUTOSCHEDULER_SO -s $AUTOSCHEDULER auto_schedule=true $MACHINE_PARAMS
  /tmp/gen -g preprocess_task_lut target=wasm-32-wasmrt-wasm_bulk_memory -o ../lib -p $AUTOSCHEDULER_SO -s $AUTOSCHEDULER auto_schedule=true $MACHINE_PARAMS
  /tmp/gen -g preprocess_task_lut target=wasm-32-wasmrt-wasm_simd128-wasm_threads-wasm_bulk_memory -o ../lib_simd -p $AUTOSCHEDULER_SO -s $AUTOSCHEDULER auto_schedule=true $MACHINE_PARAMS
  /tmp/gen -g preprocess_task_lut target=wasm-32-wasmrt-wasm_bulk_memory-wasm_threads -o ../lib_threads -p $AUTOSCHEDULER_SO -s $AUTOSCHEDULER auto_schedule=true $MACHINE_PARAMS
  /tmp/gen -g preprocess_task_lut target=wasm-32-wasmrt-wasm_simd128-wasm_threads-wasm_bulk_memory-wasm_threads -o ../lib_simd_threads -p $AUTOSCHEDULER_SO -s $AUTOSCHEDULER auto_schedule=true $MACHINE_PARAMS
  /tmp/gen -g postprocess_task target=wasm-32-wasmrt-wasm_bulk_memory -o ../lib -p $AUTOSCHEDULER_SO -s $AUTOSCHEDULER auto_schedule=true $MACHINE_PARAMS
  /tmp/gen -g postprocess_task target=wasm-32-wasmrt-wasm_simd128-wasm_bulk_memory -o ../lib_simd -p $AUTOSCHEDULER_SO -s $AUTOSCHEDULER auto_schedule=true $MACHINE_PARAMS
  /tmp/gen -g postprocess_task target=wasm-32-wasmrt-wasm_bulk_memory-wasm_threads -o ../lib_threads -p $AUTOSCHEDULER_SO -s $AUTOSCHEDULER auto_schedule=true $MACHINE_PARAMS
  /tmp/gen -g postprocess_task target=wasm-32-wasmrt-wasm_simd128-wasm_bulk_memory-wasm_threads -o ../lib_simd_threads -p $AUTOSCHEDULER_SO -s $AUTOSCHEDULER auto_schedule=true $MACHINE_PARAMS
popd

pushd /wasm
  [ -e build ] && rm -r build
  mkdir -p build
  pushd build
    emmake cmake -DCMAKE_BUILD_TYPE=Release ..
    emmake cmake --build . --config Release
    emrun --browser chrome --browser_args="--headless --disable-gpu --no-sandbox --remote-debugging-port=9222" wasm_test.html -- --gtest_color=yes
    # emrun --browser firefox --browser_args="--headless" wasm_test.html
    mv *.wasm *.js /wasm/bin
  popd
  rm -r build
popd
