// SPDX-License-Identifier: GPL-3.0-only
// Run with Playwright available via X0X_PLAYWRIGHT (module directory) or node_modules.
// node tests/host/device_palette_browser_test.mjs SITE_ROOT OUTPUT_DIR [ORIGINAL_HTML]
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {createRequire} from 'node:module';
const require = createRequire(import.meta.url);
const {chromium} = require(process.env.X0X_PLAYWRIGHT || 'playwright');
const root = process.argv[2].replace(/\/?$/, '/');
const out = process.argv[3];
const schemes = ['cream', 'dark', 'sunset', 'pink'];
const states = ['', 'lit', 'dim', 'held', 'latched', 'lit held', 'lit latched'];
fs.mkdirSync(out, {recursive: true});
const browser = await chromium.launch({executablePath: process.env.X0X_CHROMIUM, headless: true, args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required']});
const context = await browser.newContext({viewport: {width: 1200, height: 1000}, hasTouch: true});
await context.addInitScript(() => {
  window.audioChecks = {contexts: [], nodes: [], messages: [], frames: 0};
  const AC = window.AudioContext, AWN = window.AudioWorkletNode;
  window.AudioContext = class extends AC {
    constructor(options) { super(options); window.audioChecks.contexts.push(this); }
  };
  window.AudioWorkletNode = class extends AWN {
    constructor(ac, ...args) {
      super(ac, ...args);
      const analyser = ac.createAnalyser(); analyser.fftSize = 2048;
      this.connect(analyser);
      this.connect = destination => analyser.connect(destination);
      window.audioChecks.nodes.push({node: this, analyser});
      const post = this.port.postMessage.bind(this.port);
      this.port.postMessage = (...args) => { window.audioChecks.messages.push(args[0].type); return post(...args); };
      this.port.addEventListener('message', e => { if (e.data.type === 'frame') window.audioChecks.frames++; });
    }
  };
});
const page = await context.newPage();
const errors = [];
page.on('pageerror', e => errors.push(e.message));
page.on('response', r => { if (r.url().startsWith(root) && r.status() >= 400) errors.push(`${r.status()} ${r.url()}`); });
const open = async p => { await p.goto(root + 'emu/'); await p.evaluate(() => document.fonts.ready);
  // Background tabs throttle transitions; compare settled colors deterministically.
  await p.addStyleTag({content: ".btnface, .keyface { transition: none !important; }"});
};
const snapshot = p => p.evaluate(() => ({
  targets: [...document.querySelectorAll('.hit')].map(e => [e.getAttribute('aria-label'), e.style.cssText]),
  shapes: [...document.querySelectorAll('#panel *')].map(e => {
    const s = getComputedStyle(e), b = e.getBBox();
    return [e.tagName, e.textContent, b.x, b.y, b.width, b.height, s.fill, s.stroke, s.strokeWidth];
  }),
}));
await open(page);
assert.equal(await page.locator('#device-color').inputValue(), 'cream');
if (process.argv[4]) {
  const baseline = await context.newPage();
  await baseline.route(root + 'emu/', route => route.fulfill({contentType: 'text/html', body: fs.readFileSync(process.argv[4], 'utf8')}));
  await open(baseline);
  const preserved = await baseline.locator('#device-color').count() ? ['cream', 'dark'] : ['dark'];
  for (const scheme of preserved) {
    await page.selectOption('#device-color', scheme);
    if (await baseline.locator('#device-color').count()) await baseline.selectOption('#device-color', scheme);
  for (const state of states) {
    for (const p of [baseline, page]) await p.evaluate(state => {
      document.querySelectorAll('#panel .ctl').forEach(e => {
        e.classList.remove('lit', 'dim', 'held', 'latched');
        if (state) e.classList.add(...state.split(' '));
      });
    }, state);
    await page.waitForTimeout(100);
    assert.deepEqual(await snapshot(page), await snapshot(baseline), scheme + ' exact geometry/colors: ' + state);
  }
  }
  await page.evaluate(() => document.querySelectorAll('#panel .ctl').forEach(e => e.classList.remove('lit', 'dim', 'held', 'latched')));
  await baseline.close();
}
const geometry = (await snapshot(page)).targets;
const contrast = (a, b) => {
  const luminance = color => {
    const rgb = color.match(/\d+/g).slice(0, 3).map(Number).map(v => {
      v /= 255; return v <= 0.04045 ? v / 12.92 : ((v + 0.055) / 1.055) ** 2.4;
    });
    return rgb[0] * 0.2126 + rgb[1] * 0.7152 + rgb[2] * 0.0722;
  };
  const x = luminance(a), y = luminance(b);
  return (Math.max(x, y) + 0.05) / (Math.min(x, y) + 0.05);
};
const housings = new Set();
for (const scheme of schemes) {
  await page.selectOption('#device-color', scheme);
  const knobColors = await page.evaluate(() => {
    const body = getComputedStyle(document.querySelector('#panel > rect')).fill;
    const knob = document.querySelector('#panel .knob');
    return [body, getComputedStyle(knob.querySelector('text')).fill,
      getComputedStyle(knob.querySelectorAll('circle')[1]).fill, getComputedStyle(knob.querySelector('line')).stroke];
  });
  housings.add(knobColors[0]);
  assert(contrast(knobColors[0], knobColors[1]) >= 4.5, scheme + ' housing labels');
  assert(contrast(knobColors[2], knobColors[3]) >= 3, scheme + ' knob indicator');
  const keyStates = [];
  for (const state of ['', 'lit', 'dim', 'held']) {
    keyStates.push(await page.evaluate(state => {
      const group = document.querySelector('#panel .keyface.black').parentNode;
      group.classList.remove('lit', 'dim', 'held');
      if (state) group.classList.add(state);
      const style = getComputedStyle(group.querySelector('rect'));
      return [style.fill, style.stroke, style.strokeWidth];
    }, state));
  }
  assert.equal(new Set(keyStates.slice(0, 3).map(s => s[0])).size, 3, scheme + ' normal/lit/dim keys');
  assert.notEqual(keyStates[3][1], 'none', scheme + ' held outline');
  await page.evaluate(() => document.querySelectorAll('#panel .ctl').forEach(e => e.classList.remove('lit', 'dim', 'held')));
  for (const lit of [false, true]) {
    await page.evaluate(lit => document.querySelectorAll('#panel .ctl:not(.knob)').forEach(e => e.classList.toggle('lit', lit)), lit);
    await page.waitForTimeout(100);
    const colors = await page.evaluate(() => [...document.querySelectorAll('#panel .btnface')].map(face => [
      getComputedStyle(face).fill, getComputedStyle(face.parentNode.querySelector('text')).fill,
    ]));
    assert(colors.every(([face, label]) => contrast(face, label) >= 4.5), scheme + ' button contrast');
  }
}
assert.equal(housings.size, 4, 'four distinct exterior finishes');
await page.evaluate(() => document.querySelectorAll('#panel .ctl').forEach(e => e.classList.remove('lit')));
await page.selectOption('#device-color', 'cream');
await page.locator('#go').click();
await page.waitForFunction(() => document.getElementById('start').hidden && window.x0xLast);
const tap = async name => { await page.locator(`.hit[aria-label="${name}"]`).click({delay: 70}); await page.waitForTimeout(160); };
const wheel = async (name, steps) => {
  await page.locator(`.hit[aria-label="${name}"]`).hover();
  for (let i = 0; i < Math.abs(steps); i++) { await page.mouse.wheel(0, steps > 0 ? -100 : 100); await page.waitForTimeout(100); }
};
const audio = () => page.evaluate(() => {
  const {contexts, nodes, messages, frames} = window.audioChecks;
  const samples = new Float32Array(2048); nodes[0].analyser.getFloatTimeDomainData(samples);
  return {state: contexts[0].state, time: contexts[0].currentTime, contexts: contexts.length, nodes: nodes.length,
    messages: messages.length, frames, peak: Math.max(...samples.map(Math.abs))};
});
// Seed ordinary drum steps using existing pointer/keyboard controls, then start transport.
await tap('SEQ'); await tap('white key 1'); await tap('white key 5'); await tap('white key 9');
await page.locator('#device-color').blur();
await page.keyboard.press('Space', {delay: 70}); await page.waitForTimeout(500);
const peaks = [];
const touch = await context.newCDPSession(page);
for (const scheme of [...schemes, 'cream']) {
  const before = await audio();
  await page.selectOption('#device-color', scheme);
  await page.waitForTimeout(250);
  const after = await audio();
  assert.equal(after.state, 'running'); assert.equal(after.contexts, 1); assert.equal(after.nodes, 1);
  assert(after.time > before.time); assert(after.frames > before.frames);
  assert.equal(after.messages, before.messages, 'palette must send no worklet messages');
  assert.deepEqual((await snapshot(page)).targets, geometry);
  const schemePeaks = [];
  for (let i = 0; i < 40; i++) { schemePeaks.push((await audio()).peak); await page.waitForTimeout(50); }
  peaks.push(...schemePeaks);
  assert(Math.max(...schemePeaks) > 0.01, scheme + ' audio remains active after switching');
  await page.screenshot({path: `${out}/${scheme}-playing.png`});
  // Test actual pointer and keyboard holds; preserve their classes while recoloring.
  const key = page.locator('.hit[aria-label="white key 2"]');
  await key.hover(); await page.mouse.down();
  assert(await page.locator('#panel .held .keyface').count());
  await page.selectOption('#device-color', schemes[(schemes.indexOf(scheme) + 1) % schemes.length]);
  assert(await page.locator('#panel .held .keyface').count());
  await page.mouse.up(); await page.locator('#device-color').blur();
  await page.keyboard.down('KeyQ'); assert(await page.locator('#panel .held .keyface').count());
  await page.keyboard.up('KeyQ');
  const box = await key.boundingBox();
  await touch.send('Input.dispatchTouchEvent', {type: 'touchStart', touchPoints: [{x: box.x + box.width / 2, y: box.y + box.height / 2}]});
  assert(await page.locator('#panel .held .keyface').count(), scheme + ' touch hold');
  await page.waitForTimeout(70);
  await touch.send('Input.dispatchTouchEvent', {type: 'touchEnd', touchPoints: []});
  assert.equal(await page.locator('#panel .held .keyface').count(), 0);
  await page.selectOption('#device-color', scheme);
  const modifier = page.locator('.hit[aria-label="ENV"]');
  await modifier.click({button: 'right'});
  assert(await modifier.evaluate(e => e.classList.contains('latched')));
  await page.selectOption('#device-color', schemes[(schemes.indexOf(scheme) + 1) % schemes.length]);
  assert(await modifier.evaluate(e => e.classList.contains('latched')));
  await modifier.click();
  assert.equal(await modifier.evaluate(e => e.classList.contains('latched')), false);
}
assert(Math.max(...peaks) > 0.01, 'measured browser audio must be nonzero');
// The native selector's keyboard events must not reach emulator input.
await page.locator('#device-color').focus();
const messages = (await audio()).messages;
await page.keyboard.press('ArrowUp'); await page.keyboard.press('KeyQ');
assert.equal((await audio()).messages, messages);
await page.locator('#device-color').blur();
await page.keyboard.press('Space', {delay: 70}); await page.keyboard.press('Escape', {delay: 70});
await wheel('ALGORITHM', 2);
for (const voice of ['303a', '303b']) {
  await tap('EDIT'); await wheel('SELECT', 2); // enter EDIT and clamp to EXTRA
  // Compare EXTRA tag to P7.2's real-WASM RGB565 reference, independent of panel palette.
  const rgb565 = fs.readFileSync(`build/host/waveform-ui/${voice}-extra-wasm.rgb565`);
  const expected = [...rgb565.subarray(158 * 480, 229 * 480)];
  const actual = await page.evaluate(() => {
    const rgba = document.getElementById('screen').getContext('2d').getImageData(0, 158, 240, 71).data;
    const bytes = [];
    for (let i = 0; i < rgba.length; i += 4) {
      const c = ((rgba[i] >> 3) << 11) | ((rgba[i + 1] >> 2) << 5) | (rgba[i + 2] >> 3);
      bytes.push(c >> 8, c & 255);
    }
    return bytes;
  });
  await page.screenshot({path: `${out}/${voice}-extra.png`});
  fs.writeFileSync(`${out}/${voice}-extra-actual.rgb565`, Buffer.from(actual));
  assert.deepEqual(actual, expected, voice + ' EXTRA labels and values');
  const extraLCD = await page.locator('#screen').evaluate(c => c.toDataURL());
  for (const scheme of schemes) {
    await page.selectOption('#device-color', scheme);
    assert.equal(await page.locator('#screen').evaluate(c => c.toDataURL()), extraLCD, scheme + ' ' + voice + ' EXTRA unaffected');
    await page.screenshot({path: `${out}/${scheme}-${voice}-extra.png`});
  }
  await tap('EDIT'); // TONE for next voice
  if (voice === '303a') await wheel('ALGORITHM', 1);
}
// Exercise Remix and both Grids modes through real panel input in every exterior.
await wheel('ALGORITHM', -3); // 303B -> 909
for (const scheme of schemes) {
  await page.selectOption('#device-color', scheme);
  await tap('ARP'); await wheel('KNOB4', -1); await page.waitForTimeout(1200);
  const before = await page.locator('#screen').evaluate(c => c.toDataURL());
  await tap('OCT+');
  assert.notEqual(await page.locator('#screen').evaluate(c => c.toDataURL()), before, 'Grids NEW renders');
  await page.screenshot({path: `${out}/${scheme}-grids-map.png`});
  await wheel('KNOB4', 1); await tap('OCT+'); await tap('OCT-');
  await page.screenshot({path: `${out}/${scheme}-grids-euclid.png`});
  await page.locator('.hit[aria-label="HOME"]').click({button: 'right'});
  await tap('REC'); await tap('PLAY'); // Undo / Redo while HOME is latched
  await page.locator('.hit[aria-label="HOME"]').click();
  await tap('SEQ'); await wheel('SELECT', 8); // clamp to REMIX
  await wheel('KNOB1', 1); await wheel('KNOB2', 1); await wheel('KNOB3', 1);
  await page.waitForTimeout(1200);
  await page.screenshot({path: `${out}/${scheme}-remix.png`});
  await tap('HOME');
}
// LCD SCHEME changes must not alter the exterior palette.
const exterior = await page.locator('#device').getAttribute('style');
await tap('GLO'); await wheel('SELECT', 11); await wheel('ALGORITHM', 1);
assert.equal(await page.locator('#device').getAttribute('style'), exterior);
await tap('HOME'); await page.waitForTimeout(1200);
const lcd = await page.locator('#screen').evaluate(c => c.toDataURL());
for (const scheme of schemes) {
  await page.selectOption('#device-color', scheme);
  assert.equal(await page.locator('#screen').evaluate(c => c.toDataURL()), lcd, 'exterior must not recolor LCD');
}
// Visual storage is isolated and survives reload; unknown/unavailable storage falls back safely.
for (const scheme of schemes) {
  await page.selectOption('#device-color', scheme);
  await page.reload(); assert.equal(await page.locator('#device-color').inputValue(), scheme);
}
await page.evaluate(() => localStorage.setItem('x0x-emu-device-color', '__proto__'));
await page.reload(); assert.equal(await page.locator('#device-color').inputValue(), 'cream');
await page.addInitScript(() => { Storage.prototype.getItem = Storage.prototype.setItem = () => { throw Error('storage disabled'); }; });
await page.reload(); await page.selectOption('#device-color', 'dark');
assert.equal(await page.locator('#device-color').inputValue(), 'dark');
// Responsive targets still track the original SVG positions at mobile widths.
const responsiveGeometry = (await snapshot(page)).targets;
await page.setViewportSize({width: 390, height: 844});
assert.deepEqual((await snapshot(page)).targets, responsiveGeometry);
assert(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth));
for (const scheme of schemes) {
  await page.selectOption('#device-color', scheme);
  assert.deepEqual((await snapshot(page)).targets, responsiveGeometry);
  await page.screenshot({path: `${out}/${scheme}-mobile.png`});
}
const favicon = await context.newPage();
await favicon.goto(root + 'emu/favicon.svg');
assert.equal(await favicon.locator('svg').count(), 1);
await favicon.goto(root, {waitUntil: 'domcontentloaded'});
await favicon.setContent(`<body style="background:#ddd"><img src="${root}emu/favicon.svg" width="16" height="16"><img src="${root}emu/favicon.svg" width="32" height="32"><img src="${root}emu/favicon.svg" width="64" height="64"></body>`);
await favicon.waitForFunction(() => [...document.images].every(img => img.complete && img.naturalWidth > 0));
await favicon.screenshot({path: `${out}/favicon.png`});
await favicon.goto(root, {waitUntil: 'domcontentloaded'});
assert.equal(await favicon.locator('link[rel="icon"]').getAttribute('href'), 'emu/favicon.svg');
await favicon.goto(root + 'install/');
assert.equal(await favicon.locator('script, button, input').count(), 0);
assert.equal((await context.request.get(root + 'firmware/')).status(), 404);
assert.deepEqual(errors, []);
fs.writeFileSync(`${out}/results.json`, JSON.stringify({root, peak: Math.max(...peaks), checks: 'four palettes, preserved cream/dark, geometry, mouse/touch/keyboard/latched, audio continuity, EXTRA both voices, Remix/Grids, independent LCD/exterior, storage, mobile, favicon, release gate', errors}, null, 2));
await browser.close();
console.log('Device palettes Chromium: all checks passed; audio peak', Math.max(...peaks));
