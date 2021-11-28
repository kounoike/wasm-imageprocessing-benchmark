#include <emscripten.h>
#include <opencv2/opencv.hpp>
#include <torch/torch.h>
#include <torch/script.h>

namespace {
  std::shared_ptr<torch::jit::script::Module> pModule;
    torch::jit::script::Module module;
}

extern "C" {
  EMSCRIPTEN_KEEPALIVE
  void showExceptionMessage(intptr_t exceptionPtr) {
    std::cout << reinterpret_cast<std::exception *>(exceptionPtr)->what() << std::endl;
  }

  EMSCRIPTEN_KEEPALIVE
  int test() {
    module = torch::jit::load("/FaceParsing.pt");
    std::cout << "module loaded." << std::endl;
    return 0;
  }
}