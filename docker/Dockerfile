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
# with 2.0.20, building opencv simd version failed...
RUN ./emsdk install 2.0.26 && ./emsdk activate 2.0.26


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
RUN git clone https://github.com/opencv/opencv.git -b 4.5.2 --depth 1
RUN git -C /opencv checkout -b 4.5.2
WORKDIR /opencv
RUN python3  platforms/js/build_js.py build_wasm              ${OPENCV_CONFIG_FLAG}
RUN python3  platforms/js/build_js.py build_wasm_simd         ${OPENCV_CONFIG_FLAG} --simd
RUN python3  platforms/js/build_js.py build_wasm_simd_threads ${OPENCV_CONFIG_FLAG} --simd --threads

ENV OPENCV_JS_WHITELIST /opencv/platforms/js/opencv_js.config.py

RUN /emsdk/upstream/emscripten/emmake cmake --build /opencv/build_wasm
RUN /emsdk/upstream/emscripten/emmake cmake --build /opencv/build_wasm_simd
RUN /emsdk/upstream/emscripten/emmake cmake --build /opencv/build_wasm_simd_threads

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
ENV HALIDE_RELEASE_PKG=https://github.com/halide/Halide/releases/download/v12.0.1/Halide-12.0.1-x86-64-linux-5dabcaa9effca1067f907f6c8ea212f3d2b1d99a.tar.gz
WORKDIR /
RUN wget -O halide.tar.gz ${HALIDE_RELEASE_PKG}
RUN tar zxvf halide.tar.gz
RUN mv Halide-* Halide

###
FROM base AS artifact
WORKDIR /
# COPY --from=tensorflow_builder /tensorflow_src /tensorflow_src
# COPY --from=tensorflow_builder /tflite_build /tflite_build
# COPY --from=tensorflow_builder /mediapipe /mediapipe
COPY --from=opencv_builder /opencv_build /opencv_build
# COPY --from=onnx_builder /onnxruntime /onnxruntime
COPY --from=halide_builder /Halide /Halide

# COPY --from=tensorflow_builder /root/.cache /root/.cache
# COPY --from=tensorflow_builder /tflite /tflite