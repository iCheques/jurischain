const { test, expect } = require('@playwright/test');
const { execFileSync } = require('child_process');
const http = require('http');
const path = require('path');
const fs = require('fs');
const os = require('os');

const ROOT = path.resolve(__dirname, '../..');

// ─── Helper: run PHP inside the test-php Docker container ───
function dockerPhp(script, args = []) {
  const out = execFileSync(
    'docker',
    [
      'compose', '--profile', 'test',
      'run', '--rm', '-T', 'test-php',
      'php', `/src/tests/e2e/${script}`, ...args,
    ],
    { cwd: ROOT, timeout: 60000 }
  ).toString().trim();
  return JSON.parse(out);
}

// ═══════════════════════════════════════════════════════════════
// TEST 1: PHP generates challenge → Browser solves → PHP verifies
// ═══════════════════════════════════════════════════════════════
test.describe('E2E: PHP ↔ Browser roundtrip', () => {
  test('PHP generate → JS solve → PHP verify', async ({ page }) => {
    // 1) PHP generates challenge
    const challenge = dockerPhp('php_generate.php', ['2', 'E2E-TestSeed-2025']);
    console.log('PHP generated:', challenge);
    expect(challenge.seed).toBe('E2E-TestSeed-2025');
    expect(challenge.difficulty).toBe(2);
    expect(challenge.challenge).toHaveLength(64);

    // 2) Browser solves
    await page.goto('/tests/e2e/e2e.html');
    await page.waitForFunction(
      () => document.getElementById('status')?.textContent === 'Ready',
      null, { timeout: 5000 }
    );

    const result = await page.evaluate(
      ({ seed, difficulty }) => window.solve(seed, difficulty, 10000),
      { seed: challenge.seed, difficulty: challenge.difficulty }
    );
    console.log('Browser solved:', result);
    expect(result.success).toBe(true);
    expect(result.result).toMatch(/^[A-Fa-f0-9]{64}$/);

    // 3) PHP verifies the browser's solution
    const verification = dockerPhp('php_verify.php', [
      String(challenge.difficulty),
      challenge.seed,
      result.result,
    ]);
    console.log('PHP verified:', verification);
    expect(verification.valid).toBe(true);
  });

  test('PHP rejects forged/invalid token', async () => {
    const fakeToken = 'AA'.repeat(32); // 64 hex chars, but wrong answer
    const verification = dockerPhp('php_verify.php', ['2', 'E2E-TestSeed-2025', fakeToken]);
    console.log('PHP rejected forged:', verification);
    expect(verification.valid).toBe(false);
  });

  test('PHP rejects token from wrong seed', async ({ page }) => {
    // Solve with seed A
    await page.goto('/tests/e2e/e2e.html');
    await page.waitForFunction(
      () => document.getElementById('status')?.textContent === 'Ready',
      null, { timeout: 5000 }
    );
    const r = await page.evaluate(() => window.solve('SeedA', 1, 5000));
    expect(r.success).toBe(true);

    // Verify with seed B — must fail
    const verification = dockerPhp('php_verify.php', ['1', 'SeedB', r.result]);
    console.log('PHP rejected wrong seed:', verification);
    expect(verification.valid).toBe(false);
  });

  test('PHP rejects token with wrong difficulty', async ({ page }) => {
    // Solve with difficulty 1
    await page.goto('/tests/e2e/e2e.html');
    await page.waitForFunction(
      () => document.getElementById('status')?.textContent === 'Ready',
      null, { timeout: 5000 }
    );
    const r = await page.evaluate(() => window.solve('DiffTest', 1, 5000));
    expect(r.success).toBe(true);

    // Verify with difficulty 10 — must fail
    const verification = dockerPhp('php_verify.php', ['10', 'DiffTest', r.result]);
    console.log('PHP rejected wrong difficulty:', verification);
    expect(verification.valid).toBe(false);
  });
});

// ═══════════════════════════════════════════════════════════════
// TEST 2: Fresh npm package install → browser loads → solves
// ═══════════════════════════════════════════════════════════════
test.describe('E2E: Fresh npm package install', () => {
  let tmpDir;
  let tarball;

  test.beforeAll(async () => {
    // Pack the current project (skips prepublishOnly via --ignore-scripts)
    const packOut = execFileSync('npm', ['pack', '--ignore-scripts'], {
      cwd: ROOT,
      encoding: 'utf8',
    }).trim();
    const tgzName = packOut.split('\n').pop();
    tarball = path.join(ROOT, tgzName);
    expect(fs.existsSync(tarball)).toBe(true);

    // Create temp project and install the tarball
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jurischain-e2e-'));
    execFileSync('npm', ['init', '-y'], { cwd: tmpDir });
    execFileSync('npm', ['install', tarball], { cwd: tmpDir });

    // Create a test HTML that loads from node_modules
    const html = `<!DOCTYPE html>
<html><head><meta charset="UTF-8"></head>
<body>
  <div id="jurischain-captcha"></div>
  <div id="status">Loading...</div>
  <script src="/node_modules/@credithub/jurischain/dist/jurischain-bundle.js"></script>
  <script>
    window.solve = function(seed, difficulty, timeout) {
      return solveJurischain({ seed: seed, difficulty: difficulty, timeout: timeout })
        .then(function(r) { return { success: true, result: r }; })
        .catch(function(e) { return { success: false, error: e.message }; });
    };
    document.getElementById('status').textContent = 'Ready';
  </script>
</body></html>`;
    fs.writeFileSync(path.join(tmpDir, 'index.html'), html);
  });

  test.afterAll(async () => {
    // Cleanup
    if (tmpDir) fs.rmSync(tmpDir, { recursive: true, force: true });
    if (tarball) fs.rmSync(tarball, { force: true });
  });

  test('bundle from installed package solves challenge', async ({ page }) => {
    const mimeTypes = { '.html': 'text/html', '.js': 'application/javascript' };
    const rootReal = fs.realpathSync(tmpDir);
    const server = await new Promise((resolve) => {
      const s = http.createServer((req, res) => {
        const reqPath = decodeURIComponent((req.url || '/').split('?')[0]);
        const resolved = path.resolve(rootReal, '.' + reqPath);
        if (!resolved.startsWith(rootReal + path.sep) || !fs.existsSync(resolved)) {
          res.writeHead(404);
          res.end('Not Found');
          return;
        }
        const ext = path.extname(resolved);
        res.writeHead(200, { 'Content-Type': mimeTypes[ext] || 'application/octet-stream' });
        fs.createReadStream(resolved).pipe(res);
      });
      s.listen(0, '127.0.0.1', () => resolve(s));
    });
    const port = server.address().port;

    try {
      await page.goto(`http://127.0.0.1:${port}/index.html`);
      await page.waitForFunction(
        () => document.getElementById('status')?.textContent === 'Ready',
        null, { timeout: 5000 }
      );

      const r = await page.evaluate(() => window.solve('PackageTest', 1, 5000));
      console.log('Package test solved:', r);
      expect(r.success).toBe(true);
      expect(r.result).toMatch(/^[A-Fa-f0-9]{64}$/);
    } finally {
      server.close();
    }
  });
});
