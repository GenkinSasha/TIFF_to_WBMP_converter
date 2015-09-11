/* $Id: tif_aux.c,v 1.26 2010-07-01 15:33:28 dron Exp $ */

/*
 * Copyright (c) 1991-1997 Sam Leffler
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
 * TIFF Library.
 *
 * Auxiliary Support Routines.
 */
#include "tiffiop.h"
#include "tif_predict.h"
#include <math.h>

void*
_TIFFCheckRealloc(TIFF* tif, void* buffer,
		  tmsize_t nmemb, tmsize_t elem_size, const char* what)
{
	void* cp = NULL;
	tmsize_t bytes = nmemb * elem_size;

	/*
	 * XXX: Check for integer overflow.
	 */
	if (nmemb && elem_size && bytes / elem_size == nmemb)
		cp = _TIFFrealloc(buffer, bytes);

	if (cp == NULL) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			     "Failed to allocate memory for %s "
			     "(%ld elements of %ld bytes each)",
			     what,(long) nmemb, (long) elem_size);
	}

	return cp;
}

void*
_TIFFCheckMalloc(TIFF* tif, tmsize_t nmemb, tmsize_t elem_size, const char* what)
{
	return _TIFFCheckRealloc(tif, NULL, nmemb, elem_size, what);  
}

struct _Int64Parts {
	int32 low, high;
};

typedef union {
	struct _Int64Parts part;
	int64 value;
} _Int64;

/* vim: set ts=8 sts=8 sw=8 noet: */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
