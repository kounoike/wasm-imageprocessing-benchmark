// Build command:
// g++ -o gen gen.cpp ../../../../Halide-11.0.1-x86-64-linux/share/Halide/tools/GenGen.cpp -I ../../../../Halide-11.0.1-x86-64-linux/include/ -lHalide -L ../../../../Halide-11.0.1-x86-64-linux/lib/ -std=c++17 -Wl,-rpath ../../../../Halide-11.0.1-x86-64-linux/lib
// Wasm lib generate command:
// ./gen -g input_task target=wasm-32-wasmrt-wasm_simd128-wasm_threads-wasm_bulk_memory -o ../lib

#include <Halide.h>


class PostprocessTask : public Halide::Generator<PostprocessTask> {
public:
  Halide::GeneratorInput<Halide::Buffer<std::uint8_t>> input{"input", 3};
  Halide::GeneratorInput<Halide::Buffer<std::uint8_t>> background{"background", 3};
  Halide::GeneratorInput<Halide::Buffer<float_t>> inference{"inference", 3};
  Halide::GeneratorOutput<Halide::Buffer<std::uint8_t>> output{"output", 3};
  Halide::GeneratorInput<float_t> threshold{"threshold"};

  Halide::Var x{"x"}, y{"y"}, c{"c"};
  Halide::Func softmax, infExp;

  void generate() {
    infExp(x, y, c) = Halide::exp(inference(x, y, c));
    softmax(x, y) = infExp(x, y, 1) / (infExp(x, y, 0) + infExp(x, y, 0));
    output(x, y, c) = Halide::select(softmax(x, y) > threshold, input(x, y, c), background(x, y, c));
  }

  void schedule() {
    input.dim(0).set_stride(4);
    input.dim(2).set_stride(1);
    input.dim(2).set_bounds(0, 4);
    background.dim(0).set_stride(4);
    background.dim(2).set_stride(1);
    background.dim(2).set_bounds(0, 4);
    inference.dim(0).set_stride(2);
    inference.dim(2).set_stride(1);
    inference.dim(2).set_bounds(0, 2);
    output.dim(0).set_stride(4);
    output.dim(2).set_stride(1);
    output.dim(2).set_bounds(0, 4);

    // input.set_estimates({{0, 1920}, {0, 1080}, {0, 4}});
    // output.set_estimates({{0, 1920}, {0, 1080}, {0, 3}});

    output.compute_root();
    output.parallel(y).vectorize(x, 16).reorder(c, x, y).unroll(c);
  }
};


HALIDE_REGISTER_GENERATOR(PostprocessTask, postprocess_task);