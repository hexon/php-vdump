/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Jille Timmermans <jille@hexon.cx>                           |
  +----------------------------------------------------------------------+
*/

/* $ Id: $ */ 

#define _GNU_SOURCE
#include "php_vdump.h"

#if HAVE_VDUMP

static int vdump_array_element_add(zval **zvp TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key);
static void vdump_add_zval(struct vdump *vdi, char *key, zval *zv TSRMLS_DC);
static char *vdump_escape_string(const char *str, const int len, int *ret_len TSRMLS_DC);
int vdump_dump(const char *outfile TSRMLS_DC);

/* {{{ vdump_functions[] */
zend_function_entry vdump_functions[] = {
	PHP_FE(vdump_dump          , vdump_dump_arg_info)
	{ NULL, NULL, NULL }
};
/* }}} */


/* {{{ vdump_module_entry
 */
zend_module_entry vdump_module_entry = {
	STANDARD_MODULE_HEADER,
	"vdump",
	vdump_functions,
	NULL,
	NULL,
	NULL,
	NULL,
	PHP_MINFO(vdump),
	PHP_VDUMP_VERSION, 
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_VDUMP
ZEND_GET_MODULE(vdump)
#endif


/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(vdump)
{
	php_printf("An extension which provides process state dumping\n");
	php_info_print_table_start();
	php_info_print_table_row(2, "Version",PHP_VDUMP_VERSION " (alpha)");
	// php_info_print_table_row(2, "Released", "2011-06-24");
	php_info_print_table_row(2, "SVN Revision", "$Id: $");
	php_info_print_table_end();
	/* add your stuff here */

}
/* }}} */

static char *
vdump_escape_string(const char *str, const int len, int *ret_len TSRMLS_DC) {
	char *tmp_str, *tmp_str2;
	int tmp_len;

	tmp_str = php_addcslashes(str, len, &tmp_len, 0, "'\\", 2 TSRMLS_CC);
	tmp_str2 = php_str_to_str_ex(tmp_str, tmp_len, "\0", 1, "' . \"\\0\" . '", 12, ret_len, 0, NULL);

	efree(tmp_str);

	return tmp_str2;
}

static int
vdump_array_element_add(zval **zvp TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) {
	struct vdump *vdi = va_arg(args, struct vdump *);
	char *key = va_arg(args, char *);
	char *newkey;

	if(hash_key->nKeyLength == 0) {
		asprintf(&newkey, "%s[%lu]", key, hash_key->h);
	} else {
		int escaped_key_len;
		char *escaped_key = vdump_escape_string(hash_key->arKey, hash_key->nKeyLength - 1, &escaped_key_len TSRMLS_CC);

		asprintf(&newkey, "%s['%.*s']", key, escaped_key_len, escaped_key);

		efree(escaped_key);
	}

	vdump_add_zval(vdi, newkey, *zvp TSRMLS_CC);

	free(newkey);

	return 0;
}

static int
vdump_object_property_add(zval **zvp TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) {
	struct vdump *vdi = va_arg(args, struct vdump *);
	char *key = va_arg(args, char *);
	const char *class_name, *prop_name;
	char *newkey;

	if(hash_key->nKeyLength == 0) {
		asprintf(&newkey, "%s[%lu]", key, hash_key->h);
	} else {
		int escaped_key_len;
		char *escaped_key;
		zend_unmangle_property_name(hash_key->arKey, hash_key->nKeyLength -1, &class_name, &prop_name);
		escaped_key = vdump_escape_string(prop_name, strlen(prop_name), &escaped_key_len TSRMLS_CC);

		asprintf(&newkey, "%s['%.*s']", key, escaped_key_len, escaped_key);

		efree(escaped_key);
	}

	vdump_add_zval(vdi, newkey, *zvp TSRMLS_CC);

	free(newkey);

	return 0;
}

static void
vdump_add_zval(struct vdump *vdi, char *key, zval *zv TSRMLS_DC) {
	char *string;
	int string_len;
	unsigned int string_ulen;
	HashTable *ht;

	if(Z_REFCOUNT_P(zv) > 1 || Z_TYPE_P(zv) == IS_OBJECT || Z_TYPE_P(zv) == IS_ARRAY) {
		zval *zkey, **zkeyp;
		ulong arkey;
		switch(Z_TYPE_P(zv)) {
			case IS_OBJECT:
				arkey = (ulong)Z_OBJPROP_P(zv);
				break;
			case IS_ARRAY:
				arkey = (ulong)Z_ARRVAL_P(zv);
				break;
			default:
				arkey = (ulong)zv;
		}

		assert(arkey != 0);

		// printf("zval(%p) is referencing with key %lu\n", zv, arkey);

		if(zend_hash_index_find(Z_ARRVAL(vdi->references), arkey, (void **)&zkeyp) == SUCCESS) {
			zkey = *zkeyp;
			assert(Z_TYPE_P(zkey) == IS_STRING);
			if(Z_ISREF_P(zv)) {
				fprintf(vdi->fh, "\t%s = &%s;\n", key, Z_STRVAL_P(zkey));
			} else {
				fprintf(vdi->fh, "\t%s = %s;\n", key, Z_STRVAL_P(zkey));
			}
			return;
		}

		MAKE_STD_ZVAL(zkey);
		ZVAL_STRING(zkey, key, 1);
		if(zend_hash_index_update(Z_ARRVAL(vdi->references), arkey, &zkey, sizeof(zval *), NULL) == FAILURE) {
			abort();
		}
	}

	switch(Z_TYPE_P(zv)) {
		case IS_NULL:
			fprintf(vdi->fh, "\t%s = NULL;\n", key);
			break;

		case IS_LONG:
			fprintf(vdi->fh, "\t%s = %ld;\n", key, Z_LVAL_P(zv));
			break;

		case IS_DOUBLE:
			fprintf(vdi->fh, "\t%s = %.*G;\n", key, (int) EG(precision), Z_DVAL_P(zv));
			break;

		case IS_BOOL:
			if(Z_BVAL_P(zv)) {
				fprintf(vdi->fh, "\t%s = true;\n", key);
			} else {
				fprintf(vdi->fh, "\t%s = false;\n", key);
			}
			break;

		case IS_ARRAY:
			fprintf(vdi->fh, "\t%s = array();\n", key);
			zend_hash_apply_with_arguments(Z_ARRVAL_P(zv) TSRMLS_CC, (apply_func_args_t) vdump_array_element_add, 2, vdi, key);
			break;

		case IS_OBJECT:
			Z_OBJ_HANDLER_P(zv, get_class_name)(zv, &string, &string_ulen, 0 TSRMLS_CC);
			ht = Z_OBJPROP_P(zv);
			if(ht) {
				char *newkey;

				asprintf(&newkey, "$properties[%d]", Z_OBJ_HANDLE_P(zv));
				fprintf(vdi->fh, "\t%s = NULL; // %.*s\n", key, string_ulen, string);
				fprintf(vdi->fh, "\t$objRef[%d] = &%s;\n", Z_OBJ_HANDLE_P(zv), key);
				fprintf(vdi->fh, "\t$properties[%d] = array();\n", Z_OBJ_HANDLE_P(zv));
				zend_hash_apply_with_arguments(ht TSRMLS_CC, (apply_func_args_t) vdump_object_property_add, 2, vdi, newkey);
				fprintf(vdi->fh, "\t%s = vdump_load_object('%.*s', $properties[%d]);\n", key, string_ulen, string, Z_OBJ_HANDLE_P(zv));
				free(newkey);
			} else {
				fprintf(vdi->fh, "\t%s = vdump_load_object('%.*s', array());\n", key, string_ulen, string);
			}
			efree(string);
			break;

		case IS_STRING:
			string = vdump_escape_string(Z_STRVAL_P(zv), Z_STRLEN_P(zv), &string_len TSRMLS_CC);

			fprintf(vdi->fh, "\t%s = '%.*s';\n", key, string_len, string);

			efree(string);
			break;

		case IS_RESOURCE:
			fprintf(vdi->fh, "\t%s = NULL; // Resource\n", key);
			break;

		case IS_CONSTANT:
			fprintf(vdi->fh, "\t%s = NULL; // Constant\n", key);
			break;

		case IS_CONSTANT_ARRAY:
			fprintf(vdi->fh, "\t%s = NULL; // Constant array\n", key);
			break;

		default:
			abort();
	}
}

int
vdump_dump(const char *outfile TSRMLS_DC) {
	struct vdump vdi;
	zval symtable;

	memset(&vdi, 0, sizeof(vdi));

	INIT_ZVAL(vdi.references);
	array_init_size(&vdi.references, 512);

	vdi.fh = fopen(outfile, "w");
	if(vdi.fh == NULL) {
		php_error(E_WARNING, "vdump_dump: fopen() failed");
		return 0;
	}

	fprintf(vdi.fh, "<?php\n");
	fprintf(vdi.fh, "\t// VDump outfile\n");
	fprintf(vdi.fh, "\n");
	fprintf(vdi.fh, "\t$properties = array();\n");
	fprintf(vdi.fh, "\t$objRef = array();\n");
	fprintf(vdi.fh, "\t$data = array();\n");

	if(!EG(active_symbol_table)) {
		zend_rebuild_symbol_table(TSRMLS_C);
	}

	INIT_ZVAL(symtable);
	Z_SET_ISREF(symtable);
	Z_TYPE(symtable) = IS_ARRAY;
	Z_ARRVAL(symtable) = &EG(symbol_table);
	Z_ADDREF(symtable);
	vdump_add_zval(&vdi, "$data['GLOBALS']", &symtable TSRMLS_CC);
	Z_DELREF(symtable);

	zend_execute_data *ex = EG(current_execute_data);
	int frame = 0;
	while(ex) {
		if(ex->op_array) {
			char *newkey;
			if(ex->symbol_table) {
				asprintf(&newkey, "$data['stack'][%d]", frame);
				Z_ARRVAL(symtable) = ex->symbol_table;
				Z_ADDREF(symtable);
				vdump_add_zval(&vdi, newkey, &symtable TSRMLS_CC);
				Z_DELREF(symtable);
				free(newkey);
			} else {
				int i;
				for (i = 0; i < ex->op_array->last_var; i++) {
					if (ex->CVs[i]) {
						int escaped_key_len;
						char *escaped_key = vdump_escape_string(ex->op_array->vars[i].name, ex->op_array->vars[i].name_len, &escaped_key_len TSRMLS_CC);

						asprintf(&newkey, "$data['stack'][%d]['%.*s']", frame, escaped_key_len, escaped_key);
						Z_ADDREF_PP(ex->CVs[i]);
						vdump_add_zval(&vdi, newkey, *ex->CVs[i] TSRMLS_CC);
						Z_DELREF_PP(ex->CVs[i]);

						efree(escaped_key);
					}
				}
			}
		}
		ex = ex->prev_execute_data;
		frame++;
	}

	fprintf(vdi.fh, "\tunset($properties);\n");
	fprintf(vdi.fh, "\tunset($objRef);\n");
	fprintf(vdi.fh, "?>\n");
	fclose(vdi.fh);

	zval_dtor(&vdi.references);

	return 1;
}

int
vdump_dump_no_tsrm(const char *outfile) {
	TSRMLS_FETCH();
	return vdump_dump(outfile TSRMLS_CC);
}

/* {{{ proto int vdump_dump(string outfile)
  Dump to outfile */
PHP_FUNCTION(vdump_dump)
{
	const char *outfile = NULL;
	int outfile_len = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &outfile, &outfile_len) == FAILURE) {
		return;
	}

	if(vdump_dump(outfile TSRMLS_CC)) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} vdump_dump */

#endif /* HAVE_VDUMP */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
