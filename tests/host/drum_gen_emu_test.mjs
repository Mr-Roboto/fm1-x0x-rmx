// SPDX-License-Identifier: GPL-3.0-only
// Real WebAssembly panel input, renderer parity against host screenshots,
// transient-only edits, mutation/Undo statuses and restart defaults.
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
const tap = button => {
  ex.web_buttons(1 << button); render(40);
  ex.web_buttons(0); render(40);
};
const turn = (role, value) => { ex.web_enc(role, value); render(80); };
const pixels = () => Buffer.from(new Uint8Array(memory.buffer, ex.web_fb(), 240 * 240 * 2));
const hostPixels = name => {
  const png = fs.readFileSync(`build/scenarios/drum_gen/${name}.png`), chunks = [];
  for (let i = 8; i < png.length;) {
    const n = png.readUInt32BE(i);
    if (png.toString('ascii', i + 4, i + 8) === 'IDAT') chunks.push(png.subarray(i + 8, i + 8 + n));
    i += n + 12;
  }
  const raw = zlib.inflateSync(Buffer.concat(chunks)), output = Buffer.alloc(240 * 240 * 2);
  for (let y = 0; y < 240; y++) {
    assert.equal(raw[y * 721], 0);
    for (let x = 0; x < 240; x++) {
      const i = y * 721 + 1 + x * 3;
      output.writeUInt16BE(((raw[i] >> 3) << 11) | ((raw[i + 1] >> 2) << 5) | (raw[i + 2] >> 3), (y * 240 + x) * 2);
    }
  }
  return output;
};
const compare = name => {
  render(1000); // knob readout expires; values/page/footer stay visible
  const actual = pixels(), expected = hostPixels(name);
  fs.writeFileSync(`build/scenarios/drum_gen/${name}-browser.rgb565`, actual);
  assert.deepEqual(actual.subarray(0, 20 * 240 * 2), expected.subarray(0, 20 * 240 * 2), `${name}: header`);
  assert.deepEqual(actual.subarray(172 * 240 * 2), expected.subarray(172 * 240 * 2), `${name}: controls/footer`);
};
render(120);
const writes = ex.web_store_writes();
tap(8); compare('909-map');
turn(6, 1); compare('909-euclid');
tap(8); compare('909-fill');
tap(8); compare('909-offset');
const originalOffsets = pixels().subarray(172 * 240 * 2, 221 * 240 * 2);
tap(8); turn(1, 1); turn(3, 1); compare('808-map');
turn(1, -1); compare('909-euclid');
assert.equal(ex.web_store_writes(), writes, 'generator knobs do not autosave');
tap(13); // NEW
tap(12); // effective +/-2 MUTATE
tap(8); tap(8); render(1400);
assert.notDeepEqual(pixels().subarray(172 * 240 * 2, 221 * 240 * 2), originalOffsets, 'OFFSET reflects MUTATE');
ex.web_buttons(1 << 6); render(40); // HOME + REC: Undo, settings remain current
ex.web_buttons((1 << 6) | (1 << 11)); render(40);
ex.web_buttons(0); render(40);
tap(12);
assert.deepEqual(pixels().subarray(0, 20 * 240 * 2), hostPixels('new-first').subarray(0, 20 * 240 * 2));
tap(7); // save generated ordinary steps
const saved = Array.from({length: ex.web_nobj()}, (_, i) => Buffer.from(
  new Uint8Array(memory.buffer, ex.web_store(i), Math.max(0, ex.web_store_len(i)))));
assert(saved.some(bytes => bytes.length));
ex.web_boot(); render(120); tap(8);
turn(0, -1); turn(0, -1); // existing simulator UI retains its page cursor on boot
turn(6, 1); tap(8); tap(8); render(1400);
fs.writeFileSync('build/scenarios/drum_gen/restart-browser.rgb565', pixels());
assert.deepEqual(pixels().subarray(172 * 240 * 2, 221 * 240 * 2), originalOffsets, 'restart restores session defaults');
for (let i = 0; i < saved.length; i++)
  assert.deepEqual(Buffer.from(new Uint8Array(memory.buffer, ex.web_store(i), Math.max(0, ex.web_store_len(i)))), saved[i]);
console.log('drum generator emulator: both modes/pages, host renderer parity, session isolation, OFFSET mutation, NEW FIRST and restart passed');
