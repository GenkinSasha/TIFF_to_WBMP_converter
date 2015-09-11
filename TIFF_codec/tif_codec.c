/* $Id: tif_codec.c,v 1.15 2010-12-14 12:53:00 dron Exp $ */

/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library
 *
 * Builtin Compression Scheme Configuration Support.
 */
#include "tiffiop.h"

static int NotConfigured(TIFF*, int);

#if LZW_SUPPORT==0
#define TIFFInitLZW NotConfigured
#endif
#if PACKBITS_SUPPORT==0
#define TIFFInitPackBits NotConfigured
#endif
#if THUNDER_SUPPORT ==0
#define TIFFInitThunderScan NotConfigured
#endif
#if NEXT_SUPPORT==0
#define TIFFInitNeXT NotConfigured
#endif
#if JPEG_SUPPORT==0
#define TIFFInitJPEG NotConfigured
#endif
#if OJPEG_SUPPORT==0
#define TIFFInitOJPEG NotConfigured
#endif
#if CCITT_SUPPORT==0
#define TIFFInitCCITTRLE NotConfigured
#define TIFFInitCCITTRLEW NotConfigured
#define TIFFInitCCITTFax3 NotConfigured
#define TIFFInitCCITTFax4 NotConfigured
#endif
#if JBIG_SUPPORT==0
#define TIFFInitJBIG NotConfigured
#endif
#if ZIP_SUPPORT==0
#define TIFFInitZIP NotConfigured
#endif
#if PIXARLOG_SUPPORT==0
#define TIFFInitPixarLog NotConfigured
#endif
#if LOGLUV_SUPPORT==0
#define TIFFInitSGILog NotConfigured
#endif
#if LZMA_SUPPORT==0
#define TIFFInitLZMA NotConfigured
#endif

/*
 * Compression schemes statically built into the library.
 */
#ifdef VMS
const TIFFCodec _TIFFBuiltinCODECS[] = {
#else
TIFFCodec _TIFFBuiltinCODECS[] = {
#endif
//    { "None",		COMPRESSION_NONE,	TIFFInitDumpMode },
//    { "LZW",		COMPRESSION_LZW,	NULL/*TIFFInitLZW*/ },
//    { "PackBits",	COMPRESSION_PACKBITS,	NULL/*TIFFInitPackBits*/ },
//    { "ThunderScan",	COMPRESSION_THUNDERSCAN,NULL/*TIFFInitThunderScan*/ },
//    { "NeXT",		COMPRESSION_NEXT,	NULL/*TIFFInitNeXT*/ },
//    { "JPEG",		COMPRESSION_JPEG,	NULL/*TIFFInitJPEG*/ },
//    { "Old-style JPEG",	COMPRESSION_OJPEG,	NULL/*TIFFInitOJPEG*/ },
//    { "CCITT RLE",	COMPRESSION_CCITTRLE,	TIFFInitCCITTRLE },
//    { "CCITT RLE/W",	COMPRESSION_CCITTRLEW,	NULL/*TIFFInitCCITTRLEW*/ },
//    { "CCITT Group 3",	COMPRESSION_CCITTFAX3,	TIFFInitCCITTFax3 },
    { "CCITT Group 4",	COMPRESSION_CCITTFAX4,	TIFFInitCCITTFax4 },
//    { "ISO JBIG",	COMPRESSION_JBIG,	NULL/*TIFFInitJBIG*/ },
//    { "Deflate",	COMPRESSION_DEFLATE, NULL/*TIFFInitZIP*/ },
//    { "AdobeDeflate",   COMPRESSION_ADOBE_DEFLATE , NULL/*TIFFInitZIP*/ }, 
//    { "PixarLog",	COMPRESSION_PIXARLOG,	NULL/*TIFFInitPixarLog*/ },
//    { "SGILog",		COMPRESSION_SGILOG,	NULL/*TIFFInitSGILog*/ },
//    { "SGILog24",	COMPRESSION_SGILOG24,	NULL/*TIFFInitSGILog*/ },
//    { "LZMA",		COMPRESSION_LZMA,	NULL/*TIFFInitLZMA*/},
    { NULL,             0,                      NULL }
};

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
