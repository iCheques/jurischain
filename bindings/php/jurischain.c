#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_jurischain.h"
#include "zend_exceptions.h"

#include <jurischain.h>
#include <stdio.h>
#include <string.h>

#define JURISCHAIN_MAX_SEED_LEN 1024

static inline int hex_char_to_nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

/* ── Custom object storage ─────────────────────────────────────── */

typedef struct {
  jurischain_ctx_t ctx;
  zend_object std;
} jurischain_obj_t;

static zend_class_entry *jurischain_ce;
static zend_object_handlers jurischain_handlers;

static inline jurischain_obj_t *jurischain_from_obj(zend_object *obj) {
  return (jurischain_obj_t *)((char *)obj - XtOffsetOf(jurischain_obj_t, std));
}

#define Z_JURISCHAIN_P(zv) jurischain_from_obj(Z_OBJ_P(zv))

/* ── Object lifecycle ──────────────────────────────────────────── */

static zend_object *jurischain_create(zend_class_entry *ce) {
  jurischain_obj_t *intern = zend_object_alloc(sizeof(jurischain_obj_t), ce);
  memset(&intern->ctx, 0, sizeof(jurischain_ctx_t));
  zend_object_std_init(&intern->std, ce);
  object_properties_init(&intern->std, ce);
  intern->std.handlers = &jurischain_handlers;
  return &intern->std;
}

static void jurischain_free(zend_object *obj) {
  jurischain_obj_t *intern = jurischain_from_obj(obj);
  memset(&intern->ctx, 0, sizeof(jurischain_ctx_t));
  zend_object_std_dtor(&intern->std);
}

/* ── Methods ───────────────────────────────────────────────────── */

PHP_METHOD(Jurischain, __construct) {
  zend_long difficulty;
  char *seed;
  size_t seed_len;

  ZEND_PARSE_PARAMETERS_START(2, 2)
  Z_PARAM_LONG(difficulty)
  Z_PARAM_STRING(seed, seed_len)
  ZEND_PARSE_PARAMETERS_END();

  if (difficulty < 1 || difficulty > 255) {
    zend_throw_exception(zend_ce_value_error,
        "Difficulty must be between 1 and 255", 0);
    RETURN_THROWS();
  }

  if (seed_len == 0) {
    zend_throw_exception(zend_ce_value_error,
        "Seed must be a non-empty string", 0);
    RETURN_THROWS();
  }

  if (seed_len > JURISCHAIN_MAX_SEED_LEN) {
    zend_throw_exception(zend_ce_value_error,
        "Seed must not exceed 1024 bytes", 0);
    RETURN_THROWS();
  }

  jurischain_obj_t *intern = Z_JURISCHAIN_P(ZEND_THIS);
  jurischain_gen(&intern->ctx, (uint8_t)difficulty, seed, seed_len);
}

PHP_METHOD(Jurischain, solve) {
  ZEND_PARSE_PARAMETERS_NONE();
  jurischain_obj_t *intern = Z_JURISCHAIN_P(ZEND_THIS);
  RETURN_BOOL(jurischain_try(&intern->ctx));
}

PHP_METHOD(Jurischain, verify) {
  ZEND_PARSE_PARAMETERS_NONE();
  jurischain_obj_t *intern = Z_JURISCHAIN_P(ZEND_THIS);
  RETURN_BOOL(jurischain_verify(&intern->ctx));
}

PHP_METHOD(Jurischain, getChallenge) {
  ZEND_PARSE_PARAMETERS_NONE();
  jurischain_obj_t *intern = Z_JURISCHAIN_P(ZEND_THIS);

  char str[(HASH_LEN * 2) + 1];
  memset(str, 0, sizeof(str));
  for (int i = 0; i < HASH_LEN; i++) {
    snprintf(str + (i * 2), 3, "%02hhX", intern->ctx.seed[i]);
  }
  RETURN_STRINGL(str, HASH_LEN * 2);
}

PHP_METHOD(Jurischain, setResponse) {
  char *hex;
  size_t hex_len;

  ZEND_PARSE_PARAMETERS_START(1, 1)
  Z_PARAM_STRING(hex, hex_len)
  ZEND_PARSE_PARAMETERS_END();

  if (hex_len != HASH_LEN * 2) {
    zend_throw_exception(zend_ce_value_error,
        "Response must be exactly 64 hex characters", 0);
    RETURN_THROWS();
  }

  jurischain_obj_t *intern = Z_JURISCHAIN_P(ZEND_THIS);
  uint8_t parsed[HASH_LEN];

  for (int i = 0; i < HASH_LEN; i++) {
    int hi = hex_char_to_nibble(hex[i * 2]);
    int lo = hex_char_to_nibble(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0) {
      zend_throw_exception(zend_ce_value_error,
          "Response contains invalid hex characters", 0);
      RETURN_THROWS();
    }
    parsed[i] = (uint8_t)((hi << 4) | lo);
  }

  memcpy(intern->ctx.seed, parsed, HASH_LEN);
  RETURN_TRUE;
}

/* ── Arginfo ───────────────────────────────────────────────────── */

ZEND_BEGIN_ARG_INFO_EX(arginfo_construct, 0, 0, 2)
  ZEND_ARG_TYPE_INFO(0, difficulty, IS_LONG, 0)
  ZEND_ARG_TYPE_INFO(0, seed, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_solve, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_verify, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_getChallenge, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_setResponse, 0, 1, _IS_BOOL, 0)
  ZEND_ARG_TYPE_INFO(0, response, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* ── Method table ──────────────────────────────────────────────── */

static const zend_function_entry jurischain_methods[] = {
  PHP_ME(Jurischain, __construct,  arginfo_construct,    ZEND_ACC_PUBLIC)
  PHP_ME(Jurischain, solve,        arginfo_solve,        ZEND_ACC_PUBLIC)
  PHP_ME(Jurischain, verify,       arginfo_verify,       ZEND_ACC_PUBLIC)
  PHP_ME(Jurischain, getChallenge, arginfo_getChallenge, ZEND_ACC_PUBLIC)
  PHP_ME(Jurischain, setResponse,  arginfo_setResponse,  ZEND_ACC_PUBLIC)
  PHP_FE_END
};

/* ── Module lifecycle ──────────────────────────────────────────── */

PHP_MINIT_FUNCTION(jurischain) {
  zend_class_entry ce;
  INIT_CLASS_ENTRY(ce, "Jurischain", jurischain_methods);
  jurischain_ce = zend_register_internal_class(&ce);
  jurischain_ce->create_object = jurischain_create;
  jurischain_ce->ce_flags |= ZEND_ACC_FINAL | ZEND_ACC_NOT_SERIALIZABLE;

  memcpy(&jurischain_handlers, &std_object_handlers, sizeof(zend_object_handlers));
  jurischain_handlers.offset = XtOffsetOf(jurischain_obj_t, std);
  jurischain_handlers.free_obj = jurischain_free;
  jurischain_handlers.clone_obj = NULL;

  return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(jurischain) { return SUCCESS; }
PHP_RINIT_FUNCTION(jurischain) { return SUCCESS; }

PHP_MINFO_FUNCTION(jurischain) {
  php_info_print_table_start();
  php_info_print_table_header(2, "jurischain support", "enabled");
  php_info_print_table_row(2, "version", PHP_JURISCHAIN_VERSION);
  php_info_print_table_end();
}

/* ── Module entry ──────────────────────────────────────────────── */

zend_module_entry jurischain_module_entry = {
  STANDARD_MODULE_HEADER,
  "jurischain",
  NULL,
  PHP_MINIT(jurischain),
  PHP_MSHUTDOWN(jurischain),
  PHP_RINIT(jurischain),
  NULL,
  PHP_MINFO(jurischain),
  PHP_JURISCHAIN_VERSION,
  STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_JURISCHAIN
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(jurischain)
#endif
