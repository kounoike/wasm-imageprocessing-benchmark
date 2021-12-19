/* tslint:disable */
/* eslint-disable */
/**
* @param {Screen} screen
*/
export function exec1(screen: Screen): void;
/**
* @param {Screen} screen
*/
export function exec2(screen: Screen): void;
/**
* @param {Screen} screen
*/
export function exec_lut(screen: Screen): void;
/**
* @param {Screen} screen
*/
export function exec1_10(screen: Screen): void;
/**
* @param {Screen} screen
*/
export function exec_lut10(screen: Screen): void;
/**
*/
export class Screen {
  free(): void;
/**
* @param {number} width
* @param {number} height
*/
  constructor(width: number, height: number);
/**
* @returns {number}
*/
  input_pointer(): number;
/**
* @returns {number}
*/
  input_size(): number;
/**
* @returns {number}
*/
  output_pointer(): number;
/**
* @returns {number}
*/
  output_size(): number;
/**
*/
  readonly height: void;
/**
*/
  readonly width: void;
}
