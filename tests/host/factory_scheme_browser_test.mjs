// SPDX-License-Identifier: GPL-3.0-only
// Real Chromium startup, UI SAVE/reset and browser-stored historical project fixtures.
// X0X_PLAYWRIGHT / X0X_CHROMIUM select the existing local browser runtime.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {createRequire} from 'node:module';
const require = createRequire(import.meta.url);
const {chromium} = require(process.env.X0X_PLAYWRIGHT || 'playwright');
const root = process.argv[2].replace(/\/?$/, '/');
const out = process.argv[3];
fs.mkdirSync(out, {recursive: true});
const browser = await chromium.launch({executablePath: process.env.X0X_CHROMIUM,
  headless: true, args: ['--no-sandbox']});
try {
  const page = await browser.newPage(), errors = [];
  page.on('pageerror', e => errors.push(e.message));
  const tap = async name => {
    await page.locator(`.hit[aria-label="${name}"]`).click({delay: 65});
    await page.waitForTimeout(100);
  };
  const turn = async (name, n) => {
    const knob = page.locator(`.hit[aria-label="${name}"]`);
    await knob.focus();
    for (let i = 0; i < Math.abs(n); i++) {
      await knob.press(n > 0 ? 'ArrowUp' : 'ArrowDown');
      await page.waitForTimeout(80);
    }
    await knob.blur();
    await page.waitForTimeout(100);
  };
  const boot = async () => {
    await page.locator('#go').click();
    await page.waitForFunction(() => window.x0xLast);
    await page.waitForTimeout(150);
  };
  const savedTag = () => page.evaluate(() =>
    atob(JSON.parse(localStorage.getItem('x0x-emu-store'))['0']).charCodeAt(21));
  const rgb565 = (r, g, b) => [Math.round((r >> 3) * 255 / 31),
    Math.round((g >> 2) * 255 / 63), Math.round((b >> 3) * 255 / 31), 255];
  const chip = () => page.locator('#screen').evaluate(c =>
    Array.from(c.getContext('2d').getImageData(2, 9, 1, 1).data));
  await page.goto(root + 'emu/');
  await boot();
  assert.deepEqual(await chip(), rgb565(187, 143, 180));
  await tap('SAVE');
  assert.equal(await savedTag(), 0xd1);
  await page.reload(); await boot();
  assert.deepEqual(await chip(), rgb565(187, 143, 180));
  // A previously saved non-factory choice survives a new browser session.
  await tap('GLO'); await turn('SELECT', 11); await turn('ALGORITHM', 4);
  await tap('SAVE'); assert.equal(await savedTag(), 0xd5);
  await page.reload(); await boot();
  assert.equal(await savedTag(), 0xd5);
  assert.deepEqual(await chip(), rgb565(240, 224, 184));
  // Confirm the firmware's GLO reset and save/reload the new factory selection.
  await tap('GLO'); await turn('SELECT', 15); await tap('SEL'); await tap('SEL');
  await tap('SAVE'); assert.equal(await savedTag(), 0xd1);
  await page.reload(); await boot();
  assert.deepEqual(await chip(), rgb565(187, 143, 180));
  // Browser reset clears project storage while preserving independent appearance.
  await page.selectOption('#device-color', 'dark');
  await page.locator('#reset').click();
  await Promise.all([page.waitForEvent('load'), page.locator('#reset').click()]);
  assert.equal(await page.locator('#device-color').inputValue(), 'dark');
  await boot(); await tap('SAVE'); assert.equal(await savedTag(), 0xd1);
  const project = await page.evaluate(() => localStorage.getItem('x0x-emu-store'));
  // Project fixtures use the real storage loader, with no renderer or WASM patching.
  for (const [label, byte, short, expected] of [
    ['saved-default', 0xd0, false, 0xd0], ['untagged', 7, false, 7],
    ['invalid-id', 0xd6, false, 0xd6], ['short-settings', 0xd1, true, 0xd0],
  ]) {
    await page.evaluate(({project, byte, short}) => {
      const raw = JSON.parse(project), bytes = Uint8Array.from(atob(raw['0']), c => c.charCodeAt(0));
      bytes[12] = 4; // retained legacy MONO palette
      bytes[21] = byte;
      raw['0'] = btoa(String.fromCharCode(...(short ? bytes.subarray(0, 20) : bytes)));
      localStorage.setItem('x0x-emu-store', JSON.stringify(raw));
    }, {project, byte, short});
    await page.reload(); await boot();
    assert.deepEqual(await chip(), rgb565(255, 150, 40), label);
    await tap('SAVE'); assert.equal(await savedTag(), expected, label);
    await page.screenshot({path: `${out}/${label}.png`});
  }
  assert.deepEqual(errors, []);
  fs.writeFileSync(`${out}/results.json`, JSON.stringify({root,
    checks: 'WHITE LILAC fresh startup/save/reload; retained WHITE BLUE; firmware and browser factory reset; independent exterior; saved DEFAULT, untagged, invalid and short historical settings', errors}, null, 2));
  console.log('Factory scheme Chromium: all startup, persistence, reset and legacy checks passed');
} finally {
  await browser.close();
}
