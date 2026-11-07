// SPDX-License-Identifier: GPL-3.0-only
// Real HOME/key/knob input: queued LEDs, neutral controls and untouched saved project.
import fs from 'node:fs';
import assert from 'node:assert/strict';
let mem;
const {instance} = await WebAssembly.instantiate(fs.readFileSync(process.argv[2]), {
  wasi_snapshot_preview1: {clock_time_get: (_id,_p,out) => {
    new DataView(mem.buffer).setBigUint64(out,0n,true);return 0;
  }}
});
const ex=instance.exports;mem=ex.memory;ex._initialize();ex.web_boot();
const render=ms=>{for(let n=Math.round(ms*44.1);n>0;){const k=Math.min(n,128);ex.web_render(k);n-=k;}};
const tap=b=>{ex.web_buttons(1<<b);render(40);ex.web_buttons(0);render(80);};
const key=k=>{ex.web_keys(1<<k);render(40);ex.web_keys(0);render(40);};
const turn=(r,n)=>{ex.web_enc(r,n);render(80);};
const white=[0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26];
const snapshot=()=>Array.from({length:ex.web_nobj()},(_,i)=>Buffer.from(new Uint8Array(mem.buffer,ex.web_store(i),Math.max(0,ex.web_store_len(i)))));
render(120);tap(7);render(100);const saved=snapshot();
tap(6);tap(6);tap(6); // Pattern, Song, Live
assert(ex.web_lit_buttons()&(1<<6));assert(ex.web_lit_keys()&(1<<white[7]));
assert(!(ex.web_lit_keys()&(1<<25)));
key(white[8]);assert(ex.web_lit_keys()&(1<<white[8]));
key(25);assert(ex.web_lit_keys()&(1<<25));tap(10);key(white[12]);
let bright=false,dark=false;
for(let i=0;i<5;i++){
  render(80);const keys=ex.web_lit_keys();assert(keys&(1<<white[8]));
  if(keys&(1<<white[12]))bright=true;else dark=true;
}
assert(bright&&dark,'new transpose flashes while current remains lit');
render(2100);assert(ex.web_lit_keys()&(1<<white[12]));assert(!(ex.web_lit_keys()&(1<<white[8])));
key(white[2]);key(25);assert(!(ex.web_lit_keys()&(1<<25)));assert(ex.web_lit_keys()&(1<<white[2]));
turn(3,15);turn(4,-10);turn(5,5);turn(6,50);
tap(10);tap(7);render(200);assert.deepEqual(snapshot(),saved,'Live offsets/transpose leave saved sounds and sequences intact');
ex.web_boot();render(120);tap(6);tap(6);tap(6);
assert(ex.web_lit_keys()&(1<<white[7]));assert(!(ex.web_lit_keys()&(1<<25)));
console.log('Live WASM: third HOME page, immediate keys, active/queued flashing LEDs, bar commit, toggle, macros, unchanged save and neutral reboot passed');
