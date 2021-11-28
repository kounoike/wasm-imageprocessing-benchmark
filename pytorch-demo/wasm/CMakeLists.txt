cmake_minimum_required(VERSION 3.10)
project(demo LANGUAGES CXX)

# common
set(CMAKE_CXX_STANDARD 17)
set(JS_NAME ${PROJECT_NAME}.js)

add_link_options(
  "SHELL:-s INITIAL_MEMORY=196608000"
  "SHELL:-s ALLOW_MEMORY_GROWTH=1"
  "SHELL:-s USE_PTHREADS=0"
  "SHELL:-s MODULARIZE=1"
  "SHELL:-s ASSERTIONS=2"
  "SHELL:-s LLD_REPORT_UNDEFINED=1"
  "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
  "SHELL:--embed-file /wasm/FaceParsing.pt@/FaceParsing.pt"
  "SHELL:--no-entry"
)
file(GLOB sources "*.cc")
add_executable(${JS_NAME} ${sources})
target_link_options(${JS_NAME} PRIVATE 
  "SHELL:-s EXPORT_NAME=createDemoModule"
)

# OpenCV
include_directories(/opencv_build/wasm/include/opencv4)
file(GLOB opencv_core_simd "/opencv_build/wasm_simd/lib/*.a")
file(GLOB opencv_3rdparty_simd "/opencv_build/wasm_simd/lib/opencv4/3rdparty/*.a")
target_link_libraries(${JS_NAME} ${opencv_core_simd} ${opencv_3rdparty_simd})

# PyTorch
include_directories(/pytorch_build/include /pytorch_build/include/torch/csrc/api/include)

file(GLOB pytorch_libs "/pytorch_build/lib/*.a")
target_link_libraries(${JS_NAME} ${pytorch_libs})

