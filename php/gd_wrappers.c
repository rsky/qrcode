/*
 * QR Code generator extension for PHP: GD's function wrappers
 *
 * Copyright (c) 2007-2010 Ryusuke SEKIYAMA. All rights reserved.
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
 * @copyright   2007-2010 Ryusuke SEKIYAMA
 * @license     http://www.opensource.org/licenses/mit-license.php  MIT License
 */

#include "gd_wrappers.h"
#if PHP_VERSION_ID < 50300
#include <stdarg.h>
#endif
#include <main/php_output.h>

#ifndef PHP_QR_GD_WRAPPERS_DEBUG
#define PHP_QR_GD_WRAPPERS_DEBUG 0
#endif

/* {{{ globals */

ZEND_EXTERN_MODULE_GLOBALS(qr);
static int le_fake_rsrc, le_gd;

/* }}} */
/* {{{ internal function prototypes */

static zval *
_qr_init_args(int argc TSRMLS_DC, ...);

static void *
_qr_output_capture(qr_fcall_info *info, zval *args, int *size TSRMLS_DC);

static void
_rsrc_to_fake(zval **rsrc TSRMLS_DC);

/* }}} */
/* {{{ _qr_wrappers_init() */

PHP_QR_LOCAL int
_qr_wrappers_init(INIT_FUNC_ARGS)
{
	le_fake_rsrc = zend_register_list_destructors(NULL, NULL, module_number);
	le_gd = phpi_get_le_gd();

	return le_fake_rsrc;
}

/* }}} */
/* {{{ _qr_gdImageCreate() */

PHP_QR_LOCAL gdImagePtr
_qr_gdImageCreate(int sx, int sy)
{
	TSRMLS_FETCH();
	gdImagePtr im;
	zval *retval = NULL, *args, *zsx, *zsy;

	MAKE_STD_ZVAL(zsx);
	MAKE_STD_ZVAL(zsy);
	ZVAL_LONG(zsx, sx);
	ZVAL_LONG(zsy, sy);

	args = _qr_init_args(2 TSRMLS_CC, zsx, zsy);
	zend_fcall_info_call(&QRG(func_create).fci,
	                     &QRG(func_create).fcc,
	                     &retval, args TSRMLS_CC);
	if (retval) {
		if (Z_TYPE_P(retval) == IS_RESOURCE) {
			ZEND_FETCH_RESOURCE_NO_RETURN(im, gdImagePtr, &retval, -1, "Image", le_gd);
			_rsrc_to_fake(&retval TSRMLS_CC);
		}
		zval_ptr_dtor(&retval);
	} else {
		im = NULL;
	}
	zval_ptr_dtor(&args);

	return im;
}

/* }}} */
/* {{{ _qr_gdImageDestroy() */

PHP_QR_LOCAL void 
_qr_gdImageDestroy(gdImagePtr im)
{
	TSRMLS_FETCH();
	zval *zim;

	MAKE_STD_ZVAL(zim);
	ZEND_REGISTER_RESOURCE(zim, im, le_gd);
	zval_ptr_dtor(&zim);
}

/* }}} */
/* {{{ _qr_gdImageColorAllocate() */

PHP_QR_LOCAL int 
_qr_gdImageColorAllocate(gdImagePtr im, int r, int g, int b)
{
	TSRMLS_FETCH();
	int color = -1;
	zval *retval = NULL, *args, *zim, *zr, *zg, *zb;

	MAKE_STD_ZVAL(zim);
	MAKE_STD_ZVAL(zr);
	MAKE_STD_ZVAL(zg);
	MAKE_STD_ZVAL(zb);
	ZEND_REGISTER_RESOURCE(zim, im, le_gd);
	ZVAL_LONG(zr, r);
	ZVAL_LONG(zg, g);
	ZVAL_LONG(zb, b);

	args = _qr_init_args(4 TSRMLS_CC, zim, zr, zg, zb);
	zend_fcall_info_call(&QRG(func_colorallocate).fci,
	                     &QRG(func_colorallocate).fcc,
	                     &retval, args TSRMLS_CC);
	if (retval) {
		if (Z_TYPE_P(retval) == IS_LONG) {
			color = (int)Z_LVAL_P(retval);
		}
		zval_ptr_dtor(&retval);
	}
	_rsrc_to_fake(&zim TSRMLS_CC);
	zval_ptr_dtor(&args);

	return color;
}

/* }}} */
/* {{{ _qr_gdImagePaletteCopy() */

PHP_QR_LOCAL void 
_qr_gdImagePaletteCopy(gdImagePtr dst, gdImagePtr src)
{
	TSRMLS_FETCH();
	zval *retval = NULL, *args, *zdst, *zsrc;

	MAKE_STD_ZVAL(zdst);
	MAKE_STD_ZVAL(zsrc);
	ZEND_REGISTER_RESOURCE(zdst, dst, le_gd);
	ZEND_REGISTER_RESOURCE(zsrc, src, le_gd);

	args = _qr_init_args(2 TSRMLS_CC, zdst, zsrc);
	zend_fcall_info_call(&QRG(func_palettecopy).fci,
	                     &QRG(func_palettecopy).fcc,
	                     &retval, args TSRMLS_CC);
	if (retval) {
		zval_ptr_dtor(&retval);
	}
	_rsrc_to_fake(&zdst TSRMLS_CC);
	_rsrc_to_fake(&zsrc TSRMLS_CC);
	zval_ptr_dtor(&args);
}

/* }}} */
/* {{{ _qr_gdImageFill() */

PHP_QR_LOCAL void 
_qr_gdImageFill(gdImagePtr im, int x, int y, int color)
{
	TSRMLS_FETCH();
	zval *retval = NULL, *args, *zim, *zx, *zy, *zcolor;

	MAKE_STD_ZVAL(zim);
	MAKE_STD_ZVAL(zx);
	MAKE_STD_ZVAL(zy);
	MAKE_STD_ZVAL(zcolor);
	ZEND_REGISTER_RESOURCE(zim, im, le_gd);
	ZVAL_LONG(zx, x);
	ZVAL_LONG(zy, y);
	ZVAL_LONG(zcolor, color);

	args = _qr_init_args(4 TSRMLS_CC, zim, zx, zy, zcolor);
	zend_fcall_info_call(&QRG(func_fill).fci,
	                     &QRG(func_fill).fcc,
	                     &retval, args TSRMLS_CC);
	if (retval) {
		zval_ptr_dtor(&retval);
	}
	_rsrc_to_fake(&zim TSRMLS_CC);
	zval_ptr_dtor(&args);
}

/* }}} */
/* {{{ _qr_gdImageFilledRectangle() */

PHP_QR_LOCAL void 
_qr_gdImageFilledRectangle(gdImagePtr im,
                           int x1, int y1, int x2, int y2,
                           int color)
{
	TSRMLS_FETCH();
	zval *retval = NULL, *args, *zim, *zx1, *zy1, *zx2, *zy2, *zcolor;

	MAKE_STD_ZVAL(zim);
	MAKE_STD_ZVAL(zx1);
	MAKE_STD_ZVAL(zy1);
	MAKE_STD_ZVAL(zx2);
	MAKE_STD_ZVAL(zy2);
	MAKE_STD_ZVAL(zcolor);
	ZEND_REGISTER_RESOURCE(zim, im, le_gd);
	ZVAL_LONG(zx1, x1);
	ZVAL_LONG(zy1, y1);
	ZVAL_LONG(zx2, x2);
	ZVAL_LONG(zy2, y2);
	ZVAL_LONG(zcolor, color);

	args = _qr_init_args(6 TSRMLS_CC, zim, zx1, zy1, zx2, zy2, zcolor);
	zend_fcall_info_call(&QRG(func_filledrectangle).fci,
	                     &QRG(func_filledrectangle).fcc,
	                     &retval, args TSRMLS_CC);
	if (retval) {
		zval_ptr_dtor(&retval);
	}
	_rsrc_to_fake(&zim TSRMLS_CC);
	zval_ptr_dtor(&args);
}

/* }}} */
/* {{{ _qr_gdImageSetPixel() */

PHP_QR_LOCAL void 
_qr_gdImageSetPixel(gdImagePtr im, int x, int y, int color)
{
#ifdef __GNUC__
#define _expected(expr) __builtin_expect(!!(expr), 1)
#define _not_expected(expr) __builtin_expect(!!(expr), 0)
#else
#define _expected(expr) expr
#define _not_expected(expr) expr
#endif

	if (_expected(gdImageBoundsSafe(im, x, y))) {
		if (_not_expected(gdImageTrueColor(im))) {
			TSRMLS_FETCH();
			zval *retval = NULL, *args, *zim, *zx, *zy, *zcolor;

			MAKE_STD_ZVAL(zim);
			MAKE_STD_ZVAL(zx);
			MAKE_STD_ZVAL(zy);
			MAKE_STD_ZVAL(zcolor);
			ZEND_REGISTER_RESOURCE(zim, im, le_gd);
			ZVAL_LONG(zx, x);
			ZVAL_LONG(zy, y);
			ZVAL_LONG(zcolor, color);

			args = _qr_init_args(4 TSRMLS_CC, zim, zx, zy, zcolor);
			zend_fcall_info_call(&QRG(func_setpixel).fci,
			                     &QRG(func_setpixel).fcc,
			                     &retval, args TSRMLS_CC);
			if (retval) {
				zval_ptr_dtor(&retval);
			}
			_rsrc_to_fake(&zim TSRMLS_CC);
			zval_ptr_dtor(&args);
		} else {
			gdImagePalettePixel(im, x, y) = color;
		}
	}

#undef _expected
#undef _not_expected
}

/* }}} */
/* {{{ _qr_gdImageGifPtr() */

PHP_QR_LOCAL void *
_qr_gdImageGifPtr(gdImagePtr im, int *size)
{
	TSRMLS_FETCH();
	void *buf;
	zval *args, *zim;

	MAKE_STD_ZVAL(zim);
	ZEND_REGISTER_RESOURCE(zim, im, le_gd);

	args = _qr_init_args(1 TSRMLS_CC, zim);
	buf = _qr_output_capture(&QRG(func_gif), args, size TSRMLS_CC);
	_rsrc_to_fake(&zim TSRMLS_CC);
	zval_ptr_dtor(&args);

	return buf;
}

/* }}} */
/* {{{ _qr_gdImageJpegPtr() */

PHP_QR_LOCAL void *
_qr_gdImageJpegPtr(gdImagePtr im, int *size, int quality)
{
	TSRMLS_FETCH();
	void *buf;
	zval *args, *zim, *zfilename, *zquality;

	MAKE_STD_ZVAL(zim);
	MAKE_STD_ZVAL(zfilename);
	MAKE_STD_ZVAL(zquality);
	ZEND_REGISTER_RESOURCE(zim, im, le_gd);
	ZVAL_NULL(zfilename);
	ZVAL_LONG(zquality, quality);

	args = _qr_init_args(3 TSRMLS_CC, zim, zfilename, zquality);
	buf = _qr_output_capture(&QRG(func_jpeg), args, size TSRMLS_CC);
	_rsrc_to_fake(&zim TSRMLS_CC);
	zval_ptr_dtor(&args);

	return buf;
}

/* }}} */
/* {{{ _qr_gdImagePngPtrEx() */

PHP_QR_LOCAL void *
_qr_gdImagePngPtrEx(gdImagePtr im, int *size, int level, int basefilter)
{
	TSRMLS_FETCH();
	void *buf;
	zval *args, *zim, *zfilename, *zquality, *zfilter;

	MAKE_STD_ZVAL(zim);
	MAKE_STD_ZVAL(zfilename);
	MAKE_STD_ZVAL(zquality);
	MAKE_STD_ZVAL(zfilter);
	ZEND_REGISTER_RESOURCE(zim, im, le_gd);
	ZVAL_NULL(zfilename);
	ZVAL_LONG(zquality, level);
	ZVAL_LONG(zfilter, basefilter)

	args = _qr_init_args(4 TSRMLS_CC, zim, zfilename, zquality, zfilter);
	buf = _qr_output_capture(&QRG(func_png), args, size TSRMLS_CC);
	_rsrc_to_fake(&zim TSRMLS_CC);
	zval_ptr_dtor(&args);

	return buf;
}

/* }}} */
/* {{{ _qr_gdImageWBMPPtr() */

PHP_QR_LOCAL void *
_qr_gdImageWBMPPtr(gdImagePtr im, int *size, int fg)
{
	TSRMLS_FETCH();
	void *buf;
	zval *args, *zim, *zfilename, *zforeground;

	MAKE_STD_ZVAL(zim);
	MAKE_STD_ZVAL(zfilename);
	MAKE_STD_ZVAL(zforeground);
	ZEND_REGISTER_RESOURCE(zim, im, le_gd);
	ZVAL_NULL(zfilename);
	ZVAL_LONG(zforeground, fg);

	args = _qr_init_args(3 TSRMLS_CC, zim, zfilename, zforeground);
	buf = _qr_output_capture(&QRG(func_wbmp), args, size TSRMLS_CC);
	_rsrc_to_fake(&zim TSRMLS_CC);
	zval_ptr_dtor(&args);

	return buf;
}

/* }}} */
/* {{{ _qr_fcall_info_init() */

PHP_QR_LOCAL int
_qr_fcall_info_init(const char *name, qr_fcall_info *info TSRMLS_DC)
{
	MAKE_STD_ZVAL(info->name);
	ZVAL_STRING(info->name, (char *)name, 1);

#if PHP_VERSION_ID >= 50300
	return zend_fcall_info_init(info->name, 0, &info->fci, &info->fcc, NULL, NULL TSRMLS_CC);
#else
	return zend_fcall_info_init(info->name, &info->fci, &info->fcc TSRMLS_CC);
#endif
}

/* }}} */
/* {{{ _qr_fcall_info_destroy() */

PHP_QR_LOCAL void
_qr_fcall_info_destroy(qr_fcall_info *info TSRMLS_DC)
{
	zval_ptr_dtor(&info->name);
}

/* }}} */
/* {{{ _qr_init_args() */

static zval *
_qr_init_args(int argc TSRMLS_DC, ...)
{
	va_list ap;
	zval *args;

	if (argc < 0) {
		return NULL;
	}

	MAKE_STD_ZVAL(args);
	array_init(args);

#ifdef ZTS
	va_start(ap, TSRMLS_C);
#else
	va_start(ap, argc);
#endif
	while (argc > 0) {
		add_next_index_zval(args, (zval *)va_arg(ap, zval *));
		argc--;
	}
	va_end(ap);

	return args;
}

/* }}} */
/* {{{ _qr_output_capture() */

static void *
_qr_output_capture(qr_fcall_info *info, zval *args, int *size TSRMLS_DC)
{
	zval output, *retval = NULL;
	zend_bool result = 0;

	php_start_ob_buffer(NULL, 0, 1 TSRMLS_CC);

	zend_fcall_info_call(&info->fci, &info->fcc,
	                     &retval, args TSRMLS_CC);
	if (retval) {
		if (zval_is_true(retval)) {
			result = 1;
		}
		zval_ptr_dtor(&retval);
	}

	INIT_ZVAL(output);
	php_ob_get_buffer(&output TSRMLS_CC);
	php_end_ob_buffer(0, 0 TSRMLS_CC);

	if (result && Z_TYPE(output) == IS_STRING) {
		*size = (int)Z_STRLEN(output);
		return (void *)Z_STRVAL(output);
	} else {
		zval_dtor(&output);
		*size = 0;
		return NULL;
	}
}

/* }}} */
/* {{{ _rsrc_to_fake() */

static void
_rsrc_to_fake(zval **rsrc TSRMLS_DC)
{
	zend_rsrc_list_entry *le;

#if PHP_QR_GD_WRAPPERS_DEBUG
	if (Z_TYPE_PP(rsrc) != IS_RESOURCE) {
		zend_error(E_ERROR, "_rsrc_to_fake() accepts only resource value");
		return;
	}
#endif

	if (zend_hash_index_find(&EG(regular_list), (ulong)Z_LVAL_PP(rsrc), (void **)&le) == SUCCESS) {
#if PHP_QR_GD_WRAPPERS_DEBUG
		if (le->type != le_gd) {
			const char *type_name = zend_rsrc_list_get_rsrc_type(le->type TSRMLS_CC);
			zend_error(E_ERROR, "_rsrc_to_fake(): type of the reource"
					" is not '%s' (#%d) but '%s' (#%d)",
					zend_rsrc_list_get_rsrc_type(le_gd TSRMLS_CC), le_gd,
					((type_name == NULL) ? "(null)" : type_name), le->type);
			return;
		}
		if (le->refcount != 1) {
			zend_error(E_ERROR, "_rsrc_to_fake(): reference count of the resource"
					" is not 1 but %d", le->refcount);
			return;
		}
#endif
		le->ptr = NULL;
		le->type = le_fake_rsrc;
	}
}

/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
