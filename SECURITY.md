# Security Policy

## Reporting a Vulnerability

**Please do not open public GitHub issues for security vulnerabilities.**

Report them privately to **tech@credithub.com.br**, or through GitHub's
[private vulnerability reporting](https://github.com/iCheques/jurischain/security/advisories/new).

Include, as far as you can: affected component (C core, browser bundle, Node
addon, or PHP extension), version, a description of the impact, and steps to
reproduce. We aim to acknowledge reports within 5 business days and to agree on a
disclosure timeline with you. Please give us reasonable time to release a fix
before any public disclosure.

## Supported Versions

| Component | Package | Supported |
|---|---|---|
| Browser bundle | `@credithub/jurischain` | `1.1.x` |
| Native addon | `@credithub/jurischain-node` | `1.0.x` |
| PHP extension | `credithub/jurischain` | `1.1.x` |
| C core | `include/jurischain.h` | latest `master` |

Only the latest minor line receives security fixes.

## Security Model

JurisChain is a **proof-of-work (PoW) gate**, not an authentication or identity
system. A client must compute a SHA3-256 puzzle of tunable difficulty before the
server accepts its request; the server verifies the answer with a single hash.

**What it provides.** It makes automated, high-volume abuse *expensive*. The
expected work to find a solution is about `2^difficulty` hashes, while
verification is O(1). Used correctly, it raises the cost of scraping,
credential-stuffing, and request floods.

**What it does not provide.** It is not a human/bot oracle (a determined
attacker with CPU/GPU budget can solve puzzles), it does not authenticate users,
and — by itself — it does not prevent replay. These properties depend on how the
**operator** integrates it.

## Operator Responsibilities

The library is deliberately stateless. The following are **your** responsibility
and are required for the gate to be effective:

1. **Unique, cryptographically random seed per request.** Generate the seed
   server-side with a CSPRNG (e.g. `random_bytes`, `crypto.randomBytes`). Never
   derive it from predictable values such as timestamps or counters. The demo in
   `examples/web/` uses a timestamp-based seed *for demonstration only* — do not
   copy that into production.
2. **Keep the difficulty server-authoritative.** Choose and store the difficulty
   on the server. Verify with your stored value — never with a difficulty echoed
   back by the client. A client that controls the difficulty can set it to `1`
   and bypass the work.
3. **Single-use / anti-replay.** A valid solution for a given seed is reusable
   forever. Bind each seed to a session or request, mark it consumed on first
   successful verification, and reject reuse. Expire unsolved challenges.
4. **Tune difficulty per endpoint.** Use higher difficulty for sensitive or
   expensive operations, lower for cheap reads. See the difficulty guidelines in
   the [README](README.md).
5. **Verify only on the server.** Never trust a client-reported success.

## Implementation Notes

- The bindings (PHP, Node) and the CLI validate `1 ≤ difficulty ≤ 255` and
  reject empty seeds. The C core itself now also treats `difficulty == 0` as
  invalid — `jurischain_gen` refuses to build such a challenge and
  `jurischain_verify` returns `0` for it, closing the vacuous-proof footgun
  where a zero-difficulty puzzle was satisfied by any response. You should
  still keep difficulty server-authoritative (see Operator Responsibilities).
- SHA3-256 is used as a black box and is verified against NIST known-answer
  vectors in the test suite; the work factor has no known shortcut.
- The verifier handles only attacker-supplied values that are already public
  (the seed is sent to the client), so it carries no secret-dependent timing
  side channel.
