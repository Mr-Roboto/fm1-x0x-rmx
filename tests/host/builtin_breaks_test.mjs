// SPDX-License-Identifier: GPL-3.0-only
// Built-in bank geometry, decoded headroom/seams and distinct rhythmic envelopes.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {imaDecode} from '../../web/x0x_breaks.js';
execFileSync('sh',['tools/gen_builtin_break.sh','build/gen']);
const path = 'build/gen/x0x_break_bank.h';
const bank = fs.readFileSync(path,'utf8');
execFileSync('build/host/gen_builtin_break',[path]);
assert.equal(fs.readFileSync(path,'utf8'),bank,'deterministic build-time generation');
assert.match(bank,/#define X0X_NBREAKS 2/);
assert.match(bank,/#define X0X_BREAK_RATE 22050u/);
fs.mkdirSync('build/host/p9',{recursive:true});
const envs = [];
for (let k = 0; k < 2; k++) {
  const name = k ? '808 GR' : '909 GR';
  const n = +bank.match(new RegExp(`#define X0X_BREAK_${k}_N (\\d+)u`))[1];
  assert.equal(n,110250,'two bars at 96 BPM');
  assert(bank.includes(`{"${name}", X0X_BREAK_${k}, X0X_BREAK_${k}_N, 2}`));
  const bytes = bank.match(new RegExp(`X0X_BREAK_${k}\\[\\d+\\] = \\{([^}]+)`))[1].match(/\d+/g).map(Number);
  assert.equal(bytes.length,55125);
  const pcm = imaDecode(Uint8Array.from(bytes),n);
  let peak=0, power=0;
  for (const v of pcm) { peak=Math.max(peak,Math.abs(v)); power+=v*v; }
  const rms=Math.sqrt(power/n), seam=Math.abs(pcm[n-1]-pcm[0]);
  assert(peak<32000 && rms>1500 && rms<14000,`${name}: decoded headroom and useful level`);
  assert(seam<1000,`${name}: quiet loop seam`);
  const env = Array.from({length:32},(_,s)=>{
    const a=Math.round(s*n/32), b=Math.round((s+1)*n/32);
    let e=0; for(let i=a;i<b;i++) e+=pcm[i]*pcm[i];
    return Math.sqrt(e/(b-a));
  });
  envs.push(env);
  const wav=Buffer.alloc(44+n*2);
  wav.write('RIFF'); wav.writeUInt32LE(wav.length-8,4); wav.write('WAVEfmt ',8);
  wav.writeUInt32LE(16,16); wav.writeUInt16LE(1,20); wav.writeUInt16LE(1,22);
  wav.writeUInt32LE(22050,24); wav.writeUInt32LE(44100,28); wav.writeUInt16LE(2,32);
  wav.writeUInt16LE(16,34); wav.write('data',36); wav.writeUInt32LE(n*2,40);
  pcm.forEach((v,i)=>wav.writeInt16LE(v,44+i*2));
  fs.writeFileSync(`build/host/p9/${k?'808':'909'}-groove.wav`,wav);
  console.log(`${name}: ${n} samples, peak ${peak}, RMS ${rms.toFixed(1)}, seam ${seam}`);
}
const distance = Math.sqrt(envs[0].reduce((n,v,i)=>n+(v-envs[1][i])**2,0)/32);
assert(distance>1000,'distinct rhythmic energy envelopes');
console.log(`Built-in breaks: 110250 flash bytes unchanged; rhythmic envelope distance ${distance.toFixed(1)}; listening WAVs in build/host/p9/`);
