// Build command:
// g++ -o gen gen.cpp ../../../../Halide-11.0.1-x86-64-linux/share/Halide/tools/GenGen.cpp -I ../../../../Halide-11.0.1-x86-64-linux/include/ -lHalide -L ../../../../Halide-11.0.1-x86-64-linux/lib/ -std=c++17 -Wl,-rpath ../../../../Halide-11.0.1-x86-64-linux/lib
// Wasm lib generate command:
// ./gen -g input_task target=wasm-32-wasmrt-wasm_simd128-wasm_threads-wasm_bulk_memory -o ../lib

#include <Halide.h>


class PreprocessTaskLut : public Halide::Generator<PreprocessTaskLut> {
public:
  Halide::GeneratorInput<Halide::Buffer<std::uint8_t>> input{"input", 3};
  Halide::GeneratorOutput<Halide::Buffer<float_t>> output{"output", 3};

  Func lut;
  Halide::Var x{"x"}, y{"y"}, c{"c"}, i{"i"};

  void generate() {
    lut(i) = Halide::cast<float>(i / 255.f);
    output(x, y, c) = lut(input(x, y, c));
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

    lut.compute_root();

    output.compute_root();
    output.parallel(y).vectorize(x, 16).reorder(c, x, y).unroll(c);
  }
};


HALIDE_REGISTER_GENERATOR(PreprocessTaskLut, preprocess_task_lut);