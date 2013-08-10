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

#include "qrmodule.h"
#include "qr_util.h"
#include <stdio.h>
#include <structmember.h>

/* {{{ type definitions */

struct _QRCodeObject {
    PyObject_HEAD
    QRCode *qr;
    QRStructured *st;
    int pos;
    int format;
    int separator;
    int scale;
    int order;
};

/* }}} */
/* {{{ globals and constants */

#define PYQR_USE_DEFAULT_MODE INT_MIN

#define PYQR_FORMAT_PIL_IMAGE -1
#define QR_FMT_IMAGE PYQR_FORMAT_PIL_IMAGE

static PyObject *QRCodeError;

static const char *active_func_name;
static const char * const fn_qrcode         = "qr.qrcode()";
static const char * const fn_add_data       = "qr.QRCode.add_data()";
static const char * const fn_copy           = "qr.QRCode.copy()";
static const char * const fn_get_symbol     = "qr.QRCode.get_symbol()";
static const char * const fn_get_info       = "qr.QRCode.get_info()";

/* }}} */
/* {{{ pydoc */

PyDoc_STRVAR(qr_module__doc__,
"QR Code generator module for Python\n"
"\n"
"This module provides following functions:\n"
"\n"
"qrcode(data,...) -- Return a QR Code symbol as a string.\n"
"mimetype(format) -- Return a Content-Type for the given format.\n"
"extension(format[, include_dot]) -- Return a filename extension.");

PyDoc_STRVAR(qrcode__doc__,
"qrcode(data,...) -- Return a QR Code symbol as a string.\n"
"\n"
"following options are also available:\n"
"  version:   symbol version (1-40, default: auto)\n"
"  mode:      encoding mode (MN, MA, M8, MK, default: auto)\n"
"  eclevel:   error correction level (ECL_{L,M,Q,H}, default: ECL_M)\n"
"  masktype:  mask pattern (0-7, default: auto)\n"
"  maxnum:    maximum number of symbols (1-16, default: 1)\n"
"  format:    output format (default: PNG)\n"
"             Available formats are followings.\n"
"               PNG, BMP, TIFF, PBM, SVG, JSON, DIGIT, ASCII\n"
"  separator: separator pattan width (0-16, default: 4)\n"
"             '4' is the lower limit of the QR Code specification.\n"
"  order:     ordering method of symbols, in case value is\n"
"             = 0 (default): order to square as possible\n"
"             >= 1: order each NUM symbols to horizontal\n"
"             <= -1: order each NUM symbols to vertical");

PyDoc_STRVAR(mimetype__doc__,
"mimetype(format) -- Return a Content-Type\n"
"which corresponds to the given output format.");

PyDoc_STRVAR(extension__doc__,
"extension(format[, include_dot]) -- Return a filename extension\n"
"which corresponds to the given output format.\n"
"\n"
"If optional arg include_dot is False, the result will not\n"
"include leading dot.");

PyDoc_STRVAR(QRCode_class__doc__, "QR Code generator class.");

/* }}} */
/* {{{ method definitions and member definitions */

static PyMethodDef qr_methods[] = {
    { "qrcode", (PyCFunction)qr_qrcode,
        METH_VARARGS | METH_KEYWORDS, qrcode__doc__ },
    { "mimetype", qr_mimetype, METH_VARARGS, mimetype__doc__ },
    { "extension", qr_extension, METH_VARARGS, extension__doc__ },
    { NULL, NULL, 0, NULL }
};

static PyMethodDef QRCode_methods[] = {
    { "add_data", (PyCFunction)QRCode_add_data,
        METH_VARARGS | METH_KEYWORDS, NULL },
    { "copy", (PyCFunction)QRCode_copy,
        METH_NOARGS, NULL },
    { "get_symbol", (PyCFunction)QRCode_get_symbol,
        METH_VARARGS | METH_KEYWORDS, NULL },
    { "get_info", (PyCFunction)QRCode_get_info,
        METH_NOARGS, NULL },
    { NULL, NULL, 0, NULL }
};

static PyMemberDef QRCode_members[] = {
    { "format",    T_INT, offsetof(QRCodeObject, format),    0, "format" },
    { "separator", T_INT, offsetof(QRCodeObject, separator), 0, "separator" },
    { "scale",     T_INT, offsetof(QRCodeObject, scale),     0, "scale" },
    { "order",     T_INT, offsetof(QRCodeObject, order),     0, "order" },
    { NULL, 0, 0, 0, NULL }
};

/* }}} */
/* {{{ qrcode() */

static PyObject *
qr_qrcode(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {
        "data",
        "version", "mode", "eclevel", "masktype", "maxnum",
        "format", "separator", "scale", "order",
        NULL
    };

    PyObject *data = NULL;
    const char *bytes = NULL;
    int length = 0;
    PyObject *result = NULL;

    int version = -1;
    int mode = QR_EM_AUTO;
    int eclevel = QR_ECL_M;
    int masktype = -1;
    int maxnum = 1;
    int format = QR_FMT_PNG;
    int scale = 1;
    int separator = 4;
    int order = 0;

    active_func_name = fn_qrcode;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|iiiiiiiii:qrcode", kwlist,
                                     &data,
                                     &version, &mode, &eclevel,
                                     &masktype, &maxnum,
                                     &format, &separator, &scale, &order)
    ) {
        return NULL;
    }

    if (PyUnicode_Check(data)) {
        PyObject *utf8 = PyUnicode_AsUTF8String(data);
        Py_DECREF(data);
        data = utf8;
        if (data == NULL) {
            return NULL;
        }
    }

#if PY_MAJOR_VERSION >= 3
    if (PyBytes_Check(data)) {
        bytes = PyBytes_AsString(data);
        length = PyBytes_Size(data);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "data must be str or byte");
    }
#else
    if (PyString_Check(data)) {
        bytes = PyString_AsString(data);
        length = PyString_Size(data);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "data must be str or unicode");
    }
#endif

    if (PyErr_Occurred()) {
        Py_DECREF(data);
        return NULL;
    }

    if (maxnum == 1) {
        result = PyQR_Process((const qr_byte_t *)bytes, length,
                              version, mode, eclevel, masktype,
                              format, scale, separator);
    }
    else {
        if (version == -1) {
            version = 1;
        }
        result = PyQR_ProcessMulti((const qr_byte_t *)bytes, length,
                                   version, mode, eclevel, masktype, maxnum,
                                   format, scale, separator, order);
    }

    Py_DECREF(data);

    return result;
}

/* }}} */
/* {{{ qr_mimetype() */

static PyObject *
qr_mimetype(PyObject *self, PyObject *args)
{
    const char *mimetype = NULL;
    int format = -1;

    if (!PyArg_ParseTuple(args, "i:mimetype", &format)) {
        return NULL;
    }

    mimetype = qrMimeType(format);
    if (mimetype == NULL) {
        PyErr_SetString(PyExc_ValueError, qrStrError(QR_ERR_INVALID_FMT));
        return NULL;
    }

    return Py_BuildValue("s", mimetype);
}

/* }}} */
/* {{{ qr_extension() */

static PyObject *
qr_extension(PyObject *self, PyObject *args)
{
    const char *extension = NULL;
    int format = -1;
    int include_dot = 1;

    if (!PyArg_ParseTuple(args, "i|i:extension", &format, &include_dot)) {
        return NULL;
    }

    extension = qrExtension(format);
    if (extension == NULL) {
        PyErr_SetString(PyExc_ValueError, qrStrError(QR_ERR_INVALID_FMT));
        return NULL;
    }

    if (include_dot) {
        char extension_buffer[QR_EXT_MAX_LEN + 2] = {0};
        snprintf(&(extension_buffer[0]), QR_EXT_MAX_LEN + 2, ".%s", extension);
        return Py_BuildValue("s", extension_buffer);
    }
    else {
        return Py_BuildValue("s", extension);
    }
}

/* }}} */
/* {{{ QRCode_add_data() */

static PyObject *
QRCode_add_data(QRCodeObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = { "data", "mode", NULL };

    const char *data = NULL;
    int length = 0;
    int mode = PYQR_USE_DEFAULT_MODE;

    active_func_name = fn_add_data;

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "s#|i:QRCode.add_data", kwlist,
                                     &data, &length, &mode)
    ) {
        return NULL;
    }

    if (PyQR_AddData(self, (const qr_byte_t *)data, length, mode)) {
        Py_RETURN_NONE;
    }

    return NULL;
}

/* }}} */
/* {{{ QRCode_copy() */

static QRCodeObject *
QRCode_copy(QRCodeObject *self, PyObject *unused)
{
    int errcode = QR_ERR_NONE;
    QRCode *qr = NULL;
    QRStructured *st = NULL;
    QRCodeObject *copy;
    PyObject *obj;

    active_func_name = fn_copy;

    if (self->qr) {
        qr = qrClone(self->qr, &errcode);
    }
    else if (self->st) {
        st = qrsClone(self->st, &errcode);
    }

    if (qr == NULL && st == NULL) {
        PyErr_SetString(QRCodeError, qrStrError(errcode));
        return NULL;
    }

    obj = PyType_GenericNew(PyQR_TypeObject(), NULL, NULL);
    if (obj == NULL || Py_TYPE(obj) != PyQR_TypeObject()) {
        if (obj) {
            PyErr_SetString(QRCodeError, "failed to create new QRCode object");
            Py_DECREF(obj);
        }
        if (qr) {
            qrDestroy(qr);
        }
        if (st) {
            qrsDestroy(st);
        }
        return NULL;
    }

    copy = (QRCodeObject *)obj;
    copy->qr = qr;
    copy->st = st;
    copy->format = self->format;
    copy->scale = self->scale;
    copy->separator = self->separator;
    copy->order = self->order;

    return copy;
}

/* }}} */
/* {{{ QRCode_get_symbol() */

static PyObject *
QRCode_get_symbol(QRCodeObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {
        "format", "separator", "scale", "order", NULL
    };

    int format = self->format;
    int scale = self->scale;
    int separator = self->separator;
    int order = self->order;

    active_func_name = fn_get_symbol;

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "|iiii:QRCode.get_symbol", kwlist,
                                     &format, &separator, &scale, &order)
    ) {
        return NULL;
    }

    return PyQR_GetSymbol_FromObject(self, format, separator, scale, order);
}

/* }}} */
/* {{{ QRCode_get_info() */

static PyObject *
QRCode_get_info(QRCodeObject *self, PyObject *unused)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");

    active_func_name = fn_get_info;

    return NULL;
}

/* }}} */
/* {{{ PyQR_SymbolDataFromStringAndSize() */

#if PY_MAJOR_VERSION >= 3
static inline PyObject *
PyQR_SymbolDataFromStringAndSize3(qr_byte_t *bytes, int size, int format)
{
    switch (format) {
        case QR_FMT_PBM:
        case QR_FMT_SVG:
        case QR_FMT_JSON:
        case QR_FMT_DIGIT:
        case QR_FMT_ASCII:
            return PyUnicode_FromStringAndSize((char *)bytes, size);
        default:
            return PyBytes_FromStringAndSize((char *)bytes, size);
    }
}
#define PyQR_SymbolDataFromStringAndSize(bytes, size, format) \
    PyQR_SymbolDataFromStringAndSize3(bytes, size, format)
#else
static inline PyObject *
PyQR_SymbolDataFromStringAndSize2(qr_byte_t *bytes, int size)
{
    return PyString_FromStringAndSize((char *)bytes, size);
}
#define PyQR_SymbolDataFromStringAndSize(bytes, size, format) \
    PyQR_SymbolDataFromStringAndSize2(bytes, size)
#endif

/* }}} */

/* {{{ PyQR_Process() */

static PyObject *
PyQR_Process(const qr_byte_t *data, int length,
             int version, int mode, int eclevel, int masktype,
             int format, int scale, int separator)
{
    PyObject *result = NULL;
    QRCode *qr;
    qr_byte_t *symbol;
    int size = 0;
    int return_image = 0;

    qr = PyQR_Create(data, length, version, mode, eclevel, masktype);
    if (qr == NULL) {
        return NULL;
    }

    if (format == PYQR_FORMAT_PIL_IMAGE) {
        format = QR_FMT_PNG;
        return_image = 1;
    }

    symbol = PyQR_GetSymbol(qr, format, separator, scale, &size);
    if (symbol) {
        if (return_image) {
            result = PyQR_SymbolImageFromStringAndSize(symbol, size);
        }
        else {
            result = PyQR_SymbolDataFromStringAndSize(symbol, size, format);
        }
        free(symbol);
    }

    qrDestroy(qr);

    return result;
}

/* }}} */
/* {{{ PyQR_Create() */

static QRCode *
PyQR_Create(const qr_byte_t *data, int length,
            int version, int mode, int eclevel, int masktype)
{
    QRCode *qr = NULL;
    int errcode = QR_ERR_NONE;

    qr = qrInit(version, mode, eclevel, masktype, &errcode);
    if (qr == NULL) {
        if (errcode == QR_ERR_MEMORY_EXHAUSTED) {
            PyErr_SetString(PyExc_MemoryError, qrStrError(errcode));
        }
        else {
            PyErr_SetString(PyExc_ValueError, qrStrError(errcode));
        }
        return NULL;
    }

    if (!qrAddData2(qr, data, length, mode)) {
        PyErr_SetString(QRCodeError, qrGetErrorInfo(qr));
        qrDestroy(qr);
        return NULL;
    }

    if (!qrFinalize(qr)) {
        PyErr_SetString(QRCodeError, qrGetErrorInfo(qr));
        qrDestroy(qr);
        return NULL;
    }

    return qr;
}

/* }}} */
/* {{{ PyQR_GetSymbol() */

static qr_byte_t *
PyQR_GetSymbol(QRCode *qr,
               int format, int separator, int scale, int *size)
{
    qr_byte_t *symbol = NULL;
    int errcode = QR_ERR_NONE;
    QRCode *cp = qrClone(qr, &errcode);

    if (cp == NULL) {
        PyErr_SetString(QRCodeError, qrStrError(errcode));
        return NULL;
    }

    if (qrFinalize(cp)) {
        symbol = qrGetSymbol(cp, format, separator, scale, size);
        if (symbol == NULL) {
            PyQR_SetError(qrGetErrorCode(cp), qrGetErrorInfo(cp));
        }
    }
    else {
        PyQR_SetError(qrGetErrorCode(cp), qrGetErrorInfo(cp));
    }

    qrDestroy(cp);

    return symbol;
}

/* }}} */
/* {{{ PyQR_ProcessMulti() */

static PyObject *
PyQR_ProcessMulti(const qr_byte_t *data, int length,
                  int version, int mode, int eclevel, int masktype, int maxnum,
                  int format, int scale, int separator, int order)
{
    PyObject *result = NULL;
    QRStructured *st;
    qr_byte_t *symbol;
    int size = 0;
    int return_image = 0;

    st = PyQR_CreateMulti(data, length,
                          version, mode, eclevel, masktype, maxnum);
    if (st == NULL) {
        return NULL;
    }

    if (format == PYQR_FORMAT_PIL_IMAGE) {
        format = QR_FMT_PNG;
        return_image = 0;
    }

    symbol = PyQR_GetSymbols(st, format, separator, scale, order, &size);
    if (symbol) {
        if (return_image) {
            result = PyQR_SymbolImageFromStringAndSize(symbol, size);
        }
        else {
            result = PyQR_SymbolDataFromStringAndSize(symbol, size, format);
        }
        free(symbol);
    }

    qrsDestroy(st);

    return result;
}

/* }}} */
/* {{{ PyQR_CreateMulti() */

static QRStructured *
PyQR_CreateMulti(const qr_byte_t *data, int size,
                 int version, int mode, int eclevel, int masktype, int maxnum)
{
    QRStructured *st;
    int errcode = QR_ERR_NONE;

    st = qrsInit(version, mode, eclevel, masktype, maxnum, &errcode);
    if (st == NULL) {
        if (errcode == QR_ERR_MEMORY_EXHAUSTED) {
            PyErr_SetString(PyExc_MemoryError, qrStrError(errcode));
        }
        else {
            PyErr_SetString(PyExc_ValueError, qrStrError(errcode));
        }
        return NULL;
    }

    if (!qrsAddData2(st, data, size, mode)) {
        PyErr_SetString(QRCodeError, qrsGetErrorInfo(st));
        qrsDestroy(st);
        return NULL;
    }

    if (!qrsFinalize(st)) {
        PyErr_SetString(QRCodeError, qrsGetErrorInfo(st));
        qrsDestroy(st);
        return NULL;
    }

    return st;
}

/* }}} */
/* {{{ PyQR_GetSymbols() */

static qr_byte_t *
PyQR_GetSymbols(QRStructured *st,
                int format, int separator, int scale,
                int order, int *size)
{
    qr_byte_t *symbol = NULL;
    int errcode = QR_ERR_NONE;
    QRStructured *cp = qrsClone(st, &errcode);

    if (symbol == NULL) {
        PyQR_SetError(qrsGetErrorCode(st), qrsGetErrorInfo(st));
        return NULL;
    }

    if (qrsFinalize(cp)) {
        symbol = qrsGetSymbols(st, format, separator, scale, order, size);
        if (symbol == NULL) {
            PyQR_SetError(qrsGetErrorCode(cp), qrsGetErrorInfo(cp));
        }
    }
    else {
        PyQR_SetError(qrsGetErrorCode(cp), qrsGetErrorInfo(cp));
    }

    qrsDestroy(cp);

    return symbol;
}

/* }}} */
/* {{{ PyQR_AddData() */

static int
PyQR_AddData(QRCodeObject *obj, const qr_byte_t *data, int size, int mode)
{
    int result = 0;

    if (obj->qr) {
        if (mode == PYQR_USE_DEFAULT_MODE) {
            result = qrAddData(obj->qr, data, size);
        }
        else {
            result = qrAddData2(obj->qr, data, size, mode);
        }
        if (!result) {
            PyErr_SetString(QRCodeError, qrGetErrorInfo(obj->qr));
            return 0;
        }
    }
    else if (obj->st){
        if (mode == PYQR_USE_DEFAULT_MODE) {
            result = qrsAddData(obj->st, data, size);
        }
        else {
            result = qrsAddData2(obj->st, data, size, mode);
        }
        if (!result) {
            PyErr_SetString(QRCodeError, qrsGetErrorInfo(obj->st));
            return 0;
        }
    }

    return result;
}

/* }}} */
/* {{{ PyQR_GetSymbolEx() */

static PyObject *
PyQR_GetSymbol_FromObject(QRCodeObject *obj,
                          int format, int separator, int scale, int order)
{
    PyObject *result = NULL;
    qr_byte_t *symbol = NULL;
    int size = 0;
    int errcode = QR_ERR_NONE;
    int is_copy = 0;
    int return_image = 0;

    if (format == PYQR_FORMAT_PIL_IMAGE) {
        format = QR_FMT_PNG;
        return_image = 1;
    }

    if (obj->qr) {
        QRCode *qr = obj->qr;
        if (!qrIsFinalized(qr)) {
            qr = qrClone(obj->qr, &errcode);
            if (!qr) {
                PyErr_SetString(QRCodeError, qrStrError(errcode));
                return NULL;
            }
            if (!qrFinalize(qr)) {
                qrDestroy(qr);
                PyErr_SetString(QRCodeError, qrGetErrorInfo(qr));
                return NULL;
            }
            is_copy = 1;
        }
        symbol = PyQR_GetSymbol(qr, format, separator, scale, &size);
        if (is_copy) {
            qrDestroy(qr);
        }
    }
    else if (obj->st) {
        QRStructured *st = obj->st;
        if (!qrsIsFinalized(st)) {
            st = qrsClone(obj->st, &errcode);
            if (!st) {
                PyErr_SetString(QRCodeError, qrStrError(errcode));
                return NULL;
            }
            if (!qrsFinalize(st)) {
                qrsDestroy(st);
                PyErr_SetString(QRCodeError, qrsGetErrorInfo(st));
                return NULL;
            }
            is_copy = 1;
        }
        symbol = PyQR_GetSymbols(st, format, separator, scale, order, &size);
        if (is_copy) {
            qrsDestroy(st);
        }
    }
    else {
        PyErr_SetString(QRCodeError, qrStrError(QR_ERR_UNKNOWN));
    }

    if (symbol) {
        if (!PyErr_Occurred()) {
            if (return_image) {
                result = PyQR_SymbolImageFromStringAndSize(symbol, size);
            }
            else {
                result = PyQR_SymbolDataFromStringAndSize(symbol, size, format);
            }
        }
        free(symbol);
    }

    return result;
}

/* }}} */
/* {{{ PyQR_SymbolImageFromStringAndSize() */

static PyObject *
PyQR_SymbolImageFromStringAndSize(qr_byte_t *bytes, int size)
{
    PyObject *data;
    PyObject *modStringIO, *stringIOFunction, *stringIO;
    PyObject *modPILImage, *openFunction, *image = NULL;

    // import modules
    modStringIO = PyImport_ImportModule("cStringIO");
    modPILImage = PyImport_ImportModule("PIL.Image");
    if (modStringIO == NULL || modPILImage == NULL) {
        if (modStringIO != NULL) {
            Py_DECREF(modStringIO);
        }
        if (modPILImage != NULL) {
            Py_DECREF(modPILImage);
        }
        return NULL;
    }

    // get functions
    stringIOFunction = PyObject_GetAttrString(modStringIO, "StringIO");
    openFunction = PyObject_GetAttrString(modPILImage, "open");
    if (stringIOFunction == NULL || openFunction == NULL) {
        if (stringIOFunction != NULL) {
            Py_DECREF(stringIOFunction);
        }
        if (openFunction != NULL) {
            Py_DECREF(openFunction);
        }
        Py_DECREF(modStringIO);
        Py_DECREF(modPILImage);
        return NULL;
    }

    // create an Image object
#if PY_MAJOR_VERSION >= 3
    data = PyBytes_FromStringAndSize((char *)bytes, size);
#else
    data = PyString_FromStringAndSize((char *)bytes, size);
#endif
    if (data != NULL) {
        stringIO = PyObject_CallFunctionObjArgs(stringIOFunction, data, NULL);
        if (stringIO != NULL) {
            image = PyObject_CallFunctionObjArgs(openFunction, stringIO, NULL);
            Py_DECREF(stringIO);
        }
        Py_DECREF(data);
    }

    Py_DECREF(stringIOFunction);
    Py_DECREF(openFunction);
    Py_DECREF(modStringIO);
    Py_DECREF(modPILImage);

    return image;
}

/* }}} */
/* {{{ PyQR_SetError() */

static void
PyQR_SetError(int errcode, const char *errmsg)
{
    if (errcode >= QR_ERR_INVALID_ARG && errcode <= QR_ERR_EMPTY_PARAM) {
        PyErr_SetString(PyExc_ValueError, errmsg);
    }
    else if (errcode == QR_ERR_SEE_ERRNO) {
        PyErr_SetFromErrno(QRCodeError);
    }
    else {
        PyErr_SetString(QRCodeError, errmsg);
    }
}

/* }}} */
/* {{{ type object entities */

static PyTypeObject QRCodeObjectType = {
#ifdef PyVarObject_HEAD_INIT
    PyVarObject_HEAD_INIT(NULL, 0)
#else
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
#endif
    "qr.QRCode",                        /* tp_name */
    sizeof(QRCodeObject),               /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)QRCode_dealloc,         /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    QRCode_class__doc__,                /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    QRCode_methods,                     /* tp_methods */
    QRCode_members,                     /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)QRCode_init,              /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
};

/* }}} */
/* {{{ PyQrGetTypeObject() */

static PyTypeObject *
PyQR_TypeObject(void)
{
    return &QRCodeObjectType;
}

/* }}} */
/* {{{ QRCode_init() */

static int
QRCode_init(QRCodeObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {
        "version", "mode", "eclevel", "masktype", "maxnum",
        "format", "separator", "scale", "order",
        NULL
    };

    int errcode = QR_ERR_NONE;
    int version = -1;
    int mode = QR_EM_AUTO;
    int eclevel = QR_ECL_M;
    int masktype = -1;
    int maxnum = 1;

    self->format = QR_FMT_PNG;
    self->scale = 1;
    self->separator = 4;
    self->order = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "|iiiiiiiii:QRCode.__init__",
                                     kwlist,
                                     &version, &mode, &eclevel,
                                     &masktype, &maxnum,
                                     &self->format, &self->separator,
                                     &self->scale, &self->order)
    ) {
        return -1;
    }

    if (maxnum == 1) {
        self->st = NULL;
        self->qr = qrInit(version, mode, eclevel, masktype, &errcode);
    }
    else {
        if (version == -1) {
            version = 1;
        }
        self->qr = NULL;
        self->st = qrsInit(version, mode, eclevel, masktype, maxnum, &errcode);
    }

    if (self->qr == NULL && self->st == NULL) {
        if (errcode == QR_ERR_MEMORY_EXHAUSTED) {
            PyErr_SetString(PyExc_MemoryError, qrStrError(errcode));
        }
        else {
            PyErr_SetString(PyExc_ValueError, qrStrError(errcode));
        }
        return -1;
    }

    return 0;
}

/* }}} */
/* {{{ QRCode_dealloc() */

static void
QRCode_dealloc(QRCodeObject *self)
{
    if (self->st) {
        qrsDestroy(self->st);
    }
    else if (self->qr) {
        qrDestroy(self->qr);
    }
    Py_TYPE(self)->tp_free(self);
}

/* }}} */
/* {{{ initialize a module */

#if PY_MAJOR_VERSION >= 3
static PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "qr",                   /* m_name */
    qr_module__doc__,       /* m_doc */
    -1,                     /* m_size */
    qr_methods,             /* m_methods */
    NULL,                   /* m_reload */
    NULL,                   /* m_traverse */
    NULL,                   /* m_clear */
    NULL,                   /* m_free */
};
#endif

#define QR_DECLARE_CONSTANT(name) { \
    v = PyLong_FromLong(QR_##name); \
    PyDict_SetItemString(d, #name, v); \
    Py_DECREF(v); \
}

#define QR_DECLARE_CONSTANT_EX(name, value) { \
    v = PyLong_FromLong(value); \
    PyDict_SetItemString(d, #name, v); \
    Py_DECREF(v); \
}

static PyObject *
PyQR_InitModule(void)
{
    PyObject *m, *d, *v;

    Py_TYPE(&QRCodeObjectType) = &PyType_Type;
    if (PyType_Ready(&QRCodeObjectType) < 0) {
        return NULL;
    }

#if PY_MAJOR_VERSION >= 3
    m = PyModule_Create(&moduledef);
#else
    m = Py_InitModule4("qr", qr_methods,
                       qr_module__doc__,
                       NULL, PYTHON_API_VERSION);
#endif
    d = PyModule_GetDict(m);

    qrGetCurrentFunctionName = PyQR_ActiveFuncName;

    /* encoding mode (fullname) */
    QR_DECLARE_CONSTANT(EM_AUTO);
    QR_DECLARE_CONSTANT(EM_NUMERIC);
    QR_DECLARE_CONSTANT(EM_ALNUM);
    QR_DECLARE_CONSTANT(EM_8BIT);
    QR_DECLARE_CONSTANT(EM_KANJI);

    /* encoding mode (alias) */
    QR_DECLARE_CONSTANT_EX(MN, QR_EM_NUMERIC);
    QR_DECLARE_CONSTANT_EX(MA, QR_EM_ALNUM);
    QR_DECLARE_CONSTANT_EX(M8, QR_EM_8BIT);
    QR_DECLARE_CONSTANT_EX(MK, QR_EM_KANJI);

    /* error correction level (fullname) */
    QR_DECLARE_CONSTANT(ECL_L);
    QR_DECLARE_CONSTANT(ECL_M);
    QR_DECLARE_CONSTANT(ECL_Q);
    QR_DECLARE_CONSTANT(ECL_H);

    /* output format (fullname) */
    QR_DECLARE_CONSTANT(FMT_IMAGE);
    QR_DECLARE_CONSTANT(FMT_PNG);
    QR_DECLARE_CONSTANT(FMT_BMP);
    QR_DECLARE_CONSTANT(FMT_TIFF);
    QR_DECLARE_CONSTANT(FMT_PBM);
    QR_DECLARE_CONSTANT(FMT_SVG);
    QR_DECLARE_CONSTANT(FMT_JSON);
    QR_DECLARE_CONSTANT(FMT_DIGIT);
    QR_DECLARE_CONSTANT(FMT_ASCII);

    /* output format (alias) */
    QR_DECLARE_CONSTANT_EX(IMAGE, QR_FMT_IMAGE);
    QR_DECLARE_CONSTANT_EX(PNG,   QR_FMT_PNG);
    QR_DECLARE_CONSTANT_EX(BMP,   QR_FMT_BMP);
    QR_DECLARE_CONSTANT_EX(TIFF,  QR_FMT_TIFF);
    QR_DECLARE_CONSTANT_EX(PBM,   QR_FMT_PBM);
    QR_DECLARE_CONSTANT_EX(SVG,   QR_FMT_SVG);
    QR_DECLARE_CONSTANT_EX(JSON,  QR_FMT_JSON);
    QR_DECLARE_CONSTANT_EX(DIGIT, QR_FMT_DIGIT);
    QR_DECLARE_CONSTANT_EX(ASCII, QR_FMT_ASCII);

    /* add objects */
    QRCodeError = PyErr_NewException("qr.Error", PyExc_RuntimeError, NULL);
    Py_INCREF(QRCodeError);
    PyModule_AddObject(m, "Error", QRCodeError);

    Py_INCREF(&QRCodeObjectType);
    PyModule_AddObject(m, "QRCode", (PyObject *)&QRCodeObjectType);

    return m;
}

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_qr(void)
{
    return PyQR_InitModule();
}
#else
PyMODINIT_FUNC initqr(void)
{
    (void)PyQR_InitModule();
}
#endif

/* }}} */
/* {{{ PyQR_ActiveFuncName() */

static const char *
PyQR_ActiveFuncName(void)
{
    return active_func_name;
}

/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: nil;
 * End:
 * vim600: et sw=4 ts=4 fdm=marker
 * vim<600: et sw=4 ts=4
 */
