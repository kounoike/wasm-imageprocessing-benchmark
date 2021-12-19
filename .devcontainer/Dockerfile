ARG VARIANT=ubuntu-20.04
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
  && apt-get -y install --no-install-recommends \
  ca-certificates \
  firefox \
  fonts-liberation \
  gnupg \
  libappindicator1 \
  libdrm2 \
  libgbm1 \
  libjpeg-dev \
  libgtest-dev \
  libnss3 \
  libpng-dev \
  ninja-build \
  python \
  python3-dev \
  python3-numpy \
  xdg-utils \
  libasound2 \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

## node.js sources.list
RUN curl -fsSL https://deb.nodesource.com/setup_16.x | bash -

## yarn sources.list
RUN curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | apt-key add -
RUN echo "deb https://dl.yarnpkg.com/debian/ stable main" > /etc/apt/sources.list.d/yarn.list

RUN apt-get update && apt-get install -y --no-install-recommends nodejs yarn
RUN apt-get update -y \
  && apt-get install -y --no-install-recommends \
  nodejs \
  yarn \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*


RUN curl -O https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb && \
  dpkg -i google-chrome-stable_current_amd64.deb && \
  rm google-chrome-stable_current_amd64.deb

ARG EMSCRIPTEN_VERSION=3.0.0
# ### EMSCRIPTEN for base
WORKDIR /
RUN git clone https://github.com/emscripten-core/emsdk.git --depth 1
WORKDIR /emsdk
RUN ./emsdk install ${EMSCRIPTEN_VERSION} && ./emsdk activate ${EMSCRIPTEN_VERSION}
# RUN echo '. /emsdk/emsdk_env.sh > /dev/null 2>&1' >> /home/vscode/.bashrc

ARG OPENCV_VERSION=4.5.4
WORKDIR /
RUN git clone https://github.com/opencv/opencv.git -b ${OPENCV_VERSION} --depth 1
RUN git -C /opencv checkout -b ${OPENCV_VERSION}
# RUN git clone https://github.com/kounoike/opencv_contrib.git -b 4.5.3-simd --depth 1
# RUN git -C /opencv_contrib checkout 4.5.3-simd
RUN git clone https://github.com/opencv/opencv_contrib.git -b ${OPENCV_VERSION} --depth 1
RUN git -C /opencv_contrib checkout ${OPENCV_VERSION}

ENV OPENCV_CMAKE_FLAGS="\
  -DOPENCV_EXTRA_MODULES_PATH=/opencv_contrib/modules \
  -DBUILD_LIST=core,imgproc,calib3d,video,ximgproc,imgcodecs \
  -DBUILD_opencv_imgcodecs=ON \
  -DBUILD_opencv_js=OFF \
  -DBUILD_TESTS=OFF \
  -DBUILD_PERF_TESTS=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_opencv_apps=OFF \
  -DWITH_PNG=ON \
  -DWITH_JPEG=OFF \
  -DWITH_TIFF=OFF \
  -DWITH_WEBP=OFF \
  -DWITH_OPENJPEG=OFF \
  -DWITH_JASPER=OFF \
  -DWITH_OPENEXR=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  "

ENV OPENCV_CONFIG_FLAG_EMSCRIPTEN="\
  --config_only \
  --emscripten_dir=/emsdk/upstream/emscripten \
  "

ENV EMSDK=/emsdk

WORKDIR /opencv
RUN python3  platforms/js/build_js.py build_wasm                               ${OPENCV_CONFIG_FLAG_EMSCRIPTEN} $(sh -c 'for x in "$@"; do echo "--cmake_option=$x"; done' "" ${OPENCV_CMAKE_FLAGS})
RUN python3  platforms/js/build_js.py build_wasm_simd --simd                   $OPENCV_CONFIG_FLAG_EMSCRIPTEN $(sh -c 'for x in "$@"; do echo "--cmake_option=$x"; done' "" ${OPENCV_CMAKE_FLAGS})
RUN python3  platforms/js/build_js.py build_wasm_threads --threads             $OPENCV_CONFIG_FLAG_EMSCRIPTEN $(sh -c 'for x in "$@"; do echo "--cmake_option=$x"; done' "" ${OPENCV_CMAKE_FLAGS})
RUN python3  platforms/js/build_js.py build_wasm_simd_threads --simd --threads $OPENCV_CONFIG_FLAG_EMSCRIPTEN $(sh -c 'for x in "$@"; do echo "--cmake_option=$x"; done' "" ${OPENCV_CMAKE_FLAGS})

RUN cmake --build /opencv/build_wasm              --parallel $(nproc)
RUN cmake --build /opencv/build_wasm_simd         --parallel $(nproc)
RUN cmake --build /opencv/build_wasm_threads      --parallel $(nproc)
RUN cmake --build /opencv/build_wasm_simd_threads --parallel $(nproc)

RUN cmake --install /opencv/build_wasm
RUN cmake --install /opencv/build_wasm_simd
RUN cmake --install /opencv/build_wasm_threads
RUN cmake --install /opencv/build_wasm_simd_threads

## build for host
WORKDIR /opencv/build_native
RUN cmake ${OPENCV_CMAKE_FLAGS} ..
RUN cmake --build . --config Release --parallel $(nproc)
# RUN cmake --install . --prefix /usr/local
RUN cmake --install /opencv/build_native && ldconfig


# update cmake
ARG CMAKE_VERSION=3.22.1
ARG PLATFORM=linux
WORKDIR /
RUN apt-get purge -y cmake
RUN \
  curl -LO https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-${PLATFORM}-x86_64.tar.gz && \
  tar xf cmake-${CMAKE_VERSION}-${PLATFORM}-x86_64.tar.gz --directory=/usr --strip-components=1 && \
  rm cmake-${CMAKE_VERSION}-${PLATFORM}-x86_64.tar.gz

ENV HALIDE_RELEASE_PKG=https://github.com/halide/Halide/releases/download/v13.0.1/Halide-13.0.1-x86-64-linux-fb39d7e3149c0ee1e848bb9957be2ea18765c35d.tar.gz
WORKDIR /
RUN wget -O halide.tar.gz ${HALIDE_RELEASE_PKG}
RUN tar zxvf halide.tar.gz
RUN mv Halide-* Halide
