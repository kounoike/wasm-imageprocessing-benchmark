---
# try also 'default' to start simple
theme: default
# random image from a curated Unsplash collection by Anthony
# like them? see https://unsplash.com/collections/94734566/slidev
background: https://source.unsplash.com/collection/94734566/1920x1080
# apply any windi css classes to the current slide
class: 'text-center'
# https://sli.dev/custom/highlighters.html
highlighter: shiki
# some information about the slides, markdown enabled
info: |
  ## Slidev Starter Template
  Presentation slides for developers.

  Learn more at [Sli.dev](https://sli.dev)
---

# 画像処理タスクのWasmベンチマーク

バーチャル背景処理を題材に

鴻池　祐輔

twitter: @ko_noike github|qiita|zenn: kounoike

<a href="https://github.com/kounoike/wasm-imageprocessing-benchmark" target="_blank" alt="GitHub"
  class="abs-br m-6 text-xl icon-btn opacity-50 !border-none !hover:text-white">
  <carbon-logo-github />
</a>

<!--
The last comment block of each slide will be treated as slide notes. It will be visible and editable in Presenter Mode along with the slide. [Read more in the docs](https://sli.dev/guide/syntax.html#notes)
-->

---

# 題材：バーチャル背景処理 ([Meet のセグメンテーションモデル](https://zenn.dev/kounoike/articles/google-meet-bg-features))

- 一時期 [Model Card](https://github.com/PINTO0309/PINTO_model_zoo/blob/main/082_MediaPipe_Meet_Segmentation/Model.Card-Meet.Segmentation.pdf) で Apache License 表記があったが差し替えられたモノ
- モデルは非常に軽量で高速に動く
- 使うときは推論実行の他に、**前処理**/**後処理**が必要

<!--
You can have `style` tag in markdown to override the style for the current page.
Learn more: https://sli.dev/guide/syntax#embedded-styles
-->

<img style="transform: scale(0.5, 0.5);transform-origin: top right;" src="https://github.com/kounoike/wasm-imageprocessing-benchmark/raw/master/slide/images/meet_model_card.png"/>

---

# 前処理

形式の置き換え

- ブラウザでカメラ入力等から得られる画像: RGBAの4チャンネル, Uint8([0, 255])
- モデル入力(前処理の出力): RGBの3チャンネル, Float32([0.0f, 1.0f])
- 画像サイズ: 今回は~~実装がめんどくさい~~**簡略化**のため、リサイズなし（入力画像サイズ＝モデルサイズ）とする

---

# 後処理

背景画像差し替え処理の単純な実装

- カメラ画像、背景画像: RGBA, Uint8, WxHx4
- モデル出力: Float32, WxHHx2（ch0:背景confidence値、ch1:前景confidence値）
- 結果画像(後処理の出力): RGBA, Uint8, WxHx4
- Softmax関数にかけて、ch0が閾値以上: 前景、それ以外: 背景とみなす
- 前景部分はカメラ画像を、背景部分は背景画像となるように合成する
- 後処理もリサイズなしとする

---

# Softmax関数

[MIN_FLOAT, MAX_FLOAT]で出てくる値を[0.0, 1.0]にするための処理

$x = (x_1, \ldots, x_n)$ に対して$y = (y_1, \ldots, y_n)$を$y_i = \frac{\exp(x_i)}{\sum_{i=1}^{n}{\exp(x_i)}}$と定義する

$\sum_{i=1}^{n}{y_i}=1$となるため$x$と比べて扱いやすい。

従って、Softmax処理をした前景($ch1$)のConfidence値は $\frac{\exp(ch1)}{\exp(ch0) + \exp(ch1)}$ となる。

画素ごとにこの値を計算して、閾値と比べて大きければ前景、それ以外は背景とする。


---

# それぞれの処理の特徴

今回の例では前処理・後処理ともに画素単位で計算が完結する(point operationという)単純な処理

前処理は本当に単純、後処理はちょっと浮動小数点の計算が多めで条件分岐が入って厄介かな？という程度


※背景ぼかしにしてガウシアンブラーとかで近傍処理するコードにした方が良いかも

---

# 前処理: JavaScript

まずはシンプルに JavaScript で実装してみる

```js
function byJavaScript(imageData, width, height) {
  const array = new Float32Array(width * height * 3)
  for (let i = 0; i < width * height; ++i) {
    array[i * 3] = imageData.data[i * 4] / 255
    array[i * 3 + 1] = imageData.data[i * 4 + 1] / 255
    array[i * 3 + 2] = imageData.data[i * 4 + 2] / 255
  }
  return array
}
```

---

# 前処理; JavaScript(LUT)

256通りしかないのに、何度もx/255.fを計算するのももったいない→事前に計算しておく（LookUpTable/LUTと呼ばれる手法）

```js
const lut = new Float32Array(256)
for(let i = 0; i < 256; ++i) {
  lut[i] = i / 255
}
function byJavaScriptLut(imageData, width, height) {
  const array = new Float32Array(width * height * 3)
  for (let i = 0; i < width * height; ++i) {
    array[i * 3] = lut[imageData.data[i * 4]]
    array[i * 3 + 1] = lut[imageData.data[i * 4 + 1]]
    array[i * 3 + 2] = lut[imageData.data[i * 4 + 2]]
  }
  return array
}
```

---

# 前処理: C++共通

Emscriptenのあれこれの都合で、画像データの入出力はグローバルに確保したバッファ経由で行う

```cpp
namespace {
  const int MAX_WIDTH = 1920;
  const int MAX_HEIGHT = 1080;

  // for preprocess tasks
  unsigned char preprocessInputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 4];
  float preprocessOutputImageBuffer[MAX_WIDTH * MAX_HEIGHT * 3];
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
}
```

---

# 前処理: スクラッチ実装（単純）

シンプルに実装するとこんな感じ。（行・列の2重ループでも良いけど単純にしてみた）

```cpp
int preprocess_naive(int width, int height) {
  for(int i = 0; i < width * height; ++i) {
    preprocessOutputImageBuffer[i * 3] = preprocessInputImageBuffer[i * 4] / 255.f;
    preprocessOutputImageBuffer[i * 3 + 1] = preprocessInputImageBuffer[i * 4 + 1] / 255.f;
    preprocessOutputImageBuffer[i * 3 + 2] = preprocessInputImageBuffer[i * 4 + 2] / 255.f;
  }
  return 0;
}
```

---

# 前処理: スクラッチ実装（LUT）

```cpp
float lut[256];
bool lut_computed = false;
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
```

---

# 前処理: OpenCVを使ってみる

```cpp
int preprocess_opencv(int width, int height) {
  cv::Mat inputImageMat(height, width, CV_8UC4, preprocessInputImageBuffer);
  cv::Mat tmpMat;
  cv::Mat outputImageMat(height, width, CV_32FC3, preprocessOutputImageBuffer);
  cv::cvtColor(inputImageMat, tmpMat, cv::COLOR_RGBA2RGB, 0);
  tmpMat.convertTo(outputImageMat, CV_32F, 1.f/255.f, 0);
  return 0;
}
```

---

# 前処理: OpenCV+LUT

```cpp
int preprocess_opencv(int width, int height) {
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
```

---

# 前処理: Halideを使ってみる

[Halide](https://halide-lang.org/) を使うとアルゴリズムと計算スケジューラを分離して記述できる。WASMにも出来る。

```cpp
int preprocess_halide(int width, int height) {
  auto hal_src = Halide::Runtime::Buffer<uint8_t>::make_interleaved(preprocessInputImageBuffer, width, height, 4);
  auto hal_dst = Halide::Runtime::Buffer<float>::make_interleaved(preprocessOutputImageBuffer, width, height, 3);

  preprocess_task(hal_src, hal_dst);
  return 0;
}
```

```cpp
class PreprocessTask : public Halide::Generator<PreprocessTask> {
public:
  Halide::GeneratorInput<Halide::Buffer<std::uint8_t>> input{"input", 3};
  Halide::GeneratorOutput<Halide::Buffer<float_t>> output{"output", 3};

  Halide::Var x{"x"}, y{"y"}, c{"c"};

  void generate() {
    output(x, y, c) = Halide::cast<float>(input(x, y, c)) / 255.f;
  }
  //（略）
}
```

---

# 前処理: Halide+LUT

```cpp
int preprocess_halide(int width, int height) {
  auto hal_src = Halide::Runtime::Buffer<uint8_t>::make_interleaved(preprocessInputImageBuffer, width, height, 4);
  auto hal_dst = Halide::Runtime::Buffer<float>::make_interleaved(preprocessOutputImageBuffer, width, height, 3);

  preprocess_task_lut(hal_src, hal_dst);
  return 0;
}
```

```cpp
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
  //（略）
}
```

---

# 前処理: 結果＆考察

対象画像: 1920x1080（だいぶ大きめ）実行時間: 100回実行した平均をms単位で表記

<div>

|実装|単純|LUT|単純+SIMD|LUT+SIMD|
|--------|-----:|------:|----:|----:|
|JavaScript|18.73|15.42|---|---|
|C++/スクラッチ|6.56|5.90|6.57|5.66|
|C++/OpenCV|9.62|9.69|5.43|8.31|
|C++/Halide|4.08|5.96|3.99|5.58|
|Rust|9.44|(未実装)|(未実装)|(未実装)|(未実装)|

</div>

<div>

- Halide速い！（複雑になると書くのが大変）
- OpenCVは単純に使っただけでは遅くなる
    - SIMDと組み合わせるとスクラッチより速くなる
    - SIMDをしっかりインラインアセンブラのマクロで記述してあるから？

</div>

---

# 後処理: C++

普通に実装

```cpp
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
```

---

# 後処理: OpenCV

イマイチ上手く書けてない気がする。何か良い関数が他にあるかも。

```cpp
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
```

---

# 後処理: Halide


```cpp
  int postprocess_halide(int width, int height, float threshold) {
    auto hal_src = Halide::Runtime::Buffer<uint8_t>::make_interleaved(postprocessInputImageBuffer, width, height, 4);
    auto hal_bg = Halide::Runtime::Buffer<uint8_t>::make_interleaved(postprocessBackgroundImageBuffer, width, height, 4);
    auto hal_inf = Halide::Runtime::Buffer<float>::make_interleaved(postprocessInferenceResultBuffer, width, height, 2);
    auto hal_dst = Halide::Runtime::Buffer<uint8_t>::make_interleaved(postprocessOutputImageBuffer, width, height, 4);

    postprocess_task(hal_src, hal_bg, hal_inf, threshold, hal_dst);
    return 0;
  }
```

```cpp
  Halide::Func softmax, infExp;

  void generate() {
    infExp(x, y, c) = Halide::exp(inference(x, y, c));
    softmax(x, y) = infExp(x, y, 1) / (infExp(x, y, 0) + infExp(x, y, 0));
    output(x, y, c) = Halide::select(softmax(x, y) > threshold, input(x, y, c), background(x, y, c));
  }
```

---

# 後処理: 結果＆考察

対象画像: 1920x1080（だいぶ大きめ）実行時間: 100回実行した平均をms単位で表記

<div>

|実装|SIMDなし|SIMDあり
|--------|-----:|------:|
|C++/スクラッチ|17.31|17.24|
|C++/OpenCV|32.57|23.97|
|C++/Halide|30.16|13.36|

</div>

<div>

- Halide速い！（SIMD有効のときのみ）
- OpenCVはうまくフィットしていない感がある
    - cv::softmax()とかあればきっと速いのだろうけど…（dnnレイヤーならあるけど）

</div>

