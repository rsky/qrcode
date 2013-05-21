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

#include "php_qr.h"
#include <Zend/zend_exceptions.h>
#include "qr.h"
#include "qr_util.h"
#include <main/php_logos.h>
#include "qr_logos.h"

/* {{{ module globals */

PHP_QR_LOCAL ZEND_DECLARE_MODULE_GLOBALS(qr)

/* }}} */

/* {{{ module fuction prototypes */

static PHP_MINIT_FUNCTION(qr);
static PHP_MSHUTDOWN_FUNCTION(qr);
static PHP_MINFO_FUNCTION(qr);
static PHP_GINIT_FUNCTION(qr);

/* }}} */

/* {{{ qr function prototypes */

static PHP_FUNCTION(qr_get_symbol);
static PHP_FUNCTION(qr_output_symbol);
static PHP_FUNCTION(qr_mimetype);
static PHP_FUNCTION(qr_extension);

static PHP_METHOD(QRCode, __construct);
static PHP_METHOD(QRCode, setErrorHandling);
static PHP_METHOD(QRCode, setFormat);
static PHP_METHOD(QRCode, setSeparator);
static PHP_METHOD(QRCode, setMagnify);
static PHP_METHOD(QRCode, setOrder);
static PHP_METHOD(QRCode, addData);
static PHP_METHOD(QRCode, readData);
static PHP_METHOD(QRCode, finalize);
static PHP_METHOD(QRCode, getSymbol);
static PHP_METHOD(QRCode, outputSymbol);
static PHP_METHOD(QRCode, getInfo);
static PHP_METHOD(QRCode, getMimeType);
static PHP_METHOD(QRCode, getExtension);
static PHP_METHOD(QRCode, current);
static PHP_METHOD(QRCode, key);
static PHP_METHOD(QRCode, next);
static PHP_METHOD(QRCode, rewind);
static PHP_METHOD(QRCode, valid);
static PHP_METHOD(QRCode, seek);
static PHP_METHOD(QRCode, count);

/* }}} */

/* {{{ internal function prototypes */

/**
 * parse the options
 */
static void
_qr_parse_options(HashTable *options,
		int *version, int *mode, int *eclevel, int *masktype,
		int *format, int *magnify, int *separator,
		int *maxnum, int *order TSRMLS_DC);

#define qr_parse_options(op, vr, md, el, mt, ft, mg, sp, mx, od) \
	_qr_parse_options((op), (vr), (md), (el), (mt), (ft), (mg), (sp), (mx), (od) TSRMLS_CC)

/**
 * initialize a standard QR Code, add data, finalize and return it
 */
static QRCode *
_qr_create_simple(const qr_byte_t *data, int data_len,
		int version, int mode, int eclevel, int masktype TSRMLS_DC);

/**
 * initialize a structured append QR Code, add data, finalize and return it
 */
static QRStructured *
_qrs_create_simple(const qr_byte_t *data, int data_len,
		int version, int mode, int eclevel, int masktype, int maxnum TSRMLS_DC);

/**
 * get the QR Code symbol
 */
static qr_byte_t *
_qr_get_symbol(const qr_byte_t *data, int data_len,
		int version, int mode, int eclevel, int masktype,
		int format, int separator, int magnify,
		int *symbol_size TSRMLS_DC);

/**
 * get the structured append QR Code symbol
 */
static qr_byte_t *
_qrs_get_symbols(const qr_byte_t *data, int data_len,
		int version, int mode, int eclevel, int masktype, int maxnum,
		int format, int separator, int magnify, int order,
		int *symbol_size TSRMLS_DC);

/**
 * open the stream
 *
 * @param   zval *zv            The value container which contains
 *                              the stream resource or the pathname.
 * @param   char *alt_url       The alternative URL which is opened
 *                              when 'zv' is empty string or NULL.
 * @param   char *mode          The stream open mode, mostly will be "wb" or "rb".
 * @param   char **opened_path  The address which will be set the opened pathname.
 * @param   const char *title   The name of the stream which
 *                              is printed on the error message.
 */
static php_stream *
_qr_stream_open(zval *zv, char *alt_url, char *mode,
		char **opened_path, const char *title TSRMLS_DC);

#define qr_get_input_stream(zv) \
	_qr_stream_open((zv), NULL, "rb", NULL, "input" TSRMLS_CC)

#define qr_get_output_stream(zv) \
	_qr_stream_open((zv), "php://output", "wb", NULL, "output" TSRMLS_CC)

/* {{{ Argument information of qr_* functions */

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO_EX(arginfo_qr_get_symbol, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO_EX(arginfo_qr_output_symbol, 0, 0, 2)
	ZEND_ARG_INFO(0, output)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO(arginfo_qr_mimetype, 0)
	ZEND_ARG_INFO(0, format)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO_EX(arginfo_qr_extension, 0, 0, 1)
	ZEND_ARG_INFO(0, format)
	ZEND_ARG_INFO(0, include_dot)
ZEND_END_ARG_INFO()

/* }}} */
/* {{{ Argument information of QRCode methods */

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO_EX(arginfo_qrcode_ctor, 0, 0, 0)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO(arginfo_qrcode_set_eh, 0)
	ZEND_ARG_INFO(0, errmode)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO(arginfo_qrcode_set_format, 0)
	ZEND_ARG_INFO(0, format)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO(arginfo_qrcode_set_separator, 0)
	ZEND_ARG_INFO(0, separator)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO(arginfo_qrcode_set_magnify, 0)
	ZEND_ARG_INFO(0, magnify)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO(arginfo_qrcode_set_order, 0)
	ZEND_ARG_INFO(0, order)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO_EX(arginfo_qrcode_add_data, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO_EX(arginfo_qrcode_read_data, 0, 0, 1)
	ZEND_ARG_INFO(0, input)
	ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO_EX(arginfo_qrcode_output_symbol, 0, 0, 0)
	ZEND_ARG_INFO(0, output)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO_EX(arginfo_qrcode_get_image_resource, 0, 0, 0)
	ZEND_ARG_INFO(1, colors)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO_EX(arginfo_qrcode_get_extension, 0, 0, 0)
	ZEND_ARG_INFO(0, include_dot)
ZEND_END_ARG_INFO()

unsigned int sep, unsigned int mag, unsigned int *sizeZEND_BEGIN_ARG_INFO(arginfo_qrcode_seek, 0)
	ZEND_ARG_INFO(0, position)
ZEND_END_ARG_INFO()

/* }}} */
/* {{{ SPL class entries */

static zend_class_entry *ext_ce_RuntimeException;
static zend_class_entry *ext_ce_OutOfBoundsException;

static zend_class_entry *
_qr_get_class_entry(char *class_name TSRMLS_DC);

/* }}} */
/* {{{ Class definitions */

typedef struct _qrcode_object {
	zend_object std;
	QRCode *qr;
	QRStructured *st;
	int pos;
	int errmode;
	int format;
	int separator;
	int magnify;
	int order;
} qrcode_object;

#define qrcode_get_object() \
	(qrcode_object *)zend_object_store_get_object(getThis() TSRMLS_CC)

static zend_class_entry *qrcode_ce = NULL;
static zend_class_entry *qrexception_ce = NULL;

static zend_object_handlers qrcode_object_handlers;

static int
_qrcode_seek(qrcode_object *intern, int position, zend_bool force_setpos)
{
	if (intern->st != NULL && position >= 0 && position < intern->st->num) {
		intern->pos = position;
		intern->qr = intern->st->qrs[position];
		return SUCCESS;
	} else if (intern->st == NULL && position == 0) {
		intern->pos = 0;
		return SUCCESS;
	} else {
		if (force_setpos) {
			intern->pos = position;
		}
		return FAILURE;
	}
}

#define qrcode_seek(intern, position) \
	_qrcode_seek((intern), (position), 0)

#define qrcode_seek_noverify(intern, position) \
	(void)_qrcode_seek((intern), (position), 1)

static void
qrcode_object_free_storage(void *object TSRMLS_DC)
{
	qrcode_object *intern = (qrcode_object *)object;

	if (intern->st) {
		qrsDestroy(intern->st);
	} else if (intern->qr) {
		qrDestroy(intern->qr);
	}

	zend_object_std_dtor(&intern->std TSRMLS_CC);

	efree(object);
}

static zend_object_value
qrcode_object_new_ex(qrcode_object **object, zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	qrcode_object *intern;

	intern = ecalloc(1, sizeof(qrcode_object));

	if (object != NULL) {
		*object = intern;
	}

	zend_object_std_init(&intern->std, ce TSRMLS_CC);
#if PHP_API_VERSION < 20100412
	zend_hash_copy(intern->std.properties, &ce->default_properties,
			(copy_ctor_func_t)zval_add_ref, NULL, sizeof(zval *));
#else
	object_properties_init(&intern->std, ce);
#endif

	intern->qr = NULL;
	intern->st = NULL;
	intern->pos = -1;
	intern->errmode = PHP_QR_ERRMODE_EXCEPTION;
	intern->format = QR_FMT_DIGIT;
	intern->separator = 4;
	intern->magnify = 1;
	intern->order = 0;

	retval.handle = zend_objects_store_put(intern,
			(zend_objects_store_dtor_t)zend_objects_destroy_object,
			(zend_objects_free_object_storage_t)qrcode_object_free_storage,
			NULL TSRMLS_CC);
	retval.handlers = &qrcode_object_handlers;

	return retval;
}

static zend_object_value
qrcode_object_new(zend_class_entry *ce TSRMLS_DC)
{
	return qrcode_object_new_ex(NULL, ce TSRMLS_CC);
}

static zend_object_value
qrcode_object_clone(zval *zobject TSRMLS_DC)
{
	zend_object_value new_obj_val;
	zend_object *old_object;
	zend_object *new_object;
	zend_object_handle handle = Z_OBJ_HANDLE_P(zobject);
	qrcode_object *object;
	qrcode_object *source;
	int errcode = QR_ERR_NONE;

	old_object = zend_objects_get_address(zobject TSRMLS_CC);
	source = (qrcode_object *)old_object;

	new_obj_val = qrcode_object_new_ex(&object, old_object->ce TSRMLS_CC);
	new_object = &object->std;

	zend_objects_clone_members(new_object, new_obj_val, old_object, handle TSRMLS_CC);

	memcpy((void *)object + sizeof(zend_object),
			(void *)source + sizeof(zend_object),
			sizeof(qrcode_object) - sizeof(zend_object));

	if (source->st) {
		object->qr = NULL;
		object->st = qrsClone(source->st, &errcode);
	} else {
		object->qr = qrClone(source->qr, &errcode);
		object->st = NULL;
	}

	return new_obj_val;
}

#define isFinalized(intern) ((intern)->pos != -1)

#define php_qr_error(errmode, errcode, errmsg) { \
	if (PHP_QR_ERRMODE_EXCEPTION == (errmode)) { \
		zend_throw_exception_ex(qrexception_ce, errcode TSRMLS_CC, "%s", (errmsg)); \
	} else if (PHP_QR_ERRMODE_WARNING == (errmode)) { \
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", (errmsg)); \
	} \
}

#define php_qr_error_from_object(intern) \
	php_qr_error((intern)->errmode, qrGetErrorCode((intern)->qr), qrGetErrorInfo((intern)->qr))

#define php_qr_error_from_object2(intern) \
	php_qr_error((intern)->errmode, qrsGetErrorCode((intern)->st), qrsGetErrorInfo((intern)->st))

#define warn_not_finalized() \
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "QRCode is not finalized yet")

#define throw_not_finalized() \
	zend_throw_exception(qrexception_ce, "QRCode is not finalized yet", QR_ERR_STATE TSRMLS_CC)

#define error_not_finalized(errmode) { \
	if (PHP_QR_ERRMODE_EXCEPTION == (errmode)) { \
		throw_not_finalized(); \
	} else if (PHP_QR_ERRMODE_WARNING == (errmode)) {\
		warn_not_finalized(); \
	} \
}

#define warn_already_finalized() \
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "QRCode is already finalized")

#define throw_already_finalized() \
	zend_throw_exception(qrexception_ce, "QRCode is already finalized", QR_ERR_STATE TSRMLS_CC)

#define error_already_finalized(errmode) { \
	if (PHP_QR_ERRMODE_EXCEPTION == (errmode)) { \
		throw_already_finalized(); \
	} else if (PHP_QR_ERRMODE_WARNING == (errmode)) {\
		warn_already_finalized(); \
	} \
}

static zend_function_entry qrcode_methods[] = {
	PHP_ME(QRCode, __construct,      arginfo_qrcode_ctor,           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(QRCode, setErrorHandling, arginfo_qrcode_set_eh,         ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, setFormat,        arginfo_qrcode_set_format,     ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, setSeparator,     arginfo_qrcode_set_separator,  ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, setMagnify,       arginfo_qrcode_set_magnify,    ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, setOrder,         arginfo_qrcode_set_order,      ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, addData,          arginfo_qrcode_add_data,       ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, readData,         arginfo_qrcode_read_data,      ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, finalize,         NULL,                          ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, getSymbol,        NULL,                          ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, outputSymbol,     arginfo_qrcode_output_symbol,  ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, getInfo,          NULL,                          ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, getMimeType,      NULL,                          ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, getExtension,     arginfo_qrcode_get_extension,  ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, current,          NULL,                          ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, key,              NULL,                          ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, next,             NULL,                          ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, rewind,           NULL,                          ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, valid,            NULL,                          ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, seek,             arginfo_qrcode_seek,           ZEND_ACC_PUBLIC)
	PHP_ME(QRCode, count,            NULL,                          ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL, 0, 0 }
};

/* }}} Class definitions*/

/* {{{ qr_functions[] */
static zend_function_entry qr_functions[] = {
	PHP_FALIAS(qrcode, qr_get_symbol,   arginfo_qr_get_symbol)
	PHP_FE(qr_get_symbol,       arginfo_qr_get_symbol)
	PHP_FE(qr_output_symbol,    arginfo_qr_output_symbol)
	PHP_FE(qr_mimetype,         arginfo_qr_mimetype)
	PHP_FE(qr_extension,        arginfo_qr_extension)
	PHP_FE_END
};
/* }}} */

/* {{{ cross-extension dependencies */

static zend_module_dep qr_deps[] = {
	ZEND_MOD_REQUIRED("spl")
	ZEND_MOD_END
};

/* }}} */

/* {{{ qr_module_entry
 */
zend_module_entry qr_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	qr_deps,
	"qr",
	qr_functions,
	PHP_MINIT(qr),
	PHP_MSHUTDOWN(qr),
	NULL,
	NULL,
	PHP_MINFO(qr),
	PHP_QR_MODULE_VERSION,
	PHP_MODULE_GLOBALS(qr),
	PHP_GINIT(qr),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_QR
ZEND_GET_MODULE(qr)
#endif

/* {{{ QrOnUpdateLongGEZero()
 */
#define QrOnUpdateLongGEZero OnUpdateLongGEZero
/* }}} */

/* {{{ QrOnUpdateLongGTZero()
 */
static ZEND_INI_MH(QrOnUpdateLongGTZero)
{
	long l;
	long *p;
	char *base;

#ifndef ZTS
	base = (char *)mh_arg2;
#else
	base = (char *)ts_resource(*((int *)mh_arg2));
#endif

	l = zend_atoi(new_value, new_value_length);
	if (l > 0) {
		p = (long *)(base + (size_t)mh_arg1);
		*p = l;
		return SUCCESS;
	}

	return FAILURE;
}
/* }}} */

/* {{{ QrOnUpdateVersion()
 */
static ZEND_INI_MH(QrOnUpdateVersion)
{
	long version;
	long *p;
	char *base;

#ifndef ZTS
	base = (char *)mh_arg2;
#else
	base = (char *)ts_resource(*((int *)mh_arg2));
#endif

	version = zend_atoi(new_value, new_value_length);
	if (version == -1 || (version >= 1 && version <= QR_VER_MAX)) {
		p = (long *)(base + (size_t)mh_arg1);
		*p = version;
		return SUCCESS;
	}

	return FAILURE;
}
/* }}} */

/* {{{ QrOnUpdateMode()
 */
static ZEND_INI_MH(QrOnUpdateMode)
{
	long *p;
	char *base;
	const char *value;
	uint value_length;

#ifndef ZTS
	base = (char *)mh_arg2;
#else
	base = (char *)ts_resource(*((int *)mh_arg2));
#endif

	p = (long *)(base + (size_t)mh_arg1);

	if (new_value_length > 6 && strncmp(new_value, "QR_EM_", 6) == 0) {
		value = new_value + 6;
		value_length = new_value_length - 6;
	} else {
		value = new_value;
		value_length = new_value_length;
	}

	if ((value_length == 4 && strcasecmp("auto", value) == 0) ||
		(value_length == 1 && (*value == '-' || *value == 'S' || *value == 's'))) {
		*p = QR_EM_AUTO;
	} else if ((value_length == 6 && strcasecmp("numeric", value) == 0) ||
		(value_length == 1 && (*value == 'N' || *value == 'n'))) {
		*p = QR_EM_NUMERIC;
	} else if ((value_length == 5 && strcasecmp("alnum", value) == 0) ||
		(value_length == 1 && (*value == 'A' || *value == 'a'))) {
		*p = QR_EM_ALNUM;
	} else if ((value_length == 4 && strcasecmp("8bit", value) == 0) ||
		(value_length == 1 && (*value == '8' || *value == 'B' || *value == 'b'))) {
		*p = QR_EM_8BIT;
	} else if ((value_length == 4 && strcasecmp("kanji", value) == 0) ||
		(value_length == 1 && (*value == 'K' || *value == 'k'))) {
		*p = QR_EM_KANJI;
	} else {
		long mode = zend_atoi(new_value, new_value_length);
		if (mode == QR_EM_AUTO || (mode >= QR_EM_NUMERIC && mode < QR_EM_COUNT)) {
			*p = mode;
		} else {
			return FAILURE;
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ QrOnUpdateEclevel()
 */
static ZEND_INI_MH(QrOnUpdateEclevel)
{
	long *p;
	char *base;
	const char *value;
	uint value_length;

#ifndef ZTS
	base = (char *)mh_arg2;
#else
	base = (char *)ts_resource(*((int *)mh_arg2));
#endif

	p = (long *)(base + (size_t)mh_arg1);

	if (new_value_length > 7 && strncmp(new_value, "QR_ECL_", 7) == 0) {
		value = new_value + 7;
		value_length = new_value_length - 7;
	} else {
		value = new_value;
		value_length = new_value_length;
	}

	if (value_length == 1 && (*value == 'l' || *value == 'L')) {
		*p = QR_ECL_L;
	} else if (value_length == 1 && (*value == 'm' || *value == 'M')) {
		*p = QR_ECL_M;
	} else if (value_length == 1 && (*value == 'q' || *value == 'Q')) {
		*p = QR_ECL_Q;
	} else if (value_length == 1 && (*value == 'h' || *value == 'H')) {
		*p = QR_ECL_H;
	} else {
		long eclevel = zend_atoi(new_value, new_value_length);
		if (eclevel >= QR_ECL_L && eclevel < QR_ECL_COUNT) {
			*p = eclevel;
		} else {
			return FAILURE;
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ QrOnUpdateMasktype()
 */
static ZEND_INI_MH(QrOnUpdateMasktype)
{
	long masktype;
	long *p;
	char *base;

#ifndef ZTS
	base = (char *)mh_arg2;
#else
	base = (char *)ts_resource(*((int *)mh_arg2));
#endif

	masktype = zend_atoi(new_value, new_value_length);
	if (masktype == -1 || (masktype >= 0 && masktype < QR_MPT_MAX)) {
		p = (long *)(base + (size_t)mh_arg1);
		*p = masktype;
		return SUCCESS;
	}

	return FAILURE;
}
/* }}} */

/* {{{ QrOnUpdateFormat()
 */
static ZEND_INI_MH(QrOnUpdateFormat)
{
	long *p;
	char *base;
	const char *value;
	uint value_length;

#ifndef ZTS
	base = (char *)mh_arg2;
#else
	base = (char *)ts_resource(*((int *)mh_arg2));
#endif

	p = (long *)(base + (size_t)mh_arg1);

	if (new_value_length > 7 && strncmp(new_value, "QR_FMT_", 7) == 0) {
		value = new_value + 7;
		value_length = new_value_length - 7;
	} else {
		value = new_value;
		value_length = new_value_length;
	}

	if ((value_length == 5 && strcasecmp("digit", value) == 0) ||
		(value_length == 2 && strcmp("01", value) == 0)) {
		*p = QR_FMT_DIGIT;
	} else if ((value_length == 5 && strcasecmp("ascii", value) == 0) ||
		(value_length == 8 && strcasecmp("asciiart", value) == 0) ||
		(value_length == 2 && strcasecmp("aa", value) == 0)) {
		*p = QR_FMT_ASCII;
	} else if ((value_length == 4 && strcasecmp("json", value) == 0) ||
		(value_length == 10 && strcasecmp("javascript", value) == 0) ||
		(value_length == 2 && strcasecmp("js", value) == 0)) {
		*p = QR_FMT_JSON;
	} else if (value_length == 3 && strcasecmp("pbm", value) == 0) {
		*p = QR_FMT_PBM;
	} else if (value_length == 3 && strcasecmp("bmp", value) == 0) {
		*p = QR_FMT_BMP;
	} else if (value_length == 3 && strcasecmp("svg", value) == 0) {
		*p = QR_FMT_SVG;
	} else if ((value_length == 4 && strcasecmp("tiff", value) == 0) ||
		(value_length == 3 && strcasecmp("tif", value) == 0)) {
		*p = QR_FMT_TIFF;
	} else if (value_length == 3 && strcasecmp("png", value) == 0) {
		*p = QR_FMT_PNG;
	} else {
		long foramt = zend_atoi(new_value, new_value_length);
		if (foramt >= 0 && foramt < QR_FMT_COUNT) {
			*p = foramt;
		} else {
			return FAILURE;
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ ini entries */

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("qr.default_version", "-1", PHP_INI_ALL,
		QrOnUpdateVersion, default_version, zend_qr_globals, qr_globals)
	STD_PHP_INI_ENTRY("qr.default_mode", "auto", PHP_INI_ALL,
		QrOnUpdateMode, default_mode, zend_qr_globals, qr_globals)
	STD_PHP_INI_ENTRY("qr.default_eclevel", "M", PHP_INI_ALL,
		QrOnUpdateEclevel, default_eclevel, zend_qr_globals, qr_globals)
	STD_PHP_INI_ENTRY("qr.default_masktype", "-1", PHP_INI_ALL,
		QrOnUpdateMasktype, default_masktype, zend_qr_globals, qr_globals)
	STD_PHP_INI_ENTRY("qr.default_format", "digit", PHP_INI_ALL,
		QrOnUpdateFormat, default_format, zend_qr_globals, qr_globals)
	STD_PHP_INI_ENTRY("qr.default_magnify", "1", PHP_INI_ALL,
		QrOnUpdateLongGTZero, default_magnify, zend_qr_globals, qr_globals)
	STD_PHP_INI_ENTRY("qr.default_separator", "4", PHP_INI_ALL,
		QrOnUpdateLongGEZero, default_separator, zend_qr_globals, qr_globals)
	STD_PHP_INI_ENTRY("qr.default_maxnum", "1", PHP_INI_ALL,
		QrOnUpdateLongGTZero, default_maxnum, zend_qr_globals, qr_globals)
	STD_PHP_INI_ENTRY("qr.default_order", "0", PHP_INI_ALL,
		OnUpdateLong, default_order, zend_qr_globals, qr_globals)
PHP_INI_END()

/* }}} */

#define QR_REGISTER_CONSTANT(name) \
	REGISTER_LONG_CONSTANT(#name, name, CONST_PERSISTENT | CONST_CS)

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(qr)
{
	REGISTER_INI_ENTRIES();
	php_register_info_logo(QR_LOGO_GUID, QR_LOGO_TYPE, qr_logo, QR_LOGO_SIZE);

	QR_REGISTER_CONSTANT(QR_EM_AUTO);
	QR_REGISTER_CONSTANT(QR_EM_NUMERIC);
	QR_REGISTER_CONSTANT(QR_EM_ALNUM);
	QR_REGISTER_CONSTANT(QR_EM_8BIT);
	QR_REGISTER_CONSTANT(QR_EM_KANJI);
	QR_REGISTER_CONSTANT(QR_ECL_L);
	QR_REGISTER_CONSTANT(QR_ECL_M);
	QR_REGISTER_CONSTANT(QR_ECL_Q);
	QR_REGISTER_CONSTANT(QR_ECL_H);
	QR_REGISTER_CONSTANT(QR_FMT_PNG);
	QR_REGISTER_CONSTANT(QR_FMT_BMP);
	QR_REGISTER_CONSTANT(QR_FMT_TIFF);
	QR_REGISTER_CONSTANT(QR_FMT_PBM);
	QR_REGISTER_CONSTANT(QR_FMT_SVG);
	QR_REGISTER_CONSTANT(QR_FMT_JSON);
	QR_REGISTER_CONSTANT(QR_FMT_DIGIT);
	QR_REGISTER_CONSTANT(QR_FMT_ASCII);

	/* fetch exception class entries */
	ext_ce_RuntimeException = _qr_get_class_entry("runtimeexception" TSRMLS_CC);
	if (ext_ce_RuntimeException == NULL) {
		php_error(E_ERROR, "qr module initialization failure: class RuntimeException not registered");
		return FAILURE;
	}
	ext_ce_OutOfBoundsException = _qr_get_class_entry("outofboundsexception" TSRMLS_CC);
	if (ext_ce_OutOfBoundsException == NULL) {
		php_error(E_ERROR, "qr module initialization failure: class OutOfBoundsException not registered");
		return FAILURE;
	}

	/* initalize class QRCode */
	{
		zend_class_entry ce;
		zend_class_entry *ce_SeekableIterator, *ce_Countable;

		ce_SeekableIterator = _qr_get_class_entry("seekableiterator" TSRMLS_CC);
		if (ce_SeekableIterator == NULL) {
			php_error(E_ERROR, "qr module initialization failure: class SeekableIterator not registered");
			return FAILURE;
		}
		ce_Countable = _qr_get_class_entry("countable" TSRMLS_CC);
		if (ce_Countable == NULL) {
			php_error(E_ERROR, "qr module initialization failure: class Countable not registered");
			return FAILURE;
		}

		INIT_CLASS_ENTRY(ce, "QRCode", qrcode_methods);
		qrcode_ce = zend_register_internal_class(&ce TSRMLS_CC);
		if (!qrcode_ce) {
			return FAILURE;
		}
		qrcode_ce->create_object = qrcode_object_new;
		memcpy(&qrcode_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
		qrcode_object_handlers.clone_obj = qrcode_object_clone;

		zend_class_implements(qrcode_ce TSRMLS_CC, 2, ce_SeekableIterator, ce_Countable);

#define QRCODE_DECLARE_CONSTANT(name) \
		zend_declare_class_constant_long(qrcode_ce, #name, sizeof(#name) - 1, (long)QR_ ## name TSRMLS_CC)

		QRCODE_DECLARE_CONSTANT(EM_AUTO);
		QRCODE_DECLARE_CONSTANT(EM_NUMERIC);
		QRCODE_DECLARE_CONSTANT(EM_ALNUM);
		QRCODE_DECLARE_CONSTANT(EM_8BIT);
		QRCODE_DECLARE_CONSTANT(EM_KANJI);
		QRCODE_DECLARE_CONSTANT(ECL_L);
		QRCODE_DECLARE_CONSTANT(ECL_M);
		QRCODE_DECLARE_CONSTANT(ECL_Q);
		QRCODE_DECLARE_CONSTANT(ECL_H);
		QRCODE_DECLARE_CONSTANT(FMT_PNG);
		QRCODE_DECLARE_CONSTANT(FMT_BMP);
		QRCODE_DECLARE_CONSTANT(FMT_TIFF);
		QRCODE_DECLARE_CONSTANT(FMT_PBM);
		QRCODE_DECLARE_CONSTANT(FMT_SVG);
		QRCODE_DECLARE_CONSTANT(FMT_JSON);
		QRCODE_DECLARE_CONSTANT(FMT_DIGIT);
		QRCODE_DECLARE_CONSTANT(FMT_ASCII);

		zend_declare_class_constant_long(qrcode_ce,
				"ERRMODE_SILENT", sizeof("ERRMODE_SILENT") - 1,
				PHP_QR_ERRMODE_SILENT TSRMLS_CC);

		zend_declare_class_constant_long(qrcode_ce,
				"ERRMODE_WARNING", sizeof("ERRMODE_WARNING") - 1,
				PHP_QR_ERRMODE_WARNING TSRMLS_CC);

		zend_declare_class_constant_long(qrcode_ce,
				"ERRMODE_EXCEPTION", sizeof("ERRMODE_EXCEPTION") - 1,
				PHP_QR_ERRMODE_EXCEPTION TSRMLS_CC);
	}

	/* initalize class QRException */
	{
		zend_class_entry ce;
		INIT_CLASS_ENTRY(ce, "QRException", NULL);
		qrexception_ce = zend_register_internal_class_ex(&ce,
				ext_ce_RuntimeException, NULL TSRMLS_CC);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(qr)
{
	UNREGISTER_INI_ENTRIES();
	php_unregister_info_logo(QR_LOGO_GUID);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(qr)
{
	php_info_print_box_start(0);
	if (!sapi_module.phpinfo_as_text) {
		php_printf("<img src=\"");
		if (INI_INT("expose_php")) {
			if (SG(request_info).request_uri) {
				php_printf("%s", (SG(request_info).request_uri));
			}
			php_printf("?=%s", QR_LOGO_GUID);
		} else {
			php_printf("data:%s;base64,%s", QR_LOGO_TYPE, QR_LOGO_B64);
		}
		php_printf("\" alt=\"QR Code\" border=\"0\" />");
		php_printf("<p><strong>QR Code Generator Extension</strong></p>");
		php_printf("<p>QR Code &reg; is registered trademarks of DENSO WAVE INCORPORATED in JAPAN and other countries.</p>");
	} else {
		php_printf("QR Code Generator Extension");
#ifdef PHP_WIN32
		php_printf("\r\n");
#else
		php_printf("\n");
#endif
		php_printf("QR Code (R) is registered trademarks of DENSO WAVE INCORPORATED in JAPAN and other countries.");
	}
	php_info_print_box_end();

	php_info_print_table_start();
	php_info_print_table_row(2, "Module Version", PHP_QR_MODULE_VERSION);
	php_info_print_table_row(2, "Library Version", LIBQR_VERSION);
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ PHP_GINIT_FUNCTION */
static PHP_GINIT_FUNCTION(qr)
{
	qr_globals->default_version = -1;
	qr_globals->default_mode = QR_EM_AUTO;
	qr_globals->default_eclevel = QR_ECL_M;
	qr_globals->default_masktype = -1;
	qr_globals->default_format = QR_FMT_DIGIT;
	qr_globals->default_magnify = 1;
	qr_globals->default_separator = 4;
	qr_globals->default_maxnum = 1;
	qr_globals->default_order = 0;
}
/* }}} */

/* {{{ _qr_parse_options()
 * parse the options
 */
static void
_qr_parse_options(HashTable *options,
		int *version, int *mode, int *eclevel, int *masktype,
		int *format, int *magnify, int *separator,
		int *maxnum, int *order TSRMLS_DC)
{
	zval tmp, **entry;

#define QR_FETCH_OPTION(name) \
	if (zend_hash_find(options, #name, sizeof(#name), (void **)&entry) == SUCCESS) { \
		if (name != NULL) { \
			if (Z_TYPE_PP(entry) == IS_LONG) { \
				*name = (int)Z_LVAL_PP(entry); \
			} else { \
				tmp = **entry; \
				zval_copy_ctor(&tmp); \
				convert_to_long(&tmp); \
				*name = (int)Z_LVAL(tmp); \
			} \
		} \
	}

	QR_FETCH_OPTION(version)
	QR_FETCH_OPTION(mode)
	QR_FETCH_OPTION(eclevel)
	QR_FETCH_OPTION(masktype)
	QR_FETCH_OPTION(format)
	QR_FETCH_OPTION(magnify)
	QR_FETCH_OPTION(separator)
	QR_FETCH_OPTION(maxnum)
	QR_FETCH_OPTION(order)

#undef QR_FETCH_OPTION
}
/* }}} */

/* {{{ _qr_create_simple()
 * initialize a standard QR Code, add data, finalize and return it
 */
static QRCode *
_qr_create_simple(const qr_byte_t *data, int data_len,
		int version, int mode, int eclevel, int masktype TSRMLS_DC)
{
	QRCode *qr = NULL;
	int errcode = QR_ERR_NONE;

	qr = qrInit(version, mode, eclevel, masktype, &errcode);
	if (!qr) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", qrStrError(errcode));
		return NULL;
	}

	if (!qrAddData2(qr, data, data_len, mode)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", qrGetErrorInfo(qr));
		qrDestroy(qr);
		return NULL;
	}

	if (!qrFinalize(qr)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", qrGetErrorInfo(qr));
		qrDestroy(qr);
		return NULL;
	}

	return qr;
}
/* }}} */

/* {{{ _qrs_create_simple()
 * initialize a structured append QR Code, add data, finalize and return it
 */
static QRStructured *
_qrs_create_simple(const qr_byte_t *data, int data_len,
		int version, int mode, int eclevel, int masktype, int maxnum TSRMLS_DC)
{
	QRStructured *st = NULL;
	int errcode = QR_ERR_NONE;

	st = qrsInit(version, mode, eclevel, masktype, maxnum, &errcode);
	if (!st) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", qrStrError(errcode));
		return NULL;
	}

	if (!qrsAddData2(st, data, data_len, mode)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", qrsGetErrorInfo(st));
		qrsDestroy(st);
		return NULL;
	}

	if (!qrsFinalize(st)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", qrsGetErrorInfo(st));
		qrsDestroy(st);
		return NULL;
	}

	return st;
}
/* }}} */

/* {{{ _qr_get_symbol()
 * get the QR Code symbol
 */
static qr_byte_t *
_qr_get_symbol(const qr_byte_t *data, int data_len,
		int version, int mode, int eclevel, int masktype,
		int format, int separator, int magnify,
		int *symbol_size TSRMLS_DC)
{
	QRCode *qr = NULL;
	qr_byte_t *symbol = NULL;

	qr = _qr_create_simple(data, data_len, version, mode, eclevel, masktype TSRMLS_CC);
	if (!qr) {
		return NULL;
	}

	symbol = qrGetSymbol(qr, format, separator, magnify, symbol_size);
	if (!symbol) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", qrGetErrorInfo(qr));
		qrDestroy(qr);
		return NULL;
	}

	qrDestroy(qr);

	return symbol;
}
/* }}} */

/* {{{ _qrs_get_symbols()
 * get the structured append QR Code symbol
 */
static qr_byte_t *
_qrs_get_symbols(const qr_byte_t *data, int data_len,
		int version, int mode, int eclevel, int masktype, int maxnum,
		int format, int separator, int magnify, int order,
		int *symbol_size TSRMLS_DC)
{
	QRStructured *st = NULL;
	qr_byte_t *symbol = NULL;

	st = _qrs_create_simple(data, data_len, version, mode, eclevel, masktype, maxnum TSRMLS_CC);
	if (!st) {
		return NULL;
	}

	symbol = qrsGetSymbols(st, format, separator, magnify, order, symbol_size);
	if (!symbol) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", qrsGetErrorInfo(st));
		qrsDestroy(st);
		return NULL;
	}

	qrsDestroy(st);

	return symbol;
}
/* }}} */

/* {{{ _qr_stream_open()
 * open the stream
 *
 * @param   zval *zv            The value container which contains
 *                              the stream resource or the pathname.
 * @param   char *alt_url       The alternative URL which is opened
 *                              when 'zv' is empty string or NULL.
 * @param   char *mode          The stream open mode, mostly will be "wb" or "rb".
 * @param   char **opened_path  The address which will be set the opened pathname.
 * @param   const char *title   The name of the stream which
 *                              is printed on the error message.
 */
static php_stream *
_qr_stream_open(zval *zv, char *alt_url, char *mode,
		char **opened_path, const char *title TSRMLS_DC)
{
	static int options = ENFORCE_SAFE_MODE | REPORT_ERRORS;
	php_stream *stream = NULL;
	zend_bool open_alt_url = 0;

	/* open or get the stream */
	if (!zv) {
		open_alt_url = 1;
	} else {
		switch (Z_TYPE_P(zv)) {
		  case IS_RESOURCE:
			php_stream_from_zval_no_verify(stream, &zv);
			break;
		  case IS_STRING:
			if (Z_STRLEN_P(zv) > 0) {
				stream = php_stream_open_wrapper(Z_STRVAL_P(zv),
						mode, IGNORE_URL | options, opened_path);
			} else {
				open_alt_url = 1;
			}
			break;
		  case IS_NULL:
			open_alt_url = 1;
			break;
		}
	}

	/* open the stream from alternative URL */
	if (open_alt_url && alt_url) {
		stream = php_stream_open_wrapper(alt_url, mode, options, opened_path);
	}

	/* on failure */
	if (!stream) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING,
				"%s should be a valid filename or a stream resource",
				(title ? title : "parameter"));
		return NULL;
	}

	return stream;
}
/* }}} */

/* {{{ _qr_get_class_entry()
 * get the class entry
 */
static zend_class_entry *
_qr_get_class_entry(char *class_name TSRMLS_DC)
{
	zend_class_entry **pce;

	if (class_name == NULL || *class_name == '\0' ||
		zend_hash_find(CG(class_table), class_name, strlen(class_name) + 1, (void **)&pce) == FAILURE)
	{
		return NULL;
	} else {
		return *pce;
	}
}
/* }}} */

/* {{{ proto string qr_get_symbol(string data[, array options])
   */
static PHP_FUNCTION(qr_get_symbol)
{
	char *data = NULL;
	int data_len = 0;
	zval *options = NULL;

	int version = QR_DEFAULT(version);
	int mode = QR_DEFAULT(mode);
	int eclevel = QR_DEFAULT(eclevel);
	int masktype = QR_DEFAULT(masktype);
	int format = QR_DEFAULT(format);
	int magnify = QR_DEFAULT(magnify);
	int separator = QR_DEFAULT(separator);
	int maxnum = QR_DEFAULT(maxnum);
	int order = QR_DEFAULT(order);

	qr_byte_t *symbol = NULL;
	int symbol_size = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a",
			&data, &data_len, &options) == FAILURE)
	{
		return;
	}

	if (options) {
		qr_parse_options(Z_ARRVAL_P(options), &version, &mode, &eclevel, &masktype,
				&format, &magnify, &separator, &maxnum, &order);
	}

	if (maxnum == 1) {
		symbol = _qr_get_symbol((qr_byte_t *)data, data_len,
				version, mode, eclevel, masktype,
				format, separator, magnify,
				&symbol_size TSRMLS_CC);
	} else {
		if (version == -1) {
			version = 1;
		}
		symbol = _qrs_get_symbols((qr_byte_t *)data, data_len,
				version, mode, eclevel, masktype, maxnum,
				format, separator, magnify, order,
				&symbol_size TSRMLS_CC);
	}

	if (!symbol) {
		RETURN_FALSE;
	}

	ZVAL_STRINGL(return_value, (char *)symbol, symbol_size, 1);
	free(symbol);
}
/* }}} qr_get_symbol */

/* {{{ proto int qr_output_symbol(mixed output, string data[, array options])
   */
static PHP_FUNCTION(qr_output_symbol)
{
	zval *zoutput = NULL;
	php_stream *output = NULL;
	char *data = NULL;
	int data_len = 0;
	zval *options = NULL;

	int version = QR_DEFAULT(version);
	int mode = QR_DEFAULT(mode);
	int eclevel = QR_DEFAULT(eclevel);
	int masktype = QR_DEFAULT(masktype);
	int format = QR_DEFAULT(format);
	int magnify = QR_DEFAULT(magnify);
	int separator = QR_DEFAULT(separator);
	int maxnum = QR_DEFAULT(maxnum);
	int order = QR_DEFAULT(order);

	qr_byte_t *symbol = NULL;
	int symbol_size = 0;
	size_t output_size = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zs|a",
			&zoutput, &data, &data_len, &options) == FAILURE)
	{
		return;
	}

	if (options) {
		qr_parse_options(Z_ARRVAL_P(options), &version, &mode, &eclevel, &masktype,
				&format, &magnify, &separator, &maxnum, &order);
	}

	if (maxnum == 1) {
		symbol = _qr_get_symbol((qr_byte_t *)data, data_len,
				version, mode, eclevel, masktype,
				format, separator, magnify,
				&symbol_size TSRMLS_CC);
	} else {
		if (version == -1) {
			version = 1;
		}
		symbol = _qrs_get_symbols((qr_byte_t *)data, data_len,
				version, mode, eclevel, masktype, maxnum,
				format, separator, magnify, order,
				&symbol_size TSRMLS_CC);
	}

	if (!symbol) {
		RETURN_FALSE;
	}

	output = qr_get_output_stream(zoutput);
	if (!output) {
		free(symbol);
		RETURN_FALSE;
	}

	output_size = php_stream_write(output, (const char *)symbol, (size_t)symbol_size);

	if (zoutput != NULL && Z_TYPE_P(zoutput) != IS_RESOURCE) {
		php_stream_close(output);
	}
	free(symbol);

	if (output_size != (size_t)symbol_size) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", qrStrError(QR_ERR_FWRITE));
		RETURN_FALSE;
	}

	RETURN_LONG((long)output_size);
}
/* }}} qr_output_symbol */

/* {{{ proto string qr_mimetype(int format)
   */
static PHP_FUNCTION(qr_mimetype)
{
	long format = 0;

	const char *mimetype = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
			&format) == FAILURE)
	{
		return;
	}

	mimetype = qrMimeType((int)format);
	if (!mimetype) {
		RETURN_FALSE;
	}
	RETURN_STRING((char *)mimetype, 1);
}
/* }}} qr_mimetype */

/* {{{ proto string qr_extension(int format[, bool include_dot])
   */
static PHP_FUNCTION(qr_extension)
{
	long format = 0;
	zend_bool include_dot = 1;

	const char *extension = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|b",
			&format, &include_dot) == FAILURE)
	{
		return;
	}

	extension = qrExtension((int)format);
	if (!extension) {
		RETURN_FALSE;
	}
	if (include_dot) {
		char extension_buffer[QR_EXT_MAX_LEN + 2] = {0};
		snprintf(&(extension_buffer[0]), QR_EXT_MAX_LEN + 2, ".%s", extension);
		RETURN_STRING(extension_buffer, 1);
	} else {
		RETURN_STRING((char *)extension, 1);
	}
}
/* }}} qr_extension */

/* {{{ proto void QRCode::__construct([array options])
   */
static PHP_METHOD(QRCode, __construct)
{
	qrcode_object *intern = NULL;
	zval *options = NULL;
#if PHP_VERSION_ID >= 50300
	zend_error_handling error_handling;
#endif

	int version = QR_DEFAULT(version);
	int mode = QR_DEFAULT(mode);
	int eclevel = QR_DEFAULT(eclevel);
	int masktype = QR_DEFAULT(masktype);
	int maxnum = QR_DEFAULT(maxnum);
	int errcode = QR_ERR_NONE;

#if PHP_VERSION_ID >= 50300
	zend_replace_error_handling(EH_THROW, qrexception_ce, &error_handling TSRMLS_CC);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a",
			&options) == FAILURE)
	{
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}
	zend_restore_error_handling(&error_handling TSRMLS_CC);
#else
	php_set_error_handling(EH_THROW, qrexception_ce TSRMLS_CC);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a",
			&options) == FAILURE)
	{
		php_std_error_handling();
		return;
	}
	php_std_error_handling();
#endif

	intern = qrcode_get_object();
	if (intern->qr != NULL && intern->st != NULL) {
		php_qr_error(PHP_QR_ERRMODE_EXCEPTION, 0, "QRCode already initialized");
		return;
	}

	intern->format = QR_DEFAULT(format);
	intern->magnify = QR_DEFAULT(magnify);
	intern->separator = QR_DEFAULT(separator);
	intern->order = QR_DEFAULT(order);

	if (options) {
		qr_parse_options(Z_ARRVAL_P(options),
				&version, &mode, &eclevel, &masktype,
				&(intern->format), &(intern->magnify), &(intern->separator),
				&maxnum, &(intern->order));
	}

	if (maxnum == 1) {
		intern->qr = qrInit(version, mode, eclevel, masktype, &errcode);
		if (!intern->qr) {
			php_qr_error(PHP_QR_ERRMODE_EXCEPTION, errcode, qrStrError(errcode));
			return;
		}
	} else {
		if (version == -1) {
			version = 1;
		}
		intern->st = qrsInit(version, mode, eclevel, masktype, maxnum, &errcode);
		if (!intern->st) {
			php_qr_error(PHP_QR_ERRMODE_EXCEPTION, errcode, qrStrError(errcode));
			return;
		}
	}
}
/* }}} QRCode::__construct */

/* {{{ proto void QRCode::setErrorHandling(int errmode)
   */
static PHP_METHOD(QRCode, setErrorHandling)
{
	qrcode_object *intern = NULL;
	int errmode = PHP_QR_ERRMODE_EXCEPTION;
#if PHP_VERSION_ID >= 50300
	zend_error_handling error_handling;
#endif

	intern = qrcode_get_object();

#if PHP_VERSION_ID >= 50300
	zend_replace_error_handling(EH_THROW, qrexception_ce, &error_handling TSRMLS_CC);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
			&errmode) == FAILURE)
	{
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}
	zend_restore_error_handling(&error_handling TSRMLS_CC);
#else
	php_set_error_handling(EH_THROW, qrexception_ce TSRMLS_CC);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
			&errmode) == FAILURE)
	{
		php_std_error_handling();
		return;
	}
	php_std_error_handling();
#endif

	switch (errmode) {
	  case PHP_QR_ERRMODE_SILENT:
	  case PHP_QR_ERRMODE_WARNING:
	  case PHP_QR_ERRMODE_EXCEPTION:
		intern->errmode = errmode;
		break;
	  default:
		zend_throw_exception(qrexception_ce, "Invalid error mode given",
				QR_ERR_INVALID_ARG TSRMLS_CC);
	}
}
/* }}} QRCode::setErrorHandling */

/* {{{ proto void QRCode::setFormat(int format)
   */
static PHP_METHOD(QRCode, setFormat)
{
	qrcode_object *intern = NULL;
	long format = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
			&format) == FAILURE)
	{
		return;
	}

	intern = qrcode_get_object();
	intern->format = (int)format;
}
/* }}} QRCode::setFormat */

/* {{{ proto void QRCode::setSeparator(int separator)
   */
static PHP_METHOD(QRCode, setSeparator)
{
	qrcode_object *intern = NULL;
	long separator = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
			&separator) == FAILURE)
	{
		return;
	}

	intern = qrcode_get_object();
	intern->separator = (int)separator;
}
/* }}} QRCode::setSeparator */

/* {{{ proto void QRCode::setMagnify(int magnify)
   */
static PHP_METHOD(QRCode, setMagnify)
{
	qrcode_object *intern = NULL;
	long magnify = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
			&magnify) == FAILURE)
	{
		return;
	}

	intern = qrcode_get_object();
	intern->magnify = (int)magnify;
}
/* }}} QRCode::setMagnify */

/* {{{ proto void QRCode::setOrder(int order)
   */
static PHP_METHOD(QRCode, setOrder)
{
	qrcode_object *intern = NULL;
	long order = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
			&order) == FAILURE)
	{
		return;
	}

	intern = qrcode_get_object();
	if (intern->st) {
		intern->order = (int)order;
	}
}
/* }}} QRCode::setOrder */

/* {{{ _qr_add_data()
   */
static int
_qr_add_data(qrcode_object *object, qr_byte_t *data, int data_len, long mode TSRMLS_DC)
{
	int result = FALSE;

	if (object->st) {
		if (mode == PHP_QR_USE_DEFAULT_MODE) {
			result = qrsAddData(object->st, data, data_len);
		} else {
			result = qrsAddData2(object->st, data, data_len, (int)mode);
		}
		if (!result) {
			php_qr_error_from_object2(object);
			return FALSE;
		}
	} else {
		if (mode == PHP_QR_USE_DEFAULT_MODE) {
			result = qrAddData(object->qr, data, data_len);
		} else {
			result = qrAddData2(object->qr, data, data_len, (int)mode);
		}
		if (!result) {
			php_qr_error_from_object(object);
			return FALSE;
		}
	}

	return result;
}
/* }}} */

/* {{{ proto int QRCode::addData(string data[, int mode])
   */
static PHP_METHOD(QRCode, addData)
{
	qrcode_object *intern = NULL;
	char *data = NULL;
	int data_len = 0;
	long mode = PHP_QR_USE_DEFAULT_MODE;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l",
			&data, &data_len, &mode) == FAILURE)
	{
		return;
	}

	intern = qrcode_get_object();
	if (isFinalized(intern)) {
		error_already_finalized(intern->errmode);
		RETURN_FALSE;
	}

	if (!_qr_add_data(intern, (qr_byte_t *)data, data_len, mode TSRMLS_CC)) {
		RETURN_FALSE;
	}

	RETURN_LONG((long)data_len);
}
/* }}} QRCode::addData */

/* {{{ proto int QRCode::readData(mixed input[, int mode])
   */
static PHP_METHOD(QRCode, readData)
{
	qrcode_object *intern = NULL;
	zval *zinput = NULL;
	php_stream *input = NULL;
	long mode = PHP_QR_USE_DEFAULT_MODE;

	char *data = NULL;
	size_t data_len = 0;
	size_t buf_size = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|l",
			&zinput, &mode) == FAILURE)
	{
		return;
	}

	intern = qrcode_get_object();
	if (isFinalized(intern)) {
		error_already_finalized(intern->errmode);
		RETURN_FALSE;
	}

	input = qr_get_input_stream(zinput);
	if (!input) {
		RETURN_FALSE;
	}

	/* add +1 to check if the input data size is larger than the limit. */
	buf_size = (intern->st ? (QR_SRC_MAX * QR_STA_MAX) : QR_SRC_MAX) + 1;
	data = (char *)emalloc(buf_size);
	if (!data) {
		if (zinput != NULL && Z_TYPE_P(zinput) != IS_RESOURCE) {
			php_stream_close(input);
		}
		return;
	}

	data_len = php_stream_read(input, data, buf_size);

	if (zinput != NULL && Z_TYPE_P(zinput) != IS_RESOURCE) {
		php_stream_close(input);
	}

	if (!_qr_add_data(intern, (qr_byte_t *)data, (int)data_len, mode TSRMLS_CC)) {
		efree(data);
		RETURN_FALSE;
	}

	efree(data);

	RETURN_LONG((long)data_len);
}
/* }}} QRCode::readData */

/* {{{ proto bool QRCode::finalize(void)
   */
static PHP_METHOD(QRCode, finalize)
{
	qrcode_object *intern = NULL;

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	intern = qrcode_get_object();
	if (isFinalized(intern)) {
		RETURN_TRUE;
	}

	if (intern->st) {
		if (!qrsFinalize(intern->st)) {
			php_qr_error_from_object2(intern);
			RETURN_FALSE;
		}
	} else {
		if (!qrFinalize(intern->qr)) {
			php_qr_error_from_object(intern);
			RETURN_FALSE;
		}
	}

	qrcode_seek_noverify(intern, 0);

	RETURN_TRUE;
}
/* }}} QRCode::valid */

/* {{{ proto string QRCode::getSymbol(void)
   */
static PHP_METHOD(QRCode, getSymbol)
{
	qrcode_object *intern = NULL;

	qr_byte_t *symbol = NULL;
	int symbol_size = 0;

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	intern = qrcode_get_object();
	if (!isFinalized(intern)) {
		error_not_finalized(intern->errmode);
		return;
	}

	if (intern->st) {
		symbol = qrsGetSymbols(intern->st, intern->format,
				intern->separator, intern->magnify, intern->order, &symbol_size);
		if (!symbol) {
			php_qr_error_from_object2(intern);
			RETURN_FALSE;
		}
	} else {
		symbol = qrGetSymbol(intern->qr, intern->format,
				intern->separator, intern->magnify, &symbol_size);
		if (!symbol) {
			php_qr_error_from_object(intern);
			RETURN_FALSE;
		}
	}

	ZVAL_STRINGL(return_value, (char *)symbol, symbol_size, 1);
	free(symbol);
}
/* }}} QRCode::getSymbol */

/* {{{ proto int QRCode::outputSymbol([mixed output])
   */
static PHP_METHOD(QRCode, outputSymbol)
{
	qrcode_object *intern = NULL;
	zval *zoutput = NULL;
	php_stream *output = NULL;

	qr_byte_t *symbol = NULL;
	int symbol_size = 0;
	size_t output_size = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z",
			&zoutput) == FAILURE)
	{
		return;
	}

	intern = qrcode_get_object();
	if (!isFinalized(intern)) {
		error_not_finalized(intern->errmode);
		return;
	}

	if (intern->st) {
		symbol = qrsGetSymbols(intern->st, intern->format,
				intern->separator, intern->magnify, intern->order, &symbol_size);
		if (!symbol) {
			php_qr_error_from_object2(intern);
			RETURN_FALSE;
		}
	} else {
		symbol = qrGetSymbol(intern->qr, intern->format,
				intern->separator, intern->magnify, &symbol_size);
		if (!symbol) {
			php_qr_error_from_object(intern);
			RETURN_FALSE;
		}
	}

	output = qr_get_output_stream(zoutput);
	if (!output) {
		free(symbol);
		RETURN_FALSE;
	}

	output_size = php_stream_write(output, (const char *)symbol, (size_t)symbol_size);

	if (zoutput != NULL && Z_TYPE_P(zoutput) != IS_RESOURCE) {
		php_stream_close(output);
	}
	free(symbol);

	if (output_size != (size_t)symbol_size) {
		php_qr_error(intern->errmode, QR_ERR_FWRITE, qrStrError(QR_ERR_FWRITE));
		RETURN_FALSE;
	}

	RETURN_LONG((long)output_size);
}
/* }}} QRCode::outputSymbol */

/* {{{ proto array QRCode::getInfo(void)
   */
static PHP_METHOD(QRCode, getInfo)
{
	qrcode_object *intern = NULL;

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	intern = qrcode_get_object();
	array_init(return_value);

	if (intern->st) {
		add_assoc_bool(return_value, "structured",  1);
		add_assoc_long(return_value, "max_symbols", intern->st->max);
		add_assoc_long(return_value, "num_symbols", intern->st->num);
		add_assoc_long(return_value, "version",     intern->st->param.version);
		add_assoc_long(return_value, "mode",        intern->st->param.mode);
		add_assoc_long(return_value, "eclevel",     intern->st->param.eclevel);
		add_assoc_long(return_value, "masktype",    intern->st->param.masktype);
	} else {
		add_assoc_bool(return_value, "structured",  0);
		add_assoc_long(return_value, "max_symbols", 1);
		add_assoc_long(return_value, "num_symbols", 1);
		add_assoc_long(return_value, "version",     intern->qr->param.version);
		add_assoc_long(return_value, "mode",        intern->qr->param.mode);
		add_assoc_long(return_value, "eclevel",     intern->qr->param.eclevel);
		add_assoc_long(return_value, "masktype",    intern->qr->param.masktype);
	}
	add_assoc_long(return_value, "errmode",   intern->errmode);
	add_assoc_long(return_value, "format",    intern->format);
	add_assoc_long(return_value, "separator", intern->separator);
	add_assoc_long(return_value, "magnify",   intern->magnify);
	add_assoc_long(return_value, "order",     intern->order);
}
/* }}} QRCode::getInfo */

/* {{{ proto string QRCode::getMimeType(void)
   */
static PHP_METHOD(QRCode, getMimeType)
{
	qrcode_object *intern = NULL;

	const char *mimetype = NULL;

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	intern = qrcode_get_object();
	mimetype = qrMimeType(intern->format);
	if (!mimetype) {
		RETURN_FALSE;
	}
	RETURN_STRING((char *)mimetype, 1);
}
/* }}} QRCode::getMimeType */

/* {{{ proto string QRCode::getExtension([bool include_dot])
   */
static PHP_METHOD(QRCode, getExtension)
{
	zend_bool include_dot = 1;

	qrcode_object *intern = NULL;

	const char *extension = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b",
			&include_dot) == FAILURE)
	{
		return;
	}

	intern = qrcode_get_object();
	extension = qrExtension(intern->format);
	if (!extension) {
		RETURN_FALSE;
	}
	if (include_dot) {
		char extension_buffer[QR_EXT_MAX_LEN + 2] = {0};
		snprintf(&(extension_buffer[0]), QR_EXT_MAX_LEN + 2, ".%s", extension);
		RETURN_STRING(extension_buffer, 1);
	} else {
		RETURN_STRING((char *)extension, 1);
	}
}
/* }}} QRCode::getExtension */

/* {{{ proto string QRCode::current(void)
   */
static PHP_METHOD(QRCode, current)
{
	qrcode_object *intern = NULL;

	qr_byte_t *symbol = NULL;
	int symbol_size = 0;

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	intern = qrcode_get_object();
	if (!isFinalized(intern)) {
		error_not_finalized(intern->errmode);
		return;
	}

	symbol = qrGetSymbol(intern->qr,
			intern->format, intern->separator, intern->magnify, &symbol_size);
	if (!symbol) {
		php_qr_error_from_object(intern);
		RETURN_FALSE;
	}

	ZVAL_STRINGL(return_value, (char *)symbol, symbol_size, 1);
	free(symbol);
}
/* }}} QRCode::current */

/* {{{ proto int QRCode::key(void)
   */
static PHP_METHOD(QRCode, key)
{
	qrcode_object *intern = NULL;

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	intern = qrcode_get_object();
	if (!isFinalized(intern)) {
		error_not_finalized(intern->errmode);
		return;
	}

	RETURN_LONG((long)intern->pos);
}
/* }}} QRCode::key */

/* {{{ proto void QRCode::next(void)
   */
static PHP_METHOD(QRCode, next)
{
	qrcode_object *intern = NULL;

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	intern = qrcode_get_object();
	if (!isFinalized(intern)) {
		error_not_finalized(intern->errmode);
		return;
	}

	qrcode_seek_noverify(intern, intern->pos + 1);
}
/* }}} QRCode::next */

/* {{{ proto void QRCode::rewind(void)
   */
static PHP_METHOD(QRCode, rewind)
{
	qrcode_object *intern = NULL;

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	intern = qrcode_get_object();
	if (!isFinalized(intern)) {
		error_not_finalized(intern->errmode);
	}

	qrcode_seek_noverify(intern, 0);
}
/* }}} QRCode::rewind */

/* {{{ proto bool QRCode::valid(void)
   */
static PHP_METHOD(QRCode, valid)
{
	qrcode_object *intern = NULL;

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	intern = qrcode_get_object();
	if (!isFinalized(intern)) {
		error_not_finalized(intern->errmode);
	}

	if (intern->st) {
		RETURN_BOOL(intern->pos >= 0 && intern->pos < intern->st->num);
	} else {
		RETURN_BOOL(intern->pos == 0);
	}
}
/* }}} QRCode::valid */

/* {{{ proto void QRCode::seek(int position)
   */
static PHP_METHOD(QRCode, seek)
{
	long position = 0;

	qrcode_object *intern = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
			&position) == FAILURE)
	{
		return;
	}

	intern = qrcode_get_object();
	if (!isFinalized(intern)) {
		error_not_finalized(intern->errmode);
	}

	if (qrcode_seek(intern, (int)position) == FAILURE) {
		if (intern->st == NULL || intern->st->num == 1) {
			zend_throw_exception_ex(ext_ce_OutOfBoundsException, 0 TSRMLS_CC,
					"Seek position should be zero: %ld given", position);
		} else {
			zend_throw_exception_ex(ext_ce_OutOfBoundsException, 0 TSRMLS_CC,
					"Seek position should be in range from 0 to %d: %ld given",
					intern->st->num - 1, position);
		}
	}
}
/* }}} QRCode::seek */

/* {{{ proto int QRCode::count(void)
   */
static PHP_METHOD(QRCode, count)
{
	qrcode_object *intern = NULL;

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	intern = qrcode_get_object();

	if (intern->st) {
		RETURN_LONG(intern->st->num);
	} else {
		RETURN_LONG(1);
	}
}
/* }}} QRCode::count */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
