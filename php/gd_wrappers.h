/*
 * QR Code generator extension for PHP: GD's function wrappers
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

#ifndef _PHP_QR_GD_WRAPPERS_H_
#define _PHP_QR_GD_WRAPPERS_H_

#include "php_qr.h"
#include <ext/gd/php_gd.h>
#include <ext/gd/libgd/gd.h>
#include <ext/gd/libgd/gdhelpers.h>

BEGIN_EXTERN_C()

PHP_QR_LOCAL int
_qr_wrappers_init(INIT_FUNC_ARGS);

PHP_QR_LOCAL int
_qr_fcall_info_init(const char *name, qr_fcall_info *info TSRMLS_DC);

PHP_QR_LOCAL void
_qr_fcall_info_destroy(qr_fcall_info *info TSRMLS_DC);

PHP_QR_LOCAL gdImagePtr
_qr_gdImageCreate(int sx, int sy);

PHP_QR_LOCAL void
_qr_gdImageDestroy(gdImagePtr im);

PHP_QR_LOCAL int
_qr_gdImageColorAllocate(gdImagePtr im, int r, int g, int b);

PHP_QR_LOCAL void
_qr_gdImagePaletteCopy(gdImagePtr dst, gdImagePtr src);

PHP_QR_LOCAL void
_qr_gdImageFill(gdImagePtr im, int x, int y, int color);

PHP_QR_LOCAL void
_qr_gdImageFilledRectangle(gdImagePtr im,
                           int x1, int y1,
                           int x2, int y2,
                           int color);

PHP_QR_LOCAL void
_qr_gdImageSetPixel(gdImagePtr im, int x, int y, int color);

PHP_QR_LOCAL void *
_qr_gdImageGifPtr(gdImagePtr im, int *size);

PHP_QR_LOCAL void *
_qr_gdImageJpegPtr(gdImagePtr im, int *size, int quality);

PHP_QR_LOCAL void *
_qr_gdImagePngPtrEx(gdImagePtr im, int *size, int level, int basefilter);

PHP_QR_LOCAL void *
_qr_gdImageWBMPPtr(gdImagePtr im, int *size, int fg);

END_EXTERN_C()

#undef gdImageCreate
#undef gdImageDestroy
#undef gdImageColorAllocate
#undef gdImagePaletteCopy
#undef gdImageFill
#undef gdImageFilledRectangle
#undef gdImageSetPixel
#ifdef gdImageGifPtr
#undef gdImageGifPtr
#endif
#undef gdImageJpegPtr
#undef gdImagePngPtrEx
#undef gdImageWBMPPtr

#define gdImageCreate           _qr_gdImageCreate
#define gdImageDestroy          _qr_gdImageDestroy
#define gdImageColorAllocate    _qr_gdImageColorAllocate
#define gdImagePaletteCopy      _qr_gdImagePaletteCopy
#define gdImageFill             _qr_gdImageFill
#define gdImageFilledRectangle  _qr_gdImageFilledRectangle
#define gdImageSetPixel         _qr_gdImageSetPixel
#define gdImageGifPtr           _qr_gdImageGifPtr
#define gdImageJpegPtr          _qr_gdImageJpegPtr
#define gdImagePngPtrEx         _qr_gdImagePngPtrEx
#define gdImageWBMPPtr          _qr_gdImageWBMPPtr

#endif /* _PHP_QR_GD_WRAPPERS_H_ */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
