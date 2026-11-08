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
const a=await boot();for(let i=0;i<4;i++)a.enc(1,1);a.tap(9);
// Step 1: C3 with an outgoing slide. ENV cycles through TIE to SLIDE.
a.keys(1);a.keys(3);a.keys(1);a.keys(0);a.tap(2);a.tap(2);
// Steps 2 and 3 are ties, with a slide flag also on step 3.
for(const key of [2,4]){a.keys(1<<key);a.tap(2);a.keys(0);}
a.tap(2);a.tap(2);
const blackMask=[1,3,5,8,10,13,15,17,20,22,25].reduce((m,k)=>m|(1<<k),0);
assert.equal(a.x.web_lit_keys()&blackMask,0,'ties have no independent note key');
// Step 4: octave-up C4. Both octave buttons set independent, mutually exclusive flags.
a.keys(1<<6);a.keys((1<<6)|2);a.keys(1<<6);a.keys(0);a.tap(13);
assert(a.x.web_lit_buttons()&(1<<13));a.tap(12);
assert(!(a.x.web_lit_buttons()&(1<<13)) && (a.x.web_lit_buttons()&(1<<12)));
a.tap(12);a.tap(13);
a.keys(2);assert(Array.from({length:6},(_,i)=>a.x.web_juno_note(i)).includes(60));a.keys(0);
a.render(1500); // let transient feedback disappear for a screen capture
fs.writeFileSync('build/host/juno-slides-screen.rgb565',Buffer.from(new Uint8Array(a.mem.buffer,a.x.web_fb(),240*240*2)));
a.tap(10);assert(a.render(340)>0.001);
assert(Array.from({length:6},(_,i)=>a.x.web_juno_note(i)).includes(60));
a.tap(10);a.render(100);assert.equal(Array.from({length:6},(_,i)=>a.x.web_juno_gate(i)).reduce((s,n)=>s+n),0);
const b=await boot(a.saved());for(let i=0;i<4;i++)b.enc(1,1);b.tap(9);
b.keys(1<<6);assert(b.x.web_lit_buttons()&(1<<13),'octave flag survives reboot');
b.keys((1<<6)|2);assert(Array.from({length:6},(_,i)=>b.x.web_juno_note(i)).includes(60));b.keys(1<<6);b.keys(0);
b.keys(1<<2);assert.equal(b.x.web_lit_keys()&blackMask,0,'tie flag survives reboot');b.tap(2);b.keys(0);
console.log('Juno slide WASM: ENV tie/slide flags, octave exclusivity/pitch, arrow screen, tie LEDs, playback/stop and saved flags passed');
