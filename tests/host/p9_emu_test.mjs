// SPDX-License-Identifier: GPL-3.0-only
// Actual WASM panel navigation, renderer parity, stable settings and manual startup persistence.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import zlib from 'node:zlib';
let mem;
const {instance} = await WebAssembly.instantiate(fs.readFileSync(process.argv[2]), {
  wasi_snapshot_preview1: {clock_time_get: (_id,_p,out) => {
    new DataView(mem.buffer).setBigUint64(out,0n,true); return 0;
  }}
});
const ex=instance.exports; mem=ex.memory;
ex._initialize(); ex.web_boot();
const render=ms=>{const n=Math.round(ms*44.1);for(let i=0;i<n;i+=128)ex.web_render(Math.min(128,n-i));};
const tap=b=>{ex.web_buttons(1<<b);render(40);ex.web_buttons(0);render(40);};
const turn=(e,n)=>{for(let k=0;k<Math.abs(n);k++){ex.web_enc(e,Math.sign(n));render(80);}};
const compare=name=>{
  render(1600);
  const png=fs.readFileSync(`build/host/p9/${name}.png`), chunks=[];
  for(let i=8;i<png.length;){const n=png.readUInt32BE(i);if(png.toString('ascii',i+4,i+8)==='IDAT')chunks.push(png.subarray(i+8,i+8+n));i+=n+12;}
  const raw=zlib.inflateSync(Buffer.concat(chunks)),want=Buffer.alloc(240*240*2);
  for(let y=0;y<240;y++)for(let x=0;x<240;x++){
    const i=y*721+1+x*3;
    want.writeUInt16BE(((raw[i]>>3)<<11)|((raw[i+1]>>2)<<5)|(raw[i+2]>>3),(y*240+x)*2);
  }
  const got=Buffer.from(new Uint8Array(mem.buffer,ex.web_fb(),240*240*2));
  assert.deepEqual(got.subarray(158*480,229*480),want.subarray(158*480,229*480),name+' page and controls match shared firmware renderer');
};
render(40); tap(9); turn(0,1);
for(let t=0;t<4;t++){
  if(t){turn(1,1);turn(0,1);}
  compare(`remix-${t}`);
  if(t>=2){turn(0,1);compare(`keys-${t}`);turn(0,-1);}
}
turn(3,1); // K1 APPLY: BEAT -> BAR
turn(4,1); // K2 BANK
turn(5,2); // K3 PATTERN
turn(6,1); // K4 ON
turn(1,-3);
for(let t=0;t<5;t++){
  if(t)turn(1,1);
  turn(2,t+1); // manual PRESETS, one part at a time
}
tap(7);
const settings=()=>Buffer.from(new Uint8Array(mem.buffer,ex.web_store(0),ex.web_store_len(0)));
const saved=settings();
assert.equal(saved.length,40);
assert.equal(saved[4+26],1,'USB FULL default');
assert.equal(saved[4+27],0xB6,'K1 edits global APPLY');
assert.deepEqual([...saved.subarray(4+28,4+33)],[1,2,3,4,5]);
ex.web_boot();render(40);
assert.deepEqual(settings(),saved,'startup leaves saved settings untouched');
// A real PRESETS +1 after boot must use restored BREAK P6 -> P7, not P1 -> P2.
turn(1,4); turn(2,1); tap(7);
assert.equal(settings()[4+32],6,'startup restores actual playing pattern before next manual choice');
tap(5); turn(0,100); turn(0,-1); tap(1); render(40);
const version=JSON.parse(fs.readFileSync('build/emu/build-info.json','utf8')).version;
assert(Buffer.from(mem.buffer).includes(Buffer.from(`X0X ${version}  MEM `)),'actual ABOUT text carries the built version');
console.log('P9 WASM: all four Remix arrangements, 303 page order/KEYS, USB default, APPLY, five manual patterns and reboot passed');
