docker run -v $(pwd)/wasm/emscripten:/wasm wasm_benchmark_builder /bin/bash /wasm/build.sh
pushd wasm/rust
  wasm-pack build
popd
