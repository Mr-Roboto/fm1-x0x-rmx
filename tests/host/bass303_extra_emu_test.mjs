// SPDX-License-Identifier: GPL-3.0-only
// Real WASM panel events: legacy pages, EXTRA, SEL grouping, knob values and saved sound.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import zlib from 'node:zlib';
let memory;
const {instance} = await WebAssembly.instantiate(fs.readFileSync(process.argv[2]), {
  wasi_snapshot_preview1: {clock_time_get: (_id, _precision, out) => {
    new DataView(memory.buffer).setBigUint64(out, 0n, true); return 0;
  }},
});
const ex = instance.exports;
memory = ex.memory;
ex._initialize(); ex.web_boot();
const render = ms => {
  const frames = Math.round(ms * 44.1);
  for (let i = 0; i < frames; i += 128) ex.web_render(Math.min(128, frames - i));
};
const tap = button => { ex.web_buttons(1 << button); render(40); ex.web_buttons(0); render(80); };
const turn = (role, n) => { ex.web_enc(role, n); render(120); };
const steps = (role, n) => { for (let i = 0; i < Math.abs(n); i++) turn(role, Math.sign(n)); };
const pixels = () => Buffer.from(new Uint8Array(memory.buffer, ex.web_fb(), 240 * 240 * 2));
const hostPixels = name => {
  const png = fs.readFileSync(`build/host/waveform-ui/${name}.png`), chunks = [];
  for (let i = 8; i < png.length;) {
    const n = png.readUInt32BE(i);
    if (png.toString('ascii', i + 4, i + 8) === 'IDAT') chunks.push(png.subarray(i + 8, i + 8 + n));
    i += n + 12;
  }
  const raw = zlib.inflateSync(Buffer.concat(chunks)), output = Buffer.alloc(240 * 240 * 2);
  for (let y = 0; y < 240; y++) for (let x = 0; x < 240; x++) {
    const i = y * 721 + 1 + x * 3;
    output.writeUInt16BE(((raw[i] >> 3) << 11) | ((raw[i + 1] >> 2) << 5) | (raw[i + 2] >> 3), (y * 240 + x) * 2);
  }
  return output;
};
const compare = (name, list = false) => {
  render(1600);
  const actual = pixels(), expected = hostPixels(name);
  fs.writeFileSync(`build/host/waveform-ui/${name}-wasm.rgb565`, actual);
  // The fixtures save via project_save(); the panel clears the dirty indicator via SAVE.
  // Compare the full part/pattern/tempo header, excluding transport/dirty status at the right.
  for (let y = 0; y < 20; y++)
    assert(actual.subarray(y * 480, y * 480 + 434).equals(expected.subarray(y * 480, y * 480 + 434)), name + ' header');
  if (list) assert(actual.subarray(20 * 240 * 2, 158 * 240 * 2).equals(expected.subarray(20 * 240 * 2, 158 * 240 * 2)), name + ' list');
  // The first TONE snapshot precedes any button press; its initial help footer changes
  // after EDIT wraps. Compare the page tag and all knob labels/values independently.
  assert(actual.subarray(158 * 240 * 2, 229 * 240 * 2).equals(expected.subarray(158 * 240 * 2, 229 * 240 * 2)), name + ' knobs/page');
};
render(120); steps(1, 2); // 909 -> 808 -> 303A
for (const voice of ['a', 'b']) {
  compare(`303${voice}-tone`);
  tap(4); compare(`303${voice}-voice`);
  tap(4); compare(`303${voice}-extra`);
  tap(4); compare(`303${voice}-tone`); // EDIT wrap
  steps(0, 2); compare(`303${voice}-extra`);
  turn(0, 1); compare(`303${voice}-extra`); // SELECT clamp
  tap(1); steps(0, 12); compare(`303${voice}-sel-volume`, true);
  turn(1, -1); turn(1, 1); // VOLUME under MORE still edits
  steps(0, -4); compare(`303${voice}-sel-extra`, true);
  tap(6);
  for (let k = 0; k < 4; k++) turn(3 + k, k === 2 ? -1 : 1);
  compare(`303${voice}-extra-edited`);
  if (voice === 'a') turn(1, 1);
}
tap(7);
const saved = Array.from({length: ex.web_nobj()}, (_, i) => Buffer.from(
  new Uint8Array(memory.buffer, ex.web_store(i), Math.max(0, ex.web_store_len(i)))));
assert(saved.some(bytes => bytes.length));
ex.web_boot(); render(120); steps(1, 2); compare('303a-extra-edited');
turn(1, 1); compare('303b-extra-edited');
for (let i = 0; i < saved.length; i++)
  assert.deepEqual(Buffer.from(new Uint8Array(memory.buffer, ex.web_store(i), Math.max(0, ex.web_store_len(i)))), saved[i]);
console.log('303 EXTRA emulator: both voices, all pages, EDIT wrap/SELECT clamp, SEL/MORE, four knobs and save/reboot passed');
