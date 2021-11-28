const rsMod = import('./wasm/rust/pkg/wasmcopy_rs')
const rsBg = import('./wasm/rust/pkg/wasmcopy_rs_bg')

const width = 1920
const height = 1080
// const width = 256
// const height = 144
// const width = 640
// const height = 360

function byJavaScript (imageData, width, height) {
  const array = new Float32Array(width * height * 3)
  for (let i = 0; i < width * height; ++i) {
    array[i * 3] = imageData.data[i * 4] / 255
    array[i * 3 + 1] = imageData.data[i * 4 + 1] / 255
    array[i * 3 + 2] = imageData.data[i * 4 + 2] / 255
  }
  return array
}

const lut = new Float32Array(256)
for (let i = 0; i < 256; ++i) {
  lut[i] = i / 255
}

function byJavaScriptLut (imageData, width, height) {
  const array = new Float32Array(width * height * 3)
  for (let i = 0; i < width * height; ++i) {
    array[i * 3] = lut[imageData.data[i * 4]]
    array[i * 3 + 1] = lut[imageData.data[i * 4 + 1]]
    array[i * 3 + 2] = lut[imageData.data[i * 4 + 2]]
  }
  return array
}

function byWasm (wasm, func, imageData, width, height) {
  const inputOffset = wasm._getPreprocessInputImageBufferOffset()
  const outputOffset = wasm._getPreprocessOutputImageBufferOffset()
  wasm.HEAPU8.set(imageData.data, inputOffset)
  wasm[func](width, height)
  const array = new Uint8Array(
    wasm.HEAPU8.buffer,
    outputOffset,
    width * height * 3
  )
}

function postProcessByWasm (wasm, func, imageData, width, height) {
  // const inputOffset = wasm._getPostprocessInputImageBufferOffset()
  // const outputOffset = wasm._getPostprocessOutputImageBufferOffset()
  // // TODO: inferenceResult...
  // wasm.HEAPU8.set(imageData.data, inputOffset)
  wasm[func](width, height, 0.9)
  // const array = new Uint8Array(
  //   wasm.HEAPU8.buffer,
  //   outputOffset,
  //   width * height * 3
  // )
}

function jpegDecodeByWasm (wasm, func, jpegData) {
  try{
    wasm[func](jpegData.length)
  }
  catch(ex) {
    wasm._showExceptionMessage(ex)
  }
}

function byRust (screen, exec, memory, imageData, width, height) {
  const inputOffset = screen.input_pointer()
  const outputOffset = screen.output_pointer()
  const inputArray = new Uint8Array(memory.buffer)
  inputArray.set(imageData.data, inputOffset)
  exec(screen)
  const array = new Uint8Array(
    memory.buffer,
    outputOffset,
    screen.output_size()
  )
}

function do_benchmark (proc, name, fn, count) {
  const msg = `Starting benchmark for ${proc} / ${name} `
  console.log(msg)
  // const resultDiv = document.getElementById('result')
  // resultDiv.innerHTML += msg
  for (let i = 0; i < 10; ++i) {
    fn()
  }
  const start = performance.now()
  for (let i = 0; i < count; ++i) {
    fn()
  }
  const end = performance.now()
  const duration = (end - start) / count

  const tr = document.createElement('tr')
  const th = document.createElement('th')
  const td = document.createElement('td')
  th.innerText = `${proc} by ${name}`
  td.innerText = `${duration.toFixed(2)}`
  tr.appendChild(th)
  tr.appendChild(td)
  const tbody = document.getElementById('tbody')
  tbody.appendChild(tr)
}

async function benchmark () {
  const wasmModule = await createBenchmarkModule()
  const wasmSimdModule = await createBenchmarkSimdModule()

  const canvas = new OffscreenCanvas(width, height)
  const ctx = canvas.getContext('2d')
  const imageData = ctx.createImageData(width, height)

  // JPEG decode/encode
  const url = "./img/sample.jpg";
  const resp = await fetch(url);
  const jpegBlob = await resp.blob();
  const jpegData = new Uint8Array(await jpegBlob.arrayBuffer())

  const jpegOffset = wasmModule._getJpegImageBuffer()
  wasmModule.HEAPU8.set(jpegData, jpegOffset)
  const jpegOffsetSimd = wasmSimdModule._getJpegImageBuffer()
  wasmSimdModule.HEAPU8.set(jpegData, jpegOffsetSimd)


  const resultDiv = document.getElementById('result')
  const header = document.createElement('p')
  // header.innerText = `${width}x${height} image RGBA -> RGB -> /255.0 as float, 10 times for warm-up, result: ${count} times avg.`
  resultDiv.appendChild(header)

  await (async () => {
    return Promise.all([rsMod, rsBg]).then(([{ Screen, exec }, { memory }]) => {
      console.log(wasmSimdModule)
      const screen = new Screen(width, height)
      const benchmarks = [
        {
          proc: 'Preprocess',
          name: 'JavaScript',
          count: 100,
          fn: () => byJavaScript(imageData, width, height)
        },
        {
          proc: 'Preprocess',
          name: 'JavaScript(LUT)',
          count: 100,
          fn: () => byJavaScriptLut(imageData, width, height)
        },
        {
          proc: 'Preprocess',
          name: 'C++',
          count: 100,
          fn: () =>
            byWasm(wasmModule, '_preprocess_naive', imageData, width, height)
        },
        {
          proc: 'Preprocess',
          name: 'C++(LUT)',
          count: 100,
          fn: () =>
            byWasm(wasmModule, '_preprocess_lut', imageData, width, height)
        },
        {
          proc: 'Preprocess',
          name: 'OpenCV',
          count: 100,
          fn: () =>
            byWasm(wasmModule, '_preprocess_opencv', imageData, width, height)
        },
        {
          proc: 'Preprocess',
          name: 'OpenCV(LUT)',
          count: 100,
          fn: () =>
            byWasm(
              wasmModule,
              '_preprocess_opencv_lut',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Preprocess',
          name: 'Halide',
          count: 100,
          fn: () =>
            byWasm(wasmModule, '_preprocess_halide', imageData, width, height)
        },
        {
          proc: 'Preprocess',
          name: 'Halide(LUT)',
          count: 100,
          fn: () =>
            byWasm(
              wasmModule,
              '_preprocess_halide_lut',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Preprocess',
          name: 'Naive(SIMD)',
          count: 100,
          fn: () =>
            byWasm(
              wasmSimdModule,
              '_preprocess_naive',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Preprocess',
          name: 'LUT(SIMD)',
          count: 100,
          fn: () =>
            byWasm(wasmSimdModule, '_preprocess_lut', imageData, width, height)
        },
        {
          proc: 'Preprocess',
          name: 'byHand SIMD(SIMD)',
          count: 100,
          fn: () =>
            byWasm(
              wasmSimdModule,
              '_preprocess_simd',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Preprocess',
          name: 'byHand SIMD(LUT/SIMD)',
          count: 100,
          fn: () =>
            byWasm(
              wasmSimdModule,
              '_preprocess_simd_lut',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Preprocess',
          name: 'OpenCV(SIMD)',
          count: 100,
          fn: () =>
            byWasm(
              wasmSimdModule,
              '_preprocess_opencv',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Preprocess',
          name: 'OpenCV(LUT/SIMD)',
          count: 100,
          fn: () =>
            byWasm(
              wasmSimdModule,
              '_preprocess_opencv_lut',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Preprocess',
          name: 'Halide(SIMD)',
          count: 100,
          fn: () =>
            byWasm(wasmModule, '_preprocess_halide', imageData, width, height)
        },
        {
          proc: 'Preprocess',
          name: 'Halide(LUT/SIMD)',
          count: 100,
          fn: () =>
            byWasm(
              wasmModule,
              '_preprocess_halide_lut',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Preprocess',
          name: 'Rust',
          fn: () => byRust(screen, exec, memory, imageData, width, height)
        },
        {
          proc: 'Postprocess(brend)',
          name: 'C++',
          count: 100,
          fn: () =>
            postProcessByWasm(
              wasmModule,
              '_postprocess_naive_brend',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Postprocess(threshold)',
          name: 'C++',
          count: 100,
          fn: () =>
            postProcessByWasm(
              wasmModule,
              '_postprocess_naive_threshold',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Postprocess(brend)',
          name: 'OpenCV',
          count: 100,
          fn: () =>
            postProcessByWasm(
              wasmModule,
              '_postprocess_opencv_brend',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Postprocess(threshold)',
          name: 'OpenCV',
          count: 100,
          fn: () =>
            postProcessByWasm(
              wasmModule,
              '_postprocess_opencv_threshold',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Postprocess',
          name: 'Halide',
          count: 100,
          fn: () =>
            postProcessByWasm(
              wasmModule,
              '_postprocess_halide',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Postprocess(brend)',
          name: 'C++(SIMD)',
          count: 100,
          fn: () =>
            postProcessByWasm(
              wasmSimdModule,
              '_postprocess_naive_brend',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Postprocess(threshold)',
          name: 'C++(SIMD)',
          count: 100,
          fn: () =>
            postProcessByWasm(
              wasmSimdModule,
              '_postprocess_naive_threshold',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Postprocess(brend)',
          name: 'OpenCV(SIMD)',
          count: 100,
          fn: () =>
            postProcessByWasm(
              wasmSimdModule,
              '_postprocess_opencv_brend',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Postprocess(threshold)',
          name: 'OpenCV(SIMD)',
          count: 100,
          fn: () =>
            postProcessByWasm(
              wasmSimdModule,
              '_postprocess_opencv_threshold',
              imageData,
              width,
              height
            )
        },
        {
          proc: 'Postprocess',
          name: 'Halide(SIMD)',
          count: 100,
          fn: () =>
            postProcessByWasm(
              wasmSimdModule,
              '_postprocess_halide',
              imageData,
              width,
              height
            )
        },
        // {
        //   proc: 'JPEG decode',
        //   name: 'OpenCV',
        //   count: 10,
        //   fn: () =>
        //   jpegDecodeByWasm(
        //       wasmModule,
        //       '_decode_jpeg',
        //       jpegData,
        //     )
        // },
        // {
        //   proc: 'JPEG decode',
        //   name: 'OpenCV(SIMD)',
        //   count: 10,
        //   fn: () =>
        //   jpegDecodeByWasm(
        //       wasmSimdModule,
        //       '_decode_jpeg',
        //       jpegData,
        //     )
        // },
      ]

      for (const benchmark of benchmarks) {
        do_benchmark(benchmark.proc, benchmark.name, benchmark.fn, benchmark.count)
      }
    })
  })()
  console.log('executing ByRust')
  resultDiv.innerHTML += 'Done<br/>'
}

benchmark()
