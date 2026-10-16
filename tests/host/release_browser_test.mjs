// SPDX-License-Identifier: GPL-3.0-only
// Read-only local/public release verification: never requests MIDI access or writes to a device.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import {createRequire} from 'node:module';
const require = createRequire(import.meta.url);
const {chromium} = require(process.env.X0X_PLAYWRIGHT || 'playwright');
const base = process.argv[2].replace(/\/?$/, '/'), out = process.argv[3];
const pkgHash = 'eccacfa995efec6b9c819583e7bce43b3aa7a126033c8e668ae89c6a988b754c';
const wasmHash = '1e11309b6e47bd1081cc1af798ee31a99af37e67f9f60d0a8747b17ebf43e030';
const hash = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
fs.mkdirSync(out, {recursive: true});
const browser = await chromium.launch({executablePath: process.env.X0X_CHROMIUM,
  args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required']});
try {
  const context = await browser.newContext({viewport: {width: 1280, height: 1000}});
  await context.addInitScript(() => {
    const AC = window.AudioContext, AWN = window.AudioWorkletNode;
    window.releaseAudio = {};
    window.AudioContext = class extends AC {
      constructor(...a) { super(...a); window.releaseAudio.context = this; }
    };
    window.AudioWorkletNode = class extends AWN {
      constructor(ac, ...a) {
        super(ac, ...a);
        const meter = ac.createAnalyser(); meter.fftSize = 2048;
        this.connect(meter); this.connect = destination => meter.connect(destination);
        window.releaseAudio.meter = meter;
      }
    };
  });
  const page = await context.newPage(), errors = [], report = {base};
  page.on('pageerror', e => errors.push(e.message));
  page.on('response', r => {
    if (r.url().startsWith(base) && r.status() >= 400) errors.push(`${r.status()} ${r.url()}`);
  });
  const get = async path => {
    const response = await context.request.get(base + path);
    assert.equal(response.status(), 200, path);
    return await response.body();
  };
  await page.goto(base, {waitUntil: 'domcontentloaded'});
  const body = await page.locator('body').innerText();
  assert(body.includes('1.0.0 Stable') && body.includes('real M-VAVE FM-1'));
  assert(body.includes('browser-emulator') && body.includes('outro was updated'));
  assert(!/hardware validation (?:is )?pending|firmware installation is reserved|not yet tested/i.test(body));
  assert.equal(await page.locator('a[href="firmware/x0x-1.0.0.fwsc"]').count(), 1);
  await page.screenshot({path: `${out}/landing.png`, fullPage: true});
  const video = page.locator('video.performance-video');
  await video.scrollIntoViewIfNeeded();
  await page.waitForFunction(() => document.querySelector('video').readyState >= 1);
  assert.equal(await video.evaluate(v => v.duration), 84);
  await video.evaluate(v => new Promise(resolve => {
    v.addEventListener('seeked', resolve, {once: true}); v.currentTime = 83.9;
  }));
  await page.screenshot({path: `${out}/corrected-outro.png`});
  assert.equal(hash(await get('media/video/x0x-rmx-demo.mp4')),
    '0bd9721ce02a5f842c1504e3f450dc09fe4f02a796d2866bb5e690296abe43e2');
  assert.equal(hash(await get('media/video/x0x-rmx-demo-poster.png')),
    'e66e897da7736ccfd02d77e20094275324e56e3ce775ca0af2375e24411d2811');
  const pkg = await get('firmware/x0x-1.0.0.fwsc');
  assert.equal(pkg.length, 609680); assert.equal(hash(pkg), pkgHash);
  const checksum = (await get('firmware/x0x-1.0.0.fwsc.sha256')).toString().trim();
  assert.equal(checksum, `${pkgHash}  x0x-1.0.0.fwsc`);
  assert.equal(hash(await get('emu/x0x.wasm')), wasmHash);
  const info = JSON.parse(await get('emu/build-info.json'));
  assert.equal(info.version, '1.0.0'); assert.equal(info.wasm_sha256, wasmHash);
  assert.equal(info.revision, '4694051a499aeaceb84692a0d195463029bf94b0');
  await page.goto(base + 'install/');
  await page.waitForFunction(() => !document.getElementById('go').disabled);
  assert.equal(await page.locator('#status').innerText(), 'X0X 1.0.0');
  const installerSource = (await get('install/')).toString();
  const meta = JSON.parse(installerSource.match(/const meta = (\{[^\n]+\});/)[1]);
  assert.deepEqual(meta, {version: '1.0.0', product: 'FM-1_9010000', pkg: '../firmware/x0x-1.0.0.fwsc'});
  const installer = await page.locator('body').innerText();
  assert(!/beta|untested|validation pending/i.test(installer));
  assert(installer.includes('successfully tested on a real M-VAVE FM-1'));
  assert.equal(await page.locator('html').getAttribute('lang'), 'en');
  await page.screenshot({path: `${out}/installer-ready.png`, fullPage: true});
  for (const path of ['manual/', 'new/', 'breaks/']) {
    await page.goto(base + path);
    const text = await page.locator('body').innerText();
    assert(!/hardware validation (?:is )?pending|firmware installation is reserved|no validated X0X-RMX package/i.test(text), path);
  }
  await page.goto(base + 'emu/');
  await page.locator('#go').click();
  await page.waitForFunction(() => document.getElementById('start').hidden && window.x0xLast);
  const tap = async name => {
    await page.locator(`.hit[aria-label="${name}"]`).click({delay: 70});
    await page.waitForTimeout(100);
  };
  await tap('white key 1'); await tap('PLAY');
  let peak = 0;
  for (let i = 0; i < 16; i++) {
    peak = Math.max(peak, await page.evaluate(() => {
      const a = new Float32Array(2048); releaseAudio.meter.getFloatTimeDomainData(a);
      return Math.max(...a.map(Math.abs));
    }));
    await page.waitForTimeout(150);
  }
  assert(peak > 0.03, 'real emulator audio');
  await tap('PLAY'); await tap('SEQ');
  const select = page.locator('.hit[aria-label="SELECT"]'); await select.focus(); await select.press('ArrowUp');
  await page.waitForTimeout(1600);
  await page.screenshot({path: `${out}/emulator-remix.png`});
  assert.deepEqual(errors, []);
  Object.assign(report, {version: '1.0.0', product: meta.product, packageBytes: pkg.length,
    packageSha256: pkgHash, wasmSha256: wasmHash, emulatorCoreRevision: info.revision,
    installerReady: true, midiAccessRequested: false, audioPeak: peak, errors});
  fs.writeFileSync(`${out}/results.json`, JSON.stringify(report, null, 2) + '\n');
  console.log('Stable release browser checks passed: package/checksum, unchanged WASM, English installer ready, current hardware status, routes and audible emulator');
} finally { await browser.close(); }
