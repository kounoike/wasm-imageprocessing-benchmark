const rsMod = import('./wasm/rust/pkg/wasmcopy_rs')
const rsBg = import('./wasm/rust/pkg/wasmcopy_rs_bg')

const lut = new Float32Array(256)
for(let i = 0; i < 256; ++i) {
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
function byJavaScript (imageData, width, height) {
  const array = new Float32Array(width * height * 3)
  for (let i = 0; i < width * height; ++i) {
    array[i * 3] = imageData.data[i * 4] / 255
    array[i * 3 + 1] = imageData.data[i * 4 + 1] / 255
    array[i * 3 + 2] = imageData.data[i * 4 + 2] / 255
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

const count = 100
function do_benchmark (name, fn) {
  const resultDiv = document.getElementById('result')
  const msg = `Starting benchmark for ${name} `
  console.log(msg)
  resultDiv.innerHTML += msg
  for (let i = 0; i < 10; ++i) {
    fn()
  }
  const start = performance.now()
  for (let i = 0; i < count; ++i) {
    fn()
  }
  const end = performance.now()
  const duration = (end - start) / count

  resultDiv.innerHTML += 'OK.<br/>'

  const tr = document.createElement('tr')
  const th = document.createElement('th')
  const td = document.createElement('td')
  th.innerText = `by ${name}`
  td.innerText = `${duration.toFixed(2)}`
  tr.appendChild(th)
  tr.appendChild(td)
  const tbody = document.getElementById('tbody')
  tbody.appendChild(tr)
}

async function benchmark () {
  const wasmModule = await createBenchmarkModule()
  const wasmSimdModule = await createBenchmarkSimdModule()

  const width = 1920
  const height = 1080
  // const width = 640
  // const height = 480
  const canvas = new OffscreenCanvas(width, height)
  const ctx = canvas.getContext('2d')
  const imageData = ctx.createImageData(width, height)

  const resultDiv = document.getElementById('result')
  const header = document.createElement('p')
  header.innerText = `${width}x${height} image RGBA -> RGB -> /255.0 as float, 10 times for warm-up, result: ${count} times avg.`
  resultDiv.appendChild(header)

  const benchmarks = [
    {
      name: 'JavaScript',
      fn: () => byJavaScript(imageData, width, height)
    },
    {
      name: 'JavaScript(LUT)',
      fn: () => byJavaScriptLut(imageData, width, height)
    },
    {
      name: 'C++',
      fn: () =>
        byWasm(wasmModule, '_preprocess_naive', imageData, width, height)
    },
    {
      name: 'C++(LUT)',
      fn: () => byWasm(wasmModule, '_preprocess_lut', imageData, width, height)
    },
    {
      name: 'OpenCV',
      fn: () =>
        byWasm(wasmModule, '_preprocess_opencv', imageData, width, height)
    },
    {
      name: 'OpenCV(LUT)',
      fn: () =>
        byWasm(wasmModule, '_preprocess_opencv_lut', imageData, width, height)
    },
    {
      name: 'Halide',
      fn: () =>
        byWasm(wasmModule, '_preprocess_halide', imageData, width, height)
    },
    {
      name: 'Halide(LUT)',
      fn: () =>
        byWasm(wasmModule, '_preprocess_halide_lut', imageData, width, height)
    },
    {
      name: 'Naive(SIMD)',
      fn: () =>
        byWasm(wasmSimdModule, '_preprocess_naive', imageData, width, height)
    },
    {
      name: 'LUT(SIMD)',
      fn: () =>
        byWasm(wasmSimdModule, '_preprocess_lut', imageData, width, height)
    },
    {
      name: 'OpenCV(SIMD)',
      fn: () =>
        byWasm(wasmSimdModule, '_preprocess_opencv', imageData, width, height)
    },
    {
      name: 'OpenCV(LUT/SIMD)',
      fn: () =>
        byWasm(wasmSimdModule, '_preprocess_opencv_lut', imageData, width, height)
    },
    {
      name: 'Halide(SIMD)',
      fn: () =>
        byWasm(wasmModule, '_preprocess_halide', imageData, width, height)
    },
    {
      name: 'Halide(LUT/SIMD)',
      fn: () =>
        byWasm(wasmModule, '_preprocess_halide_lut', imageData, width, height)
    },
  ]

  for (const benchmark of benchmarks) {
    do_benchmark(benchmark.name, benchmark.fn)
  }
  console.log('executing ByRust')
  await (async () => {
    return Promise.all([rsMod, rsBg]).then(([{ Screen, exec }, { memory }]) => {
      const screen = new Screen(width, height)
      return do_benchmark('Rust', () =>
        byRust(screen, exec, memory, imageData, width, height)
      )
    })
  })()
  resultDiv.innerHTML += 'Done<br/>'
}

benchmark()
