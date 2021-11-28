// Build command:
// g++ -o gen gen.cpp ../../../../Halide-11.0.1-x86-64-linux/share/Halide/tools/GenGen.cpp -I ../../../../Halide-11.0.1-x86-64-linux/include/ -lHalide -L ../../../../Halide-11.0.1-x86-64-linux/lib/ -std=c++17 -Wl,-rpath ../../../../Halide-11.0.1-x86-64-linux/lib
// Wasm lib generate command:
// ./gen -g input_task target=wasm-32-wasmrt-wasm_simd128-wasm_threads-wasm_bulk_memory -o ../lib

#include <Halide.h>


class PreprocessTask : public Halide::Generator<PreprocessTask> {
public:
  Halide::GeneratorInput<Halide::Buffer<std::uint8_t>> input{"input", 3};
  Halide::GeneratorOutput<Halide::Buffer<float_t>> output{"output", 3};

  Halide::Var x{"x"}, y{"y"}, c{"c"};

  void generate() {
    output(x, y, c) = Halide::cast<float>(input(x, y, c)) / 255.f;
  }

  void schedule() {
    input.dim(0).set_stride(4);
    input.dim(2).set_stride(1);
    input.dim(2).set_bounds(0, 4);
    output.dim(0).set_stride(3);
    output.dim(2).set_stride(1);
    output.dim(2).set_bounds(0, 3);

    // input.set_estimates({{0, 1920}, {0, 1080}, {0, 4}});
    // output.set_estimates({{0, 1920}, {0, 1080}, {0, 3}});



    // output.compute_root();
    // output.parallel(y).vectorize(x, 16).reorder(c, x, y).unroll(c);
    output.reorder(c, x, y).unroll(c);
  }
};


HALIDE_REGISTER_GENERATOR(PreprocessTask, preprocess_task);
