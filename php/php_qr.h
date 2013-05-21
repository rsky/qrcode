/**
 * QR Code generator extension for PHP
 *
 * Copyright (c) 2007-2013 Ryusuke SEKIYAMA. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @package     php-qr
 * @author      Ryusuke SEKIYAMA <rsky0711@gmail.com>
 * @copyright   2007-2013 Ryusuke SEKIYAMA
 * @license     http://www.opensource.org/licenses/mit-license.php  MIT License
 */

#ifndef _PHP_QR_H_
#define _PHP_QR_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include <php_ini.h>
#include <SAPI.h>
#include <ext/standard/info.h>
#include <Zend/zend_extensions.h>
#ifdef ZTS
#include "TSRM.h"
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
#define PHP_QR_LOCAL __attribute__((visibility("hidden")))
#else
#define PHP_QR_LOCAL
#endif

#define PHP_QR_MODULE_VERSION "0.4.0"

#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif
#define TRUE 1
#define FALSE 0

#define PHP_QR_USE_DEFAULT_MODE INT_MIN

#define PHP_QR_ERRMODE_SILENT 0
#define PHP_QR_ERRMODE_WARNING 1
#define PHP_QR_ERRMODE_EXCEPTION 2

/* {{{ module globals */

ZEND_BEGIN_MODULE_GLOBALS(qr)
	long default_version;
	long default_mode;
	long default_eclevel;
	long default_masktype;
	long default_format;
	long default_magnify;
	long default_separator;
	long default_maxnum;
	long default_order;
ZEND_END_MODULE_GLOBALS(qr)

#ifdef ZTS
#define QRG(v) TSRMG(qr_globals_id, zend_qr_globals *, v)
#else
#define QRG(v) (qr_globals.v)
#endif

#define QR_DEFAULT(name) ((int)QRG(default_##name))

/* }}} */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _PHP_QR_H_ */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
