const { test, expect } = require('@playwright/test');

const READY = () => document.getElementById('status')?.textContent === 'Ready';

test.describe('JurisChain Browser Bundle', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/tests/browser/test.html');
    await page.waitForFunction(READY, null, { timeout: 5000 });
  });

  test('solveJurischain is defined globally', async ({ page }) => {
    const t = await page.evaluate(() => typeof solveJurischain);
    expect(t).toBe('function');
  });

  test('solves difficulty 1 → 64-char hex', async ({ page }) => {
    const r = await page.evaluate(() => window.runTest('Seed1', 1, 5000));
    expect(r.success).toBe(true);
    expect(r.result).toMatch(/^[A-Fa-f0-9]{64}$/);
  });

  test('solves difficulty 2', async ({ page }) => {
    const r = await page.evaluate(() => window.runTest('Seed2', 2, 5000));
    expect(r.success).toBe(true);
    expect(r.result).toHaveLength(64);
  });

  test('different seeds → different hashes', async ({ page }) => {
    const a = await page.evaluate(() => window.runTest('AAA', 1, 5000));
    await page.goto('/tests/browser/test.html');
    await page.waitForFunction(READY, null, { timeout: 5000 });
    const b = await page.evaluate(() => window.runTest('BBB', 1, 5000));
    expect(a.success && b.success).toBe(true);
    expect(a.result).not.toBe(b.result);
  });

  test('rejects empty seed', async ({ page }) => {
    const r = await page.evaluate(() => window.runTest('', 1, 2000));
    expect(r.success).toBe(false);
    expect(r.error).toContain('seed');
  });

  test('rejects difficulty 0', async ({ page }) => {
    const r = await page.evaluate(() => window.runTest('X', 0, 2000));
    expect(r.success).toBe(false);
    expect(r.error).toContain('difficulty');
  });

  test('rejects difficulty 256', async ({ page }) => {
    const r = await page.evaluate(() => window.runTest('X', 256, 2000));
    expect(r.success).toBe(false);
    expect(r.error).toContain('difficulty');
  });

  test('timeout fires on high difficulty and subsequent solve still succeeds', async ({ page }) => {
    const timedOut = await page.evaluate(() => window.runTest('HighDiffSeed', 30, 100));
    expect(timedOut.success).toBe(false);
    expect(timedOut.error).toContain('timed out');

    const recovered = await page.evaluate(() => window.runTest('TestSeed', 10, 10000));
    expect(recovered.success).toBe(true);
    expect(recovered.result).toBe(
      'C19F75A5F960505DC5500675B0C6FD673B3047B040A17D47928599565E587065'
    );
  });

  test('rejects boolean/null/NaN/float difficulty and negative timeout', async ({ page }) => {
    for (const badDiff of [true, false, null, '', 1.5, -1]) {
      const r = await page.evaluate((d) => window.runTest('X', d, 2000), badDiff);
      expect(r.success).toBe(false);
      expect(r.error).toContain('difficulty');
    }
    const badTimeout = await page.evaluate(() => window.runTest('X', 1, -10));
    expect(badTimeout.success).toBe(false);
    expect(badTimeout.error).toContain('timeout');
  });

  test('serializes concurrent calls without race condition', async ({ page }) => {
    const [a, b] = await page.evaluate(() =>
      Promise.all([
        window.runTest('ConcurrentA', 1, 5000),
        window.runTest('ConcurrentB', 1, 5000),
      ])
    );
    expect(a.success && b.success).toBe(true);
    expect(a.result).toMatch(/^[A-Fa-f0-9]{64}$/);
    expect(b.result).toMatch(/^[A-Fa-f0-9]{64}$/);
    expect(a.result).not.toBe(b.result);
  });
});
