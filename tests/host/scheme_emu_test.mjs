// SPDX-License-Identifier: GPL-3.0-only
// Exercise the same GLOBAL selector through WebAssembly input and storage exports.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import zlib from 'node:zlib';

let mem;
const {instance} = await WebAssembly.instantiate(fs.readFileSync(process.argv[2]), {
  wasi_snapshot_preview1: {clock_time_get: (_id, _p, out) => {
    new DataView(mem.buffer).setBigUint64(out, 0n, true); return 0;
  }},
});
const ex = instance.exports;
mem = ex.memory;
ex._initialize();
ex.web_boot();
const render = ms => {
  const frames = Math.round(ms * 44.1);
  for (let i = 0; i < frames; i += 128) ex.web_render(Math.min(128, frames - i));
};
const buttons = ['FX', 'SEL', 'ENV', 'LFO', 'EDIT', 'GLO', 'HOME', 'SAVE', 'ARP', 'SEQ', 'PLAY', 'REC'];
const tap = name => {
  ex.web_buttons(1 << buttons.indexOf(name)); render(40);
  ex.web_buttons(0); render(40);
};
const turn = (role, n) => {
  for (let i = 0; i < Math.abs(n); i++) { ex.web_enc(role, Math.sign(n)); render(80); }
};
const fb = () => Buffer.from(new Uint8Array(mem.buffer, ex.web_fb(), 240 * 240 * 2));
const settings = () => new Uint8Array(mem.buffer, ex.web_store(0), ex.web_store_len(0));
const crc32 = bytes => {
  let c = 0xffffffff;
  for (const b of bytes) {
    c ^= b;
    for (let k = 0; k < 8; k++) c = (c >>> 1) ^ (c & 1 ? 0xedb88320 : 0);
  }
  return (c ^ 0xffffffff) >>> 0;
};
const chunk = (name, data) => {
  const body = Buffer.concat([Buffer.from(name), data]);
  const head = Buffer.alloc(4), tail = Buffer.alloc(4);
  head.writeUInt32BE(data.length); tail.writeUInt32BE(crc32(body));
  return Buffer.concat([head, body, tail]);
};
const screenshot = path => {
  const pixels = fb(), raw = Buffer.alloc(240 * (1 + 240 * 3));
  for (let y = 0; y < 240; y++)
    for (let x = 0; x < 240; x++) {
      const c = pixels.readUInt16BE((y * 240 + x) * 2);
      const i = y * 721 + 1 + x * 3;
      raw[i] = (c >> 11) * 255 / 31;
      raw[i + 1] = ((c >> 5) & 63) * 255 / 63;
      raw[i + 2] = (c & 31) * 255 / 31;
    }
  const head = Buffer.alloc(13);
  head.writeUInt32BE(240, 0); head.writeUInt32BE(240, 4); head[8] = 8; head[9] = 2;
  fs.writeFileSync(path, Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]),
    chunk('IHDR', head), chunk('IDAT', zlib.deflateSync(raw)), chunk('IEND', Buffer.alloc(0))]));
};
// Read the host PNG's pixels independently for renderer parity of each part chip.
const host_chip = path => {
  const png = fs.readFileSync(path), chunks = [];
  for (let i = 8; i < png.length;) {
    const n = png.readUInt32BE(i);
    if (png.toString('ascii', i + 4, i + 8) === 'IDAT') chunks.push(png.subarray(i + 8, i + 8 + n));
    i += n + 12;
  }
  const raw = zlib.inflateSync(Buffer.concat(chunks));
  const i = 9 * 721 + 1 + 2 * 3;
  assert.equal(raw[9 * 721], 0); // host PNG uses unfiltered RGB rows
  return ((raw[i] >> 3) << 11) | ((raw[i + 1] >> 2) << 5) | (raw[i + 2] >> 3);
};
const out = 'build/host/schemes/browser';
fs.mkdirSync(out, {recursive: true});
const parts = ['909', '808', '303A', '303B', 'BREAK'];
const schemeFrames = [];
render(120);
for (let s = 0; s < 6; s++) {
  tap('GLO'); turn(0, 11); // existing GLOBAL rows; SELECT skips section headers
  turn(1, s);
  screenshot(`${out}/${s}-global.png`);
  tap('SAVE');
  assert.equal(settings()[4 + 17], 0xd0 + s);
  assert.equal(settings()[4 + 8], 1); // legacy AMBER byte is never reinterpreted
  ex.web_boot(); render(120);
  tap('SEQ');
  const chips = [];
  for (let p = 0; p < 5; p++) {
    const pixels = fb(), chip = pixels.readUInt16BE((9 * 240 + 2) * 2);
    assert.equal(chip, host_chip(`build/host/schemes/${s}-${parts[p]}.png`));
    chips.push(chip);
    screenshot(`${out}/${s}-${parts[p]}.png`);
    if (p < 4) turn(1, 1);
  }
  assert.equal(new Set(chips).size, 5);
  schemeFrames.push(fb());
  // Return to DEFAULT for the next independent selector input.
  tap('GLO'); turn(0, 11); // First GLOBAL opening after reboot starts at the top.
  const remembered = fb().subarray(20 * 240 * 2, 158 * 240 * 2);
  for (const view of ['FX', 'LFO', 'SEQ']) {
    tap(view); tap('SEL'); turn(0, 3); tap('HOME'); tap('GLO');
    assert.deepEqual(fb().subarray(20 * 240 * 2, 158 * 240 * 2), remembered);
  }
  screenshot(`${out}/${s}-global-reopen.png`);
  turn(1, -5); tap('SAVE');
  ex.web_boot(); render(120);
}
assert.equal(new Set(schemeFrames.map(crc32)).size, 6);
// Factory reset through the confirmation UI and then a save/reboot.
tap('GLO'); turn(0, 11); turn(1, 5); turn(0, 4);
tap('SEL'); tap('SEL'); tap('SAVE');
assert.equal(settings()[4 + 17], 0xd0);
ex.web_boot(); render(120);
// Old short settings keep MONO and default to the original scheme.
settings()[4 + 8] = 4;
ex.web_store_set_len(0, 20);
ex.web_boot(); render(120);
tap('SAVE');
assert.equal(settings()[4 + 8], 4);
assert.equal(settings()[4 + 17], 0xd0);
console.log('Browser schemes: six real GLOBAL choices, host part-color parity, save/reboot/reset, old settings GLOBAL cursor/scroll across menus and 42 LCD screenshots passed');
