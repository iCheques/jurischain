/*
 * JurisChain core library — native C test.
 *
 * Exercises the header-only library directly (no language bindings):
 *   - SHA3-256 against NIST known-answer vectors
 *   - solve -> verify roundtrip across a range of difficulties
 *   - the difficulty boundaries that are multiples of 64 (64/128/192),
 *     which previously triggered a shift-by-64 undefined behaviour
 *   - tamper rejection (wrong seed / wrong difficulty)
 *
 * Build:  gcc tests/test_c.c -Iinclude -o test_c
 * Tip:    add -fsanitize=undefined,address to catch UB / memory bugs.
 */
#include <stdio.h>
#include <string.h>

#include "jurischain.h"

static int failures = 0;

static void check(int cond, const char *name) {
  printf("  %s %s\n", cond ? "[ OK ]" : "[FAIL]", name);
  if (!cond) failures++;
}

static void to_hex(const uint8_t *in, int len, char *out) {
  static const char *h = "0123456789abcdef";
  int i;
  for (i = 0; i < len; i++) {
    out[i * 2] = h[in[i] >> 4];
    out[i * 2 + 1] = h[in[i] & 0xF];
  }
  out[len * 2] = '\0';
}

static void test_sha3_vectors(void) {
  /* NIST SHA3-256 known-answer vectors. */
  struct { const char *in; const char *expect; } v[] = {
    {"", "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a"},
    {"abc", "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532"},
  };
  size_t i;
  printf("SHA3-256 known-answer vectors:\n");
  for (i = 0; i < sizeof(v) / sizeof(v[0]); i++) {
    uint8_t md[HASH_LEN];
    char hex[HASH_LEN * 2 + 1];
    sha3(v[i].in, strlen(v[i].in), md, HASH_LEN);
    to_hex(md, HASH_LEN, hex);
    check(strcmp(hex, v[i].expect) == 0, v[i].in[0] ? v[i].in : "(empty)");
  }
}

static void solve(jurischain_ctx_t *c) {
  size_t tries = 0;
  while (!jurischain_try(c)) {
    if (++tries > 5000000UL) break; /* safety valve */
  }
}

static void test_roundtrip(void) {
  uint8_t diffs[] = {1, 8, 16, 64, 128, 192};
  const char *seed = "JurisChainCTest";
  size_t i;
  printf("solve -> verify roundtrip:\n");
  for (i = 0; i < sizeof(diffs); i++) {
    char name[32];
    jurischain_ctx_t client, server;
    uint8_t d = diffs[i];

    /* High difficulties would take astronomically long to solve; for those
     * we only assert that an unsolved challenge does NOT verify. */
    jurischain_gen(&client, d, seed, strlen(seed));
    snprintf(name, sizeof(name), "difficulty %u", d);

    if (d <= 16) {
      solve(&client);
      /* transfer solution to a fresh server-side context */
      jurischain_gen(&server, d, seed, strlen(seed));
      memcpy(server.seed, client.seed, HASH_LEN);
      check(jurischain_verify(&client) && jurischain_verify(&server), name);
    } else {
      check(jurischain_verify(&client) == 0, name);
    }
  }
}

static void test_tamper(void) {
  const char *seed = "TamperSeed";
  jurischain_ctx_t client;
  jurischain_ctx_t wrong_seed, wrong_diff;
  uint8_t d = 8;

  printf("tamper rejection:\n");
  jurischain_gen(&client, d, seed, strlen(seed));
  solve(&client);

  /* Same solution, different seed -> must fail. */
  jurischain_gen(&wrong_seed, d, "OtherSeed", 9);
  memcpy(wrong_seed.seed, client.seed, HASH_LEN);
  check(jurischain_verify(&wrong_seed) == 0, "wrong seed rejected");

  /* Same solution, higher difficulty -> must (almost certainly) fail. */
  jurischain_gen(&wrong_diff, 24, seed, strlen(seed));
  memcpy(wrong_diff.seed, client.seed, HASH_LEN);
  check(jurischain_verify(&wrong_diff) == 0, "wrong difficulty rejected");
}

static void test_null_safety(void) {
  printf("NULL safety:\n");
  check(jurischain_verify(NULL) == 0, "verify(NULL) == 0");
  check(jurischain_try(NULL) == 0, "try(NULL) == 0");
}

static void test_zero_difficulty(void) {
  /* d == 0 is a vacuous proof: with no required zero bits, ANY response
   * trivially satisfies the check. The core must reject it rather than
   * accept arbitrary input. gen() must also refuse to build such a
   * challenge. */
  jurischain_ctx_t c;
  printf("zero-difficulty rejection:\n");

  /* gen() leaves the context untouched for d == 0; verifying a zeroed
   * context must fail. */
  memset(&c, 0, sizeof(c));
  jurischain_gen(&c, 0, "AnySeed", 7);
  check(jurischain_verify(&c) == 0, "gen(d=0) does not build a valid challenge");

  /* Even a hand-crafted d == 0 payload must not verify. */
  memset(&c, 0, sizeof(c));
  c.payload[HASH_LEN] = 0;
  check(jurischain_verify(&c) == 0, "verify rejects d=0 directly");
}

int main(void) {
  printf("=== JurisChain core C test ===\n");
  test_sha3_vectors();
  test_roundtrip();
  test_tamper();
  test_null_safety();
  test_zero_difficulty();

  if (failures == 0) {
    printf("\n=== All core C tests passed! ===\n");
    return 0;
  }
  printf("\n=== %d core C test(s) FAILED ===\n", failures);
  return 1;
}
