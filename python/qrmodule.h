/**
 * QR Code generator module for Python
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
 * @package     pyqr
 * @author      Ryusuke SEKIYAMA <rsky0711@gmail.com>
 * @copyright   2007-2013 Ryusuke SEKIYAMA
 * @license     http://www.opensource.org/licenses/mit-license.php  MIT License
 */

#ifndef __QRMODULE_H__
#define __QRMODULE_H__

#include <Python.h>
#include "qr.h"
#include "qr_util.h"

/* {{{ method prototypes */

static PyObject *
qr_qrcode(PyObject *self, PyObject *args, PyObject *keywds);

static PyObject *
qr_mimetype(PyObject *self, PyObject *args);

static PyObject *
qr_extension(PyObject *self, PyObject *args);

/* }}} */
/* {{{ internal function prototypes */

static PyObject *
_qr_process(const qr_byte_t *data, int data_len, FILE *out,
		int version, int mode, int eclevel, int masktype,
		int format, int magnify, int separator);

static PyObject *
_qrs_process(const qr_byte_t *data, int data_len, FILE *out,
		int version, int mode, int eclevel, int masktype, int maxnum,
		int format, int magnify, int separator, int order);

static QRCode *
_qr_create_simple(const qr_byte_t *data, int data_len,
		int version, int mode, int eclevel, int masktype);

static QRStructured *
_qrs_create_simple(const qr_byte_t *data, int data_len,
		int version, int mode, int eclevel, int masktype, int maxnum);

static qr_byte_t *
_qr_get_symbol(QRCode *qr,
		int format, int separator, int magnify, int *symbol_size);

static qr_byte_t *
_qrs_get_symbols(QRStructured * st,
		int format, int separator, int magnify, int order, int *symbol_size);

static int
_qr_output_symbol(QRCode *qr, FILE *out,
		int format, int separator, int magnify);

static int
_qrs_output_symbols(QRStructured * st, FILE *out,
		int format, int separator, int magnify, int order);

static void
_qr_set_get_error(int errcode, const char *errmsg);

static void
_qr_set_output_error(int errcode, const char *errmsg);

/* }}} */

#endif /* __QRMODULE_H__ */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
