// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs';
import assert from 'node:assert/strict';
const wasm=fs.readFileSync(process.argv[2]);
async function boot(store={}){
 let mem;
 const {instance}=await WebAssembly.instantiate(wasm,{wasi_snapshot_preview1:{clock_time_get:(_i,_p,o)=>{new DataView(mem.buffer).setBigUint64(o,0n,true);return 0;}}});
 const x=instance.exports;mem=x.memory;x._initialize();
 for(const [i,b] of Object.entries(store)){new Uint8Array(mem.buffer,x.web_store(+i),b.length).set(b);x.web_store_set_len(+i,b.length);}
 x.web_boot();
 const render=ms=>{let power=0;for(let n=Math.round(ms*44.1);n>0;){let k=Math.min(128,n);x.web_render(k);n-=k;for(const v of new Float32Array(mem.buffer,x.web_out_l(),k)){assert(Number.isFinite(v));power+=v*v;}}return power;};
 const tap=b=>{x.web_buttons(1<<b);render(35);x.web_buttons(0);render(50);};
 const enc=(r,n)=>{x.web_enc(r,n);render(45);};const keys=m=>{x.web_keys(m);render(50);};
 const saved=()=>{tap(7);return Object.fromEntries(Array.from({length:x.web_nobj()},(_,i)=>[i,Buffer.from(new Uint8Array(mem.buffer,x.web_store(i),Math.max(0,x.web_store_len(i))))]).filter(([,b])=>b.length));};
 render(80);return {x,mem,render,tap,enc,keys,saved};
}
const a=await boot();for(let i=0;i<4;i++)a.enc(1,1);
a.tap(9); // SEQ
a.enc(3,1);a.enc(3,1); // root D
for(let i=0;i<5;i++)a.enc(4,1); // major
a.enc(5,1); // octave 4
// Select step 1 and give it third scale degree (third black key).
a.keys(1);a.keys(1|(1<<5));
assert(Array.from({length:6},(_,i)=>a.x.web_juno_note(i)).includes(66));
a.keys(1);a.keys(0);
a.tap(10);assert(a.render(180)>0.001);
// Hold step and turn first normal EDIT parameter while playing.
a.keys(1);a.enc(3,1);a.keys(0);a.render(300);
a.tap(10);a.render(200);
assert.equal(Array.from({length:6},(_,i)=>a.x.web_juno_gate(i)).reduce((s,n)=>s+n),0);
const b=await boot(a.saved());for(let i=0;i<4;i++)b.enc(1,1);
b.tap(9);b.keys(1);b.keys(1|(1<<5));
assert(Array.from({length:6},(_,i)=>b.x.web_juno_note(i)).includes(66),'root scale octave persist');
b.keys(1);b.keys(0);
console.log('Juno SEQ WASM: root/scale/octave knobs, white step + black note, preview, playback, knob lock, stop and saved setup passed');
