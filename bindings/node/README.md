<div align="center">

# @credithub/jurischain-node

**Native Node.js binding for [JurisChain](https://github.com/credithub/jurischain) — a SHA3-256 Proof-of-Work CAPTCHA.**
Challenge the *terminals*, not the *humans*. No tracking, no third parties, no image grids.

[![npm](https://img.shields.io/npm/v/@credithub/jurischain-node?logo=npm&color=cb3837)](https://www.npmjs.com/package/@credithub/jurischain-node)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Node](https://img.shields.io/badge/node-%3E%3D14-339933?logo=node.js&logoColor=white)](package.json)

</div>

---

## TL;DR

```bash
npm install @credithub/jurischain-node
```

```js
const crypto = require('crypto');
const { Jurischain } = require('@credithub/jurischain-node');

// 1. Server issues a challenge: a random seed + a difficulty (1–255)
const seed = crypto.randomBytes(16).toString('hex');
const difficulty = 10;

// 2. Client burns CPU solving the Proof-of-Work
const client = new Jurischain(difficulty, seed);
while (!client.solveStep());          // ~2^difficulty SHA3-256 hashes
const solution = client.readChallenge();

// 3. Server verifies in O(1) — a single hash comparison
const server = new Jurischain(difficulty, seed);
server.challengeResponse(solution);
console.log(server.verify());         // → true
```

That's the whole API. Keep reading for the *why* and the details.

---

## Why?

Traditional captchas (reCAPTCHA, hCaptcha) track users, sell data, and depend on
external servers. JurisChain flips the model: instead of challenging *humans*, it
challenges the *machine*.

A client must solve a SHA3-256 Proof-of-Work puzzle before its request is accepted.
This makes automated abuse computationally expensive while keeping the experience
seamless — no image grids, no cookies, no surveillance. Verification on the server
is a single hash comparison, so it costs you almost nothing.

This package is the **native Node.js addon** (C++ via [NAN](https://github.com/nodejs/nan) +
[node-gyp](https://github.com/nodejs/node-gyp)) wrapping the same header-only C core
that powers the browser, PHP, and Python bindings — so the client and server speak
exactly the same protocol, bit for bit.

## Installation

```bash
npm install @credithub/jurischain-node
```

This is a **native addon**: it compiles from source on `npm install` via `node-gyp`.
You need a C++ toolchain and Python 3 on the build machine:

| Platform | Prerequisites |
|---|---|
| **Linux** | `build-essential` (gcc/g++, make) and `python3` |
| **macOS** | Xcode Command Line Tools (`xcode-select --install`) |
| **Windows** | [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/) + Python 3 |

Requires **Node.js ≥ 14**.

## How It Works

```
┌──────────┐       seed + difficulty       ┌──────────┐
│  Server   │ ───────────────────────────▸ │  Client   │
│           │                              │           │
│           │                              │  SHA3-256 │
│           │                              │  PoW loop │
│           │        solution hash         │     ⏳    │
│  verify() │ ◂─────────────────────────── │           │
└──────────┘                              └──────────┘
```

1. **Server** generates a random `seed` and picks a `difficulty` (1–255).
2. **Client** receives `{ seed, difficulty }` and iterates SHA3-256 until the
   result has `difficulty` leading zero bits.
3. **Client** sends the solution hash back.
4. **Server** verifies in O(1).

Higher difficulty = exponentially more work for the client, linearly tunable by
the server. See the [main project README](https://github.com/credithub/jurischain#readme)
for the full design rationale.

## API

The package exports a single class, `Jurischain`.

```js
const { Jurischain } = require('@credithub/jurischain-node');
// ESM / TypeScript:  import { Jurischain } from '@credithub/jurischain-node';
```

| Member | Returns | Description |
|---|---|---|
| `new Jurischain(difficulty, seed)` | instance | Create a challenge. `difficulty` is an integer **1–255**; `seed` is a **non-empty string**. Throws `TypeError` / `RangeError` on bad input. |
| `solveStep()` | `boolean` | Run **one** PoW iteration (a single SHA3-256 hash). Returns `true` once the challenge is solved. Call it in a loop on the client. |
| `readChallenge()` | `string` | The current state as a 64-character hex string. After the challenge is solved this is the **solution** to send to the server. |
| `challengeResponse(response)` | `boolean` | Load a client's 64-character hex solution into the context so it can be checked. Throws `RangeError` if the length isn't exactly 64, or `Error` on invalid hex. |
| `verify()` | `boolean` | `true` if the current state satisfies the difficulty. **Run this server-side.** |

> **Note:** `solveStep()` runs a tight, CPU-bound loop and blocks the event loop
> while it runs. For low difficulties this is microseconds; for high difficulties,
> solve inside a [Worker thread](https://nodejs.org/api/worker_threads.html) so you
> don't stall your server.

## Client / Server Example

A minimal [Express](https://expressjs.com/) flow: the server hands out challenges,
the client solves them, the server verifies.

```js
const crypto = require('crypto');
const express = require('express');
const { Jurischain } = require('@credithub/jurischain-node');

const app = express();
app.use(express.json());

const DIFFICULTY = 12;
const issued = new Map(); // seed -> expiry (use Redis/etc. in production)

// 1. Hand out a fresh, single-use challenge
app.get('/challenge', (req, res) => {
  const seed = crypto.randomBytes(16).toString('hex');
  issued.set(seed, Date.now() + 60_000); // valid for 60s
  res.json({ seed, difficulty: DIFFICULTY });
});

// 2. Verify the submitted solution
app.post('/submit', (req, res) => {
  const { seed, solution } = req.body;

  // Only accept seeds we issued and haven't seen redeemed (anti-replay)
  const expiry = issued.get(seed);
  if (!expiry || expiry < Date.now()) {
    return res.status(400).json({ ok: false, error: 'unknown or expired seed' });
  }
  issued.delete(seed);

  const verifier = new Jurischain(DIFFICULTY, seed);
  verifier.challengeResponse(solution);

  res.json({ ok: verifier.verify() });
});

app.listen(3000);
```

```js
// Client side (Node, browser, or anything that can hash):
const { seed, difficulty } = await (await fetch('/challenge')).json();

const c = new Jurischain(difficulty, seed);
while (!c.solveStep());            // grind the Proof-of-Work
const solution = c.readChallenge();

const { ok } = await (await fetch('/submit', {
  method: 'POST',
  headers: { 'content-type': 'application/json' },
  body: JSON.stringify({ seed, solution }),
})).json();
```

> In the browser, use [`@credithub/jurischain`](https://www.npmjs.com/package/@credithub/jurischain)
> (WASM/ASM.js) on the client instead of this native addon — both implement the
> identical protocol, so a browser-solved challenge verifies cleanly here.

## Difficulty Guidelines

`difficulty` is the number of leading zero **bits** required in the hash, so the
expected work grows as `2^difficulty`.

| Difficulty | Avg. tries | Typical time | Use case |
|---|---|---|---|
| 8–10 | ~500 | < 1s | Login forms, page views |
| 14–16 | ~30k | 2–5s | API rate limiting |
| 18–20 | ~200k | 10–30s | Heavy abuse prevention |

Tune per endpoint: low for reads, high for sensitive or expensive operations.

## Security Considerations

- The **seed must be cryptographically random** and unique per request — reusing
  seeds enables replay attacks. Track issued seeds and accept each only once.
- **Verify server-side only.** Never trust a client's claim that it solved the
  challenge; recompute with `verify()`.
- **Tune difficulty per endpoint** — higher for sensitive operations, lower for reads.
- SHA3-256 is **not reversible** — the server verifies by recomputing, never by
  storing solutions.

## Related Packages

| Package | Platform | Install |
|---|---|---|
| [`@credithub/jurischain`](https://www.npmjs.com/package/@credithub/jurischain) | Browser (WASM/ASM.js) | `npm install @credithub/jurischain` |
| **`@credithub/jurischain-node`** | Node.js (native addon) | `npm install @credithub/jurischain-node` |
| [`credithub/jurischain`](https://github.com/credithub/jurischain/tree/master/bindings/php) | PHP 8 extension | `pie install credithub/jurischain` |

## License

[MIT](LICENSE) — created by BIPBOP, maintained by
[Lucas Fernando Amorim](https://github.com/lfamorim) and
[CreditHub](https://credithub.com.br).
