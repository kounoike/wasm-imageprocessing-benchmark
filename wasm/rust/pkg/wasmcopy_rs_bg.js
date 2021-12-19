import * as wasm from './wasmcopy_rs_bg.wasm';

const lTextDecoder = typeof TextDecoder === 'undefined' ? (0, module.require)('util').TextDecoder : TextDecoder;

let cachedTextDecoder = new lTextDecoder('utf-8', { ignoreBOM: true, fatal: true });

cachedTextDecoder.decode();

let cachegetUint8Memory0 = null;
function getUint8Memory0() {
    if (cachegetUint8Memory0 === null || cachegetUint8Memory0.buffer !== wasm.memory.buffer) {
        cachegetUint8Memory0 = new Uint8Array(wasm.memory.buffer);
    }
    return cachegetUint8Memory0;
}

function getStringFromWasm0(ptr, len) {
    return cachedTextDecoder.decode(getUint8Memory0().subarray(ptr, ptr + len));
}

function _assertClass(instance, klass) {
    if (!(instance instanceof klass)) {
        throw new Error(`expected instance of ${klass.name}`);
    }
    return instance.ptr;
}
/**
* @param {Screen} screen
*/
export function exec1(screen) {
    _assertClass(screen, Screen);
    wasm.exec1(screen.ptr);
}

/**
* @param {Screen} screen
*/
export function exec2(screen) {
    _assertClass(screen, Screen);
    wasm.exec2(screen.ptr);
}

/**
* @param {Screen} screen
*/
export function exec_lut(screen) {
    _assertClass(screen, Screen);
    wasm.exec_lut(screen.ptr);
}

/**
* @param {Screen} screen
*/
export function exec1_10(screen) {
    _assertClass(screen, Screen);
    wasm.exec1_10(screen.ptr);
}

/**
* @param {Screen} screen
*/
export function exec_lut10(screen) {
    _assertClass(screen, Screen);
    wasm.exec_lut10(screen.ptr);
}

/**
*/
export class Screen {

    static __wrap(ptr) {
        const obj = Object.create(Screen.prototype);
        obj.ptr = ptr;

        return obj;
    }

    __destroy_into_raw() {
        const ptr = this.ptr;
        this.ptr = 0;

        return ptr;
    }

    free() {
        const ptr = this.__destroy_into_raw();
        wasm.__wbg_screen_free(ptr);
    }
    /**
    */
    get width() {
        var ret = wasm.__wbg_get_screen_width(this.ptr);
        return ret >>> 0;
    }
    /**
    */
    get height() {
        var ret = wasm.__wbg_get_screen_height(this.ptr);
        return ret >>> 0;
    }
    /**
    * @param {number} width
    * @param {number} height
    */
    constructor(width, height) {
        var ret = wasm.screen_new(width, height);
        return Screen.__wrap(ret);
    }
    /**
    * @returns {number}
    */
    input_pointer() {
        var ret = wasm.screen_input_pointer(this.ptr);
        return ret;
    }
    /**
    * @returns {number}
    */
    input_size() {
        var ret = wasm.screen_input_size(this.ptr);
        return ret >>> 0;
    }
    /**
    * @returns {number}
    */
    output_pointer() {
        var ret = wasm.screen_output_pointer(this.ptr);
        return ret;
    }
    /**
    * @returns {number}
    */
    output_size() {
        var ret = wasm.screen_output_size(this.ptr);
        return ret >>> 0;
    }
}

export function __wbindgen_throw(arg0, arg1) {
    throw new Error(getStringFromWasm0(arg0, arg1));
};

