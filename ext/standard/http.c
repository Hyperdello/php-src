/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Sara Golemon <pollita@php.net>                              |
   +----------------------------------------------------------------------+
*/

#include "php_http.h"
#include "php_ini.h"
#include "url.h"

static void php_url_encode_scalar(zval *scalar, smart_str *form_str,
	int encoding_type, zend_ulong index_int,
	const char *index_string, size_t index_string_len,
	const char *num_prefix, size_t num_prefix_len,
	const char *key_prefix, size_t key_prefix_len,
	const char *key_suffix, size_t key_suffix_len,
	const zend_string *arg_sep)
{
	if (form_str->s) {
		smart_str_append(form_str, arg_sep);
	}
	/* Simple key=value */
	if (key_prefix) {
		smart_str_appendl(form_str, key_prefix, key_prefix_len);
	}
	if (index_string) {
		zend_string *encoded_key;
		if (encoding_type == PHP_QUERY_RFC3986) {
			encoded_key = php_raw_url_encode(index_string, index_string_len);
		} else {
			encoded_key = php_url_encode(index_string, index_string_len);
		}
		smart_str_append(form_str, encoded_key);
		zend_string_free(encoded_key);
	} else {
		/* Numeric key */
		if (num_prefix) {
			smart_str_appendl(form_str, num_prefix, num_prefix_len);
		}
		smart_str_append_long(form_str, index_int);
	}
	if (key_suffix) {
		smart_str_appendl(form_str, key_suffix, key_suffix_len);
	}
	smart_str_appendc(form_str, '=');

	switch (Z_TYPE_P(scalar)) {
		case IS_STRING: {
			zend_string *encoded_data;
			if (encoding_type == PHP_QUERY_RFC3986) {
				encoded_data = php_raw_url_encode(Z_STRVAL_P(scalar), Z_STRLEN_P(scalar));
			} else {
				encoded_data = php_url_encode(Z_STRVAL_P(scalar), Z_STRLEN_P(scalar));
			}
			smart_str_append(form_str, encoded_data);
			zend_string_free(encoded_data);
			break;
		}
		case IS_LONG:
			smart_str_append_long(form_str, Z_LVAL_P(scalar));
			break;
		case IS_DOUBLE: {
			zend_string *encoded_data;
			zend_string *tmp = zend_double_to_str(Z_DVAL_P(scalar));
			if (encoding_type == PHP_QUERY_RFC3986) {
				encoded_data = php_raw_url_encode(ZSTR_VAL(tmp), ZSTR_LEN(tmp));
			} else {
				encoded_data = php_url_encode(ZSTR_VAL(tmp), ZSTR_LEN(tmp));
			}
			smart_str_append(form_str, encoded_data);
			zend_string_free(tmp);
			zend_string_free(encoded_data);
			break;
		}
		case IS_FALSE:
			smart_str_appendc(form_str, '0');
			break;
		case IS_TRUE:
			smart_str_appendc(form_str, '1');
			break;
		/* All possible types are either handled here or previously */
		EMPTY_SWITCH_DEFAULT_CASE();
	}
}

/* {{{ php_url_encode_hash */
PHPAPI void php_url_encode_hash_ex(HashTable *ht, smart_str *formstr,
				const char *num_prefix, size_t num_prefix_len,
				const char *key_prefix, size_t key_prefix_len,
				const char *key_suffix, size_t key_suffix_len,
				zval *type, const zend_string *arg_sep, int enc_type)
{
	zend_string *key = NULL;
	char *newprefix, *p;
	const char *prop_name;
	size_t newprefix_len, prop_len;
	zend_ulong idx;
	zval *zdata = NULL;
	ZEND_ASSERT(ht);

	if (GC_IS_RECURSIVE(ht)) {
		/* Prevent recursion */
		return;
	}

	if (!arg_sep) {
		arg_sep = zend_ini_str("arg_separator.output", strlen("arg_separator.output"), false);
		if (ZSTR_LEN(arg_sep) == 0) {
			arg_sep = ZSTR_CHAR('&');
		}
	}

	ZEND_HASH_FOREACH_KEY_VAL(ht, idx, key, zdata) {
		bool is_dynamic = 1;
		if (Z_TYPE_P(zdata) == IS_INDIRECT) {
			zdata = Z_INDIRECT_P(zdata);
			if (Z_ISUNDEF_P(zdata)) {
				continue;
			}

			is_dynamic = 0;
		}

		/* handling for private & protected object properties */
		if (key) {
			prop_name = ZSTR_VAL(key);
			prop_len = ZSTR_LEN(key);

			if (type != NULL && zend_check_property_access(Z_OBJ_P(type), key, is_dynamic) != SUCCESS) {
				/* property not visible in this scope */
				continue;
			}

			if (ZSTR_VAL(key)[0] == '\0' && type != NULL) {
				const char *tmp;
				zend_unmangle_property_name_ex(key, &tmp, &prop_name, &prop_len);
			} else {
				prop_name = ZSTR_VAL(key);
				prop_len = ZSTR_LEN(key);
			}
		} else {
			prop_name = NULL;
			prop_len = 0;
		}

		ZVAL_DEREF(zdata);
		if (Z_TYPE_P(zdata) == IS_ARRAY || Z_TYPE_P(zdata) == IS_OBJECT) {
			if (key) {
				zend_string *ekey;
				if (enc_type == PHP_QUERY_RFC3986) {
					ekey = php_raw_url_encode(prop_name, prop_len);
				} else {
					ekey = php_url_encode(prop_name, prop_len);
				}
				newprefix_len = key_suffix_len + ZSTR_LEN(ekey) + key_prefix_len + 3 /* %5B */;
				newprefix = emalloc(newprefix_len + 1);
				p = newprefix;

				if (key_prefix) {
					memcpy(p, key_prefix, key_prefix_len);
					p += key_prefix_len;
				}

				memcpy(p, ZSTR_VAL(ekey), ZSTR_LEN(ekey));
				p += ZSTR_LEN(ekey);
				zend_string_free(ekey);

				if (key_suffix) {
					memcpy(p, key_suffix, key_suffix_len);
					p += key_suffix_len;
				}
				*(p++) = '%';
				*(p++) = '5';
				*(p++) = 'B';
				*p = '\0';
			} else {
				char *ekey;
				size_t ekey_len;
				/* Is an integer key */
				ekey_len = spprintf(&ekey, 0, ZEND_LONG_FMT, idx);
				newprefix_len = key_prefix_len + num_prefix_len + ekey_len + key_suffix_len + 3 /* %5B */;
				newprefix = emalloc(newprefix_len + 1);
				p = newprefix;

				if (key_prefix) {
					memcpy(p, key_prefix, key_prefix_len);
					p += key_prefix_len;
				}

				if (num_prefix) {
					memcpy(p, num_prefix, num_prefix_len);
					p += num_prefix_len;
				}

				memcpy(p, ekey, ekey_len);
				p += ekey_len;
				efree(ekey);

				if (key_suffix) {
					memcpy(p, key_suffix, key_suffix_len);
					p += key_suffix_len;
				}
				*(p++) = '%';
				*(p++) = '5';
				*(p++) = 'B';
				*p = '\0';
			}
			GC_TRY_PROTECT_RECURSION(ht);
			php_url_encode_hash_ex(HASH_OF(zdata), formstr, NULL, 0, newprefix, newprefix_len, "%5D", 3, (Z_TYPE_P(zdata) == IS_OBJECT ? zdata : NULL), arg_sep, enc_type);
			GC_TRY_UNPROTECT_RECURSION(ht);
			efree(newprefix);
		} else if (Z_TYPE_P(zdata) == IS_NULL || Z_TYPE_P(zdata) == IS_RESOURCE) {
			/* Skip these types */
			continue;
		} else {
			php_url_encode_scalar(zdata, formstr,
				enc_type, idx,
				prop_name, prop_len,
				num_prefix, num_prefix_len,
				key_prefix, key_prefix_len,
				key_suffix, key_suffix_len,
				arg_sep);
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

	/* If there is a prefix we need to close the key with an encoded ] ("%5D") */
/* {{{ Generates a form-encoded query string from an associative array or object. */
PHP_FUNCTION(http_build_query)
{
	zval *formdata;
	char *prefix = NULL;
	size_t prefix_len = 0;
	zend_string *arg_sep = NULL;
	smart_str formstr = {0};
	zend_long enc_type = PHP_QUERY_RFC1738;

	ZEND_PARSE_PARAMETERS_START(1, 4)
		Z_PARAM_ARRAY_OR_OBJECT(formdata)
		Z_PARAM_OPTIONAL
		Z_PARAM_STRING(prefix, prefix_len)
		Z_PARAM_STR(arg_sep)
		Z_PARAM_LONG(enc_type)
	ZEND_PARSE_PARAMETERS_END();

	php_url_encode_hash_ex(HASH_OF(formdata), &formstr, prefix, prefix_len, NULL, 0, NULL, 0, (Z_TYPE_P(formdata) == IS_OBJECT ? formdata : NULL), arg_sep, (int)enc_type);

	RETURN_STR(smart_str_extract(&formstr));
}
/* }}} */
