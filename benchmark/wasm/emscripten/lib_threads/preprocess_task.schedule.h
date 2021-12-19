#ifndef preprocess_task_SCHEDULE_H
#define preprocess_task_SCHEDULE_H

// MACHINE GENERATED -- DO NOT EDIT
// This schedule was automatically generated by Adams2019
// for target=wasm-32-wasmrt-wasm_bulk_memory-wasm_threads  // NOLINT
// with machine_params=8,16777216,10

#include "Halide.h"


inline void apply_schedule_preprocess_task(
    ::Halide::Pipeline pipeline,
    ::Halide::Target target
) {
    using ::Halide::Func;
    using ::Halide::MemoryType;
    using ::Halide::RVar;
    using ::Halide::TailStrategy;
    using ::Halide::Var;
    Func output = pipeline.get_func(1);
    Var c(output.get_schedule().dims()[2].var);
    Var x(output.get_schedule().dims()[0].var);
    Var xi("xi");
    Var y(output.get_schedule().dims()[1].var);
    output
        .split(x, x, xi, 15, TailStrategy::ShiftInwards)
        .vectorize(c)
        .compute_root()
        .reorder({c, xi, y, x})
        .parallel(x)
        .reorder_storage(c, x, y);

}

#endif  // preprocess_task_SCHEDULE_H
