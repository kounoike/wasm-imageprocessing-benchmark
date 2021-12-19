#ifndef __WASM_HPP__
#define __WASM_HPP__

extern "C" {
unsigned char *getPreprocessInputImageBufferOffset();
float *getPreprocessOutputImageBufferOffset();

void initialize_lut(void);
int preprocess_naive(int width, int height);
int preprocess_lut(int width, int height);
// int preprocess_simd(int width, int height);
// int preprocess_simd_lut(int width, int height);
int preprocess_thread_naive(int width, int height);
int preprocess_thread_lut(int width, int height);

int preprocess_opencv(int width, int height);
int preprocess_opencv_lut(int width, int height);

int preprocess_halide(int width, int height);
int preprocess_halide_lut(int width, int height);
}

#endif
