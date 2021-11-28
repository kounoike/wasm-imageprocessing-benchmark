cmake_minimum_required(VERSION 3.10)
project(benchmark LANGUAGES CXX)

# common
set(CMAKE_CXX_STANDARD 17)
set(JS_NAME ${PROJECT_NAME}.js)
set(JS_SIMD_NAME ${PROJECT_NAME}_simd.js)
add_link_options(
  "SHELL:-s INITIAL_MEMORY=196608000"
  "SHELL:-s ALLOW_MEMORY_GROWTH=1"
  "SHELL:-s USE_PTHREADS=0"
  "SHELL:-s MODULARIZE=1"
  "SHELL:-s ASSERTIONS=1"
  "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
)
add_compile_options(
  "-O3"
)
file(GLOB sources "*.cc")
add_executable(${JS_NAME} ${sources})
add_executable(${JS_SIMD_NAME} ${sources})
target_link_options(${JS_NAME} PRIVATE 
  "SHELL:-s EXPORT_NAME=createBenchmarkModule"
)
target_link_options(${JS_SIMD_NAME} PRIVATE 
  "SHELL:-s EXPORT_NAME=createBenchmarkSimdModule"
)

# OpenCV
include_directories(/opencv_build/wasm/include/opencv4)
file(GLOB opencv_core "/opencv_build/wasm/lib/*.a")
file(GLOB opencv_3rdparty "/opencv_build/wasm/lib/opencv4/3rdparty/*.a")
target_link_libraries(${JS_NAME} ${opencv_core} ${opencv_3rdparty})

file(GLOB opencv_core_simd "/opencv_build/wasm_simd/lib/*.a")
file(GLOB opencv_3rdparty_simd "/opencv_build/wasm_simd/lib/opencv4/3rdparty/*.a")
target_link_libraries(${JS_SIMD_NAME} ${opencv_core_simd} ${opencv_3rdparty_simd})

# Halide
include_directories(/Halide/include)

file(GLOB halide_libs "lib/*.a")
target_link_libraries(${JS_NAME} ${halide_libs})

file(GLOB halide_libs_simd "lib_simd/*.a")
target_link_libraries(${JS_SIMD_NAME} ${halide_libs_simd})



