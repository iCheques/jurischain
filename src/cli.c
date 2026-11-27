#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jurischain.h"

static const char *defaultSeed = "W235XX";

int main(int argc, char *argv[]) {
  jurischain_ctx_t challenge;

  size_t tries = 0;
  size_t seed_len = 0;
  int i = 0;
  long requested = 1;
  uint8_t difficulty;
  const char *initialSeed = argc > 2 ? argv[2] : defaultSeed;

  if (argc > 1) {
    char *endptr = NULL;
    errno = 0;
    requested = strtol(argv[1], &endptr, 10);
    if (endptr == argv[1] || *endptr != '\0') {
      fprintf(stderr, "Error: difficulty must be an integer (got '%s')\n", argv[1]);
      return 1;
    }
    if (errno == ERANGE) {
      requested = requested > 0 ? 255 : 1;
    }
  }

  /* Clamp into the valid range BEFORE narrowing to uint8_t. Doing the cast
   * first would let e.g. 256 wrap to 0 (a vacuous, never-solvable challenge)
   * and 257 to 1. */
  if (requested < 1) requested = 1;
  if (requested > 255) requested = 255;
  difficulty = (uint8_t)requested;

  seed_len = strlen(initialSeed);
  if (seed_len == 0) {
    fprintf(stderr, "Error: seed must be a non-empty string\n");
    return 1;
  }

  printf("Difficulty: %u\nChallenge: %s\r\n", difficulty, initialSeed);

  jurischain_gen(&challenge, difficulty, initialSeed, seed_len);
  while (!jurischain_try(&challenge))
    tries++;

  printf("[RESPONSE] ");
  for (i = 0; i < HASH_LEN; i++)
    printf("%02X", challenge.seed[i]);
  printf("\r\nTries: %zu\r\n", tries);
  return !jurischain_verify(&challenge);
}
