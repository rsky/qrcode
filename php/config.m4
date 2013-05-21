dnl
dnl $Id$
dnl

PHP_ARG_ENABLE(qr, [whether to enable QR Code support],
[  --enable-qr           Enable QR Code support], yes, yes)

PHP_ARG_ENABLE(qr-zlib-dir, [zlib install prefix],
[  --with-qr-zlib-dir[[=DIR]]  QR: zlib install prefix], yes, no)

if test "$PHP_QR" != "no"; then
    QR_SOURCES="php_qr.c libqr/qr.c libqr/qrcnv.c"
    QR_SOURCES="$QR_SOURCES libqr/qrcnv_bmp.c libqr/qrcnv_png.c"
    QR_SOURCES="$QR_SOURCES libqr/qrcnv_svg.c libqr/qrcnv_tiff.c"
    dnl TODO: check for zlib
    PHP_ADD_LIBRARY_WITH_PATH(z, , QR_SHARED_LIBADD)
    PHP_SUBST(QR_SHARED_LIBADD)
    AC_DEFINE(HAVE_QR, 1, [ ])
    PHP_NEW_EXTENSION(qr, $QR_SOURCES , $ext_shared)
fi
