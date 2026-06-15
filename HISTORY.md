# History

The story of JurisChain, from a 2019 idea to a multi-platform proof-of-work
library. For the precise, version-by-version list of changes, see
[CHANGELOG.md](CHANGELOG.md); this document is the narrative behind it.

## The idea (2019)

JurisChain was created at **BIPBOP** out of a very concrete problem. Brazil's
[Lei 11.419, Art. 14](http://www.planalto.gov.br/ccivil_03/_Ato2004-2006/2006/Lei/L11419.htm#art14)
requires electronic judicial systems to be built on open standards and to remain
continuously available. Conventional captchas sit awkwardly against that mandate:
they depend on external, proprietary services, they track users, and they fail
closed when the third party is unreachable.

The insight was to flip the question. Instead of challenging the *humans* at the
terminals, challenge the *terminals themselves*. A client must spend real CPU
solving a SHA3-256 proof-of-work puzzle before its request is accepted; the
server verifies the answer in a single hash. Abuse becomes computationally
expensive while legitimate users see only a brief, self-hosted, dependency-free
check — no image grids, no cookies, no surveillance. The name nods both to the
judiciary (*juris*) and to the chained-hashing technique that inspired the
design.

The first release shipped the essentials: a header-only SHA3-256 core in C, a
native CLI solver, an early Node.js interface, the beginnings of a PHP
extension, and bilingual documentation.

## Quiet years (2020–2021)

The project saw incremental maintenance: moving the browser handshake to
`window.jurischain`, reworking how the seed and difficulty are passed in, an
initial GitHub Actions setup, and housekeeping around the build. The core
algorithm stayed stable — proof, in its own way, that the original design held
up.

## Rebrand and modernization (2026)

The library found a new home and steward at **[CreditHub](https://credithub.com.br)**,
maintained by [Lucas Fernando Amorim](https://github.com/lfamorim). The `1.1.x`
line, published as **`@credithub/jurischain`**, modernized everything around the
core without disturbing it:

- A Promise-based browser API compiled to ASM.js / WebAssembly with Emscripten,
  shipped as a single UMD bundle that works via `<script>`, `require`, or
  `import`.
- Complete TypeScript definitions.
- An end-to-end Playwright suite and a Dockerized build/test pipeline spanning C,
  Node, PHP, and Python.
- ES5 output, Terser minification, and a clean publishable package.

## Hardening (2026)

The `1.1.3` release turned a careful eye on the foundations. A security and
correctness pass fixed two instances of undefined behaviour in the verifier (a
shift-by-64 on difficulties that are multiples of 64, and a strict-aliasing read
of the digest), made the core a true header-only library with internal linkage,
and repaired build paths that only worked by accident inside Docker — most
notably a committed symlink that left fresh clones unable to build the Node
addon.

Crucially, the proof-of-work output did not change by a single bit: every fix
was validated against pre-refactor reference solutions, SHA3-256 NIST vectors,
and cross-checks between the PHP and Node bindings producing identical results
for the same input.

That release also gave the project the scaffolding of a mature library:
sanitizer-backed native tests for the C core, fast per-language CI jobs, an
automated npm release workflow on version tags, and distribution of the PHP
extension through [PIE](https://github.com/php/pie) and Packagist.

## Credits

Created by **BIPBOP**. Maintained by
[Lucas Fernando Amorim](https://github.com/lfamorim) and
[CreditHub](https://credithub.com.br). Released under the [MIT License](LICENSE).
