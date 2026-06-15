# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Versions refer to the npm package `@credithub/jurischain` (browser bundle) and the
header-only C library, unless noted. The native addon `@credithub/jurischain-node`
and the PHP extension track their own version numbers, called out where relevant.

## [Unreleased]

### Added
- PHP extension packaging for [PIE](https://github.com/php/pie), the PHP
  Foundation's official extension installer — `pie install credithub/jurischain`.
- Root `composer.json` (`type: php-ext`, `build-path: bindings/php`) for
  Packagist distribution.
- CI job that validates `composer.json` and builds the extension with the real
  `pie.phar`.
- Project documentation: `CHANGELOG.md`, `HISTORY.md`, `SECURITY.md`, and
  `CONTRIBUTING.md`.

### Changed
- `bindings/php/config.m4` now defaults the extension to enabled and resolves the
  core header via `PHP_ADD_INCLUDE(../../include)`, so a plain
  `phpize && ./configure && make` builds from a checkout with no manual flags.

### Security
- **Core:** closed the vacuous-proof footgun — `jurischain_verify` now returns
  `0` for `difficulty == 0` (previously any response satisfied a zero-difficulty
  puzzle), and `jurischain_gen` refuses to build such a challenge.
- **CLI:** clamp the requested difficulty into `1..255` before narrowing to
  `uint8_t`. Previously `256` wrapped to `0` (and `257` to `1`); combined with
  the `d == 0` fix that wrapped value would have looped forever.
- **Demo:** `examples/web/index.html` now derives its seed from
  `crypto.getRandomValues` instead of a timestamp, with a comment stating that
  production seeds must be generated server-side with a CSPRNG.

### Fixed
- Added a sanitized regression test (`tests/test_c.c`) covering the
  zero-difficulty rejection.

## [1.1.3] - 2026-06-10

### Fixed
- **Core (security/UB):** shift-by-64 undefined behaviour in `jurischain_verify`
  when the difficulty is a multiple of 64 (64/128/192). Confirmed with UBSan;
  now guarded.
- **Core (UB):** strict-aliasing / unaligned read — the digest is now copied into
  64-bit words with `memcpy` instead of a raw `(uint64_t *)` cast over a byte
  buffer.
- **Node binding:** the committed `jurischain.h` symlink pointed at a
  non-existent path, so fresh clones could not build the addon. The header is now
  resolved through `binding.gyp` include paths, with a `prepack` step that copies
  a real header into the published tarball.
- **PHP test:** the `.phpt` test used a procedural API that no longer exists;
  rewritten against the actual OOP class API.
- `scripts/genstats.py` invoked the wrong binary name and passed a raw bytes
  repr as the seed.

### Changed
- Core library is now genuinely header-only: all functions have internal linkage
  (`static inline`), so the header can be included from multiple translation
  units without duplicate-symbol errors.
- `Makefile` drops the no-op floating-point fast-math flags and the
  non-portable `-march=native` from the default integer-only build; adds
  `-Wall -Wextra`, a sanitized `test-c` target, and `.PHONY`.
- `requirements.txt` drops the deprecated `pysha3` (its functionality is in
  `hashlib` since Python 3.6).
- Version sync: core header and PHP extension bumped to `1.1.3`; native addon
  `@credithub/jurischain-node` bumped to `1.0.2`.

### Added
- `tests/test_c.c`: direct core coverage (SHA3-256 NIST vectors, solve/verify
  roundtrip, the 64/128/192 difficulty boundaries, tamper rejection, NULL
  safety), run under UBSan + ASan.
- Fast native CI jobs (sanitized C core, PHP extension + phpt, Node addon,
  Python smoke) alongside the existing Emscripten and Docker jobs.
- `release.yml`: automated npm publish for both packages on `v*` tags, with a
  generated GitHub Release.

### Security
- Output of the proof-of-work is byte-for-byte identical to `1.1.2`; verified
  against pre-refactor reference solutions, Python `hashlib` SHA3-256 vectors,
  and cross-checked between the PHP and Node bindings.

## [1.1.0] - 2026-04-11

The "@credithub" line: rebrand and full modernization of the original BIPBOP
project. Released incrementally through `1.1.1` and `1.1.2`.

### Added
- Promise-based browser API (`solveJurischain`) compiled to ASM.js / WebAssembly
  via Emscripten, shipped as a single UMD bundle.
- Complete TypeScript type definitions (global, module, `Window`,
  `CustomEvent`).
- End-to-end browser test suite (Playwright) and Dockerized build/test pipeline.
- MIT license.

### Changed
- ES5 output and Babel (loose mode) transpilation for legacy-browser and
  Browserify compatibility; Terser minification wired into the publish pipeline.

### Fixed
- Serialized solver calls to prevent a race condition when multiple challenges
  are solved concurrently.
- `window.jurischain` initialization handling in the browser entry point.

## [1.0.0] - 2019-09-15

Initial public release as `bipbop/jurischain`.

### Added
- Header-only SHA3-256 (Keccak) proof-of-work core in C.
- Native CLI solver.
- First Node.js interface and PHP extension scaffolding.
- Bilingual (English / Portuguese) documentation, inspired by the Brazilian
  judiciary's need for open, self-hosted access control under
  [Lei 11.419, Art. 14](http://www.planalto.gov.br/ccivil_03/_Ato2004-2006/2006/Lei/L11419.htm#art14).

[Unreleased]: https://github.com/iCheques/jurischain/compare/v1.1.3...HEAD
[1.1.3]: https://github.com/iCheques/jurischain/compare/v1.1.0...v1.1.3
[1.1.0]: https://github.com/iCheques/jurischain/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/iCheques/jurischain/releases/tag/v1.0.0
