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
assert.equal(a.x.web_juno_gate_mask(),0xadad);
a.tap(1);a.enc(1,1);assert.equal(a.x.web_juno_extra(0),1);a.enc(1,1);assert.equal(a.x.web_juno_extra(0),2);
a.enc(0,1);a.enc(1,50);assert(a.x.web_juno_extra(1)>0);
a.enc(0,1);a.enc(1,1);assert.equal(a.x.web_juno_extra(2),1);
a.keys(1);a.keys(0);assert.equal(a.x.web_juno_gate_mask(),0xadac);assert.equal(a.x.web_juno_gate(0),0,'gate edits never trigger notes');
a.tap(6);a.tap(8);assert.equal(a.x.web_juno_extra(8),1);
a.enc(3,1);assert.equal(a.x.web_juno_extra(9),1,'DOWN arp order');
a.enc(4,1);assert.equal(a.x.web_juno_extra(10),3,'triplet rate');
a.enc(5,1);assert.equal(a.x.web_juno_extra(11),1,'two octaves');
a.keys((1<<0)|(1<<7)|(1<<12));assert(a.render(600)>0.001,'ARP and trance gate audible with transport stopped');a.keys(0);a.render(1500);
assert.equal(Array.from({length:6},(_,i)=>a.x.web_juno_gate(i)).reduce((s,n)=>s+n),0);
a.tap(8);assert.equal(a.x.web_juno_extra(8),0);
const before=Array.from({length:14},(_,i)=>a.x.web_juno_extra(i));
const b=await boot(a.saved());assert.deepEqual(Array.from({length:14},(_,i)=>b.x.web_juno_extra(i)),before,'all extras and gate pattern survive SAVE/reboot');
assert.equal(b.x.web_juno_gate_mask(),0xadac);
console.log('Juno performance WASM: SEL EXTRA modes/glide, trance switch/white gate key, ARP button/orders/rates/octaves/free playback and persistent extras passed');
