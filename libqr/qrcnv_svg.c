/*
 * QR Code Generator Library: Symbol Converters for SVG
 *
 * Core routines were originally written by Junn Ohta.
 * Based on qr.c Version 0.1: 2004/4/3 (Public Domain)
 *
 * @package     libqr
 * @author      Ryusuke SEKIYAMA <rsky0711@gmail.com>
 * @copyright   2006-2013 Ryusuke SEKIYAMA
 * @license     http://www.opensource.org/licenses/mit-license.php  MIT License
 */

#include "qrcnv.h"

/* {{{ constants */

#define QRCNV_SVG_BUFFER_UNIT 8192

#define QRCNV_SVG_BASE_TAGS_TMPL \
	"<?xml version=\"1.0\" standalone=\"no\"?>\n" \
	"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"\n" \
	"  \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n" \
	"<svg width=\"%d\" height=\"%d\" version=\"1.1\"\n" \
	"  xmlns=\"http://www.w3.org/2000/svg\"\n" \
	"  xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n" \
	" <desc>QR Code (version=%d, ecl=%s%s)</desc>\n" \
	" <defs>\n" \
	"  <rect id=\"m\" width=\"1\" height=\"1\" fill=\"black\"/>\n" \
	"  <g id=\"p\">\n" \
	"   <rect x=\"0\" y=\"0\" width=\"7\" height=\"7\" fill=\"black\"/>\n" \
	"   <rect x=\"1\" y=\"1\" width=\"5\" height=\"5\" fill=\"white\"/>\n" \
	"   <rect x=\"2\" y=\"2\" width=\"3\" height=\"3\" fill=\"black\"/>\n" \
	"  </g>\n" \
	" </defs>\n" \
	" <rect x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"white\"/>\n"

#define QRCNV_SVG_GROUP_TAGS_TMPL \
	" <g transform=\"translate(%d, %d) scale(%d)\">\n" \
	"  <use xlink:href=\"#p\"/>\n" \
	"  <use xlink:href=\"#p\" transform=\"translate(%d, 0)\"/>\n" \
	"  <use xlink:href=\"#p\" transform=\"translate(0, %d)\"/>\n"

/* }}} */
/* {{{ memory reallocation macro */

#define QRCNV_SVG_REALLOC(reqsize) { \
	while (*size + (reqsize) > bufsize) { \
		bufsize += QRCNV_SVG_BUFFER_UNIT; \
		wbuf = realloc(wbuf, (size_t)bufsize); \
		if (wbuf == NULL) { \
			QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION); \
		} \
		wptr = wbuf + *size; \
	} \
}

/* }}} */
/* {{{ rectangle writing macro */

#define qrSvgWriteRectangle(qr, i, j) { \
	if (qrIsBlack((qr), (i), (j))) { \
		QRCNV_SVG_REALLOC(64); \
		*size += snprintf(wptr, 64, \
				"  <use xlink:href=\"#m\" x=\"%d\" y=\"%d\"/>\n", j, i); \
		wptr = wbuf + *size; \
	} \
}

/* }}} */
/* {{{ qrSymbolToSVG() */

/*
 * 生成されたQRコードシンボルをSVGに変換する
 */
QR_API qr_byte_t *
qrSymbolToSVG(QRCode *qr, int sep, int mag, int *size)
{
	qr_byte_t *sbuf;
	char *wbuf, *wptr;
	int bufsize;
	int i, j, dim, imgdim, sepdim;

	QRCNV_CHECK_STATE();
	QRCNV_GET_SIZE();

	/*
	 * SVGを初期化する
	 */
	bufsize = QRCNV_SVG_BUFFER_UNIT;
	wbuf = (char *)malloc((size_t)bufsize);
	if (wbuf == NULL) {
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}
	*size = snprintf(wbuf, (size_t)bufsize,
			QRCNV_SVG_BASE_TAGS_TMPL QRCNV_SVG_GROUP_TAGS_TMPL,
			imgdim, imgdim,
			qr->param.version, qr_eclname[qr->param.eclevel], "",
			imgdim, imgdim,
			sepdim, sepdim, mag, dim - 7, dim - 7);
	wptr = wbuf + *size;

	/*
	 * 暗モジュールを配置する
	 */
	for (i = 0; i < 8; i++) {
		for (j = 8; j < dim - 8; j++) {
			qrSvgWriteRectangle(qr, i, j);
		}
	}
	for (i = 8; i < dim - 8; i++) {
		for (j = 0; j < dim; j++) {
			qrSvgWriteRectangle(qr, i, j);
		}
	}
	for (i = dim - 8; i < dim; i++) {
		for (j = 8; j < dim; j++) {
			qrSvgWriteRectangle(qr, i, j);
		}
	}

	/*
	 * SVGを閉じる
	 */
	QRCNV_SVG_REALLOC(16);
	*size += snprintf(wptr, 16, " </g>\n</svg>\n");

	/*
	 * SVGをコピーする
	 */
	sbuf = (qr_byte_t *)malloc((size_t)(*size + 1));
	if (sbuf == NULL) {
		free(wbuf);
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}
	memcpy(sbuf, wbuf, (size_t)*size);
	sbuf[*size] = '\0';
	free(wbuf);

	return sbuf;
}

/* }}} */
/* {{{ qrsSymbolsToSVG() */

/*
 * 構造的連接用qrSymbolToSVG()
 */
QR_API qr_byte_t *
qrsSymbolsToSVG(QRStructured *st, int sep, int mag, int order, int *size)
{
	QRCode *qr = st->cur;
	qr_byte_t *sbuf;
	char *wbuf, *wptr;
	int bufsize;
	int i, j, k, l;
	int cols, rows, pos, xdim, ydim, zdim;
	int dim, imgdim, sepdim;
	char extrainfo[32];

	QRCNV_SA_CHECK_STATE();
	QRCNV_SA_IF_ONE(qrSymbolToSVG);
	QRCNV_SA_GET_SIZE();

	/*
	 * SVGを初期化する
	 */
	bufsize = QRCNV_SVG_BUFFER_UNIT;
	wbuf = (char *)malloc((size_t)bufsize);
	if (wbuf == NULL) {
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}
	snprintf(&(extrainfo[0]), 32, ", structured-append=%d", st->num);
	*size = snprintf(wbuf, (size_t)bufsize,
			QRCNV_SVG_BASE_TAGS_TMPL,
			xdim, ydim,
			st->param.version, qr_eclname[st->param.eclevel], extrainfo,
			imgdim, imgdim);
	wptr = wbuf + *size;

	/*
	 * シンボルを書き込む
	 */
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
			/*
			 * 開始タグ + 位置検出パターン
			 */
			QRCNV_SVG_REALLOC(1024);
			*size += snprintf(wptr, 1024,
					QRCNV_SVG_GROUP_TAGS_TMPL,
					l * (sepdim + zdim) + sepdim,
					k * (sepdim + zdim) + sepdim,
					mag, dim - 7, dim - 7);
			wptr = wbuf + *size;
			/*
			 * 暗モジュール
			 */
			for (i = 0; i < 8; i++) {
				for (j = 8; j < dim - 8; j++) {
					qrSvgWriteRectangle(st->qrs[pos], i, j);
				}
			}
			for (i = 8; i < dim - 8; i++) {
				for (j = 0; j < dim; j++) {
					qrSvgWriteRectangle(st->qrs[pos], i, j);
				}
			}
			for (i = dim - 8; i < dim; i++) {
				for (j = 8; j < dim; j++) {
					qrSvgWriteRectangle(st->qrs[pos], i, j);
				}
			}
			/*
			 * 終了タグ
			 */
			QRCNV_SVG_REALLOC(8);
			*size += snprintf(wptr, 8, " </g>\n");
			wptr = wbuf + *size;
		}
	}

	/*
	 * SVGを閉じる
	 */
	QRCNV_SVG_REALLOC(16);
	*size += snprintf(wptr, 16, "</svg>\n");

	/*
	 * SVGをコピーする
	 */
	sbuf = (qr_byte_t *)malloc((size_t)(*size + 1));
	if (sbuf == NULL) {
		free(wbuf);
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}
	memcpy(sbuf, wbuf, (size_t)*size);
	sbuf[*size] = '\0';
	free(wbuf);

	return sbuf;
}

/* }}} */
