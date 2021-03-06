const rsMod = import('./wasm/rust/pkg/wasmcopy_rs.js')
const rsBg = import('./wasm/rust/pkg/wasmcopy_rs_bg.wasm')

const canUseThreads = 'SharedArrayBuffer' in window

const width = 1920
const height = 1080
// const width = 256
// const height = 144
// const width = 640
// const height = 360
// const width = 640
// const height = 480

function byJavaScript (imageData, array, width, height) {
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

function byJavaScriptLut (imageData, array, width, height) {
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
  // const array = new Uint8Array(
  //   wasm.HEAPU8.buffer,
  //   outputOffset,
  //   width * height * 3
  // )
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
  try {
    wasm[func](jpegData.length)
  } catch (ex) {
    wasm._showExceptionMessage(ex)
  }
}

function byRust (screen, inputArray, func, memory, imageData, width, height) {
  const inputOffset = screen.input_pointer()
  const outputOffset = screen.output_pointer()
  inputArray.set(imageData.data, inputOffset)
  func(screen)
  // const array = new Uint8Array(
  //   memory.buffer,
  //   outputOffset,
  //   screen.output_size()
  // )
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
  th.innerText = `${name}`
  td.innerText = `${duration.toFixed(2)}`
  tr.appendChild(th)
  tr.appendChild(td)
  const tbody = document.getElementById('tbody')
  tbody.appendChild(tr)
}

async function benchmark () {
  const wasmModule = await createBenchmarkModule()
  const wasmThreadsModule = canUseThreads
    ? await createBenchmarkThreadsModule()
    : null
  const wasmSimdModule = await createBenchmarkSimdModule()
  const wasmSimdThreadsModule = canUseThreads
    ? await createBenchmarkSimdThreadsModule()
    : null
  wasmModule._initialize_lut()
  wasmSimdModule._initialize_lut()
  if (canUseThreads) {
    wasmThreadsModule._initialize_lut()
    wasmSimdThreadsModule._initialize_lut()
  }

  const canvas = document.createElement('canvas')
  canvas.width = width
  canvas.height = height
  const ctx = canvas.getContext('2d')
  const imageData = ctx.createImageData(width, height)
  const array = new Float32Array(width * height * 3)

  // JPEG decode/encode
  const url = './img/sample.jpg'
  const resp = await fetch(url)
  const jpegBlob = await resp.blob()
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
    return Promise.all([rsMod, rsBg]).then(
      ([{ Screen, exec1, exec_lut, exec1_10, exec_lut10 }, { memory }]) => {
        const screen = new Screen(width, height)
        const inputArray = new Uint8Array(memory.buffer)
        const benchmarks = [
          {
            proc: 'Preprocess',
            name: 'JavaScript/Naive',
            count: 100,
            fn: () => byJavaScript(imageData, array, width, height)
          },
          {
            proc: 'Preprocess',
            name: 'JavaScript/LUT',
            count: 100,
            fn: () => byJavaScriptLut(imageData, array, width, height)
          },
          {
            proc: 'Preprocess',
            name: 'C++/Naive',
            count: 100,
            fn: () =>
              byWasm(wasmModule, '_preprocess_naive', imageData, width, height)
          },
          {
            proc: 'Preprocess',
            name: 'C++/LUT',
            count: 100,
            fn: () =>
              byWasm(wasmModule, '_preprocess_lut', imageData, width, height)
          },
          // {
          //   proc: 'Preprocess',
          //   name: 'C++/LUT/UINT32',
          //   count: 100,
          //   fn: () =>
          //     byWasm(wasmModule, '_preprocess_lut_u32', imageData, width, height)
          // },
          // {
          //   proc: 'Preprocess',
          //   name: 'C++/LUT/UINT32/struct',
          //   count: 100,
          //   fn: () =>
          //     byWasm(wasmModule, '_preprocess_lut_struct', imageData, width, height)
          // },
          {
            proc: 'Preprocess',
            name: 'C++/LUT/UINT64',
            count: 100,
            fn: () =>
              byWasm(
                wasmModule,
                '_preprocess_lut_u64',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++/Naive/parallel_for',
            count: 100,
            fn: () =>
              byWasm(
                wasmModule,
                '_preprocess_thread_naive',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++/LUT/parallel_for',
            count: 100,
            fn: () =>
              byWasm(
                wasmModule,
                '_preprocess_thread_lut',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++/OpenCV/cvtColor+convertTo',
            count: 100,
            fn: () =>
              byWasm(wasmModule, '_preprocess_opencv', imageData, width, height)
          },
          {
            proc: 'Preprocess',
            name: 'C++/OpenCV/cvtColor+LUT',
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
            name: 'C++/Halide/Naive',
            count: 100,
            fn: () =>
              byWasm(wasmModule, '_preprocess_halide', imageData, width, height)
          },
          {
            proc: 'Preprocess',
            name: 'C++/Halide/LUT',
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
            name: 'C++(SIMD)/Naive',
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
            name: 'C++(SIMD)/LUT',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdModule,
                '_preprocess_lut',
                imageData,
                width,
                height
              )
          },
          // {
          //   proc: 'Preprocess',
          //   name: 'C++(SIMD)/LUT/UINT32',
          //   count: 100,
          //   fn: () =>
          //     byWasm(wasmSimdModule, '_preprocess_lut_u32', imageData, width, height)
          // },
          // {
          //   proc: 'Preprocess',
          //   name: 'C++(SIMD)/LUT/UINT32/struct',
          //   count: 100,
          //   fn: () =>
          //     byWasm(wasmSimdModule, '_preprocess_lut_struct', imageData, width, height)
          // },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD)/LUT/UINT64',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdModule,
                '_preprocess_lut_u64',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD)/Naive/HandWrittenSIMD',
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
            name: 'C++(SIMD)/LUT/HandWrittenSIMD',
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
            name: 'C++(SIMD)/Naive/parallel_for',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdModule,
                '_preprocess_thread_naive',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD)/LUT/parallel_for',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdModule,
                '_preprocess_thread_lut',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD)/OpenCV/cvtColor+convertTo',
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
            name: 'C++(SIMD)/OpenCV/cvtColor+LUT',
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
            name: 'C++(SIMD)/Halide/Naive',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdModule,
                '_preprocess_halide',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD)/Halide/LUT',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdModule,
                '_preprocess_halide_lut',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(Threads)/Naive',
            count: 100,
            fn: () =>
              byWasm(
                wasmThreadsModule,
                '_preprocess_naive',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(Threads)/LUT',
            count: 100,
            fn: () =>
              byWasm(
                wasmThreadsModule,
                '_preprocess_lut',
                imageData,
                width,
                height
              )
          },
          // {
          //   proc: 'Preprocess',
          //   name: 'C++/LUT/UINT32',
          //   count: 100,
          //   fn: () =>
          //     byWasm(wasmThreadsModule, '_preprocess_lut_u32', imageData, width, height)
          // },
          // {
          //   proc: 'Preprocess',
          //   name: 'C++/LUT/UINT32/struct',
          //   count: 100,
          //   fn: () =>
          //     byWasm(wasmThreadsModule, '_preprocess_lut_struct', imageData, width, height)
          // },
          {
            proc: 'Preprocess',
            name: 'C++(Threads)/LUT/UINT64',
            count: 100,
            fn: () =>
              byWasm(
                wasmThreadsModule,
                '_preprocess_lut_u64',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(Threads)/Naive/parallel_for',
            count: 100,
            fn: () =>
              byWasm(
                wasmThreadsModule,
                '_preprocess_thread_naive',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(Threads)/LUT/parallel_for',
            count: 100,
            fn: () =>
              byWasm(
                wasmThreadsModule,
                '_preprocess_thread_lut',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(Threads)/OpenCV/cvtColor+convertTo',
            count: 100,
            fn: () =>
              byWasm(
                wasmThreadsModule,
                '_preprocess_opencv',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(Threads)/OpenCV/cvtColor+LUT',
            count: 100,
            fn: () =>
              byWasm(
                wasmThreadsModule,
                '_preprocess_opencv_lut',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(Threads)/Halide/Naive',
            count: 100,
            fn: () =>
              byWasm(
                wasmThreadsModule,
                '_preprocess_halide',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(Threads)/Halide/LUT',
            count: 100,
            fn: () =>
              byWasm(
                wasmThreadsModule,
                '_preprocess_halide_lut',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD/Threads)/Naive',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdThreadsModule,
                '_preprocess_naive',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD/Threads)/LUT',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdThreadsModule,
                '_preprocess_lut',
                imageData,
                width,
                height
              )
          },
          // {
          //   proc: 'Preprocess',
          //   name: 'C++(SIMD)/LUT/UINT32',
          //   count: 100,
          //   fn: () =>
          //     byWasm(wasmSimdModule, '_preprocess_lut_u32', imageData, width, height)
          // },
          // {
          //   proc: 'Preprocess',
          //   name: 'C++(SIMD)/LUT/UINT32/struct',
          //   count: 100,
          //   fn: () =>
          //     byWasm(wasmSimdModule, '_preprocess_lut_struct', imageData, width, height)
          // },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD/Threads)/LUT/UINT64',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdThreadsModule,
                '_preprocess_lut_u64',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD/Threads)/Naive/HandWrittenSIMD',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdThreadsModule,
                '_preprocess_simd',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD/Threads)/LUT/HandWrittenSIMD',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdThreadsModule,
                '_preprocess_simd_lut',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD/Threads)/Naive/parallel_for',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdThreadsModule,
                '_preprocess_thread_naive',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD/Threads)/LUT/parallel_for',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdThreadsModule,
                '_preprocess_thread_lut',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD/Threads)/OpenCV/cvtColor+convertTo',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdThreadsModule,
                '_preprocess_opencv',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD/Threads)/OpenCV/cvtColor+LUT',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdThreadsModule,
                '_preprocess_opencv_lut',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD/Threads)/Halide/Naive',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdThreadsModule,
                '_preprocess_halide',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'C++(SIMD/Threads)/Halide/LUT',
            count: 100,
            fn: () =>
              byWasm(
                wasmSimdThreadsModule,
                '_preprocess_halide_lut',
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'Rust/Naive',
            count: 100,
            fn: () =>
              byRust(
                screen,
                inputArray,
                exec1,
                memory,
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'Rust/LUT',
            count: 100,
            fn: () =>
              byRust(
                screen,
                inputArray,
                exec_lut,
                memory,
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'Rust/Naive*10',
            count: 100,
            fn: () =>
              byRust(
                screen,
                inputArray,
                exec1_10,
                memory,
                imageData,
                width,
                height
              )
          },
          {
            proc: 'Preprocess',
            name: 'Rust/LUT*10',
            count: 100,
            fn: () =>
              byRust(
                screen,
                inputArray,
                exec_lut10,
                memory,
                imageData,
                width,
                height
              )
          }
          // {
          //   proc: 'Postprocess(brend)',
          //   name: 'C++',
          //   count: 100,
          //   fn: () =>
          //     postProcessByWasm(
          //       wasmModule,
          //       '_postprocess_naive_brend',
          //       imageData,
          //       width,
          //       height
          //     )
          // },
          // {
          //   proc: 'Postprocess(threshold)',
          //   name: 'C++',
          //   count: 100,
          //   fn: () =>
          //     postProcessByWasm(
          //       wasmModule,
          //       '_postprocess_naive_threshold',
          //       imageData,
          //       width,
          //       height
          //     )
          // },
          // {
          //   proc: 'Postprocess(brend)',
          //   name: 'OpenCV',
          //   count: 100,
          //   fn: () =>
          //     postProcessByWasm(
          //       wasmModule,
          //       '_postprocess_opencv_brend',
          //       imageData,
          //       width,
          //       height
          //     )
          // },
          // {
          //   proc: 'Postprocess(threshold)',
          //   name: 'OpenCV',
          //   count: 100,
          //   fn: () =>
          //     postProcessByWasm(
          //       wasmModule,
          //       '_postprocess_opencv_threshold',
          //       imageData,
          //       width,
          //       height
          //     )
          // },
          // {
          //   proc: 'Postprocess',
          //   name: 'Halide',
          //   count: 100,
          //   fn: () =>
          //     postProcessByWasm(
          //       wasmModule,
          //       '_postprocess_halide',
          //       imageData,
          //       width,
          //       height
          //     )
          // },
          // {
          //   proc: 'Postprocess(brend)',
          //   name: 'C++(SIMD)',
          //   count: 100,
          //   fn: () =>
          //     postProcessByWasm(
          //       wasmSimdModule,
          //       '_postprocess_naive_brend',
          //       imageData,
          //       width,
          //       height
          //     )
          // },
          // {
          //   proc: 'Postprocess(threshold)',
          //   name: 'C++(SIMD)',
          //   count: 100,
          //   fn: () =>
          //     postProcessByWasm(
          //       wasmSimdModule,
          //       '_postprocess_naive_threshold',
          //       imageData,
          //       width,
          //       height
          //     )
          // },
          // {
          //   proc: 'Postprocess(brend)',
          //   name: 'OpenCV(SIMD)',
          //   count: 100,
          //   fn: () =>
          //     postProcessByWasm(
          //       wasmSimdModule,
          //       '_postprocess_opencv_brend',
          //       imageData,
          //       width,
          //       height
          //     )
          // },
          // {
          //   proc: 'Postprocess(threshold)',
          //   name: 'OpenCV(SIMD)',
          //   count: 100,
          //   fn: () =>
          //     postProcessByWasm(
          //       wasmSimdModule,
          //       '_postprocess_opencv_threshold',
          //       imageData,
          //       width,
          //       height
          //     )
          // },
          // {
          //   proc: 'Postprocess',
          //   name: 'Halide(SIMD)',
          //   count: 100,
          //   fn: () =>
          //     postProcessByWasm(
          //       wasmSimdModule,
          //       '_postprocess_halide',
          //       imageData,
          //       width,
          //       height
          //     )
          // },
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
          if (canUseThreads || !benchmark.name.includes('Thread')) {
            do_benchmark(
              benchmark.proc,
              benchmark.name,
              benchmark.fn,
              benchmark.count
            )
          }
        }
      }
    )
  })()
  console.log('executing ByRust')
  resultDiv.innerHTML += 'Done<br/>'
}

benchmark()
