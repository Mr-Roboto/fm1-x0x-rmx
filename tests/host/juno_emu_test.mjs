// SPDX-License-Identifier: GPL-3.0-only
// Actual emulator UI/audio/save path, not a separate synth mock.
import fs from 'node:fs';
import assert from 'node:assert/strict';
const bytes=fs.readFileSync(process.argv[2]);
async function boot(saved={}) {
 let mem;
 const {instance}=await WebAssembly.instantiate(bytes,{wasi_snapshot_preview1:{clock_time_get:(_i,_p,o)=>{new DataView(mem.buffer).setBigUint64(o,0n,true);return 0;}}});
 const x=instance.exports;mem=x.memory;x._initialize();
 for(const [i,b] of Object.entries(saved)){new Uint8Array(mem.buffer,x.web_store(+i),b.length).set(b);x.web_store_set_len(+i,b.length);}
 x.web_boot();
 const render=ms=>{let power=0;for(let n=Math.round(ms*44.1);n>0;){let k=Math.min(128,n);x.web_render(k);n-=k;for(const ptr of [x.web_out_l(),x.web_out_r()])for(const y of new Float32Array(mem.buffer,ptr,k)){assert(Number.isFinite(y));assert(Math.abs(y)<=1);power+=y*y;}}return power;};
 const tap=b=>{x.web_buttons(1<<b);render(35);x.web_buttons(0);render(55);};
 const enc=(r,n)=>{x.web_enc(r,n);render(40);};
 const keys=m=>{x.web_keys(m);render(45);};
 const save=()=>{tap(7);render(80);return Object.fromEntries(Array.from({length:x.web_nobj()},(_,i)=>[i,Buffer.from(new Uint8Array(mem.buffer,x.web_store(i),Math.max(0,x.web_store_len(i))))]).filter(([,b])=>b.length));};
 render(100);return {x,mem,render,tap,enc,keys,save};
}
let a=await boot();
for(let i=0;i<4;i++)a.enc(1,1);a.tap(4);
for(let i=0;i<12;i++)a.enc(0,-1);
assert.equal(a.x.web_juno_param(0),1);
assert.equal(a.x.web_juno_param(43),50);
const original=Array.from({length:44},(_,i)=>a.x.web_juno_param(i));
const maxima=[5,3,5,127,127,3,100,127,127,127,127,2,127,127,127,3,3,127,127,127,127,127,127,127,127,127,127,127,127,3,127,12,127,127,127,2,3,127,127,127,1,127,127,127];
for(let page=0;page<11;page++){
 for(let knob=0;knob<4;knob++){
  const index=page*4+knob;
  for(let j=0;j<13;j++)a.enc(3+knob,1000);
  assert.equal(a.x.web_juno_param(index),maxima[index],`control ${index} reachable`);
  a.x.web_buttons(1<<2);a.render(30);a.enc(3+knob,1);a.x.web_buttons(0);a.render(30);
  assert.equal(a.x.web_juno_param(index),original[index],`ENV reset ${index}`);
 }
 a.tap(4);
}
a.tap(4); // MIX wraps to DCO
const chord=[0,4,7,11,14,19].reduce((m,k)=>m|(1<<k),0);
a.keys(chord);assert.equal(Array.from({length:6},(_,i)=>a.x.web_juno_gate(i)).reduce((s,v)=>s+v),6);
assert(a.render(500)>0.1,'six-voice chord audible while transport stopped');
a.keys(chord|(1<<23));assert.equal(Array.from({length:6},(_,i)=>a.x.web_juno_gate(i)).reduce((s,v)=>s+v),6,'seventh note steals one of six');
a.tap(13);a.tap(4);a.keys(0);assert.equal(Array.from({length:6},(_,i)=>a.x.web_juno_gate(i)).reduce((s,v)=>s+v),0,'release uses latched pitch after octave/page changes');
a.keys(1<<7);assert(Array.from({length:6},(_,i)=>a.x.web_juno_note(i)).includes(72),'octave affects new notes');
a.tap(6);a.enc(1,-1);a.keys(0);assert.equal(Array.from({length:6},(_,i)=>a.x.web_juno_gate(i)).reduce((s,v)=>s+v),0,'release survives changing part/view');
a.render(2500);
a.x.web_juno_midi(0xb0,64,127);a.x.web_juno_midi(0x90,60,110);a.x.web_juno_midi(0x80,60,0);a.render(150);
assert(Array.from({length:6},(_,i)=>a.x.web_juno_note(i)).includes(60),'MIDI sustain holds');
a.x.web_juno_midi(0xb0,64,0);a.render(2500);assert(Array.from({length:6},(_,i)=>a.x.web_juno_note(i)).every(n=>n<0),'pedal release clears voices');
a.x.web_juno_midi(0x90,0,100);a.render(50);a.x.web_juno_midi(0x90,0,0);a.render(2500);assert(Array.from({length:6},(_,i)=>a.x.web_juno_note(i)).every(n=>n<0),'MIDI note zero releases');
// Save a non-default parameter in each of the three parameter banks.
a.enc(1,1);a.tap(4);for(let i=0;i<12;i++)a.enc(0,-1);
a.enc(3,1);for(let i=0;i<4;i++)a.enc(0,1);a.enc(4,1);for(let i=0;i<4;i++)a.enc(0,1);a.enc(3,1);
const patch=Array.from({length:44},(_,i)=>a.x.web_juno_param(i));assert.notDeepEqual(patch,original);
const saved=a.save();const b=await boot(saved);assert.deepEqual(Array.from({length:44},(_,i)=>b.x.web_juno_param(i)),patch,'all Juno banks survive SAVE/reboot');
b.x.web_juno_midi(0x90,60,100);b.x.web_juno_midi(0xb0,120,0);assert(Array.from({length:6},(_,i)=>b.x.web_juno_note(i)).every(n=>n<0),'panic');
const c=await boot();for(let i=0;i<4;i++)c.enc(1,1);c.tap(4);
const patches=new Set();
for(let preset=0;preset<64;preset++){
 c.enc(2,1);assert.equal(c.x.web_juno_preset_index(),preset);
 assert.equal(c.x.web_juno_preset_edited(),0);
 patches.add(Array.from({length:44},(_,i)=>c.x.web_juno_param(i)).join(','));
}
assert.equal(patches.size,64,'all 64 different patches reachable from real PRESETS knob');
c.enc(2,1);assert.equal(c.x.web_juno_preset_index(),0);
c.enc(2,-1);assert.equal(c.x.web_juno_preset_index(),63);
c.enc(3,1);assert.equal(c.x.web_juno_preset_edited(),1);
const tuned=Array.from({length:44},(_,i)=>c.x.web_juno_param(i));
const d=await boot(c.save());assert.equal(d.x.web_juno_preset_index(),63);assert.equal(d.x.web_juno_preset_edited(),1);
assert.deepEqual(Array.from({length:44},(_,i)=>d.x.web_juno_param(i)),tuned,'edited patch and parent name survive reboot');
c.enc(1,-1);c.enc(2,1);assert.equal(c.x.web_juno_preset_index(),63);assert.deepEqual(Array.from({length:44},(_,i)=>c.x.web_juno_param(i)),tuned,'other parts retain pattern-knob behavior');
console.log('JUNO presets WASM: 64 unique sounds from real PRESETS knob, wraparound, edited marker and SAVE/reload with parent identity passed');
// A portable framebuffer for visual inspection without a browser test cycle.
fs.mkdirSync('build/host',{recursive:true});const fb=new Uint16Array(a.mem.buffer,a.x.web_fb(),240*240);const rgb=Buffer.alloc(240*240*3);for(let i=0;i<fb.length;i++){rgb[i*3]=((fb[i]>>11)&31)*255/31;rgb[i*3+1]=((fb[i]>>5)&63)*255/63;rgb[i*3+2]=(fb[i]&31)*255/31;}
fs.writeFileSync('build/host/juno-screen.ppm',Buffer.concat([Buffer.from('P6\n240 240\n255\n'),rgb]));
console.log('JUNO WASM: all 44 controls, 12 pages, six-note chords/stealing, octave/page/part-safe releases, MIDI sustain/note-zero/panic, finite audio and all-bank SAVE/reboot passed');

