dnl
dnl $ Id: $
dnl

PHP_ARG_ENABLE(qr, [whether to enable QR Code support],
[  --enable-qr           Enable QR Code support], yes, yes)

PHP_ARG_ENABLE(qr-gd, [whether to QR Code GD support],
[  --enable-qr-gd        Enable GIF, JPEG, PNG, WBMP output support
                        by the PHP GD extension], yes, no)

PHP_ARG_WITH(qr-tiff, [whether to QR Code TIFF support],
[  --with-qr-tiff[[=DIR]]  Enable zlib compressed TIFF output support.
                        DIR is zlib install prefix], no, no)

if test "$PHP_QR" != "no"; then

  dnl
  dnl Check if the locations of sed and awk are defined
  dnl
  if test "x" = "x$SED"; then
    SED=sed
  fi
  if test "x" = "x$AWK"; then
    AWK=awk
  fi

  AC_DEFINE(QR_STATIC_BUILD, 1, [build libqr as static library])
  QR_SOURCES="php_qr.c libqr/qr.c libqr/qrcnv.c libqr/qrcnv_bmp.c libqr/qrcnv_svg.c"

  dnl
  dnl Check the gd support
  dnl
  if test "$PHP_QR_GD" != "no"; then
    AC_DEFINE(PHP_QR_ENABLE_GD, 1, [enable GD support])
    AC_DEFINE(PHP_QR_GD_BUNDLED, 1, [use gd extension])
    AC_DEFINE(QR_ENABLE_GD, 1, [enable GD support in libqr])
    QR_SOURCES="$QR_SOURCES libqr/qrcnv_gd.c"
  fi

  dnl
  dnl Check the zlib support
  dnl
  if test "$PHP_QR_TIFF" != "no"; then
    if test "$PHP_QR_TIFF" != "yes"; then
      if test -r "$PHP_QR_TIFF/include/zlib.h"; then
        QR_ZLIB_DIR="$PHP_QR_TIFF"
      fi
    else
      AC_MSG_CHECKING([for zlib-dir in default path])
      for i in /usr /usr/local; do
        if test -r "$i/include/zlib.h"; then
          QR_ZLIB_DIR=$i
          AC_MSG_RESULT([found in $i])
          break
        fi
      done
      if test "x" = "x$QR_ZLIB_DIR"; then
        AC_MSG_ERROR([not found])
      fi
    fi

    PHP_ADD_INCLUDE($QR_ZLIB_DIR/include)

    dnl
    dnl Check the zlib headers and types
    dnl
    export OLD_CPPFLAGS="$CPPFLAGS"
    export CPPFLAGS="$CPPFLAGS $INCLUDES"
    AC_CHECK_HEADER([zlib.h], [], AC_MSG_ERROR(['zlib.h' header not found]))
    export CPPFLAGS="$OLD_CPPFLAGS"

    dnl
    dnl Check the zlib library
    dnl
    PHP_CHECK_LIBRARY(z, zlibVersion,
      [
        PHP_ADD_LIBRARY_WITH_PATH(z, $QR_ZLIB_DIR/lib, QR_SHARED_LIBADD)
      ],[
        AC_MSG_ERROR([wrong zlib library version or lib not found. Check config.log for more information])
      ],[
        -L$QR_ZLIB_DIR/lib
      ])

    AC_DEFINE(PHP_QR_ENABLE_TIFF, 1, [enable TIFF support])
    AC_DEFINE(QR_ENABLE_TIFF, 1, [enable TIFF support in libqr])
    QR_SOURCES="$QR_SOURCES libqr/qrcnv_tiff.c"
  fi

  PHP_ADD_INCLUDE(./libqr)
  PHP_SUBST(QR_SHARED_LIBADD)
  AC_DEFINE(HAVE_QR, 1, [ ])

  GDEXTRA_PHP_VERNUM=`"$PHP_CONFIG" --version | $AWK -F. '{ printf "%d", ($1 * 100 + $2) * 100 }'`
  if test "$GDEXTRA_PHP_VERNUM" -ge 50300; then
     QR_SOURCES="$QR_SOURCES gd_wrappers.c"
     AC_DEFINE(PHP_QR_USE_GD_WRAPPERS, 1, [ ])
  else
     AC_DEFINE(PHP_QR_USE_GD_WRAPPERS, 0, [ ])
  fi

  PHP_NEW_EXTENSION(qr, $QR_SOURCES , $ext_shared)
fi
