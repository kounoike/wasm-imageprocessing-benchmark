#include <emscripten.h>
#include <algorithm>

// for OpenCV
#include <opencv2/opencv.hpp>

// for Halide
#include <HalideBuffer.h>
#include "lib/preprocess_task.h"
#include "lib/preprocess_task_lut.h"

namespace {
  const int MAX_WIDTH = 1920;
  const int MAX_HEIGHT = 1080;

  // for preprocess tasks
  unsigned char preprocessInputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 4];
  float preprocessOutputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 3];

  // for LUT execution cache
  float lut[256];
  bool lut_computed = false;
}

extern "C" {
  // 共通
  EMSCRIPTEN_KEEPALIVE
  unsigned char * getPreprocessInputImageBufferOffset() {
    return preprocessInputImageBuffer;
  }

  EMSCRIPTEN_KEEPALIVE
  float * getPreprocessOutputImageBufferOffset() {
    return preprocessOutputImageBuffer;
  }

  // naive copy
  EMSCRIPTEN_KEEPALIVE
  int preprocess_naive(int width, int height) {
    for(int i = 0; i < width * height; ++i) {
      preprocessOutputImageBuffer[i * 3] = preprocessInputImageBuffer[i * 4] / 255.f;
      preprocessOutputImageBuffer[i * 3 + 1] = preprocessInputImageBuffer[i * 4 + 1] / 255.f;
      preprocessOutputImageBuffer[i * 3 + 2] = preprocessInputImageBuffer[i * 4 + 2] / 255.f;
    }
    return 0;
  }

  // by LUT
  EMSCRIPTEN_KEEPALIVE
  int preprocess_lut(int width, int height) {
    if (!lut_computed) {
      for(int i = 0; i < 256; ++i) {
        lut[i] = i / 255.f;
      }
      lut_computed = true;
    }
    for(int i = 0; i < width * height; ++i) {
      preprocessOutputImageBuffer[i * 3] = lut[preprocessInputImageBuffer[i * 4]];
      preprocessOutputImageBuffer[i * 3 + 1] = lut[preprocessInputImageBuffer[i * 4 + 1]];
      preprocessOutputImageBuffer[i * 3 + 2] = lut[preprocessInputImageBuffer[i * 4 + 2]];
    }
    return 0;
  }

  // by OpenCV
  EMSCRIPTEN_KEEPALIVE
  int preprocess_opencv(int width, int height) {
    cv::Mat inputImageMat(height, width, CV_8UC4, preprocessInputImageBuffer);
    cv::Mat tmpMat;
    cv::Mat outputImageMat(height, width, CV_32FC3, preprocessOutputImageBuffer);
    cv::cvtColor(inputImageMat, tmpMat, cv::COLOR_RGBA2RGB, 0);
    tmpMat.convertTo(outputImageMat, CV_32F, 1.f/255.f, 0);
    return 0;
  }

  // by OpenCV LUT
  EMSCRIPTEN_KEEPALIVE
  int preprocess_opencv_lut(int width, int height) {
    if (!lut_computed) {
      for(int i = 0; i < 256; ++i) {
        lut[i] = i / 255.f;
      }
      lut_computed = true;
    }
    cv::Mat lutMat(1, 256, CV_32FC1, lut);
    cv::Mat inputImageMat(height, width, CV_8UC4, preprocessInputImageBuffer);
    cv::Mat tmpMat;
    cv::Mat outputImageMat(height, width, CV_32FC3, preprocessOutputImageBuffer);

    cv::cvtColor(inputImageMat, tmpMat, cv::COLOR_RGBA2RGB, 0);
    cv::LUT(tmpMat, lutMat, outputImageMat);
    return 0;
  }

  // by Halide
  EMSCRIPTEN_KEEPALIVE
  int preprocess_halide(int width, int height) {
    auto hal_src = Halide::Runtime::Buffer<uint8_t>::make_interleaved(preprocessInputImageBuffer, width, height, 4);
    auto hal_dst = Halide::Runtime::Buffer<float>::make_interleaved(preprocessOutputImageBuffer, width, height, 3);

    preprocess_task(hal_src, hal_dst);
    return 0;
  }

  // by Halide LUT
  EMSCRIPTEN_KEEPALIVE
  int preprocess_halide_lut(int width, int height) {
    auto hal_src = Halide::Runtime::Buffer<uint8_t>::make_interleaved(preprocessInputImageBuffer, width, height, 4);
    auto hal_dst = Halide::Runtime::Buffer<float>::make_interleaved(preprocessOutputImageBuffer, width, height, 3);

    preprocess_task_lut(hal_src, hal_dst);
    return 0;
  }
}

