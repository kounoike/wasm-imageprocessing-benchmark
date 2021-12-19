#include <benchmark/benchmark.h>

#include <functional>
#include <opencv2/opencv.hpp>

#ifdef EMSCRIPTEN
const char *imgDir = "/images";
const char *imgPath = "/images/image.png";
#else
const char *imgDir = "../images";
const char *imgPath = "../images/image.png";
#endif

#include "wasm.hpp"

template <int func(int, int)>
void BM_Template(benchmark::State &state) {
  const int width = state.range(0);
  const int height = state.range(1);

  cv::Mat origImg = cv::imread(imgPath, cv::IMREAD_COLOR);
  if (origImg.empty()) {
    state.SkipWithError("Load image error");
  }
  cv::Mat img;
  cv::resize(origImg, img, cv::Size(width, height));
  if (img.empty()) {
    state.SkipWithError("Resize error");
  }

  unsigned char *inputImageBuffer = getPreprocessInputImageBufferOffset();
  cv::Mat inputMat(height, width, CV_8UC3, inputImageBuffer);
  // float *outputImageBuffer = getPreprocessOutputImageBufferOffset();

  for (auto _ : state) {
    func(width, height);
  }
}

BENCHMARK_TEMPLATE(BM_Template, preprocess_naive)
    ->Name("naive")
    ->Args({320, 180})
    ->Args({640, 360})
    ->Args({1280, 720})
    ->Args({1920, 1080});

BENCHMARK_TEMPLATE(BM_Template, preprocess_lut)
    ->Name("lut")
    ->Args({320, 180})
    ->Args({640, 360})
    ->Args({1280, 720})
    ->Args({1920, 1080});

BENCHMARK_TEMPLATE(BM_Template, preprocess_thread_naive)
    ->Name("thread_naive")
    ->Args({320, 180})
    ->Args({640, 360})
    ->Args({1280, 720})
    ->Args({1920, 1080});

BENCHMARK_TEMPLATE(BM_Template, preprocess_thread_lut)
    ->Name("thread_lut")
    ->Args({320, 180})
    ->Args({640, 360})
    ->Args({1280, 720})
    ->Args({1920, 1080});

BENCHMARK_TEMPLATE(BM_Template, preprocess_opencv)
    ->Name("opencv")
    ->Args({320, 180})
    ->Args({640, 360})
    ->Args({1280, 720})
    ->Args({1920, 1080});

BENCHMARK_TEMPLATE(BM_Template, preprocess_opencv_lut)
    ->Name("opencv_lut")
    ->Args({320, 180})
    ->Args({640, 360})
    ->Args({1280, 720})
    ->Args({1920, 1080});

BENCHMARK_TEMPLATE(BM_Template, preprocess_halide)
    ->Name("halide")
    ->Args({320, 180})
    ->Args({640, 360})
    ->Args({1280, 720})
    ->Args({1920, 1080});

BENCHMARK_TEMPLATE(BM_Template, preprocess_halide_lut)
    ->Name("halide_lut")
    ->Args({320, 180})
    ->Args({640, 360})
    ->Args({1280, 720})
    ->Args({1920, 1080});
