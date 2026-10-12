// SPDX-License-Identifier: GPL-3.0-only
// Run the same Remix scenario through browser input exports and host firmware.
import fs from 'node:fs';
import assert from 'node:assert/strict';
const [wasmPath, wavPath] = process.argv.slice(2);
let mem;
const {instance} = await WebAssembly.instantiate(fs.readFileSync(wasmPath), {
  wasi_snapshot_preview1: {clock_time_get: (_id, _p, out) => {
    new DataView(mem.buffer).setBigUint64(out, 0n, true); return 0;
  }},
});
const ex = instance.exports;
mem = ex.memory;
ex._initialize();
ex.web_boot();
const audio = [];
const render = ms => {
  const n = Math.round(ms * 44.1);
  for (let i = 0; i < n; i += 128) {
    const k = Math.min(128, n - i);
    ex.web_render(k);
    audio.push(...new Float32Array(mem.buffer, ex.web_out_l(), k));
  }
};
const buttons = ['FX', 'SEL', 'ENV', 'LFO', 'EDIT', 'GLO', 'HOME', 'SAVE', 'ARP', 'SEQ', 'PLAY', 'REC'];
const encoders = ['SELECT', 'ALGO', 'PRESET', 'K1', 'K2', 'K3', 'K4'];
const white = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26];
const black = [1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25];
for (const line of fs.readFileSync('tests/scenarios/remix_parity.x0x', 'utf8').split('\n')) {
  const [cmd, a, b, c] = line.trim().split(/\s+/);
  if (!cmd || cmd.startsWith('#') || cmd === 'wav' || cmd === 'wavstop') continue;
  if (cmd === 'wait') render(+a);
  else if (cmd === 'tap') {
    ex.web_buttons(1 << buttons.indexOf(a)); render(40);
    ex.web_buttons(0); render(40);
  } else if (cmd === 'tapkey') {
    ex.web_keys(1 << (a[0] === 'w' ? white : black)[+a.slice(1)]); render(60);
    ex.web_keys(0); render(30);
  } else if (cmd === 'turn') {
    for (let k = 0; k < Math.abs(+b); k++) {
      ex.web_enc(encoders.indexOf(a), Math.sign(+b)); render(c ? +c : 80);
    }
  } else throw new Error(`Unsupported scenario command: ${line}`);
}
const wav = fs.readFileSync(wavPath);
const host = new Int16Array(wav.buffer, wav.byteOffset + 44, (wav.length - 44) / 2);
const n = Math.min(host.length / 2, audio.length);
let diff = 0, peak = 0;
for (let i = 0; i < n; i++) {
  const delta = Math.abs(host[2 * i] / 32767 - audio[i]);
  diff = Math.max(diff, delta);
  peak = Math.max(peak, Math.abs(audio[i]));
}
console.log(`Remix browser/host parity: ${n} samples, peak ${peak.toFixed(3)}, max difference ${diff.toExponential(2)}`);
assert(n > 300000 && peak > 0.05 && diff < 1e-4);
