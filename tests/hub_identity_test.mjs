// SPDX-License-Identifier: GPL-3.0-only
// Offline audit of fetched live Hub code/catalogue and a locally built package.
// node tests/hub_identity_test.mjs HUB.html catalog.json PACKAGE.fwsc
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
import {pack7} from '../web/fm1ota.js';

const html = fs.readFileSync(process.argv[2], 'utf8');
const catalog = JSON.parse(fs.readFileSync(process.argv[3], 'utf8'));
const raw = fs.readFileSync(process.argv[4]);
const fn = name => {
  const start = html.indexOf(`function ${name}(`);
  assert(start >= 0, `live Hub function ${name} exists`);
  const end = html.indexOf('\n}', start);
  assert(end > start);
  return html.slice(start, end + 2);
};
const loader = html.match(/^const OUR_LOADER = .*;$/m)?.[0];
assert(loader, 'live Hub loader recognition rule exists');
const audit = vm.runInNewContext(
  'const BLOCKS=20, BLK=48, KEEP=47; let cat = catalog.firmwares;\n' +
  ['productOf', 'logicalImage', 'unpack7', 'parseIdentity', 'whoIs'].map(fn).join('\n') +
  '\n' + loader + '\n({productOf, logicalImage, parseIdentity, whoIs, OUR_LOADER})', {catalog});
const product = audit.productOf(raw);
assert.match(product, /^FM-1_987\d{6}$/);
const unrelated = catalog.firmwares.filter(f => f.id !== 'x0xrmx');
assert(!unrelated.some(f => (f.product || '').startsWith('FM-1_987') ||
  (f.ident && (new RegExp(`^(?:${f.ident})$`)).test(product))),
  'no unrelated Hub package or family claims the RMX identity');
const frame = text => [0xf0, ...pack7([0, 0x59, 0x11, 0, 0, 0,
  ...Buffer.from(text), ...Array(28 - text.length).fill(0)]), 0xf7];
assert.equal(audit.parseIdentity(frame(product)).text, product);
assert(audit.OUR_LOADER(audit.parseIdentity(frame('ota-FM-1_900'))));
const recognized = audit.whoIs(product);
assert.equal(recognized.fw?.id, 'x0xrmx', 'live Hub family matching selects X0X-RMX');
assert.equal(recognized.exact, catalog.firmwares.find(f => f.id === 'x0xrmx').product === product);
// Simulate a future catalogue refresh without changing anything on the Hub.
const rmx = catalog.firmwares.find(f => f.id === 'x0xrmx');
rmx.product = product;
rmx.version = '1.1.0';
assert.equal(audit.whoIs(product).fw.id, 'x0xrmx');
assert.equal(audit.whoIs(product).exact, true);
assert.equal(audit.logicalImage(raw).length, raw.length - 20);
console.log(`Hub snapshot ${catalog.updated}: ${catalog.firmwares.length} packages, no namespace collision`);
console.log('Live Hub package decoder, USB identity parser, recovery loader and X0X-RMX family recognition passed');
console.log('Simulated future catalogue refresh: exact X0X-RMX recognition passed; no Hub changes made');
