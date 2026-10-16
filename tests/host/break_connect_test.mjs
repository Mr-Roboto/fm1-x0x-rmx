// SPDX-License-Identifier: GPL-3.0-only
// Exercise the uploader's actual Link and discovery with simulated Web MIDI ports.
import fs from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const page = fs.readFileSync('web/x0x_breaks.html', 'utf8');
const code = page.slice(page.indexOf('class Link {'), page.indexOf('function unpack7(a)'));
async function fixture(name, reply, connected = true, outputName = name) {
  const sent = [];
  const input = {name, state: connected ? 'connected' : 'disconnected', open: async () => {}};
  const output = {name: outputName, open: async () => {}, send: packet => {
    sent.push(packet);
    if (reply) queueMicrotask(() => input.onmidimessage({data: reply}));
  }};
  const context = vm.createContext({HDR: [0x7D,0x46,0x4C], navigator: {
    requestMIDIAccess: async options => {
      assert.equal(options.sysex, true);
      return {inputs: new Map([[1,input]]), outputs: new Map([[1,output]])};
    }}, setTimeout: (f) => setTimeout(f, 5), clearTimeout});
  vm.runInContext(code + '\nglobalThis.discover = connect;', context);
  let ok = false;
  try { await context.discover(); ok = true; } catch (_) {}
  return {ok, sent};
}
const response = [0xF0,0x7D,0x46,0x4C,1,1,3,0xF7];
for (const name of ['FM-1 Midi 2', 'FM-1', 'FM-1 MIDI', 'X0X', 'Felucca']) {
  const {ok,sent} = await fixture(name,response);
  assert(ok, name);
  assert.equal(sent.length,1);
  assert.deepEqual(Array.from(sent[0]),[0xF0,0x7D,0x46,0x4C,1,0xF7]);
}
for (const name of ['Other MIDI', 'FM-10 Midi 2', 'My FM-1 Synth', 'FM-1 Midi 2 extra']) {
  const {ok,sent} = await fixture(name,response);
  assert(!ok && !sent.length, name);
}
assert(!(await fixture('FM-1 Midi 2',response,false)).ok);
assert(!(await fixture('FM-1 Midi 2',response,true,'Other MIDI')).ok);
assert(!(await fixture('FM-1 Midi 2',null)).ok);
assert(!(await fixture('FM-1 Midi 2',[0xF0,0x7E,0x46,0x4C,1,0xF7])).ok);
assert(!(await fixture('FM-1 Midi 2',[0xF0,0x7D,0x46,0x4C,2,0xF7])).ok);
console.log('Break uploader: Windows names, paired ports, identity/command handshake and unrelated-device exclusion passed');
