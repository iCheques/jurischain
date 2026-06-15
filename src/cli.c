#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "jurischain.h"

static char *defaultSeed = "W235XX";

int main(int argc, char *argv[]) {
  jurischain_ctx_t challenge;

  size_t tries = 0;
  int i = 0;
  /* Clamp into the valid range BEFORE narrowing to uint8_t. Doing the cast
   * first would let e.g. 256 wrap to 0 (a vacuous, never-solvable challenge)
   * and 257 to 1. */
  int requested = argc > 1 ? atoi(argv[1]) : 1;
  uint8_t difficulty;
  char *initialSeed = argc > 2 ? argv[2] : defaultSeed;

  if (requested < 1) requested = 1;
  if (requested > 255) requested = 255;
  difficulty = (uint8_t)requested;

  printf("Difficulty: %u\nChallenge: %s\r\n", difficulty, initialSeed);

  jurischain_gen(&challenge, difficulty, initialSeed,
          strlen(initialSeed) * sizeof(char));
  while (!jurischain_try(&challenge))
    tries++;

  printf("[RESPONSE] ");
  for (i = 0; i < HASH_LEN; i++)
    printf("%02X", challenge.seed[i]);
  printf("\r\nTries: %lu\r\n", tries);
  return !jurischain_verify(&challenge);
}
