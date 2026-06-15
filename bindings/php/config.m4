PHP_ARG_ENABLE(jurischain, whether to enable jurischain support,
[  --enable-jurischain          Enable jurischain support], yes)

if test "$PHP_JURISCHAIN" != "no"; then
  AC_DEFINE(HAVE_JURISCHAIN, 1, [ Have jurischain support ])
  PHP_NEW_EXTENSION(jurischain, jurischain.c, $ext_shared)
  dnl Resolve the core header from the repository layout (bindings/php -> include/)
  dnl so the extension builds standalone via `phpize && ./configure && make`,
  dnl which is exactly what PIE (https://github.com/php/pie) executes.
  PHP_ADD_INCLUDE([$ext_srcdir/../../include])
  PHP_ADD_INCLUDE([$ext_srcdir])
fi
