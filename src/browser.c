#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "emscripten.h"
#include "jurischain.h"

#define JURISCHAIN_MAX_SEED_LEN 1024

static jurischain_ctx_t pow_i;
static int solving = 0;

static const char *loading =
    "<div class='jurischain-captcha__marker'><div "
    "class='jurischain-captcha__checkbox--loading'></div></div>"
    "<div class='jurischain-captcha__text'>Aguarde enquanto validamos "
    "seu acesso.</div>";

static const char *completed =
    "<div class='jurischain-captcha__marker'><div "
    "class='jurischain-captcha__checkbox--completed'></div></div>"
    "<div class='jurischain-captcha__text'>Seu acesso foi validado "
    "com sucesso.</div>";

EM_JS(int, jurischainGetDifficulty, (void), {
  var d = (typeof window !== 'undefined' && window.jurischain)
    ? Number(window.jurischain.difficulty)
    : 0;
  return isFinite(d) ? (d | 0) : 0;
})

EM_JS(void, jurischainGetSeed, (char *buf, int maxLen), {
  var s = (typeof window !== 'undefined' &&
           window.jurischain &&
           typeof window.jurischain.seed === 'string')
    ? window.jurischain.seed
    : '';
  stringToUTF8(s, buf, maxLen);
})

EM_JS(void, jurischainElement, (const char *content, int solved, const char *challenge), {
  if (solved) {
    var solution = UTF8ToString(challenge, 64);
    var evt;
    try { evt = new CustomEvent('jurischain', { detail: solution }); }
    catch(e) { evt = document.createEvent('CustomEvent'); evt.initCustomEvent('jurischain', false, false, solution); }
    document.dispatchEvent(evt);
  }

  var el = document.getElementById('jurischain-captcha');
  if (el) {
    el.innerHTML = UTF8ToString(content);
    if (solved) {
      var input = document.createElement('input');
      input.type = 'hidden';
      input.name = 'jurischain';
      input.value = UTF8ToString(challenge, 64);
      el.appendChild(input);
    }
  }
})

void EMSCRIPTEN_KEEPALIVE cancel_solve(void) {
  if (solving) {
    solving = 0;
    emscripten_cancel_main_loop();
  }
}

void EMSCRIPTEN_KEEPALIVE try_solve(void) {
  if (!solving) {
    emscripten_cancel_main_loop();
    return;
  }

  for (int attempt = 0; attempt < 15; attempt++) {
    if (!jurischain_try(&pow_i))
      continue;
    char challenge[(HASH_LEN * 2) + 1] = { 0, };
    for (int j = 0; j < HASH_LEN; j++)
      snprintf(challenge + (j * 2), 3, "%02hhX", pow_i.seed[j]);
    solving = 0;
    emscripten_cancel_main_loop();
    jurischainElement(completed, 1, challenge);
    return;
  }
}

int main(void) {
  char seed[JURISCHAIN_MAX_SEED_LEN + 1] = { 0, };
  int difficulty = jurischainGetDifficulty();
  size_t seed_len = 0;

  jurischainGetSeed(seed, (int)sizeof(seed));
  seed_len = strlen(seed);

  if (seed_len == 0 || difficulty < 1 || difficulty > 255)
    return 1;

  jurischainElement(loading, 0, NULL);
  jurischain_gen(&pow_i, (uint8_t)difficulty, seed, seed_len);
  solving = 1;
  emscripten_set_main_loop(try_solve, 0, 0);
  return 0;
}
