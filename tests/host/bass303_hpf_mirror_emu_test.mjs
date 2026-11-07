// SPDX-License-Identifier: GPL-3.0-only
// Use the real SEL/MORE controls, then inspect the saved project and reboot.
import fs from 'node:fs';
import assert from 'node:assert/strict';
let mem;
const {instance} = await WebAssembly.instantiate(fs.readFileSync(process.argv[2]), {
  wasi_snapshot_preview1: {clock_time_get: (_id, _p, out) => {
    new DataView(mem.buffer).setBigUint64(out, 0n, true); return 0;
  }}
});
const ex = instance.exports; mem = ex.memory; ex._initialize(); ex.web_boot();
const render = ms => {
  const frames = Math.round(ms * 44.1);
  for (let i = 0; i < frames; i += 128) ex.web_render(Math.min(128, frames - i));
};
const tap = b => { ex.web_buttons(1 << b); render(40); ex.web_buttons(0); render(80); };
const turn = (role, amount) => {
  for (let i = 0; i < Math.abs(amount); i++) { ex.web_enc(role, Math.sign(amount)); render(120); }
};
const sound = () => Buffer.from(new Uint8Array(mem.buffer, ex.web_store(1), ex.web_store_len(1)));
const at = (s, voice, param) => s[4 + (2 * 12 + voice) * 16 + param];
render(120); tap(7); render(100); const initial = sound();
turn(1, 2); turn(0, 3); // 303A, fourth EDIT page: MIX
turn(3, 40); turn(4, 5); turn(5, 5); turn(6, 5);
tap(7); render(100); assert.equal(at(sound(), 0, 13), 40);
turn(1, 1); turn(0, 3); // 303B MIX
turn(3, 80); turn(4, -5); turn(5, 5); turn(6, -5);
tap(1); turn(0, 30); // final row: MIRROR TRANSPOSE
turn(1, -12); turn(0, -1); turn(1, 1); // -12 semitones; MIRROR A ON
tap(7); render(100);
const saved = sound();
assert.equal(at(saved, 0, 13), 40);
assert.equal(at(saved, 1, 13), 80);
assert.equal(at(saved, 1, 14), 1);
assert.equal(at(saved, 1, 15), 12);
const mixAt = (s, part, param) => s[4 + (5 * 12 + part) * 16 + param];
for (const [part, param, delta] of [[2,3,5],[2,1,5],[2,2,5],[3,3,-5],[3,1,5],[3,2,-5]])
  assert.equal(mixAt(saved,part,param), mixAt(initial,part,param) + delta);
ex.web_boot(); render(120); assert.deepEqual(sound(), saved);
console.log('303 WASM: fourth MIX page on both voices, all four knobs, shared pan/sends, B-only mirror, saved sound and reboot passed');
