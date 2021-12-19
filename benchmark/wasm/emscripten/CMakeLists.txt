cmake_minimum_required(VERSION 3.10)
project(benchmark LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
  googletest
  # URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
  URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.6.0
)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 8.0.1
)
set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_MakeAvailable(googletest googlebenchmark fmt)

# common
set(JS_NAME ${PROJECT_NAME})
set(JS_SIMD_NAME ${PROJECT_NAME}_simd)
set(JS_THREADS_NAME ${PROJECT_NAME}_threads)
set(JS_SIMD_THREADS_NAME ${PROJECT_NAME}_simd_threads)
set(sources "wasm.cc")

if(CMAKE_CROSSCOMPILING)
add_executable(${JS_NAME} ${sources})
add_executable(${JS_SIMD_NAME} ${sources})
add_executable(${JS_THREADS_NAME} ${sources})
add_executable(${JS_SIMD_THREADS_NAME} ${sources})
target_link_options(${JS_NAME} PRIVATE 
"SHELL:-s TOTAL_MEMORY=196608000"
"SHELL:-s ALLOW_MEMORY_GROWTH=1"
"SHELL:-s USE_PTHREADS=0"
"SHELL:-s MODULARIZE=1"
"SHELL:-s ASSERTIONS=1"
"SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
"SHELL:-s EXPORT_NAME=createBenchmarkModule"
)
# target_compile_options(${JS_SIMD_NAME} PRIVATE "-O3" "-msimd128")
target_link_options(${JS_SIMD_NAME} PRIVATE 
"SHELL:-s TOTAL_MEMORY=196608000"
"SHELL:-s ALLOW_MEMORY_GROWTH=1"
"SHELL:-s USE_PTHREADS=0"
"SHELL:-s MODULARIZE=1"
"SHELL:-s ASSERTIONS=1"
"SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
"SHELL:-s EXPORT_NAME=createBenchmarkSimdModule"
)

target_compile_options(${JS_THREADS_NAME} PRIVATE -matomics -mbulk-memory)
target_link_options(${JS_THREADS_NAME} PRIVATE 
"SHELL:-s TOTAL_MEMORY=196608000"
"SHELL:-s ALLOW_MEMORY_GROWTH=1"
"SHELL:-s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=8"
"SHELL:-s MODULARIZE=1"
"SHELL:-s ASSERTIONS=1"
"SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
"SHELL:-s EXPORT_NAME=createBenchmarkThreadsModule"
)

target_compile_options(${JS_SIMD_THREADS_NAME} PRIVATE -matomics -mbulk-memory)
target_link_options(${JS_SIMD_THREADS_NAME} PRIVATE 
"SHELL:-s TOTAL_MEMORY=196608000"
"SHELL:-s ALLOW_MEMORY_GROWTH=1"
"SHELL:-s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=8"
"SHELL:-s MODULARIZE=1"
"SHELL:-s ASSERTIONS=1"
"SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
"SHELL:-s EXPORT_NAME=createBenchmarkSimdThreadsModule"
)

# OpenCV
include_directories(/opencv/build_wasm/install/include/opencv4)
file(GLOB opencv_core "/opencv/build_wasm/install/lib/*.a")
file(GLOB opencv_3rdparty "/opencv/build_wasm/install/lib/opencv4/3rdparty/*.a")
target_link_libraries(${JS_NAME} ${opencv_core} ${opencv_3rdparty})

file(GLOB opencv_core_simd "/opencv/build_wasm_simd/install/lib/*.a")
file(GLOB opencv_3rdparty_simd "/opencv/build_wasm_simd/install/lib/opencv4/3rdparty/*.a")
target_link_libraries(${JS_SIMD_NAME} ${opencv_core_simd} ${opencv_3rdparty_simd})

file(GLOB opencv_core_threads "/opencv/build_wasm_threads/install/lib/*.a")
file(GLOB opencv_3rdparty_threads "/opencv/build_wasm_threads/install/lib/opencv4/3rdparty/*.a")
target_link_libraries(${JS_THREADS_NAME} ${opencv_core_threads} ${opencv_3rdparty_threads})

file(GLOB opencv_core_simd_threads "/opencv/build_wasm_simd_threads/install/lib/*.a")
file(GLOB opencv_3rdparty_simd_threads "/opencv/build_wasm_simd_threads/install/lib/opencv4/3rdparty/*.a")
target_link_libraries(${JS_SIMD_THREADS_NAME} ${opencv_core_simd_threads} ${opencv_3rdparty_simd_threads})
else()
  find_package(OpenCV REQUIRED)
endif()

# Halide
include_directories(/Halide/include)

if(CMAKE_CROSSCOMPILING)
file(GLOB halide_libs "lib/*.a")
target_link_libraries(${JS_NAME} ${halide_libs})

file(GLOB halide_libs_simd "lib_simd/*.a")
target_link_libraries(${JS_SIMD_NAME} ${halide_libs_simd})

file(GLOB halide_libs_threads "lib_threads/*.a")
target_link_libraries(${JS_THREADS_NAME} ${halide_libs_threads})

file(GLOB halide_libs_simd_threads "lib_simd_threads/*.a")
target_link_libraries(${JS_SIMD_THREADS_NAME} ${halide_libs_simd_threads})
else()
file(GLOB halide_libs_native "lib_native/*.a")
endif()

# # Test
# add_executable(wasm_test wasm_test.cc)
# target_link_libraries(wasm_test gtest_main)
# set_property(TARGET wasm_test PROPERTY SUFFIX ".html")
# target_link_options(wasm_test PRIVATE "--emrun")

# Benchmark
if(CMAKE_CROSSCOMPILING)
  add_executable(wasm_perf wasm_perf.cc wasm.cc)
  target_include_directories(wasm_perf PUBLIC "/opencv/build_wasm/install/include/opencv4")
  target_link_libraries(wasm_perf benchmark_main ${opencv_core} ${opencv_3rdparty} ${halide_libs})
  target_link_options(wasm_perf PRIVATE "--emrun")
  set_property(TARGET wasm_perf PROPERTY SUFFIX ".html")
  target_link_options(wasm_perf PRIVATE 
    "SHELL:--embed-file ../images@/images"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    # "SHELL:-s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=16 -s PROXY_TO_PTHREAD"
    # "SHELL:-s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=16"
    "SHELL:-s USE_PTHREADS=0"
    "SHELL:-s TOTAL_MEMORY=183500800"
  )
else()
  add_executable(wasm_perf wasm_perf.cc wasm.cc)
  find_package(OpenCV REQUIRED)
  find_package(Halide REQUIRED)
  target_include_directories(wasm_perf PUBLIC ${OpenCV_INCLUDE_DIRS})
  target_link_libraries(wasm_perf benchmark_main ${OpenCV_LIBRARIES} ${halide_libs_native} dl)
endif()
