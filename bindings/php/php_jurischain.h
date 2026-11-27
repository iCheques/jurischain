/*
 * JurisChain PHP 8 Extension
 *
 * SHA3-256 Proof-of-Work CAPTCHA extension for PHP 8.1+.
 * Licensed under the MIT License — see the root LICENSE file.
 */

#ifndef PHP_JURISCHAIN_H
#define PHP_JURISCHAIN_H

extern zend_module_entry jurischain_module_entry;
#define phpext_jurischain_ptr &jurischain_module_entry

#define PHP_JURISCHAIN_VERSION "1.1.4"

#if defined(ZTS) && defined(COMPILE_DL_JURISCHAIN)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif /* PHP_JURISCHAIN_H */
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
