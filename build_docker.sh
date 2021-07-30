#!/bin/bash

set -xe

# docker build --build-arg http_proxy=$http_proxy --target tensorflow_builder docker
docker build --build-arg http_proxy=$http_proxy --target opencv_builder docker
# docker build --build-arg http_proxy=$http_proxy --target onnx_builder docker
docker build --build-arg http_proxy=$http_proxy -t wasm_benchmark_builder docker
