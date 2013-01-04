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

#ifndef PHP_VDUMP_H
#define PHP_VDUMP_H

#include "php.h"
#include "ext/standard/php_string.h"
#include <stdio.h>

#ifdef  __cplusplus
extern "C" {
#endif

struct vdump {
	FILE *fh;
	zval references;
};

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>

#ifdef HAVE_VDUMP
#define PHP_VDUMP_VERSION "0.1"


#include <php_ini.h>
#include <SAPI.h>
#include <ext/standard/info.h>
#include <Zend/zend_extensions.h>
#ifdef  __cplusplus
} // extern "C" 
#endif
#ifdef  __cplusplus
extern "C" {
#endif

extern zend_module_entry vdump_module_entry;
#define phpext_vdump_ptr &vdump_module_entry

#ifdef PHP_WIN32
#define PHP_VDUMP_API __declspec(dllexport)
#else
#define PHP_VDUMP_API
#endif

PHP_MINFO_FUNCTION(vdump);

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_FUNCTION(vdump_dump);

ZEND_BEGIN_ARG_INFO_EX(vdump_dump_arg_info, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, outfile)
ZEND_END_ARG_INFO()

#ifdef  __cplusplus
} // extern "C" 
#endif

#endif /* PHP_HAVE_VDUMP */

#endif /* PHP_VDUMP_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
