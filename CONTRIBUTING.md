# Contributing to JurisChain

Thanks for your interest in improving JurisChain! This guide covers how to set
up, test, and submit changes.

## Ground rules

- Be respectful and constructive in issues, reviews, and discussion.
- For security issues, **do not** open a public issue. Follow
  [SECURITY.md](SECURITY.md).
- The proof-of-work output is a compatibility contract. A change that alters the
  hash produced for a given `(seed, difficulty)` is a **breaking change** and
  must be called out explicitly — solutions made by older clients must keep
  verifying.

## Project layout

```
include/        Header-only C core (the source of truth for the algorithm)
src/            CLI solver + Emscripten browser entry
bindings/
  browser/      JS Promise wrapper + bundle API
  node/         Node.js native addon (NAN + node-gyp)
  php/          PHP 8 extension (phpize / PIE)
tests/          Native C, Node, PHP, Python, browser and E2E tests
docker/         Dockerfiles + nginx config
```

See the [README](README.md#project-structure) for the full tree.

## Development setup

You only need the toolchains for the parts you touch.

### C core / CLI

```bash
make cli                 # build the native solver
make test-c              # build + run the core test under UBSan + ASan
```

`make test-c` is the fastest way to validate a change to `include/jurischain.h`.
It checks SHA3-256 against NIST vectors, the solve/verify roundtrip, the
difficulty boundaries (64/128/192), tamper rejection, and NULL safety.

### Browser bundle (Emscripten)

```bash
make all                 # build ASM.js/WASM + the UMD bundle
npm install
npx playwright test --project=browser
```

### Node addon

```bash
cd bindings/node && npm install
node ../../tests/test_node.js
```

### PHP extension

```bash
cd bindings/php
phpize && ./configure && make
php -dextension=modules/jurischain.so ../../tests/test_php.php
```

### Everything, via Docker

```bash
./build.sh test          # PHP + Node + Python + ASM.js in containers
```

## Making a change

1. Fork and create a feature branch.
2. Make your change. Match the surrounding style (the C core targets portable,
   warning-free C compiled with `-Wall -Wextra`).
3. Add or update tests. New behaviour needs coverage; bug fixes need a
   regression test.
4. Run the relevant suites above. For C changes, run `make test-c` (sanitized).
5. Update [CHANGELOG.md](CHANGELOG.md) under `## [Unreleased]`.
6. Open a pull request describing the change and how you tested it.

## Pull request checklist

- [ ] Tests pass for the components you touched.
- [ ] New/changed behaviour is covered by tests.
- [ ] `CHANGELOG.md` updated.
- [ ] No change to PoW output, or it is clearly flagged as breaking.
- [ ] No secrets, tokens, or generated artifacts committed.

## Releasing (maintainers)

Publishing is automated by [`release.yml`](.github/workflows/release.yml):

1. Bump the version in `package.json` (and `bindings/node/package.json` if the
   addon changed; keep `include/jurischain.h` and `php_jurischain.h` in sync).
2. Record the release in `CHANGELOG.md`.
3. Merge to `master`, then tag: `git tag vX.Y.Z && git push origin vX.Y.Z`.
4. CI builds, tests, publishes to npm, and creates a GitHub Release.

Requires the `NPM_TOKEN` repository secret (an npm **automation** token with
publish rights to the `@credithub` scope).
