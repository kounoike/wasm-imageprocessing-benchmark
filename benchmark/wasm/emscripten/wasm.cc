#include <emscripten.h>
#include <algorithm>
#include <cmath>
#include <iostream>

// for OpenCV
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

// for Halide
#include <HalideBuffer.h>
#include "lib/preprocess_task.h"
#include "lib/preprocess_task_lut.h"
#include "lib/postprocess_task.h"

namespace {
  const int MAX_WIDTH = 1920;
  const int MAX_HEIGHT = 1080;

  // for preprocess tasks
  unsigned char preprocessInputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 4];
  float preprocessOutputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 3];

  // for LUT execution cache
  float lut[256];
  bool lut_computed = false;

  // for postproces tasks
  float postprocessInferenceResultBuffer[MAX_WIDTH * MAX_HEIGHT * 2];
  unsigned char postprocessInputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 4];
  unsigned char postprocessBackgroundImageBuffer[MAX_WIDTH * MAX_HEIGHT * 4];
  unsigned char postprocessOutputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 4];

  // JPEG encode/decode
  const int MAX_JPEG_LENGTH = 20 * 1024 * 1024;
  const int MAX_JPEG_WIDTH = 5000;
  const int MAX_JPEG_HEIGHT = 2700;
  unsigned char jpegImageBuffer[MAX_JPEG_LENGTH];
  unsigned char jpegLoadedImageBuffer[MAX_JPEG_WIDTH * MAX_JPEG_HEIGHT * 3];
}

extern "C" {
  EMSCRIPTEN_KEEPALIVE
  void showExceptionMessage(intptr_t exceptionPtr) {
    std::cout << reinterpret_cast<std::exception *>(exceptionPtr)->what() << std::endl;
  }

  // 共通: 前処理
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

  // 共通: 後処理
  EMSCRIPTEN_KEEPALIVE
  unsigned char * getPostprocessInputImageBufferOffset() {
    return postprocessInputImageBuffer;
  }
  EMSCRIPTEN_KEEPALIVE
  unsigned char * getPostprocessBackgroundImageBufferOffset() {
    return postprocessBackgroundImageBuffer;
  }
  EMSCRIPTEN_KEEPALIVE
  unsigned char * getPostprocessOutputImageBufferOffset() {
    return postprocessOutputImageBuffer;
  }
  EMSCRIPTEN_KEEPALIVE
  float * getPostprocessInferenceResultBufferOffset() {
    return postprocessInferenceResultBuffer;
  }

  // naive
  EMSCRIPTEN_KEEPALIVE
  int postprocess_naive(int width, int height, float threshold) {
    for(int i = 0; i < width * height; ++i) {
      float bg = postprocessInferenceResultBuffer[i * 2];
      float fg = postprocessInferenceResultBuffer[i * 2 + 1];
      float bgexp = std::expf(bg);
      float fgexp = std::expf(fg);
      float val = fgexp / (fgexp + bgexp);
      if (val > threshold) {
        postprocessOutputImageBuffer[i * 4] = postprocessInputImageBuffer[i * 4];
        postprocessOutputImageBuffer[i * 4 + 1] = postprocessInputImageBuffer[i * 4 + 1];
        postprocessOutputImageBuffer[i * 4 + 2] = postprocessInputImageBuffer[i * 4 + 2];
        postprocessOutputImageBuffer[i * 4 + 3] = postprocessInputImageBuffer[i * 4 + 3];
      } else {
        postprocessOutputImageBuffer[i * 4] = postprocessBackgroundImageBuffer[i * 4];
        postprocessOutputImageBuffer[i * 4 + 1] = postprocessBackgroundImageBuffer[i * 4 + 1];
        postprocessOutputImageBuffer[i * 4 + 2] = postprocessBackgroundImageBuffer[i * 4 + 2];
        postprocessOutputImageBuffer[i * 4 + 3] = postprocessBackgroundImageBuffer[i * 4 + 3];
      }
    }
    return 0;
  }

  // OpenCV
  EMSCRIPTEN_KEEPALIVE
  int postprocess_opencv(int width, int height, float threshold) {
    cv::Mat inputImageMat(height, width, CV_8UC4, postprocessInputImageBuffer);
    cv::Mat backgroundImageMat(height, width, CV_8UC4, postprocessBackgroundImageBuffer);
    cv::Mat outputImageMat(height, width, CV_8UC4, postprocessOutputImageBuffer);
    cv::Mat inferenceResultMat(height, width, CV_32FC2, postprocessInferenceResultBuffer);
    cv::Mat ch[2];
    cv::split(inferenceResultMat, ch);
    cv::Mat bgExp, fgExp, sumMat, softmaxMat, maskMat;
    cv::exp(ch[0], bgExp);
    cv::exp(ch[1], fgExp);
    cv::add(bgExp, fgExp, sumMat);
    cv::divide(fgExp, sumMat, softmaxMat);
    softmaxMat.convertTo(maskMat, CV_8U, 255, 0);
    cv::threshold(maskMat, maskMat, 255 * threshold, 255, cv::THRESH_BINARY);
    backgroundImageMat.copyTo(outputImageMat);
    inputImageMat.copyTo(outputImageMat, maskMat);

    return 0;
  }

  // Halide
  EMSCRIPTEN_KEEPALIVE
  int postprocess_halide(int width, int height, float threshold) {
    auto hal_src = Halide::Runtime::Buffer<uint8_t>::make_interleaved(postprocessInputImageBuffer, width, height, 4);
    auto hal_bg = Halide::Runtime::Buffer<uint8_t>::make_interleaved(postprocessBackgroundImageBuffer, width, height, 4);
    auto hal_inf = Halide::Runtime::Buffer<float>::make_interleaved(postprocessInferenceResultBuffer, width, height, 2);
    auto hal_dst = Halide::Runtime::Buffer<uint8_t>::make_interleaved(postprocessOutputImageBuffer, width, height, 4);

    postprocess_task(hal_src, hal_bg, hal_inf, threshold, hal_dst);
    return 0;
  }


  // JPEG Decode
  EMSCRIPTEN_KEEPALIVE
  unsigned char * getJpegImageBuffer() {
    return jpegImageBuffer;
  }

  EMSCRIPTEN_KEEPALIVE
  unsigned char * getJpegLoadedImageBuffer() {
    return jpegLoadedImageBuffer;
  }

  EMSCRIPTEN_KEEPALIVE
  int decode_jpeg(int length) {
    std::vector<uint8_t> buf(jpegImageBuffer, jpegImageBuffer + length);
    cv::Mat jpegImageMat(MAX_JPEG_HEIGHT, MAX_JPEG_WIDTH, CV_8UC3, jpegLoadedImageBuffer);
    cv::Mat tmpMat = cv::imdecode(cv::Mat(buf), cv::IMREAD_UNCHANGED);
    tmpMat.copyTo(jpegImageMat);
    return 0;
  }
}

