/*
 * QR Code Generator Library: Symbol Converters for TIFF with zip compression
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
#if defined(__BIG_ENDIAN__) || defined(__LITTLE_ENDIAN__)
#include <stdint.h>
#endif
#include <zlib.h>

/* {{{ constants */

/* maximum size of a strip: 8KB */
#define QRCNV_TIFF_STRIP_SIZE 8192

/* maximum size of a deflated strip: same to strip */
#define QRCNV_TIFF_ZBUFFER_SIZE QRCNV_TIFF_STRIP_SIZE

/* memory allocation unit: 8KB */
#define QRCNV_TIFF_BUFFER_UNIT 8192

/* size of the TIFF header [short(2) + short(2) + long(4)] */
#define QRCNV_TIFF_HEADER_SIZE 8

/* number of IFD (image file directory) entries */
#define QRCNV_TIFF_IFD_ENTRIES 12

/* size of the IFD */
#define QRCNV_TIFF_IFD_SIZE (2 + 12 * QRCNV_TIFF_IFD_ENTRIES + 2)

/* offset for the StripByteCounts IFD tag */
#define QRCNV_TIFF_IFD_STRIPBYTECOUNTS_OFFSET (QRCNV_TIFF_HEADER_SIZE + 2 + 12 * 7)

/* offset for the image data or the strip information tables */
#define QRCNV_TIFF_DATA_OFFSET (QRCNV_TIFF_HEADER_SIZE + QRCNV_TIFF_IFD_SIZE + 8 + 8)

/* data type codes */
#define QRCNV_TIFF_T_SHORT      3
#define QRCNV_TIFF_T_LONG       4
#define QRCNV_TIFF_T_RATIONAL   5

/* compression methods */
#define QRCNV_TIFF_COMPRESSION_NONE 1
#define QRCNV_TIFF_COMPRESSION_ZIP  8

/* deflate compression level: use zlib's default */
#ifndef QRCNV_TIFF_DEFLATE_LEVEL
#define QRCNV_TIFF_DEFLATE_LEVEL Z_DEFAULT_COMPRESSION
#endif

/* }}} */
/* {{{ memory reallocation macro */

#define QRCNV_TIFF_REALLOC(reqsize) { \
	while (*size + (reqsize) > wsize) { \
		wsize += QRCNV_TIFF_BUFFER_UNIT; \
		wbuf = realloc(wbuf, (size_t)wsize); \
		if (wbuf == NULL) { \
			free(rbuf); \
			if (compression == QRCNV_TIFF_COMPRESSION_ZIP) { \
				deflateEnd(&zst); \
			} \
			QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION); \
		} \
		wptr = wbuf + *size; \
	} \
}

/* }}} */
/* {{{ deflate error handler macro */

#define QRCNV_TIFF_DEFLATE_RETURN_FAILURE(errinfo) { \
	char _info[128]; \
	if (zst.msg) { \
		snprintf(&(_info[0]), 128, "%s", zst.msg); \
	} else { \
		snprintf(&(_info[0]), 128, "%s", (errinfo)); \
	} \
	free(rbuf); \
	free(wbuf); \
	deflateEnd(&zst); \
	QRCNV_RETURN_FAILURE(QR_ERR_DEFLATE, _info); \
}

/* }}} */
/* {{{ tiff data writing macro */

#define qrTiffWriteAscii(ptr, n) { \
	*(ptr)++ = (qr_byte_t)((n) & 0xffU); \
}

#if defined(__BIG_ENDIAN__)
#define qrTiffWriteShort(ptr, n) { \
	uint16_t us = (uint16_t)((n) & 0xffffU); \
	qr_byte_t *tmp = (qr_byte_t *)&us; \
	*(ptr)++ = tmp[0]; \
	*(ptr)++ = tmp[1]; \
}
#elif defined(__LITTLE_ENDIAN__)
#define qrTiffWriteShort(ptr, n) { \
	uint16_t us = (uint16_t)((n) & 0xffffU); \
	qr_byte_t *tmp = (qr_byte_t *)&us; \
	*(ptr)++ = tmp[1]; \
	*(ptr)++ = tmp[0]; \
}
#else
#define qrTiffWriteShort(ptr, n) { \
	*(ptr)++ = ((n) >> 8) & 0xffU; \
	*(ptr)++ = (n) & 0xffU; \
}
#endif

#if defined(__BIG_ENDIAN__)
#define qrTiffWriteLong(ptr, n) { \
	uint32_t ul = (uint32_t)(n); \
	qr_byte_t *tmp = (qr_byte_t *)&ul; \
	*(ptr)++ = tmp[0]; \
	*(ptr)++ = tmp[1]; \
	*(ptr)++ = tmp[2]; \
	*(ptr)++ = tmp[3]; \
}
#elif defined(__LITTLE_ENDIAN__)
#define qrTiffWriteLong(ptr, n) { \
	uint32_t ul = (uint32_t)(n); \
	qr_byte_t *tmp = (qr_byte_t *)&ul; \
	*(ptr)++ = tmp[3]; \
	*(ptr)++ = tmp[2]; \
	*(ptr)++ = tmp[1]; \
	*(ptr)++ = tmp[0]; \
}
#else
#define qrTiffWriteLong(ptr, n) { \
	*(ptr)++ = ((n) >> 24) & 0xffU; \
	*(ptr)++ = ((n) >> 16) & 0xffU; \
	*(ptr)++ = ((n) >> 8) & 0xffU; \
	*(ptr)++ = (n) & 0xffU; \
}
#endif

#define qrTiffWriteRational(ptr, m, n) { \
	qrTiffWriteLong((ptr), (m)); \
	qrTiffWriteLong((ptr), (n)); \
}

#define qrTiffWriteIfd(ptr, tag, type, count, data) { \
	qrTiffWriteShort((ptr), (tag)); \
	qrTiffWriteShort((ptr), (type)); \
	qrTiffWriteLong((ptr), (count)); \
	if ((type) == QRCNV_TIFF_T_SHORT) { \
		qrTiffWriteShort((ptr), (data)); \
		qrTiffWriteShort((ptr), 0); \
	} else { \
		qrTiffWriteLong((ptr), (data)); \
	} \
}

/* }}} */
/* {{{ tiff strip writing macro */

#define qrTiffWriteStrip() { \
	if (compression == QRCNV_TIFF_COMPRESSION_ZIP) { \
		if (deflateReset(&zst) != Z_OK) { \
			QRCNV_TIFF_DEFLATE_RETURN_FAILURE("deflateReset()"); \
		} \
		zst.next_in = &(sbuf[0]); \
		zst.avail_in = (uInt)ssize; \
		zst.next_out = &(zbuf[0]); \
		zst.avail_out = QRCNV_TIFF_ZBUFFER_SIZE; \
		if (deflate(&zst, Z_FINISH) != Z_STREAM_END) { \
			QRCNV_TIFF_DEFLATE_RETURN_FAILURE("deflate()"); \
		} \
		zptr = &(zbuf[0]); \
		zsize = (int)zst.total_out; \
	} else { \
		zptr = &(sbuf[0]); \
		zsize = ssize; \
	} \
	if (totalstrips > 1) { \
		qrTiffUpdateStripInfoTables(wbuf, totalstrips, snum++, *size, zsize); \
	} else { \
		qrTiffUpdateStripByteCount(wbuf, zsize); \
	} \
	QRCNV_TIFF_REALLOC(zsize); \
	memcpy(wptr, zptr, (size_t)zsize); \
	wptr += zsize; \
	*size += zsize; \
}

/* }}} */
/* {{{ utility macro */

#define qrTiffNextPixel() { \
	if (pxshift == 0) { \
		rptr++; \
		pxshift = 7; \
	} else { \
		pxshift--; \
	} \
}

#define qrTiffEOR() { \
	pxshift = 7; \
	if (++rnum == rowsperstrip) { \
		qrTiffWriteStrip(); \
		memset(&(sbuf[0]), 0, QRCNV_TIFF_STRIP_SIZE); \
		sptr = &(sbuf[0]); \
		ssize = 0; \
		rnum = 0; \
	} \
}

/* }}} */
/* {{{ function prototypes */

static qr_byte_t *
qrTiffWriteHeader(qr_byte_t *bof, int width, int height,
		int rowsperstrip, int totalstrips, int compression);

static void
qrTiffUpdateStripByteCount(qr_byte_t *bof, int size);

static void
qrTiffUpdateStripInfoTables(qr_byte_t *bof,
		int totalstrips, int stripnumber, int offset, int size);

/* }}} */
/* {{{ qrSymbolToTIFF() */

/*
 * 生成されたQRコードシンボルを
 * ビッグエンディアン・モノクロ2値 (正順) のTIFFに変換する
 * 2倍以上に拡大するときは ZIP (deflate) 圧縮も適用される
 */
QR_API qr_byte_t *
qrSymbolToTIFF(QRCode *qr, int sep, int mag, int *size)
{
	qr_byte_t *rbuf, *wbuf;
	qr_byte_t sbuf[QRCNV_TIFF_STRIP_SIZE];
	qr_byte_t zbuf[QRCNV_TIFF_ZBUFFER_SIZE];
	qr_byte_t *rptr, *sptr, *wptr, *zptr;
	int rsize, ssize, wsize, zsize; /* size_t, size_t, size_t, size_t */
	int rowsperstrip, totalstrips, compression; /* uint32_t, uint32_t, uint16_t */
	int rnum, snum, pxshift;
	int i, j, ix, jx, dim, imgdim, sepdim;
	z_stream zst;

	QRCNV_CHECK_STATE();
	QRCNV_GET_SIZE();

	if (mag > 1) {
		compression = QRCNV_TIFF_COMPRESSION_ZIP;
	} else {
		compression = QRCNV_TIFF_COMPRESSION_NONE;
	}

	/*
	 * 一行あたりのバイト数、ストリップあたりの行数、総ストリップ数を計算する
	 */
	rsize = (imgdim + 7) / 8;
	rowsperstrip = QRCNV_TIFF_STRIP_SIZE / rsize;
	if (rowsperstrip == 0) {
		QRCNV_RETURN_FAILURE(QR_ERR_WIDTH_TOO_LARGE, NULL);
	} else if (rowsperstrip > imgdim) {
		rowsperstrip = imgdim;
	}
	totalstrips = (imgdim + rowsperstrip - 1) / rowsperstrip;
	wsize = QRCNV_TIFF_BUFFER_UNIT;

	/*
	 * メモリを確保し、画像を初期化する
	 */
	rbuf = (qr_byte_t *)malloc((size_t)rsize);
	if (rbuf == NULL) {
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}
	wbuf = (qr_byte_t *)malloc((size_t)wsize);
	if (wbuf == NULL) {
		free(rbuf);
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}
	wptr = qrTiffWriteHeader(wbuf, imgdim, imgdim, rowsperstrip, totalstrips, compression);
	*size = (int)(wptr - wbuf);

	/*
	 * deflate圧縮ストリームを初期化する
	 */
	if (compression == QRCNV_TIFF_COMPRESSION_ZIP) {
		zst.zalloc = Z_NULL;
		zst.zfree  = Z_NULL;
		zst.opaque = Z_NULL;
		if (deflateInit(&zst, QRCNV_TIFF_DEFLATE_LEVEL) != Z_OK) {
			free(rbuf);
			free(wbuf);
			QRCNV_RETURN_FAILURE(QR_ERR_DEFLATE, "deflateInit()");
		}
	}

	/*
	 * シンボルを書き込む
	 */
	memset(&(sbuf[0]), 0, QRCNV_TIFF_STRIP_SIZE);
	sptr = &(sbuf[0]);
	ssize = 0;
	rnum = 0;
	snum = 0;
	pxshift = 7;
	/* 分離パターン (上) */
	for (i = 0; i < sepdim; i++) {
		sptr += rsize;
		ssize += rsize;
		qrTiffEOR();
	}
	for (i = 0; i < dim; i++) {
		memset(rbuf, 0, (size_t)rsize);
		rptr = rbuf;
		/* 分離パターン (左) */
		for (j = 0; j < sepdim; j++) {
			qrTiffNextPixel();
		}
		/* シンボル本体 */
		for (j = 0; j < dim; j++) {
			if (qrIsBlack(qr, i, j)) {
				for (jx = 0; jx < mag; jx++) {
					*rptr |= 1 << pxshift;
					qrTiffNextPixel();
				}
			} else {
				for (jx = 0; jx < mag; jx++) {
					qrTiffNextPixel();
				}
			}
		}
		/* 行をmag回繰り返し書き込む */
		for (ix = 0; ix < mag; ix++) {
			memcpy(sptr, rbuf, (size_t)rsize);
			sptr += rsize;
			ssize += rsize;
			qrTiffEOR();
		}
	}
	/* 分離パターン (下) */
	for (i = 0; i < sepdim; i++) {
		sptr += rsize;
		ssize += rsize;
		qrTiffEOR();
	}
	/* 書き込まれていないストリップを書き込む */
	if (ssize > 0) {
		qrTiffWriteStrip();
	}
	free(rbuf);

	/*
	 * deflate圧縮ストリームを開放する
	 */
	if (compression == QRCNV_TIFF_COMPRESSION_ZIP) {
		if (deflateEnd(&zst) != Z_OK) {
			free(wbuf);
			QRCNV_RETURN_FAILURE(QR_ERR_DEFLATE, "deflateEnd()");
		}
	}

	/*
	 * 余分に確保したメモリ領域を切り詰める
	 */
	wbuf = (qr_byte_t *)realloc(wbuf, (size_t)*size);
	if (wbuf == NULL) {
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}

	return wbuf;
}

/* }}} */
/* {{{ qrsSymbolsToTIFF() */

/*
 * 生成された構造的連接QRコードシンボルを
 * ビッグエンディアン・モノクロ2値 (正順) のTIFFに変換する
 * 2倍以上に拡大するときは ZIP (deflate) 圧縮も適用される
 */
QR_API qr_byte_t *
qrsSymbolsToTIFF(QRStructured *st, int sep, int mag, int order, int *size)
{
	QRCode *qr = st->cur;
	qr_byte_t *rbuf, *wbuf;
	qr_byte_t sbuf[QRCNV_TIFF_STRIP_SIZE];
	qr_byte_t zbuf[QRCNV_TIFF_ZBUFFER_SIZE];
	qr_byte_t *rptr, *sptr, *wptr, *zptr;
	int rsize, ssize, wsize, zsize; /* size_t, size_t, size_t, size_t */
	int rowsperstrip, totalstrips, compression; /* uint32_t, uint32_t, uint16_t */
	int rnum, snum, pxshift;
	int i, j, k, ix, jx, kx;
	int cols, rows, pos, xdim, ydim, zdim;
	int dim, imgdim, sepdim;
	z_stream zst;

	QRCNV_SA_CHECK_STATE();
	QRCNV_SA_IF_ONE(qrSymbolToTIFF);
	QRCNV_SA_GET_SIZE();

	if (mag > 1) {
		compression = QRCNV_TIFF_COMPRESSION_ZIP;
	} else {
		compression = QRCNV_TIFF_COMPRESSION_NONE;
	}

	/*
	 * 一行あたりのバイト数、ストリップあたりの行数、総ストリップ数を計算する
	 */
	rsize = (xdim + 7) / 8;
	rowsperstrip = QRCNV_TIFF_STRIP_SIZE / rsize;
	if (rowsperstrip == 0) {
		QRCNV_RETURN_FAILURE(QR_ERR_WIDTH_TOO_LARGE, NULL);
	} else if (rowsperstrip > ydim) {
		rowsperstrip = ydim;
	}
	totalstrips = (ydim + rowsperstrip - 1) / rowsperstrip;
	wsize = QRCNV_TIFF_BUFFER_UNIT;

	/*
	 * メモリを確保し、画像を初期化する
	 */
	rbuf = (qr_byte_t *)malloc((size_t)rsize);
	if (rbuf == NULL) {
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}
	wbuf = (qr_byte_t *)malloc((size_t)wsize);
	if (wbuf == NULL) {
		free(rbuf);
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}
	wptr = qrTiffWriteHeader(wbuf, xdim, ydim, rowsperstrip, totalstrips, compression);
	*size = (int)(wptr - wbuf);

	/*
	 * deflate圧縮ストリームを初期化する
	 */
	if (compression == QRCNV_TIFF_COMPRESSION_ZIP) {
		zst.zalloc = Z_NULL;
		zst.zfree  = Z_NULL;
		zst.opaque = Z_NULL;
		if (deflateInit(&zst, QRCNV_TIFF_DEFLATE_LEVEL) != Z_OK) {
			free(rbuf);
			free(wbuf);
			QRCNV_RETURN_FAILURE(QR_ERR_DEFLATE, "deflateInit()");
		}
	}

	/*
	 * シンボルを書き込む
	 */
	memset(&(sbuf[0]), 0, QRCNV_TIFF_STRIP_SIZE);
	sptr = &(sbuf[0]);
	ssize = 0;
	rnum = 0;
	snum = 0;
	pxshift = 7;
	for (k = 0; k < rows; k++) {
		/* 分離パターン (上) */
		for (i = 0; i < sepdim; i++) {
			sptr += rsize;
			ssize += rsize;
			qrTiffEOR();
		}
		for (i = 0; i < dim; i++) {
			memset(rbuf, 0, (size_t)rsize);
			rptr = rbuf;
			for (kx = 0; kx < cols; kx++) {
				/* 分離パターン (左) */
				for (j = 0; j < sepdim; j++) {
					qrTiffNextPixel();
				}
				/* シンボル本体 */
				if (order < 0) {
					pos = k + rows * kx;
				} else {
					pos = cols * k + kx;
				}
				if (pos >= st->num) {
					break;
				}
				for (j = 0; j < dim; j++) {
					if (qrIsBlack(st->qrs[pos], i, j)) {
						for (jx = 0; jx < mag; jx++) {
							*rptr |= 1 << pxshift;
							qrTiffNextPixel();
						}
					} else {
						for (jx = 0; jx < mag; jx++) {
							qrTiffNextPixel();
						}
					}
				}
			}
			/* 行をmag回繰り返し書き込む */
			for (ix = 0; ix < mag; ix++) {
				memcpy(sptr, rbuf, (size_t)rsize);
				sptr += rsize;
				ssize += rsize;
				qrTiffEOR();
			}
		}
	}
	/* 分離パターン (下) */
	for (i = 0; i < sepdim; i++) {
		sptr += rsize;
		ssize += rsize;
		qrTiffEOR();
	}
	/* 書き込まれていないストリップを書き込む */
	if (ssize > 0) {
		qrTiffWriteStrip();
	}
	free(rbuf);

	/*
	 * deflate圧縮ストリームを開放する
	 */
	if (compression == QRCNV_TIFF_COMPRESSION_ZIP) {
		if (deflateEnd(&zst) != Z_OK) {
			free(wbuf);
			QRCNV_RETURN_FAILURE(QR_ERR_DEFLATE, "deflateEnd()");
		}
	}

	/*
	 * 余分に確保したメモリ領域を切り詰める
	 */
	wbuf = (qr_byte_t *)realloc(wbuf, (size_t)*size);
	if (wbuf == NULL) {
		QRCNV_RETURN_FAILURE2(QR_ERR_MEMORY_EXHAUSTED, _QR_FUNCTION);
	}

	return wbuf;
}

/* }}} */
/* {{{ qrTiffWriteHeader() */

/*
 * TIFFヘッダを書き込む
 */
static qr_byte_t *
qrTiffWriteHeader(qr_byte_t *bof, int width, int height,
		int rowsperstrip, int totalstrips, int compression)
{
	qr_byte_t *ptr;
	int xresoffset, yresoffset;
	int stripoffsets, stripbytecounts;

	xresoffset = QRCNV_TIFF_HEADER_SIZE + QRCNV_TIFF_IFD_SIZE;
	yresoffset = xresoffset + 8;
	if (totalstrips > 1) {
		/* offset for the offset table */
		stripoffsets = yresoffset + 8;
		/* offset for the byte count table */
		stripbytecounts = stripoffsets + 4 * totalstrips;
	} else {
		/* offset for the image data */
		stripoffsets = yresoffset + 8;
		/* pseudo strip size */
		stripbytecounts = 0;
	}

	ptr = bof;

	/* TIFF header */
	qrTiffWriteAscii(ptr, 'M'); /* byte order mark */
	qrTiffWriteAscii(ptr, 'M'); /* Motorola (big endian) */
	qrTiffWriteShort(ptr, 42);  /* version (magic) number */
	qrTiffWriteLong(ptr, QRCNV_TIFF_HEADER_SIZE);

	/* IFD entry count */
	qrTiffWriteShort(ptr, QRCNV_TIFF_IFD_ENTRIES);

	/* IFD entry - ImageWidth */
	qrTiffWriteIfd(ptr, 0x0100, QRCNV_TIFF_T_LONG, 1, width);

	/* IFD entry - ImageLength */
	qrTiffWriteIfd(ptr, 0x0101, QRCNV_TIFF_T_LONG, 1, height);

	/* IFD entry - BitsPerSample */
	qrTiffWriteIfd(ptr, 0x0102, QRCNV_TIFF_T_SHORT, 1, 1); /* monochrome */

	/* IFD entry - Compression */
	qrTiffWriteIfd(ptr, 0x0103, QRCNV_TIFF_T_SHORT, 1, compression);

	/* IFD entry - PhotometricInterpretation */
	qrTiffWriteIfd(ptr, 0x0106, QRCNV_TIFF_T_SHORT, 1, 0); /* black code */

	/* IFD entry - StripOffsets */
	qrTiffWriteIfd(ptr, 0x0111, QRCNV_TIFF_T_LONG, totalstrips, stripoffsets);

	/* IFD entry - RowsPerStrip */
	qrTiffWriteIfd(ptr, 0x0116, QRCNV_TIFF_T_LONG, 1, rowsperstrip);

	/* IFD entry - StripByteCounts */
	qrTiffWriteIfd(ptr, 0x0117, QRCNV_TIFF_T_LONG, totalstrips, stripbytecounts);

	/* IFD entry - XResolution */
	qrTiffWriteIfd(ptr, 0x011A, QRCNV_TIFF_T_RATIONAL, 1, xresoffset);

	/* IFD entry - YResolution */
	qrTiffWriteIfd(ptr, 0x011B, QRCNV_TIFF_T_RATIONAL, 1, yresoffset);

	/* IFD entry - ResolutionUnit */
	qrTiffWriteIfd(ptr, 0x0128, QRCNV_TIFF_T_SHORT, 1, 2); /* inch */

	/* IFD entry - ColorMap */
	qrTiffWriteIfd(ptr, 0x0140, QRCNV_TIFF_T_SHORT, 3 * 2, 0); /* no colormap */

	/* IFD pointer */
	qrTiffWriteShort(ptr, 0);

	/* resolutions */
	qrTiffWriteRational(ptr, 150, 1); /* XResolution - 150 dpi */
	qrTiffWriteRational(ptr, 150, 1); /* YResolution - 150 dpi */

	/*
	 * skip offset table region
	 */
	if (totalstrips > 1) {
		ptr += 4 * totalstrips;
	}

	/*
	 * skip byte count table region
	 */
	if (totalstrips > 1) {
		ptr += 4 * totalstrips;
	}

	return ptr;
}

/* }}} */
/* {{{ qrTiffUpdateStripByteCount() */

/*
 * ストリップ長を更新する
 */
static void
qrTiffUpdateStripByteCount(qr_byte_t *bof, int size)
{
	qr_byte_t *ptr;

	ptr = bof + QRCNV_TIFF_IFD_STRIPBYTECOUNTS_OFFSET + 2 + 2 + 4;
	qrTiffWriteLong(ptr, size);
}

/* }}} */
/* {{{ qrTiffUpdateStripInfoTables() */

/*
 * オフセットテーブルとストリップ長テーブルを更新する
 */
static void
qrTiffUpdateStripInfoTables(qr_byte_t *bof,
		int totalstrips, int stripnumber, int offset, int size)
{
	qr_byte_t *optr, *sptr;

	optr = bof + QRCNV_TIFF_DATA_OFFSET + 4 * stripnumber;
	sptr = bof + QRCNV_TIFF_DATA_OFFSET + 4 * totalstrips + 4 * stripnumber;
	qrTiffWriteLong(optr, offset);
	qrTiffWriteLong(sptr, size);
}

/* }}} */
