/*
 * QR Code Generator Library: Symbol Converters for GIF/JPEG/PNG/WBMP with GD Library
 *
 * Core routines were originally written by Junn Ohta.
 * Based on qr.c Version 0.1: 2004/4/3 (Public Domain)
 *
 * @package     libqr
 * @author      Ryusuke SEKIYAMA <rsky0711@gmail.com>
 * @copyright   2006-2010 Ryusuke SEKIYAMA
 * @license     http://www.opensource.org/licenses/mit-license.php  MIT License
 */

#include "qrcnv.h"

#ifdef PHP_QR_GD_BUNDLED
/* {{{ PHP bundled GD */

#if PHP_QR_USE_GD_WRAPPERS
#include "gd_wrappers.h"
#else
#include <ext/gd/libgd/gd.h>
#include <ext/gd/libgd/gdhelpers.h>
#endif

#ifdef gdImageGifPtr
#if !PHP_QR_USE_GD_WRAPPERS
void *php_gd_gdImageGifPtr(gdImagePtr im, int *size);
#endif
#else
void *gdImageGifPtr(gdImagePtr im, int *size);
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
#define QR_GD_API __attribute__((visibility("hidden")))
#else
#define QR_GD_API
#endif

#define QRCNV_PNG_BASEFILTER , 0

/* }}} */
#else
/* {{{ GD library */

#include <gd.h>

#define QR_GD_API static

#define QRCNV_PNG_BASEFILTER

/* }}} */
#endif

/* whether enable GIF animation output or not */
#if !defined(PHP_QR_GD_BUNDLED) && !defined(QR_GD_DISABLE_GIFANIM)
#define QRCNV_ENABLE_GIFANIM
#endif

/* use the IJG JPEG library's default quality */
#ifndef QRCNV_JPEG_QUALITY
#define QRCNV_JPEG_QUALITY -1
#endif

/* use the zlib library's default compression level */
#ifndef QRCNV_PNG_LEVEL
#define QRCNV_PNG_LEVEL -1
#endif

/* {{{ function prototypes */

QR_GD_API gdImagePtr
qrSymbolToGdImagePtr(QRCode *qr, int sep, int mag, int *fgcolor, int *bgcolor);

QR_GD_API gdImagePtr
qrsSymbolsToGdImagePtr(QRStructured *st,
		int sep, int mag, int order, int *fgcolor, int *bgcolor);

static qr_byte_t *
qrSymbolToImage(QRCode *qr, int fmt, int sep, int mag, int *size);

static qr_byte_t *
qrsSymbolsToImage(QRStructured *st, int fmt, int sep, int mag, int order, int *size);

static gdImagePtr
qrGdImageCreate(int width, int height,
		int *fgcolor, int fgred, int fggreen, int fgblue,
		int *bgcolor, int bgred, int bggreen, int bgblue);

static qr_byte_t *
qrGdImageFinalize(QRCode *qr, gdImagePtr im, int fmt, int *size);

static void
qrGdDrawFinderPatterns(gdImagePtr im, int fgcolor, int bgcolor,
		int dim, int xoffset, int yoffset, int mag);

static void
qrGdDrawDarkModules(QRCode *qr, gdImagePtr im, int fgcolor,
		int xfrom, int yfrom, int xto, int yto,
		int xoffset, int yoffset, int mag);

#ifdef QRCNV_ENABLE_GIFANIM
static void
qrsFreeGIFAnim(gdImagePtr ims[], int num, qr_byte_t *sbuf, void *ibuf);
#endif

/* }}} */
/* {{{ qrSymbolToGdImagePtr() */

/*
 * 生成されたQRコードシンボルをGD形式のイメージに変換する
 */
QR_GD_API gdImagePtr
qrSymbolToGdImagePtr(QRCode *qr, int sep, int mag, int *fgcolor, int *bgcolor)
{
	gdImagePtr im;
	int black, white;
	int m;
	int dim, imgdim, sepdim;
	int *size = NULL;

	QRCNV_CHECK_STATE();
	QRCNV_GET_SIZE();

	/*
	 * 画像と描画色・背景色を初期化する
	 */
	im = qrGdImageCreate(imgdim, imgdim, &black, 0, 0, 0, &white, 255, 255, 255);
	if (im == NULL) {
		QRCNV_RETURN_FAILURE3(QR_ERR_IMAGECREATE, "%dx%d", imgdim, imgdim);
	}
	if (fgcolor) {
		*fgcolor = black;
	}
	if (bgcolor) {
		*bgcolor = white;
	}

	/*
	 * 位置検出パターンを描画する
	 */
	qrGdDrawFinderPatterns(im, black, white, dim, sepdim, sepdim, mag);

	/*
	 * 暗モジュールを描画する
	 */
	m = dim - 8;
	qrGdDrawDarkModules(qr, im, black, 8, 0, m, 8, sepdim, sepdim, mag);
	qrGdDrawDarkModules(qr, im, black, 0, 8, dim, m, sepdim, sepdim, mag);
	qrGdDrawDarkModules(qr, im, black, 8, m, dim, dim, sepdim, sepdim, mag);

	return im;
}

/* }}} */
/* {{{ qrSymbolToImage() */

/*
 * 生成されたQRコードシンボルをGDを用いてfmtで指定された画像形式に変換する
 */
static qr_byte_t *
qrSymbolToImage(QRCode *qr, int fmt, int sep, int mag, int *size)
{
	gdImagePtr im;

	im = qrSymbolToGdImagePtr(qr, sep, mag, NULL, NULL);
	if (im == NULL) {
		if (size) {
			*size = -1;
		}
		return NULL;
	}

	return qrGdImageFinalize(qr, im, fmt, size);
}

/* }}} */
/* {{{ qrsSymbolsToGdImagePtr() */

/*
 * 生成されたQRコードシンボルをGD形式のイメージに変換する
 */
QR_GD_API gdImagePtr
qrsSymbolsToGdImagePtr(QRStructured *st,
		int sep, int mag, int order, int *fgcolor, int *bgcolor)
{
	QRCode *qr = st->cur;
	gdImagePtr im;
	int black, white;
	int k, l, m;
	int xs, ys;
	int cols, rows, pos, xdim, ydim, zdim;
	int dim, imgdim, sepdim;
	int *size = NULL;

	QRCNV_SA_CHECK_STATE();
	if (st->num == 1) {
		return qrSymbolToGdImagePtr(st->qrs[0], sep, mag, NULL, NULL);
	}
	QRCNV_SA_GET_SIZE();

	/*
	 * 画像と描画色・背景色を初期化する
	 */
	im = qrGdImageCreate(xdim, ydim, &black, 0, 0, 0, &white, 255, 255, 255);
	if (im == NULL) {
		QRCNV_RETURN_FAILURE3(QR_ERR_IMAGECREATE, "%dx%d", xdim, ydim);
	}
	if (fgcolor) {
		*fgcolor = black;
	}
	if (bgcolor) {
		*bgcolor = white;
	}

	/*
	 * シンボルを描画する
	 */
	m = dim - 8;
	for (k = 0; k < rows; k++) {
		for (l = 0; l < cols; l++) {
			if (order < 0) {
				pos = k + rows * l;
			} else {
				pos = cols * k + l;
			}
			if (pos >= st->num) {
				break;
			}
			xs = l * (sepdim + zdim) + sepdim;
			ys = k * (sepdim + zdim) + sepdim;
			/*
			 * 位置検出パターンを描画する
			 */
			qrGdDrawFinderPatterns(im, black, white, dim, xs, ys, mag);
			/*
			 * 暗モジュールを描画する
			 */
			qrGdDrawDarkModules(st->qrs[pos], im, black, 8, 0, m, 8, xs, ys, mag);
			qrGdDrawDarkModules(st->qrs[pos], im, black, 0, 8, dim, m, xs, ys, mag);
			qrGdDrawDarkModules(st->qrs[pos], im, black, 8, m, dim, dim, xs, ys, mag);
		}
	}

	return im;
}

/* }}} */
/* {{{ qrsSymbolsToImage() */

/*
 * 生成された構造的連接QRコードシンボルをGDを用いてfmtで指定された画像形式に変換する
 */
static qr_byte_t *
qrsSymbolsToImage(QRStructured *st, int fmt, int sep, int mag, int order, int *size)
{
	gdImagePtr im;

	im = qrsSymbolsToGdImagePtr(st, sep, mag, order, NULL, NULL);
	if (im == NULL) {
		if (size) {
			*size = -1;
		}
		return NULL;
	}

	return qrGdImageFinalize(st->cur, im, fmt, size);
}

/* }}} */
#ifdef QRCNV_ENABLE_GIFANIM
/* {{{ qrsSymbolsToGIFAnim() */

/*
 * 生成された構造的連接QRコードシンボルをGDを用いてGIFアニメーションに変換する
 */
QR_API qr_byte_t *
qrsSymbolsToGIFAnim(QRStructured *st, int sep, int mag, int delay, int *size)
{
	QRCode *qr = st->cur;
	gdImagePtr ims[QR_STA_MAX];
	qr_byte_t *sbuf;
	void *ibuf;
	int frame_size;
	int black, white;
	int i, m;
	int dim, imgdim, sepdim;

	QRCNV_SA_CHECK_STATE();
	if (st->num == 1) {
		return qrSymbolToImage(st->qrs[0], QR_FMT_GIF, sep, mag, size);
	}
	QRCNV_GET_SIZE();
	m = dim - 8;

	/*
	 * 最初のシンボルと描画色・背景色を初期化する
	 */
	ims[0] = qrGdImageCreate(imgdim, imgdim, &black, 0, 0, 0, &white, 255, 255, 255);
	if (ims[0] == NULL) {
		QRCNV_RETURN_FAILURE3(QR_ERR_IMAGECREATE, "image[0]: %dx%d", imgdim, imgdim);
	}
	/*
	 * 位置検出パターンを描画する
	 */
	qrGdDrawFinderPatterns(ims[0], black, white, dim, sepdim, sepdim, mag);
	/*
	 * 暗モジュールを描画する
	 */
	qrGdDrawDarkModules(st->qrs[0], ims[0], black, 8, 0, m, 8, sepdim, sepdim, mag);
	qrGdDrawDarkModules(st->qrs[0], ims[0], black, 0, 8, dim, m, sepdim, sepdim, mag);
	qrGdDrawDarkModules(st->qrs[0], ims[0], black, 8, m, dim, dim, sepdim, sepdim, mag);

	/*
	 * GIFアニメーションヘッダを取得する
	 */
	ibuf = gdImageGifAnimBeginPtr(ims[0], &frame_size, 1, 0);
	if (ibuf == NULL) {
		qrsFreeGIFAnim(ims, 1, NULL, NULL);
		QRCNV_RETURN_FAILURE(QR_ERR_IMAGEFRAME, NULL);
	}
	/*
	 * GIFアニメーションヘッダを書き込む
	 */
	sbuf = (qr_byte_t *)malloc((size_t)frame_size);
	if (sbuf == NULL) {
		qrsFreeGIFAnim(ims, 1, NULL, ibuf);
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}
	memcpy(sbuf, ibuf, (size_t)frame_size);
	*size = frame_size;
	gdFree(ibuf);

	/*
	 * フレームを取得する
	 */
	ibuf = gdImageGifAnimAddPtr(ims[0], &frame_size, 0, 0, 0, delay, 1, NULL);
	if (ibuf == NULL) {
		qrsFreeGIFAnim(ims, 1, sbuf, NULL);
		QRCNV_RETURN_FAILURE(QR_ERR_IMAGEFRAME, NULL);
	}
	/*
	 * フレームを書き込む
	 */
	sbuf = (qr_byte_t *)realloc(sbuf, (size_t)(*size + frame_size));
	if (sbuf == NULL) {
		qrsFreeGIFAnim(ims, 1, NULL, ibuf);
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}
	memcpy(sbuf + *size, ibuf, (size_t)frame_size);
	*size += frame_size;
	gdFree(ibuf);

	/*
	 * 2枚目以降のシンボルを描画し、フレームを書き込む
	 */
	for (i = 1; i < st->num; i++) {
		/*
		 * 画像を初期化する
		 */
		ims[i] = gdImageCreate(imgdim, imgdim);
		if (ims[i] == NULL) {
			qrsFreeGIFAnim(ims, i, sbuf, NULL);
			QRCNV_RETURN_FAILURE3(QR_ERR_IMAGECREATE, "image[%d]: %dx%d", i, imgdim, imgdim);
		}
		gdImagePaletteCopy(ims[i], ims[0]);
		gdImageFill(ims[i], 0, 0, white);
		/*
		 * 位置検出パターンを描画する
		 */
		qrGdDrawFinderPatterns(ims[i], black, white, dim, sepdim, sepdim, mag);
		/*
		 * 暗モジュールを描画する
		 */
		qrGdDrawDarkModules(st->qrs[i], ims[i], black, 8, 0, m, 8, sepdim, sepdim, mag);
		qrGdDrawDarkModules(st->qrs[i], ims[i], black, 0, 8, dim, m, sepdim, sepdim, mag);
		qrGdDrawDarkModules(st->qrs[i], ims[i], black, 8, m, dim, dim, sepdim, sepdim, mag);
		/*
		 * フレームを取得する
		 */
		ibuf = gdImageGifAnimAddPtr(ims[i], &frame_size, 0, 0, 0, delay, 1, ims[i - 1]);
		if (ibuf == NULL) {
			qrsFreeGIFAnim(ims, i + 1, sbuf, NULL);
			QRCNV_RETURN_FAILURE(QR_ERR_IMAGEFRAME, NULL);
		}
		/*
		 * フレームを書き込む
		 */
		sbuf = (qr_byte_t *)realloc(sbuf, (size_t)(*size + frame_size));
		if (sbuf == NULL) {
			qrsFreeGIFAnim(ims, i + 1, NULL, ibuf);
			QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
		}
		memcpy(sbuf + *size, ibuf, (size_t)frame_size);
		*size += frame_size;
		gdFree(ibuf);
	}

	/*
	 * 終端マーカーを取得する
	 */
	ibuf = gdImageGifAnimEndPtr(&frame_size);
	if (ibuf == NULL) {
		qrsFreeGIFAnim(ims, st->num, sbuf, NULL);
		QRCNV_RETURN_FAILURE(QR_ERR_IMAGEFRAME, NULL);
	}
	/*
	 * 終端マーカーを書き込む
	 */
	sbuf = (qr_byte_t *)realloc(sbuf, (size_t)(*size + frame_size));
	if (sbuf == NULL) {
		gdFree(ibuf);
		for (i = 0; i < st->num; i++) {
			gdImageDestroy(ims[i]);
		}
		qrsFreeGIFAnim(ims, st->num, NULL, ibuf);
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}
	memcpy(sbuf + *size, ibuf, (size_t)frame_size);
	*size += frame_size;
	qrsFreeGIFAnim(ims, st->num, NULL, ibuf);

	return sbuf;
}

/* }}} */
#endif
/* {{{ qrGdImageCreate() */

/*
 * 画像と描画色・背景色を初期化する
 */
static gdImagePtr
qrGdImageCreate(int width, int height,
		int *fgcolor, int fgred, int fggreen, int fgblue,
		int *bgcolor, int bgred, int bggreen, int bgblue)
{
	gdImagePtr im;

	im = gdImageCreate(width, height);
	if (im == NULL) {
		return NULL;
	}

	*fgcolor = gdImageColorAllocate(im, fgred, fggreen, fgblue);
	*bgcolor = gdImageColorAllocate(im, bgred, bggreen, bgblue);
	gdImageFill(im, 0, 0, *bgcolor);

	return im;
}

/* }}} */
/* {{{ qrGdImageFinalize() */

/*
 * 生成したイメージを指定された画像形式に変換し、リソースを破棄する
 */
static qr_byte_t *
qrGdImageFinalize(QRCode *qr, gdImagePtr im, int fmt, int *size)
{
	qr_byte_t *sbuf;
	void *ibuf;

	switch (fmt) {
	  case QR_FMT_GIF:
		ibuf = gdImageGifPtr(im, size);
		break;
	  case QR_FMT_JPEG:
		ibuf = gdImageJpegPtr(im, size, QRCNV_JPEG_QUALITY);
		break;
	  case QR_FMT_PNG:
		ibuf = gdImagePngPtrEx(im, size, QRCNV_PNG_LEVEL QRCNV_PNG_BASEFILTER);
		break;
	  case QR_FMT_WBMP:
		ibuf = gdImageWBMPPtr(im, size, 0);
		break;
	  default:
		gdImageDestroy(im);
		QRCNV_RETURN_FAILURE(QR_ERR_INVALID_FMT, NULL);
	}
	if (ibuf == NULL) {
		gdImageDestroy(im);
		QRCNV_RETURN_FAILURE(QR_ERR_IMAGEFORMAT, NULL);
	}

	sbuf = (qr_byte_t *)malloc((size_t)*size);
	if (sbuf == NULL) {
		gdFree(ibuf);
		gdImageDestroy(im);
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}
	memcpy(sbuf, ibuf, (size_t)*size);
	gdFree(ibuf);
	gdImageDestroy(im);

	return sbuf;
}

/* }}} */
/* {{{ qrGdDrawFinderPatterns() */

/*
 * 位置検出パターンを描画する
 */
static void
qrGdDrawFinderPatterns(gdImagePtr im, int fgcolor, int bgcolor,
		int dim, int xoffset, int yoffset, int mag)
{
	int x, y, z1, z2, z3;

	z1 = 7 * mag - 1;
	z2 = 5 * mag - 1;
	z3 = 3 * mag - 1;

	/*
	 * 左上
	 */
	x = xoffset;
	y = yoffset;
	gdImageFilledRectangle(im, x, y, x + z1, y + z1, fgcolor);
	x += mag;
	y += mag;
	gdImageFilledRectangle(im, x, y, x + z2, y + z2, bgcolor);
	x += mag;
	y += mag;
	gdImageFilledRectangle(im, x, y, x + z3, y + z3, fgcolor);

	/*
	 * 右上
	 */
	x = xoffset + (dim - 7) * mag;
	y = yoffset;
	gdImageFilledRectangle(im, x, y, x + z1, y + z1, fgcolor);
	x += mag;
	y += mag;
	gdImageFilledRectangle(im, x, y, x + z2, y + z2, bgcolor);
	x += mag;
	y += mag;
	gdImageFilledRectangle(im, x, y, x + z3, y + z3, fgcolor);

	/*
	 * 左下
	 */
	x = xoffset;
	y = yoffset + (dim - 7) * mag;
	gdImageFilledRectangle(im, x, y, x + z1, y + z1, fgcolor);
	x += mag;
	y += mag;
	gdImageFilledRectangle(im, x, y, x + z2, y + z2, bgcolor);
	x += mag;
	y += mag;
	gdImageFilledRectangle(im, x, y, x + z3, y + z3, fgcolor);
}

/* }}} */
/* {{{ qrGdDrawDarkModules() */

/*
 * 暗モジュールを描画する
 */
static void
qrGdDrawDarkModules(QRCode *qr, gdImagePtr im, int fgcolor,
		int xfrom, int yfrom, int xto, int yto,
		int xoffset, int yoffset, int mag)
{
	int i, j;
	if (mag == 1) {
		for (i = yfrom; i < yto; i++) {
			for (j = xfrom; j < xto; j++) {
				if (qrIsBlack(qr, i, j)) {
					gdImageSetPixel(im, j + xoffset, i + yoffset, fgcolor);
				}
			}
		}
	} else {
		int x, y, z;
		z = mag - 1;
		for (i = yfrom; i < yto; i++) {
			for (j = xfrom; j < xto; j++) {
				if (qrIsBlack(qr, i, j)) {
					x = j * mag + xoffset;
					y = i * mag + yoffset;
					gdImageFilledRectangle(im, x, y, x + z, y + z, fgcolor);
				}
			}
		}
	}
}

/* }}} */
#ifdef QRCNV_ENABLE_GIFANIM
/* {{{ qrsFreeGIFAnim() */

/*
 * qrsSymbolsToGIFAnim() で動的に確保したメモリを開放する
 */
static void
qrsFreeGIFAnim(gdImagePtr ims[], int num, qr_byte_t *sbuf, void *ibuf)
{
	int i;
	for (i = 0; i < num; i++) {
		gdImageDestroy(ims[i]);
	}
	if (sbuf != NULL) {
		free(sbuf);
	}
	if (ibuf != NULL) {
		gdFree(ibuf);
	}
}

/* }}} */
#endif
/* {{{ Wrappers of qrSymbolToImage() */

/*
 * 生成されたQRコードシンボルをGDを用いてGIF画像に変換する
 */
QR_API qr_byte_t *
qrSymbolToGIF(QRCode *qr, int sep, int mag, int *size)
{
#ifdef QR_NO_GD_GIF
	QRCNV_RETURN_FAILURE(QR_ERR_INVALID_FMT, _QR_FUNCTION);
#else
	return qrSymbolToImage(qr, QR_FMT_GIF, sep, mag, size);
#endif
}

/*
 * 生成されたQRコードシンボルをGDを用いてJPEG画像に変換する
 */
QR_API qr_byte_t *
qrSymbolToJPEG(QRCode *qr, int sep, int mag, int *size)
{
#ifdef QR_NO_GD_JPEG
	QRCNV_RETURN_FAILURE(QR_ERR_INVALID_FMT, _QR_FUNCTION);
#else
	return qrSymbolToImage(qr, QR_FMT_JPEG, sep, mag, size);
#endif
}

/*
 * 生成されたQRコードシンボルをGDを用いてPNG画像に変換する
 */
QR_API qr_byte_t *
qrSymbolToPNG(QRCode *qr, int sep, int mag, int *size)
{
#ifdef QR_NO_GD_PNG
	QRCNV_RETURN_FAILURE(QR_ERR_INVALID_FMT, _QR_FUNCTION);
#else
	return qrSymbolToImage(qr, QR_FMT_PNG, sep, mag, size);
#endif
}

/*
 * 生成されたQRコードシンボルをGDを用いてWBMP画像に変換する
 */
QR_API qr_byte_t *
qrSymbolToWBMP(QRCode *qr, int sep, int mag, int *size)
{
#ifdef QR_NO_GD_WBMP
	QRCNV_RETURN_FAILURE(QR_ERR_INVALID_FMT, _QR_FUNCTION);
#else
	return qrSymbolToImage(qr, QR_FMT_WBMP, sep, mag, size);
#endif
}

/* }}} */
/* {{{ Wrappers of qrsSymbolsToImage() */

/*
 * 生成されたQRコードシンボルをGDを用いてGIF画像に変換する
 */
QR_API qr_byte_t *
qrsSymbolsToGIF(QRStructured *st, int sep, int mag, int order, int *size)
{
#ifdef QR_NO_GD_GIF
	QRCNV_RETURN_FAILURE(QR_ERR_INVALID_FMT, _QR_FUNCTION);
#else
	return qrsSymbolsToImage(st, QR_FMT_GIF, sep, mag, order, size);
#endif
}

/*
 * 生成されたQRコードシンボルをGDを用いてJPEG画像に変換する
 */
QR_API qr_byte_t *
qrsSymbolsToJPEG(QRStructured *st, int sep, int mag, int order, int *size)
{
#ifdef QR_NO_GD_JPEG
	QRCNV_RETURN_FAILURE(QR_ERR_INVALID_FMT, _QR_FUNCTION);
#else
	return qrsSymbolsToImage(st, QR_FMT_JPEG, sep, mag, order, size);
#endif
}

/*
 * 生成されたQRコードシンボルをGDを用いてPNG画像に変換する
 */
QR_API qr_byte_t *
qrsSymbolsToPNG(QRStructured *st, int sep, int mag, int order, int *size)
{
#ifdef QR_NO_GD_PNG
	QRCNV_RETURN_FAILURE(QR_ERR_INVALID_FMT, _QR_FUNCTION);
#else
	return qrsSymbolsToImage(st, QR_FMT_PNG, sep, mag, order, size);
#endif
}

/*
 * 生成されたQRコードシンボルをGDを用いてWBMP画像に変換する
 */
QR_API qr_byte_t *
qrsSymbolsToWBMP(QRStructured *st, int sep, int mag, int order, int *size)
{
#ifdef QR_NO_GD_WBMP
	QRCNV_RETURN_FAILURE(QR_ERR_INVALID_FMT, _QR_FUNCTION);
#else
	return qrsSymbolsToImage(st, QR_FMT_WBMP, sep, mag, order, size);
#endif
}

/* }}} */
