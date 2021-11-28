#!/bin/bash

set -xe

cd /emsdk
. ./emsdk_env.sh


pushd /wasm
  [ -e build ] && rm -r build
  mkdir -p build
  [ -e bin ] || mkdir -p bin
  pushd build
    emmake cmake -DCMAKE_BUILD_TYPE=Release ..
    emmake cmake --build . --config Release
    mv *.wasm *.js /wasm/bin
  popd
  rm -r build
popd