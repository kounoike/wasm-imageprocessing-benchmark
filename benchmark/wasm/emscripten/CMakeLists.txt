cmake_minimum_required(VERSION 3.10)
project(benchmark LANGUAGES CXX)

# common
set(CMAKE_CXX_STANDARD 17)
set(JS_NAME ${PROJECT_NAME}.js)
set(JS_SIMD_NAME ${PROJECT_NAME}_simd.js)
add_compile_options(
  "-O3"
)
set(sources "wasm.cc")
add_executable(${JS_NAME} ${sources})
add_executable(${JS_SIMD_NAME} ${sources})
target_link_options(${JS_NAME} PRIVATE 
"SHELL:-s INITIAL_MEMORY=196608000"
"SHELL:-s ALLOW_MEMORY_GROWTH=1"
"SHELL:-s USE_PTHREADS=0"
"SHELL:-s MODULARIZE=1"
"SHELL:-s ASSERTIONS=1"
"SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
"SHELL:-s EXPORT_NAME=createBenchmarkModule"
)
# target_compile_options(${JS_SIMD_NAME} PRIVATE "-O3" "-msimd128")
target_link_options(${JS_SIMD_NAME} PRIVATE 
"SHELL:-s INITIAL_MEMORY=196608000"
"SHELL:-s ALLOW_MEMORY_GROWTH=1"
"SHELL:-s USE_PTHREADS=0"
"SHELL:-s MODULARIZE=1"
"SHELL:-s ASSERTIONS=1"
"SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
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


# Test
include(FetchContent)
FetchContent_Declare(
  googletest
  # URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
  URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(wasm_test wasm_test.cc)
target_link_libraries(wasm_test gtest_main)
set_property(TARGET wasm_test PROPERTY SUFFIX ".html")
target_link_options(wasm_test PRIVATE "--emrun")
