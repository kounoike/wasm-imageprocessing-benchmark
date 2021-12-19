extern crate wasm_bindgen;

use array_macro::*;
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub struct Screen {
  input_bytes: Vec<u8>,
  output_bytes: Vec<f32>,
  lut: [f32; 256],
  #[wasm_bindgen(readonly)]
  pub width: usize,
  #[wasm_bindgen(readonly)]
  pub height: usize,
}

fn create_buffer_u8(width: usize, height: usize, channel: usize) -> Vec<u8> {
  let size = width * height * channel;
  let mut bytes = Vec::with_capacity(size);
  bytes.resize(size, 0);
  bytes
}

fn create_buffer_f32(width: usize, height: usize, channel: usize) -> Vec<f32> {
  let size = width * height * channel;
  let mut bytes = Vec::<f32>::with_capacity(size);
  bytes.resize(size, 0f32);
  bytes
}

#[wasm_bindgen]
impl Screen {
  #[wasm_bindgen(constructor)]
  pub fn new(width: usize, height: usize) -> Screen {
    Screen {
      input_bytes: create_buffer_u8(width, height, 4),
      output_bytes: create_buffer_f32(width, height, 3),
      lut: array![i => (i as f32) / 255f32; 256],
      width,
      height,
    }
  }

  pub fn input_pointer(&self) -> *const u8 {
    self.input_bytes.as_ptr()
  }

  pub fn input_size(&self) -> usize {
    self.input_bytes.len()
  }

  pub fn output_pointer(&self) -> *const f32 {
    self.output_bytes.as_ptr()
  }

  pub fn output_size(&self) -> usize {
    self.output_bytes.len()
  }
}

#[wasm_bindgen]
pub fn exec1(screen: &mut Screen) {
  for i in 0..screen.width * screen.height {
    screen.output_bytes[i * 3] = (screen.input_bytes[i * 4] as f32) / 255f32;
    screen.output_bytes[i * 3 + 1] = (screen.input_bytes[i * 4 + 1] as f32) / 255f32;
    screen.output_bytes[i * 3 + 2] = (screen.input_bytes[i * 4 + 2] as f32) / 255f32;
  }
}

#[wasm_bindgen]
pub fn exec2(screen: &mut Screen) {
  let output_bytes = &mut screen.output_bytes;
  let input_bytes = &screen.input_bytes;
  for i in 0..screen.width * screen.height {
    output_bytes[i * 3] = (input_bytes[i * 4] as f32) / 255f32;
    output_bytes[i * 3 + 1] = (input_bytes[i * 4 + 1] as f32) / 255f32;
    output_bytes[i * 3 + 2] = (input_bytes[i * 4 + 2] as f32) / 255f32;
  }
}

#[wasm_bindgen]
pub fn exec_lut(screen: &mut Screen) {
  for i in 0..screen.width * screen.height {
    screen.output_bytes[i * 3] = screen.lut[screen.input_bytes[i * 4] as usize];
    screen.output_bytes[i * 3 + 1] = screen.lut[screen.input_bytes[i * 4 + 1] as usize];
    screen.output_bytes[i * 3 + 2] = screen.lut[screen.input_bytes[i * 4 + 2] as usize];
  }
}

#[wasm_bindgen]
pub fn exec1_10(screen: &mut Screen) {
  for _ in 0..10 {
    for i in 0..screen.width * screen.height {
      screen.output_bytes[i * 3] = (screen.input_bytes[i * 4] as f32) / 255f32;
      screen.output_bytes[i * 3 + 1] = (screen.input_bytes[i * 4 + 1] as f32) / 255f32;
      screen.output_bytes[i * 3 + 2] = (screen.input_bytes[i * 4 + 2] as f32) / 255f32;
    }
  }
}

#[wasm_bindgen]
pub fn exec_lut10(screen: &mut Screen) {
  for _ in 0..10 {
    for i in 0..screen.width * screen.height {
      screen.output_bytes[i * 3] = screen.lut[screen.input_bytes[i * 4] as usize];
      screen.output_bytes[i * 3 + 1] = screen.lut[screen.input_bytes[i * 4 + 1] as usize];
      screen.output_bytes[i * 3 + 2] = screen.lut[screen.input_bytes[i * 4 + 2] as usize];
    }
  }
}

// #[wasm_bindgen]
// pub fn exec2(screen: &mut Screen) {
//     for i in 0..screen.width * screen.height {
//         screen.output_bytes[i * 3] = screen.input_bytes[i * 4].saturating_add(10);
//         screen.output_bytes[i * 3 + 1] = screen.input_bytes[i * 4].saturating_add(10);
//         screen.output_bytes[i * 3 + 1] = screen.input_bytes[i * 4].saturating_add(10);
//     }
// }
