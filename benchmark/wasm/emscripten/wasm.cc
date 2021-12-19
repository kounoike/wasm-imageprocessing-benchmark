#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include <algorithm>
#include <cmath>
#include <iostream>

// for OpenCV
#include <opencv2/opencv.hpp>
// jpeg/png
#include <opencv2/imgcodecs.hpp>
// simd
#include <opencv2/core/hal/intrin.hpp>

// for Halide
#include <HalideBuffer.h>

#include "lib/postprocess_task.h"
#include "lib/preprocess_task.h"
#include "lib/preprocess_task_lut.h"

using namespace cv;

namespace {
const int MAX_WIDTH = 1920;
const int MAX_HEIGHT = 1080;

// for preprocess tasks
unsigned char inputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 4];
float outputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 3];

// for LUT execution cache
float lut[256];

// for postproces tasks
float postprocessInferenceResultBuffer[MAX_WIDTH * MAX_HEIGHT];
unsigned char postprocessInputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 4];
unsigned char postprocessBackgroundImageBuffer[MAX_WIDTH * MAX_HEIGHT * 4];
unsigned char postprocessOutputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 4];
float postprocessFloatInputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 3];
float postprocessFloatBgImageBuffer[MAX_WIDTH * MAX_HEIGHT * 3];

// JPEG encode/decode
const int MAX_JPEG_LENGTH = 20 * 1024 * 1024;
const int MAX_JPEG_WIDTH = 5000;
const int MAX_JPEG_HEIGHT = 2700;
unsigned char jpegImageBuffer[MAX_JPEG_LENGTH];
unsigned char jpegLoadedImageBuffer[MAX_JPEG_WIDTH * MAX_JPEG_HEIGHT * 3];

// struct
struct alignas(32) RGBA8U {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

struct RGB32F {
  float r;
  float g;
  float b;
};
}  // namespace

extern "C" {
EMSCRIPTEN_KEEPALIVE
void showExceptionMessage(intptr_t exceptionPtr) {
  std::cout << reinterpret_cast<std::exception *>(exceptionPtr)->what()
            << std::endl;
}

// 共通: 前処理
EMSCRIPTEN_KEEPALIVE
unsigned char *getPreprocessInputImageBufferOffset() {
  return inputImageBuffer;
}

EMSCRIPTEN_KEEPALIVE
float *getPreprocessOutputImageBufferOffset() { return outputImageBuffer; }

// naive copy
EMSCRIPTEN_KEEPALIVE
int preprocess_naive(int width, int height) {
  for (int i = 0; i < width * height; ++i) {
    outputImageBuffer[i * 3] = inputImageBuffer[i * 4] / 255.f;
    outputImageBuffer[i * 3 + 1] = inputImageBuffer[i * 4 + 1] / 255.f;
    outputImageBuffer[i * 3 + 2] = inputImageBuffer[i * 4 + 2] / 255.f;
  }
  return 0;
}

EMSCRIPTEN_KEEPALIVE
void initialize_lut(void) {
  for (int i = 0; i < 256; ++i) {
    lut[i] = i / 255.f;
  }
}

// by LUT
EMSCRIPTEN_KEEPALIVE
int preprocess_lut(int width, int height) {
  for (int i = 0; i < width * height; ++i) {
    outputImageBuffer[i * 3] = lut[inputImageBuffer[i * 4]];
    outputImageBuffer[i * 3 + 1] = lut[inputImageBuffer[i * 4 + 1]];
    outputImageBuffer[i * 3 + 2] = lut[inputImageBuffer[i * 4 + 2]];
  }
  return 0;
}

// by LUT Read as UINT32
EMSCRIPTEN_KEEPALIVE
int preprocess_lut_u32(int width, int height) {
  uint32_t *inputBuffer32 = (uint32_t *)inputImageBuffer;
  for (int i = 0; i < width * height; ++i) {
    const uint32_t &rgba = inputBuffer32[i];
    outputImageBuffer[i * 3] = lut[rgba & 0xff];
    outputImageBuffer[i * 3 + 1] = lut[rgba >> 8 & 0xff];
    outputImageBuffer[i * 3 + 2] = lut[rgba >> 16 & 0xff];
  }
  return 0;
}

// by LUT Read as struct
EMSCRIPTEN_KEEPALIVE
int preprocess_lut_struct(int width, int height) {
  RGBA8U *inputBuffer = (RGBA8U *)inputImageBuffer;
  for (int i = 0; i < width * height; ++i) {
    const RGBA8U &rgba = inputBuffer[i];
    outputImageBuffer[i * 3] = lut[rgba.r];
    outputImageBuffer[i * 3 + 1] = lut[rgba.g];
    outputImageBuffer[i * 3 + 2] = lut[rgba.b];
  }
  return 0;
}

// by LUT Read as UINT32, Write as struct
EMSCRIPTEN_KEEPALIVE
int preprocess_lut_u32_struct(int width, int height) {
  uint32_t *inputBuffer32 = (uint32_t *)inputImageBuffer;
  RGB32F *outputBuffer = (RGB32F *)outputImageBuffer;
  for (int i = 0; i < width * height; ++i) {
    uint32_t rgba = inputBuffer32[i];
    outputBuffer[i] = RGB32F{.r = lut[rgba & 0xff],
                             .g = lut[(rgba >> 8) & 0xff],
                             .b = lut[(rgba >> 16) & 0xff]};
  }
  return 0;
}

// by LUT Read as UINT64
EMSCRIPTEN_KEEPALIVE
int preprocess_lut_u64(int width, int height) {
  uint64_t *inputBuffer64 = (uint64_t *)inputImageBuffer;
  for (int i = 0; i < width * height / 2; ++i) {
    uint64_t rgba2 = inputBuffer64[i];
    outputImageBuffer[i * 3 + 0] = lut[(rgba2 & 0x00000000000000ffLLU)];
    outputImageBuffer[i * 3 + 1] = lut[(rgba2 & 0x000000000000ff00LLU) >> 8];
    outputImageBuffer[i * 3 + 2] = lut[(rgba2 & 0x0000000000ff0000LLU) >> 16];
    outputImageBuffer[i * 3 + 3] = lut[(rgba2 & 0x000000ff00000000LLU) >> 32];
    outputImageBuffer[i * 3 + 4] = lut[(rgba2 & 0x0000ff0000000000LLU) >> 40];
    outputImageBuffer[i * 3 + 5] = lut[(rgba2 & 0x00ff000000000000LLU) >> 48];
  }
  return 0;
}

// by Hand SIMD
EMSCRIPTEN_KEEPALIVE
int preprocess_simd(int width, int height) {
  int i = 0;
  int len = width * height;
  v_uint8x16 ri, gi, bi, ai;
  v_float32x4 rf00, rf01, rf10, rf11, gf00, gf01, gf10, gf11, bf00, bf01, bf10,
      bf11;
  v_float32x4 f255 = v_setall_f32(255.0f);

  v_uint16x8 u16_0, u16_1;
  v_uint32x4 u32_00, u32_01, u32_10, u32_11;
  for (; i <= len - 16; i += 16) {
    v_load_deinterleave(inputImageBuffer + i * 4, ri, gi, bi, ai);

    v_expand(ri, u16_0, u16_1);
    v_expand(u16_0, u32_00, u32_01);
    rf00 = v_cvt_f32(v_reinterpret_as_s32(u32_00)) / f255;
    rf01 = v_cvt_f32(v_reinterpret_as_s32(u32_01)) / f255;
    v_expand(u16_1, u32_10, u32_11);
    rf10 = v_cvt_f32(v_reinterpret_as_s32(u32_10)) / f255;
    rf11 = v_cvt_f32(v_reinterpret_as_s32(u32_11)) / f255;

    v_expand(gi, u16_0, u16_1);
    v_expand(u16_0, u32_00, u32_01);
    gf00 = v_cvt_f32(v_reinterpret_as_s32(u32_00)) / f255;
    gf01 = v_cvt_f32(v_reinterpret_as_s32(u32_01)) / f255;
    v_expand(u16_1, u32_10, u32_11);
    gf10 = v_cvt_f32(v_reinterpret_as_s32(u32_10)) / f255;
    gf11 = v_cvt_f32(v_reinterpret_as_s32(u32_11)) / f255;

    v_expand(bi, u16_0, u16_1);
    v_expand(u16_0, u32_00, u32_01);
    bf00 = v_cvt_f32(v_reinterpret_as_s32(u32_00)) / f255;
    bf01 = v_cvt_f32(v_reinterpret_as_s32(u32_01)) / f255;
    v_expand(u16_1, u32_10, u32_11);
    bf10 = v_cvt_f32(v_reinterpret_as_s32(u32_10)) / f255;
    bf11 = v_cvt_f32(v_reinterpret_as_s32(u32_11)) / f255;

    v_store_interleave(outputImageBuffer + i * 3, rf00, gf00, bf00);
    v_store_interleave(outputImageBuffer + (i + 4) * 3, rf01, gf01, bf01);
    v_store_interleave(outputImageBuffer + (i + 8) * 3, rf10, gf10, bf10);
    v_store_interleave(outputImageBuffer + (i + 12) * 3, rf11, gf11, bf11);
  }
  for (; i < len; ++i) {
    outputImageBuffer[i * 3] = inputImageBuffer[i * 4] / 255.0f;
    outputImageBuffer[i * 3 + 1] = inputImageBuffer[i * 4 + 1] / 255.0f;
    outputImageBuffer[i * 3 + 2] = inputImageBuffer[i * 4 + 2] / 255.0f;
  }
  return 0;
}

// by LUT_SIMD
EMSCRIPTEN_KEEPALIVE
int preprocess_simd_lut(int width, int height) {
  int i = 0;
  int len = width * height;
  v_uint8x16 ri, gi, bi, ai;
  v_float32x4 rf00, rf01, rf10, rf11, gf00, gf01, gf10, gf11, bf00, bf01, bf10,
      bf11;

  v_uint16x8 u16_0, u16_1;
  v_uint32x4 u32_00, u32_01, u32_10, u32_11;
  for (; i <= len - 16; i += 16) {
    v_load_deinterleave(inputImageBuffer + i * 4, ri, gi, bi, ai);

    v_expand(ri, u16_0, u16_1);
    v_expand(u16_0, u32_00, u32_01);
    v_expand(u16_1, u32_10, u32_11);
    rf00 = v_lut(lut, v_reinterpret_as_s32(u32_00));
    rf01 = v_lut(lut, v_reinterpret_as_s32(u32_10));
    rf10 = v_lut(lut, v_reinterpret_as_s32(u32_01));
    rf11 = v_lut(lut, v_reinterpret_as_s32(u32_11));

    v_expand(gi, u16_0, u16_1);
    v_expand(u16_0, u32_00, u32_01);
    v_expand(u16_1, u32_10, u32_11);
    gf00 = v_lut(lut, v_reinterpret_as_s32(u32_00));
    gf01 = v_lut(lut, v_reinterpret_as_s32(u32_10));
    gf10 = v_lut(lut, v_reinterpret_as_s32(u32_01));
    gf11 = v_lut(lut, v_reinterpret_as_s32(u32_11));

    v_expand(bi, u16_0, u16_1);
    v_expand(u16_0, u32_00, u32_01);
    v_expand(u16_1, u32_10, u32_11);
    bf00 = v_lut(lut, v_reinterpret_as_s32(u32_00));
    bf01 = v_lut(lut, v_reinterpret_as_s32(u32_10));
    bf10 = v_lut(lut, v_reinterpret_as_s32(u32_01));
    bf11 = v_lut(lut, v_reinterpret_as_s32(u32_11));

    v_store_interleave(outputImageBuffer + i * 3, rf00, gf00, bf00);
    v_store_interleave(outputImageBuffer + (i + 4) * 3, rf01, gf01, bf01);
    v_store_interleave(outputImageBuffer + (i + 8) * 3, rf10, gf10, bf10);
    v_store_interleave(outputImageBuffer + (i + 12) * 3, rf11, gf11, bf11);
  }
  for (; i < len; ++i) {
    outputImageBuffer[i * 3] = lut[inputImageBuffer[i * 4]];
    outputImageBuffer[i * 3 + 1] = lut[inputImageBuffer[i * 4 + 1]];
    outputImageBuffer[i * 3 + 2] = lut[inputImageBuffer[i * 4 + 2]];
  }
  return 0;
}

// Thread
EMSCRIPTEN_KEEPALIVE
int preprocess_thread_naive(int width, int height) {
  parallel_for_(cv::Range(0, width * height), [&](const cv::Range &range) {
    for (int r = range.start; r < range.end; ++r) {
      outputImageBuffer[r * 3] = inputImageBuffer[r * 4] / 255.f;
      outputImageBuffer[r * 3 + 1] = inputImageBuffer[r * 4 + 1] / 255.f;
      outputImageBuffer[r * 3 + 2] = inputImageBuffer[r * 4 + 2] / 255.f;
    }
  });
  return 0;
}

// Thread
EMSCRIPTEN_KEEPALIVE
int preprocess_thread_lut(int width, int height) {
  parallel_for_(cv::Range(0, width * height), [&](const cv::Range &range) {
    for (int r = range.start; r < range.end; ++r) {
      outputImageBuffer[r * 3] = lut[inputImageBuffer[r * 4]];
      outputImageBuffer[r * 3 + 1] = lut[inputImageBuffer[r * 4 + 1]];
      outputImageBuffer[r * 3 + 2] = lut[inputImageBuffer[r * 4 + 2]];
    }
  });
  return 0;
}

// by OpenCV
EMSCRIPTEN_KEEPALIVE
int preprocess_opencv(int width, int height) {
  cv::Mat inputImageMat(height, width, CV_8UC4, inputImageBuffer);
  cv::Mat tmpMat;
  cv::Mat outputImageMat(height, width, CV_32FC3, outputImageBuffer);
  cv::cvtColor(inputImageMat, tmpMat, cv::COLOR_RGBA2RGB, 0);
  tmpMat.convertTo(outputImageMat, CV_32F, 1.f / 255.f, 0);
  return 0;
}

// by OpenCV LUT
EMSCRIPTEN_KEEPALIVE
int preprocess_opencv_lut(int width, int height) {
  cv::Mat lutMat(1, 256, CV_32FC1, lut);
  cv::Mat inputImageMat(height, width, CV_8UC4, inputImageBuffer);
  cv::Mat tmpMat;
  cv::Mat outputImageMat(height, width, CV_32FC3, outputImageBuffer);

  cv::cvtColor(inputImageMat, tmpMat, cv::COLOR_RGBA2RGB, 0);
  cv::LUT(tmpMat, lutMat, outputImageMat);
  return 0;
}

// by Halide
EMSCRIPTEN_KEEPALIVE
int preprocess_halide(int width, int height) {
  auto hal_src = Halide::Runtime::Buffer<uint8_t>::make_interleaved(
      inputImageBuffer, width, height, 4);
  auto hal_dst = Halide::Runtime::Buffer<float>::make_interleaved(
      outputImageBuffer, width, height, 3);

  preprocess_task(hal_src, hal_dst);
  return 0;
}

// by Halide LUT
EMSCRIPTEN_KEEPALIVE
int preprocess_halide_lut(int width, int height) {
  auto hal_src = Halide::Runtime::Buffer<uint8_t>::make_interleaved(
      inputImageBuffer, width, height, 4);
  auto hal_dst = Halide::Runtime::Buffer<float>::make_interleaved(
      outputImageBuffer, width, height, 3);

  preprocess_task_lut(hal_src, hal_dst);
  return 0;
}

// 共通: 後処理
EMSCRIPTEN_KEEPALIVE
unsigned char *getPostprocessInputImageBufferOffset() {
  return postprocessInputImageBuffer;
}
EMSCRIPTEN_KEEPALIVE
unsigned char *getPostprocessBackgroundImageBufferOffset() {
  return postprocessBackgroundImageBuffer;
}
EMSCRIPTEN_KEEPALIVE
unsigned char *getPostprocessOutputImageBufferOffset() {
  return postprocessOutputImageBuffer;
}
EMSCRIPTEN_KEEPALIVE
float *getPostprocessInferenceResultBufferOffset() {
  return postprocessInferenceResultBuffer;
}

// naive
EMSCRIPTEN_KEEPALIVE
int postprocess_naive_old(int width, int height, float threshold) {
  for (int i = 0; i < width * height; ++i) {
    float bg = postprocessInferenceResultBuffer[i * 2];
    float fg = postprocessInferenceResultBuffer[i * 2 + 1];
    float bgexp = std::exp(bg);
    float fgexp = std::exp(fg);
    float val = fgexp / (fgexp + bgexp);
    if (val > threshold) {
      postprocessOutputImageBuffer[i * 4] = postprocessInputImageBuffer[i * 4];
      postprocessOutputImageBuffer[i * 4 + 1] =
          postprocessInputImageBuffer[i * 4 + 1];
      postprocessOutputImageBuffer[i * 4 + 2] =
          postprocessInputImageBuffer[i * 4 + 2];
      postprocessOutputImageBuffer[i * 4 + 3] =
          postprocessInputImageBuffer[i * 4 + 3];
    } else {
      postprocessOutputImageBuffer[i * 4] =
          postprocessBackgroundImageBuffer[i * 4];
      postprocessOutputImageBuffer[i * 4 + 1] =
          postprocessBackgroundImageBuffer[i * 4 + 1];
      postprocessOutputImageBuffer[i * 4 + 2] =
          postprocessBackgroundImageBuffer[i * 4 + 2];
      postprocessOutputImageBuffer[i * 4 + 3] =
          postprocessBackgroundImageBuffer[i * 4 + 3];
    }
  }
  return 0;
}

// naive brend
EMSCRIPTEN_KEEPALIVE
int postprocess_naive_brend(int width, int height, float threshold) {
  for (int i = 0; i < width * height; ++i) {
    float alpha1 = postprocessInferenceResultBuffer[i];
    float alpha2 = 1.0f - alpha1;
    postprocessOutputImageBuffer[i * 4] = cv::saturate_cast<uint8_t>(
        (postprocessFloatInputImageBuffer[i * 3] * alpha1 +
         postprocessFloatBgImageBuffer[i * 3] * alpha2) *
        255.0f);
    postprocessOutputImageBuffer[i * 4 + 1] = cv::saturate_cast<uint8_t>(
        (postprocessFloatInputImageBuffer[i * 3 + 1] * alpha1 +
         postprocessFloatBgImageBuffer[i * 3 + 1] * alpha2) *
        255.0f);
    postprocessOutputImageBuffer[i * 4 + 2] = cv::saturate_cast<uint8_t>(
        (postprocessFloatInputImageBuffer[i * 3 + 2] * alpha1 +
         postprocessFloatBgImageBuffer[i * 3 + 2] * alpha2) *
        255.0f);
    postprocessOutputImageBuffer[i * 4 + 3] = 255;
  }
  return 0;
}

// naive threshold
EMSCRIPTEN_KEEPALIVE
int postprocess_naive_threshold(int width, int height, float threshold) {
  for (int i = 0; i < width * height; ++i) {
    float alpha1 = postprocessInferenceResultBuffer[i];
    if (alpha1 > threshold) {
      postprocessOutputImageBuffer[i * 4] = postprocessInputImageBuffer[i * 4];
      postprocessOutputImageBuffer[i * 4 + 1] =
          postprocessInputImageBuffer[i * 4 + 1];
      postprocessOutputImageBuffer[i * 4 + 2] =
          postprocessInputImageBuffer[i * 4 + 2];
    } else {
      postprocessOutputImageBuffer[i * 4] =
          postprocessBackgroundImageBuffer[i * 4];
      postprocessOutputImageBuffer[i * 4 + 1] =
          postprocessBackgroundImageBuffer[i * 4 + 1];
      postprocessOutputImageBuffer[i * 4 + 2] =
          postprocessBackgroundImageBuffer[i * 4 + 2];
    }
    postprocessOutputImageBuffer[i * 4 + 3] = 255;
  }
  return 0;
}

// OpenCV
EMSCRIPTEN_KEEPALIVE
int postprocess_opencv(int width, int height, float threshold) {
  cv::Mat inputImageMat(height, width, CV_8UC4, postprocessInputImageBuffer);
  cv::Mat backgroundImageMat(height, width, CV_8UC4,
                             postprocessBackgroundImageBuffer);
  cv::Mat outputImageMat(height, width, CV_8UC4, postprocessOutputImageBuffer);
  cv::Mat inferenceResultMat(height, width, CV_32FC2,
                             postprocessInferenceResultBuffer);
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

// OpenCV
EMSCRIPTEN_KEEPALIVE
int postprocess_opencv_brend(int width, int height, float threshold) {
  cv::Mat inputImageMat(height, width, CV_32FC3,
                        postprocessFloatInputImageBuffer);
  cv::Mat backgroundImageMat(height, width, CV_32FC3,
                             postprocessFloatBgImageBuffer);
  cv::Mat outputImageMat(height, width, CV_8UC4, postprocessOutputImageBuffer);
  cv::Mat inferenceResultMat(height, width, CV_32FC1,
                             postprocessInferenceResultBuffer);
  cv::Mat channels[3] = {inferenceResultMat, inferenceResultMat,
                         inferenceResultMat};
  cv::Mat inferenceResult3Mat;
  cv::merge(channels, 3, inferenceResult3Mat);

  cv::Mat invertInferenceResultMat = 1.0f - inferenceResult3Mat;
  cv::Mat fgMat = inputImageMat.mul(inferenceResult3Mat);
  cv::Mat bgMat = backgroundImageMat.mul(invertInferenceResultMat);
  cv::Mat tmpMat;
  cv::add(fgMat, bgMat, tmpMat);
  tmpMat.convertTo(tmpMat, CV_8U, 255.0f, 0.0f);
  cv::cvtColor(tmpMat, outputImageMat, cv::COLOR_RGB2RGBA);

  return 0;
}

// OpenCV
EMSCRIPTEN_KEEPALIVE
int postprocess_opencv_threshold(int width, int height, float threshold) {
  cv::Mat inputImageMat(height, width, CV_8UC4, postprocessInputImageBuffer);
  cv::Mat backgroundImageMat(height, width, CV_8UC4,
                             postprocessBackgroundImageBuffer);
  cv::Mat outputImageMat(height, width, CV_8UC4, postprocessOutputImageBuffer);
  cv::Mat inferenceResultMat(height, width, CV_32FC1,
                             postprocessInferenceResultBuffer);

  cv::Mat maskMat;
  inferenceResultMat.convertTo(maskMat, CV_8U, 255.0f, 0.0f);
  cv::threshold(maskMat, maskMat, 255 * threshold, 255, cv::THRESH_BINARY);

  backgroundImageMat.copyTo(outputImageMat);
  inputImageMat.copyTo(outputImageMat, maskMat);

  return 0;
}

// Halide
EMSCRIPTEN_KEEPALIVE
int postprocess_halide(int width, int height, float threshold) {
  auto hal_src = Halide::Runtime::Buffer<uint8_t>::make_interleaved(
      postprocessInputImageBuffer, width, height, 4);
  auto hal_bg = Halide::Runtime::Buffer<uint8_t>::make_interleaved(
      postprocessBackgroundImageBuffer, width, height, 4);
  auto hal_inf = Halide::Runtime::Buffer<float>::make_interleaved(
      postprocessInferenceResultBuffer, width, height, 2);
  auto hal_dst = Halide::Runtime::Buffer<uint8_t>::make_interleaved(
      postprocessOutputImageBuffer, width, height, 4);

  postprocess_task(hal_src, hal_bg, hal_inf, threshold, hal_dst);
  return 0;
}

// JPEG Decode
EMSCRIPTEN_KEEPALIVE
unsigned char *getJpegImageBuffer() { return jpegImageBuffer; }

EMSCRIPTEN_KEEPALIVE
unsigned char *getJpegLoadedImageBuffer() { return jpegLoadedImageBuffer; }

EMSCRIPTEN_KEEPALIVE
int decode_jpeg(int length) {
  std::vector<uint8_t> buf(jpegImageBuffer, jpegImageBuffer + length);
  cv::Mat jpegImageMat(MAX_JPEG_HEIGHT, MAX_JPEG_WIDTH, CV_8UC3,
                       jpegLoadedImageBuffer);
  cv::Mat tmpMat = cv::imdecode(cv::Mat(buf), cv::IMREAD_UNCHANGED);
  tmpMat.copyTo(jpegImageMat);
  return 0;
}
}
