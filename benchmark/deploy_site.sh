#!/bin/bash

set -xe

cd $(dirname $0)

mkdir -p dist

cp index.html index.js _headers dist/

mkdir -p dist/img
cp img/*.jpg dist/img

mkdir -p dist/wasm/emscripten/bin
cp wasm/emscripten/bin/* dist/wasm/emscripten/bin

mkdir -p dist/wasm/rust/pkg
cp wasm/rust/pkg/*.{js,wasm} dist/wasm/rust/pkg
