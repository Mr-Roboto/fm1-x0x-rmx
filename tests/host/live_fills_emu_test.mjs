// SPDX-License-Identifier: GPL-3.0-only
// Exercise the actual spare keys, simultaneous holds, audio routing and release cleanup.
import fs from 'node:fs';
import assert from 'node:assert/strict';
let mem;
const {instance}=await WebAssembly.instantiate(fs.readFileSync(process.argv[2]),{
  wasi_snapshot_preview1:{clock_time_get:(_id,_p,out)=>{new DataView(mem.buffer).setBigUint64(out,0n,true);return 0;}}
});
const ex=instance.exports;mem=ex.memory;ex._initialize();ex.web_boot();
const render=ms=>{
  let power=0;
  for(let n=Math.round(ms*44.1);n>0;){
    const k=Math.min(n,128);ex.web_render(k);n-=k;
    const l=new Float32Array(mem.buffer,ex.web_out_l(),k),r=new Float32Array(mem.buffer,ex.web_out_r(),k);
    for(let i=0;i<k;i++){assert(Number.isFinite(l[i])&&Number.isFinite(r[i]));power+=l[i]*l[i]+r[i]*r[i];}
  }
  return power;
};
const tap=b=>{ex.web_buttons(1<<b);render(40);ex.web_buttons(0);render(80);};
const keys=mask=>{ex.web_keys(mask);render(60);};
const saved=()=>Array.from({length:ex.web_nobj()},(_,i)=>Buffer.from(new Uint8Array(mem.buffer,ex.web_store(i),Math.max(0,ex.web_store_len(i)))));
const pads=[13,15,17,20,22],all=pads.reduce((m,k)=>m|(1<<k),0);
render(120);tap(7);render(100);const before=saved();tap(6);tap(6);tap(6);
keys(all);assert.equal(ex.web_lit_keys()&all,0,'stopped presses do not arm');keys(0);
tap(10);render(100);assert.equal(ex.web_lit_keys()&all,0);
for(const k of [1,3,5,8,10]){keys(1<<k);keys(0);} // mute all sequenced parts
keys(all);assert.equal(ex.web_lit_keys()&all,all);assert(render(400)>0.01,'fills audible through the normal mixer despite sequence mutes');
keys(all^(1<<pads[1]));assert.equal(ex.web_lit_keys()&all,all^(1<<pads[1]));
keys(0);assert.equal(ex.web_lit_keys()&all,0);
keys(1<<pads[4]);assert(render(600)>0.01);
tap(4);tap(6);tap(6);tap(6); // leave and return while P2's physical key is still held
assert.equal(ex.web_lit_keys()&all,0,'leaving Live clears held state');keys(0);
keys(1<<pads[2]);assert(ex.web_lit_keys()&(1<<pads[2]));
tap(10);assert.equal(ex.web_lit_keys()&all,0,'transport stop clears fills');
tap(10);render(200);assert.equal(ex.web_lit_keys()&all,0,'restart does not resume a stale hold');
keys(0);keys(1<<pads[2]);assert(ex.web_lit_keys()&(1<<pads[2]));
keys((1<<pads[2])|(1<<25));assert(ex.web_lit_keys()&(1<<25));keys(1<<pads[2]);
assert(ex.web_lit_keys()&(1<<pads[2]),'transpose timing key leaves the fill held');
keys(0);tap(10);tap(7);render(200);assert.deepEqual(saved(),before,'all stored sounds/patterns/automation unchanged');
console.log('Live fills WASM: five physical spare keys, simultaneous holds, audio with sequence mutes, independent release, exit/stop/restart cleanup, RHS toggle and unchanged save passed');
