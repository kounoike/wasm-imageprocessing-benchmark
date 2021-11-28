# syntax=docker/dockerfile:1
FROM ubuntu:20.04 AS base
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y
RUN apt-get install -y --no-install-recommends \
  build-essential \
  ca-certificates \
  cmake \
  curl \
  git \
  gnupg \
  nodejs \
  python \
  python3-dev \
  python3-numpy \
  rsync \
  wget

### EMSCRIPTEN for base
RUN git clone https://github.com/emscripten-core/emsdk.git --depth 1
WORKDIR /emsdk
RUN ./emsdk install 3.0.0 && ./emsdk activate 3.0.0


# ### STAGE:tensorflow_builder
# FROM base AS tensorflow_builder

# RUN curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > /etc/apt/trusted.gpg.d/bazel.gpg
# RUN echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" > /etc/apt/sources.list.d/bazel.list
# RUN apt-get update -y && apt-get install -y --no-install-recommends bazel-3.7.2
# RUN ln -s /usr/bin/bazel-3.7.2 /usr/bin/bazel

# ### MediaPipe
# WORKDIR /
# RUN git clone https://github.com/google/mediapipe.git -b v0.8.4 --depth 1

# ### Tensorflow
# WORKDIR /
# RUN git clone https://github.com/tensorflow/tensorflow.git tensorflow_src
# RUN git -C /tensorflow_src checkout 9d461da4cb0af2f737bbfc68cca3f6445f1ceb60  # May 15, 2021 latest

# RUN sed -i 's/"crosstool_top": "\/\/external:android\/emscripten"/"crosstool_top": "\/\/emscripten_toolchain\/everything"/' /tensorflow_src/tensorflow/BUILD

# WORKDIR /tflite
# COPY ./tflite/* ./
# RUN bazel build --config wasm -c opt :dummy
# WORKDIR /tflite_build/wasm
# RUN rsync -a -L --include '*/' --include '*.a' --include '*.lo' --include '*.h*' --exclude '*' /tflite/bazel-bin/ ./
# WORKDIR /tflite
# RUN bazel build --config wasm_simd -c opt :dummy
# WORKDIR /tflite_build/wasm_simd
# RUN rsync -a -L --include '*/' --include '*.a' --include '*.lo' --include '*.h*' --exclude '*' /tflite/bazel-bin/ ./


### STAGE: OpenCV_builder
FROM base AS opencv_builder
ENV OPENCV_CONFIG_FLAG="\
  --config_only \
  --emscripten_dir=/emsdk/upstream/emscripten \
  --cmake_option=-DBUILD_LIST=core,imgproc,imgcodecs \
  --cmake_option=-DBUILD_opencv_imgcodecs=ON \
  --cmake_option=-DBUILD_opencv_js=OFF \
  --cmake_option=-DWITH_PNG=ON \
  --cmake_option=-DWITH_JPEG=ON \
  --cmake_option=-DWITH_TIFF=OFF \
  --cmake_option=-DWITH_WEBP=OFF \
  --cmake_option=-DWITH_OPENJPEG=OFF \
  --cmake_option=-DWITH_JASPER=OFF \
  --cmake_option=-DWITH_OPENEXR=OFF \
  "

WORKDIR /
RUN git clone https://github.com/opencv/opencv.git -b 4.5.4 --depth 1
RUN git -C /opencv checkout -b 4.5.4
WORKDIR /opencv
RUN python3  platforms/js/build_js.py build_wasm              ${OPENCV_CONFIG_FLAG}
RUN python3  platforms/js/build_js.py build_wasm_simd         ${OPENCV_CONFIG_FLAG} --simd
RUN python3  platforms/js/build_js.py build_wasm_simd_threads ${OPENCV_CONFIG_FLAG} --simd --threads

ENV OPENCV_JS_WHITELIST /opencv/platforms/js/opencv_js.config.py

RUN /emsdk/upstream/emscripten/emmake cmake --build /opencv/build_wasm --config Release
RUN /emsdk/upstream/emscripten/emmake cmake --build /opencv/build_wasm_simd --config Release
# RUN /emsdk/upstream/emscripten/emmake cmake --build /opencv/build_wasm_simd_threads --config Release

RUN mkdir -p /opencv_build/wasm && /emsdk/upstream/emscripten/emmake cmake --install /opencv/build_wasm --prefix /opencv_build/wasm
RUN mkdir -p /opencv_build/wasm_simd && /emsdk/upstream/emscripten/emmake cmake --install /opencv/build_wasm_simd --prefix /opencv_build/wasm_simd
# RUN mkdir -p /opencv_build/wasm_simd_threads && /emsdk/upstream/emscripten/emmake cmake --install /opencv/build_wasm_simd_threads --prefix /opencv_build/wasm_simd_threads

# ### STAGE:onnx_builder
# FROM base AS onnx_builder
# WORKDIR /
# RUN git clone https://github.com/microsoft/onnxruntime.git
# WORKDIR /onnxruntime
# RUN python3 tools/ci_build/build.py --build_dir /onnxruntime/build/wasm --build_wasm --enable_wasm_threads --config Release --parallel --skip_tests

### STAGE:Halide_builder
FROM base AS halide_builder
ENV HALIDE_RELEASE_PKG=https://github.com/halide/Halide/releases/download/v13.0.1/Halide-13.0.1-x86-64-linux-fb39d7e3149c0ee1e848bb9957be2ea18765c35d.tar.gz
WORKDIR /
RUN wget -O halide.tar.gz ${HALIDE_RELEASE_PKG}
RUN tar zxvf halide.tar.gz
RUN mv Halide-* Halide


FROM base AS pytorch_builder
RUN apt-get install -y --no-install-recommends python3 python3-venv python3-pip unzip wget
WORKDIR /
RUN git clone --recursive https://github.com/pytorch/pytorch.git -b v1.9.0 --depth 1
COPY *.patch .
WORKDIR /pytorch
RUN git checkout -b v1.9.0
RUN patch -p1 < /pytorch.patch
WORKDIR /pytorch/third_party/cpuinfo
RUN patch -p1 < /cpuinfo.patch
WORKDIR /pytorch/third_party/XNNPACK
RUN patch -p1 < /xnnpack.patch
WORKDIR /pytorch/build
RUN pip3 install -r ../requirements.txt
ARG CMAKE_OPTS="\
  -DBUILD_TEST=OFF \
  -DBUILD_PYTHON=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -DUSE_LLVM=ON \
  -DUSE_CCACHE=OFF \
  -DUSE_CUDA=OFF \
  -DUSE_ROCM=OFF \
  -DUSE_MKLDNN=OFF \
  -DUSE_NUMPY=OFF \
  -DUSE_KINETO=OFF \
  -DUSE_DISTRIBUTED=OFF \
  -DUSE_PYTORCH_QNNPACK=OFF \
  -DUSE_NNPACK=OFF \
  -DUSE_QNNPACK=OFF \
  -DUSE_XNNPACK=ON \
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
  -DPYTHON_EXECUTABLE=/usr/bin/python3 \
  -DCMAKE_CXX_FLAGS=-msimd128 \
  "
#/emsdk/upstream/emscripten/emcmake cmake   -DBUILD_TEST=OFF   -DBUILD_PYTHON=OFF   -DBUILD_SHARED_LIBS=OFF   -DUSE_CUDA=OFF 
# -DUSE_ROCM=OFF   -DUSE_MKLDNN=OFF   -DUSE_NUMPY=OFF   -DUSE_KINETO=OFF   -DUSE_DISTRIBUTED=OFF   -DUSE_EIGEN_FOR_BLAS=ON
# -DUSE_PYTORCH_QNNPACK=OFF   -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY   -DCOMPILER_WORKS=TRUE  
# -DLIBRARY_TYPE=STATIC   -DPYTHON_EXECUTABLE=/usr/bin/python3 -DUSE_LLVM=ON 
# -DCOMPILE_OPTIONS='-fexceptions -msimd128' -DCMAKE_CXX_FLAGS='-fexceptions -msimd128' ..

# /emsdk/upstream/emscripten/emcmake cmake -DBUILD_TEST=OFF   -DBUILD_PYTHON=OFF   -DBUILD_SHARED_LIBS=OFF   -DUSE_LLVM=ON   -DUSE_CUDA=OFF   -DUSE_ROCM=OFF   -DUSE_MKLDNN=OFF   -DUSE_NUMPY=OFF   -DUSE_KINETO=OFF   -DUSE_DISTRIBUTED=OFF   -DUSE_PYTORCH_QNNPACK=OFF   -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY   -DPYTHON_EXECUTABLE=/usr/bin/python3   -DINTERN_BUILD_MOBILE=ON   -DCMAKE_CXX_FLAGS="-fexceptions -msimd128" -G"Unix Makefiles" ..

ENV BUILD_PYTORCH_MOBILE_WITH_HOST_TOOLCHAIN=1
RUN /emsdk/upstream/emscripten/emcmake cmake ${CMAKE_OPTS} ..
RUN /emsdk/upstream/emscripten/emmake cmake --build . --config Release -j$(nproc)
RUN /emsdk/upstream/emscripten/emmake cmake --install . --prefix /pytorch_build

###
FROM base AS artifact
WORKDIR /
# COPY --from=tensorflow_builder /tensorflow_src /tensorflow_src
# COPY --from=tensorflow_builder /tflite_build /tflite_build
# COPY --from=tensorflow_builder /mediapipe /mediapipe
COPY --from=opencv_builder /opencv_build /opencv_build
# COPY --from=onnx_builder /onnxruntime /onnxruntime
COPY --from=halide_builder /Halide /Halide
# COPY --from=pytorch_builder /libtorch_build /libtorch_build
# COPY --from=pytorch_builder /pytorch /pytorch
# COPY --from=pytorch_builder /protoc /protoc
# COPY --from=pytorch_builder /pytorch_build /pytorch_build

# COPY --from=tensorflow_builder /root/.cache /root/.cache
# COPY --from=tensorflow_builder /tflite /tflite
