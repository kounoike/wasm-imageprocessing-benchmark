# wasm-imageprocessing-benchmark

画像処理用途のWASMベンチマーク。

## 構成

### ./docker/

C++ WASM(Emscripten)ビルド用のDockerイメージ（ライブラリもあれこれ入れてある）

### ./benchmark/

Google Meet Segmentationモデルの前処理・後処理を想定したベンチマークのソースコード

- ./benchmark/wasm/emscripten: C++
  - スクラッチ実装
  - [OpenCV](https://opencv.org/)利用
  - [Halide](https://halide-lang.org/)利用
- ./benchmark/wasm/rust: Rust版（付け焼刃）

### ./slide

プレゼンスライドのソースコード([slidev](https://sli.dev/))

## ビルド方法

### C++

`./build_docker.sh` でDockerイメージをビルド→`benchrmark`ディレクトリで `./build_wasm.sh` を実行

### Rust

`wasm-pack`コマンドで何かいい具合に（リリースビルドのオプション付けたけど忘れた）

### js

webpackなのでbenchmarkディレクトリで`yarn && yarn serve`でOKのはず
