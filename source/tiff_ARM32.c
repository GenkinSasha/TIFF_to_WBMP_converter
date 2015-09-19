/* $Id: tif_dir.c,v 1.113 2012-06-14 20:32:53 fwarmerdam Exp $ */

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
 * TIFF Library.
 * Size optimized version of TIFF library functions for CCITT-FAX4 decodind
 */
#include "..\TIFF_codec\tiff_conf.h"
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef NEED_LIBPORT
# include "libport.h"
#endif

#include "..\TIFF_codec\tiffio.h"
#include "..\TIFF_codec\tiffiop.h"
#include "..\TIFF_codec\tif_fax3.h"
#include "..\TIFF_codec\t4.h"

#define NOSTRIP ((uint32)(-1))       /* undefined state */
#define NOTILE ((uint32)(-1))         /* undefined state */


static int TIFFCheckRead_ARM32(TIFF* tif, int tiles)
{
	if (tiles ^ isTiled(tif)) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, tiles ?
		    "Can not read tiles from a stripped image" :
		    "Can not read scanlines from a tiled image");
		return (0);
	}
	return (1);
}

/*
 * Set state to appear as if a
 * strip has just been read in.
 */
static int
TIFFStartStrip_ARM32(TIFF* tif, uint32 strip)
{
	TIFFDirectory *td = &tif->tif_dir;

    if (!_TIFFFillStriles( tif ) || !tif->tif_dir.td_stripbytecount)
        return 0;

	if ((tif->tif_flags & TIFF_CODERSETUP) == 0) {
		if (!(*tif->tif_setupdecode)(tif))
			return (0);
		tif->tif_flags |= TIFF_CODERSETUP;
	}
	tif->tif_curstrip = strip;
	tif->tif_row = (strip % td->td_stripsperimage) * td->td_rowsperstrip;
        tif->tif_flags &= ~TIFF_BUF4WRITE;

	if (tif->tif_flags&TIFF_NOREADRAW)
	{
		tif->tif_rawcp = NULL;
		tif->tif_rawcc = 0;  
	}
	else
	{
		tif->tif_rawcp = tif->tif_rawdata;
		tif->tif_rawcc = (tmsize_t)td->td_stripbytecount[strip];
	}
	return ((*tif->tif_predecode)(tif,
			(uint16)(strip / td->td_stripsperimage)));
}


static int
TIFFFillStripPartial_ARM32( TIFF *tif, int strip, tmsize_t read_ahead, int restart )
{
	static const char module[] = "TIFFFillStripPartial";
	register TIFFDirectory *td = &tif->tif_dir;
        uint64 unused_data;
        uint64 read_offset;
        tmsize_t cc, to_read;
        /* tmsize_t bytecountm; */
        
        if (!_TIFFFillStriles( tif ) || !tif->tif_dir.td_stripbytecount)
            return 0;
        
        /*
         * Expand raw data buffer, if needed, to hold data
         * strip coming from file (perhaps should set upper
         * bound on the size of a buffer we'll use?).
         */

        /* bytecountm=(tmsize_t) td->td_stripbytecount[strip]; */
        if (read_ahead*2 > tif->tif_rawdatasize) {
                assert( restart );
                
                tif->tif_curstrip = NOSTRIP;
                if ((tif->tif_flags & TIFF_MYBUFFER) == 0) {
                        TIFFErrorExt(tif->tif_clientdata, module,
                                     "Data buffer too small to hold part of strip %lu",
                                     (unsigned long) strip);
                        return (0);
                }
                if (!TIFFReadBufferSetup(tif, 0, read_ahead*2))
                        return (0);
        }

        if( restart )
        {
                tif->tif_rawdataloaded = 0;
                tif->tif_rawdataoff = 0;
        }

        /*
        ** If we are reading more data, move any unused data to the
        ** start of the buffer.
        */
        if( tif->tif_rawdataloaded > 0 )
                unused_data = tif->tif_rawdataloaded - (tif->tif_rawcp - tif->tif_rawdata);
        else
                unused_data = 0;
        
        if( unused_data > 0 )
        {
		assert((tif->tif_flags&TIFF_BUFFERMMAP)==0);
                memmove( tif->tif_rawdata, tif->tif_rawcp, unused_data );
        }

        /*
        ** Seek to the point in the file where more data should be read.
        */
        read_offset = td->td_stripoffset[strip]
                + tif->tif_rawdataoff + tif->tif_rawdataloaded;

        if (!SeekOK(tif, read_offset)) {
                TIFFErrorExt(tif->tif_clientdata, module,
                             "Seek error at scanline %lu, strip %lu",
                             (unsigned long) tif->tif_row, (unsigned long) strip);
                return 0;
        }

        /*
        ** How much do we want to read?
        */
        to_read = tif->tif_rawdatasize - unused_data;
        if( (uint64) to_read > td->td_stripbytecount[strip] 
            - tif->tif_rawdataoff - tif->tif_rawdataloaded )
        {
                to_read = td->td_stripbytecount[strip]
                        - tif->tif_rawdataoff - tif->tif_rawdataloaded;
        }

	assert((tif->tif_flags&TIFF_BUFFERMMAP)==0);
        cc = TIFFReadFile(tif, tif->tif_rawdata + unused_data, to_read);

        if (cc != to_read) {
#if defined(__WIN32__) && (defined(_MSC_VER) || defined(__MINGW32__))
                TIFFErrorExt(tif->tif_clientdata, module,
                             "Read error at scanline %lu; got %I64u bytes, expected %I64u",
                             (unsigned long) tif->tif_row,
                             (unsigned __int64) cc,
                             (unsigned __int64) to_read);
#else
                TIFFErrorExt(tif->tif_clientdata, module,
                             "Read error at scanline %lu; got %llu bytes, expected %llu",
                             (unsigned long) tif->tif_row,
                             (unsigned long long) cc,
                             (unsigned long long) to_read);
#endif
                return 0;
        }
        
        tif->tif_rawdataoff = tif->tif_rawdataoff + tif->tif_rawdataloaded - unused_data ;
        tif->tif_rawdataloaded = unused_data + to_read;

        tif->tif_rawcp = tif->tif_rawdata;
                        
        if (!isFillOrder(tif, td->td_fillorder) &&
            (tif->tif_flags & TIFF_NOBITREV) == 0) {
		assert((tif->tif_flags&TIFF_BUFFERMMAP)==0);
                TIFFReverseBits(tif->tif_rawdata + unused_data, to_read );
	}

        /*
        ** When starting a strip from the beginning we need to
        ** restart the decoder.
        */
        if( restart )
                return TIFFStartStrip_ARM32(tif, strip);
        else
                return 1;
}

/*
 * Seek to a random row+sample in a file.
 *
 * Only used by TIFFReadScanline, and is only used on
 * strip organized files.  We do some tricky stuff to try
 * and avoid reading the whole compressed raw data for big
 * strips.
 */
static int TIFFSeek_ARM32(TIFF* tif, uint32 row/*, uint16 sample*/ )
{
	register TIFFDirectory *td = &tif->tif_dir;
	uint32 strip;
        int    whole_strip;
	tmsize_t read_ahead = 0;

        /*
        ** Establish what strip we are working from.
        */
	if (row >= td->td_imagelength) {	/* out of range */
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		    "%lu: Row out of range, max %lu",
		    (unsigned long) row,
		    (unsigned long) td->td_imagelength);
		return (0);
	}

		strip = row / td->td_rowsperstrip;

        whole_strip = 1;

        /*
         * If we haven't loaded this strip, do so now, possibly
         * only reading the first part.
         */
	if (strip != tif->tif_curstrip) {	/* different strip, refill */
                
        	if (!TIFFFillStrip(tif, strip))
            	return (0);
	}

        /*
        ** If we already have some data loaded, do we need to read some more?
        */
        else if( !whole_strip )
        {
                if( ((tif->tif_rawdata + tif->tif_rawdataloaded) - tif->tif_rawcp) < read_ahead 
                    && (uint64) tif->tif_rawdataoff+tif->tif_rawdataloaded < td->td_stripbytecount[strip] )
                {
                        if( !TIFFFillStripPartial_ARM32(tif,strip,read_ahead,0) )
                                return 0;
                }
        }

        if (row < tif->tif_row) {
		/*
		 * Moving backwards within the same strip: backup
		 * to the start and then decode forward (below).
		 *
		 * NB: If you're planning on lots of random access within a
		 * strip, it's better to just read and decode the entire
		 * strip, and then access the decoded data in a random fashion.
		 */

                if( tif->tif_rawdataoff != 0 )
                {
                        if( !TIFFFillStripPartial_ARM32(tif,strip,read_ahead,1) )
                                return 0;
                }
                else
                {
                        if (!TIFFStartStrip_ARM32(tif, strip))
                                return (0);
                }
	}
        
	if (row != tif->tif_row) {
		/*
		 * Seek forward to the desired row.
		 */

                /* TODO: Will this really work with partial buffers? */
                
		if (!(*tif->tif_seek)(tif, row - tif->tif_row))
			return (0);
		tif->tif_row = row;
	}

	return (1);
}

int TIFFDefaultDirectory_ARM32(TIFF* tif)
{
	register TIFFDirectory* td = &tif->tif_dir;

	_TIFFmemset(td, 0, sizeof (*td));
	td->td_fillorder = FILLORDER_MSB2LSB;
	td->td_bitspersample = 1;
	td->td_threshholding = THRESHHOLD_BILEVEL;
	td->td_orientation = ORIENTATION_TOPLEFT;
	td->td_samplesperpixel = 1;
	td->td_rowsperstrip = (uint32) -1;
	td->td_stripbytecountsorted = 1; /* Our own arrays always sorted. */  
	td->td_resolutionunit = RESUNIT_INCH;
	td->td_sampleformat = SAMPLEFORMAT_UINT;
	td->td_imagedepth = 1;
	tif->tif_postdecode = _TIFFNoPostDecode;  
	tif->tif_foundfield = NULL;
	td->td_compression = COMPRESSION_CCITTFAX4; 
	/*
	 * NB: The directory is marked dirty as a result of setting
	 * up the default compression scheme.  However, this really
	 * isn't correct -- we want TIFF_DIRTYDIRECT to be set only
	 * if the user does something.  We could just do the setup
	 * by hand, but it seems better to use the normal mechanism
	 * (i.e. TIFFSetField).
	 */

	/*
	 * As per http://bugzilla.remotesensing.org/show_bug.cgi?id=19
	 * we clear the ISTILED flag when setting up a new directory.
	 * Should we also be clearing stuff like INSUBIFD?
	 */
	tif->tif_flags &= ~TIFF_ISTILED;

	return (1);
}

/**********************************************************************************/
TIFF tiff_struct, *tif=&tiff_struct; //global variables
static uint32 read_ptr=0, write_ptr=0;
uint32 tiff_file_size =0;

static tmsize_t ReadProc_ARM32(void* mapped_tiff, void* buf, tmsize_t size)
{	tmsize_t p;
	if ((read_ptr+size) <= tiff_file_size)
	{	
		p = size;
		memcpy(buf, mapped_tiff+read_ptr, p);
		read_ptr+=size;
	}
	else
	{
		p = tiff_file_size - read_ptr;
		memcpy(buf, mapped_tiff+read_ptr, p);
		read_ptr=tiff_file_size;
	}
	return(p);
}

static uint32 SeekProc_ARM32(void* mapped_tiff, uint32 off, int whence)
{
	switch(whence)
	{
		case SEEK_SET:
			read_ptr=write_ptr=off;
			break;
		case SEEK_CUR:
			read_ptr+=off;
			write_ptr+=off;
			break;
		case SEEK_END:
			read_ptr=tiff_file_size-off;
			write_ptr=tiff_file_size-off;			
			break;
		default:
			break;
	}	
	return(read_ptr);
}

static int CloseProc_ARM32()
{
	return (0);
}

static tmsize_t SizeProc_ARM32()
{
	return(tiff_file_size);
}

/**********************************************************************************/

/**********************************************************************************/
typedef struct {
	uint16 tdir_tag;        /* see below */
	uint16 tdir_type;       /* data type; see below */
	uint32 tdir_count;      /* number of items; length in spec */
	union {
		uint16 toff_short;
		uint32 toff_long;
	} tdir_offset;		/* either offset or the data itself if fits */
} TIFFDirEntryClassic;

typedef struct {
	uint16 tdir_tag;        /* see below */
	uint16 tdir_type;       /* data type; see below */
	uint64 tdir_count;      /* number of items; length in spec */
	union {
		uint16 toff_short;
		uint32 toff_long;
		uint64 toff_long8;
	} tdir_offset;		/* either offset or the data itself if fits */
} TIFFDirEntryBig;

const uint8 TIFFLabelSize[] =  
{
	0, /* TIFF_NOTYPE = 0,       placeholder */
	1, /* TIFF_BYTE = 1,         8-bit unsigned integer */
	0, /* TIFF_ASCII = 2,        8-bit bytes w/ last byte null */
	2, /* TIFF_SHORT = 3,        16-bit unsigned integer */
	4, /* TIFF_LONG = 4,         32-bit unsigned integer */
	8, /* TIFF_RATIONAL = 5,     64-bit unsigned fraction */
	1, /* TIFF_SBYTE = 6,        !8-bit signed integer */
	1, /* TIFF_UNDEFINED = 7,    !8-bit untyped data */
	2, /* TIFF_SSHORT = 8,       !16-bit signed integer */
	4, /* TIFF_SLONG = 9,        !32-bit signed integer */
	8, /* TIFF_SRATIONAL = 10,   !64-bit signed fraction */
	4, /* TIFF_FLOAT = 11,       !32-bit IEEE floating point */
	8, /* TIFF_DOUBLE = 12,      !64-bit IEEE floating point */
	4, /* TIFF_IFD = 13,         %32-bit unsigned integer (offset) */
	8, /* TIFF_LONG8 = 16,       BigTIFF 64-bit unsigned integer */
	8, /* TIFF_SLONG8 = 17,      BigTIFF 64-bit signed integer */
	8 /* TIFF_IFD8 = 18         BigTIFF 64-bit unsigned integer (offset) */
};

int TiffFindAndReadTag(TIFF* tif, int TagNumber, TIFFDirEntry* pDirEntry, uint8* TagValue)
{
	uint16* IFD_ptr; int TagSizeLocal;	
	TIFFDirEntryClassic dir_entry;
	TIFFDirEntryClassic* dir_entry_ptr;
	TIFFDirEntryClassic* next_IFD_ptr;
	int dir_number, IFD_NumberOfTags;
	
	if ((TagNumber < 254) || (TagNumber > 338)) //must be check of tags list
	{
		TIFFErrorExt(tif->tif_clientdata, "TiffReadTag", "Extension tag %d is not supported", TagNumber);	
	}
	if(tif->tif_diroff == NULL)
		return (-1);
	IFD_ptr  = (uint16*)((uint8*)tif->tif_clientdata + tif->tif_diroff);
	IFD_NumberOfTags = *IFD_ptr;
	if(IFD_NumberOfTags == 0)
		return (-1);	
	printf("Found IFD with %u tags\n", IFD_NumberOfTags);
	IFD_ptr++; 
	//read directory entry
	do{
		for(dir_number =0; dir_number < IFD_NumberOfTags; dir_number++)  //loop for all dir. entries
		{
		//read 	
			if(memcpy(&dir_entry, IFD_ptr, sizeof (TIFFDirEntryClassic)))
			{
				if(dir_entry.tdir_tag == TagNumber)
				{
					if(dir_entry.tdir_type == TIFF_ASCII)
						TagSizeLocal = dir_entry.tdir_count;
					else
						TagSizeLocal = TIFFLabelSize[dir_entry.tdir_type]; 
					if(TagValue == NULL)
						return (0); //tag exist, but no place to copy to
					if(dir_entry.tdir_count * dir_entry.tdir_type > 4)  //offset
					{ 
						memcpy(TagValue, tif->tif_clientdata + dir_entry.tdir_offset.toff_long, TagSizeLocal);
					}
					else  //value
					{
						memcpy(TagValue, &dir_entry.tdir_offset.toff_long, TagSizeLocal);						
					}
					if(pDirEntry != NULL)
					{
						//*TagSize = TagSizeLocal;
						pDirEntry->tdir_count = TagSizeLocal;
						pDirEntry->tdir_offset.toff_long = dir_entry.tdir_offset.toff_long;
						pDirEntry->tdir_tag = TagNumber;
						pDirEntry->tdir_type = dir_entry.tdir_type;
					}
					return (0);	
				}
			}				
			IFD_ptr += sizeof(TIFFDirEntryClassic)/sizeof(uint16);
		}
		next_IFD_ptr = IFD_ptr;  //last IFD?
	} while(*((unsigned long *)next_IFD_ptr));
	return (-1);
}

int TIFFReadDirectory_my(TIFF* tif)
{ int result, temp_tag_val;
	TIFFDefaultDirectory_ARM32(tif);
	result = TiffFindAndReadTag(tif, TIFFTAG_IMAGELENGTH, NULL, &tif->tif_dir.td_imagelength);
	tif->tif_dir.td_rowsperstrip = 0;
	result += TiffFindAndReadTag(tif, TIFFTAG_ROWSPERSTRIP, NULL, &tif->tif_dir.td_rowsperstrip);
	result += TiffFindAndReadTag(tif, TIFFTAG_IMAGEWIDTH, NULL, &tif->tif_dir.td_imagewidth);
	result += TiffFindAndReadTag(tif, TIFFTAG_COMPRESSION, NULL, &tif->tif_dir.td_compression);
	result += TiffFindAndReadTag(tif, TIFFTAG_PHOTOMETRIC, NULL, &tif->tif_dir.td_photometric);
	result += TiffFindAndReadTag(tif, TIFFTAG_SAMPLESPERPIXEL, NULL, &tif->tif_dir.td_samplesperpixel);
	result += TiffFindAndReadTag(tif, TIFFTAG_BITSPERSAMPLE, NULL, &tif->tif_dir.td_bitspersample);
	result += TiffFindAndReadTag(tif, TIFFTAG_FILLORDER, NULL, &tif->tif_dir.td_fillorder);
	result += TiffFindAndReadTag(tif, TIFFTAG_ORIENTATION, NULL, &tif->tif_dir.td_orientation);
	result += TiffFindAndReadTag(tif, TIFFTAG_RESOLUTIONUNIT, NULL, &tif->tif_dir.td_resolutionunit);
	result += TiffFindAndReadTag(tif, TIFFTAG_PLANARCONFIG, NULL, &tif->tif_dir.td_planarconfig);//return -1
	result += TiffFindAndReadTag(tif, TIFFTAG_STRIPOFFSETS, NULL, &temp_tag_val);	
	tif->tif_dir.td_nstrips = TIFFNumberOfStrips(tif);
	tif->tif_dir.td_stripsperimage = tif->tif_dir.td_nstrips;	
	if (tif->tif_dir.td_planarconfig == PLANARCONFIG_SEPARATE)
		tif->tif_dir.td_stripsperimage /= tif->tif_dir.td_samplesperpixel;
		
	tif->tif_dir.td_stripoffset = (uint64 *)_TIFFmalloc(tif->tif_dir.td_nstrips * sizeof(uint64));
	tif->tif_dir.td_stripoffset[0] = temp_tag_val;
	
	tif->tif_dir.td_stripbytecount = (uint64*)_TIFFmalloc(tif->tif_dir.td_nstrips * sizeof (uint64));	
	result += TiffFindAndReadTag(tif, TIFFTAG_STRIPBYTECOUNTS, NULL, &temp_tag_val);	
	tif->tif_dir.td_stripbytecount[0] = temp_tag_val;
	(*tif->tif_setupdecode)(tif); //Fax3SetupState
//	Fax3SetupState(tif); //my patch
	tif->tif_scanlinesize = TIFFScanlineSize(tif);
	if (!tif->tif_scanlinesize) {
		TIFFErrorExt(tif->tif_clientdata, "TIFFReadDirectory_my",  "Cannot handle zero scanline size");
		return (0);
	}
	return result;
}

TIFF* TIFFClientOpen_ARM32_my(uint8* clientdata)
{
	int m;
	const char* cp;
	const char name[]={"TIFF file in memory"};

	memset(tif, 0, sizeof (*tif));
	
	tif->tif_name = (char *)tif + sizeof (TIFF);
	tif->tif_curdir = (uint16) -1;		/* non-existent directory */
	tif->tif_curoff = 0;
	tif->tif_curstrip = (uint32) -1;	/* invalid strip */
	tif->tif_row = (uint32) -1;		/* read/write pre-increment */
	tif->tif_clientdata = clientdata;

	_TIFFSetDefaultCompressionState(tif);    /* setup default state */
	tif->tif_setupdecode = TIFFInitCCITTFax4;// Sasha
	/*
	 * Default is to return data MSB2LSB and enable the
	 * use of memory-mapped files and strip chopping when
	 * a file is opened read-only.
	 */
	tif->tif_flags = FILLORDER_MSB2LSB;
		tif->tif_flags |= TIFF_MAPPED;
		tif->tif_base = clientdata;//my patch

	/*
	 * Read in TIFF header.
	 */
	if(!ReadProc_ARM32(tif->tif_clientdata, &tif->tif_header, sizeof (TIFFHeaderClassic)))
	{
		/*
		 * Setup header and write.
		 */
		#ifdef WORDS_BIGENDIAN
		tif->tif_header.common.tiff_magic = tif->tif_flags & TIFF_SWAB
		    ? TIFF_LITTLEENDIAN : TIFF_BIGENDIAN;
		#else
		tif->tif_header.common.tiff_magic = tif->tif_flags & TIFF_SWAB
		    ? TIFF_BIGENDIAN : TIFF_LITTLEENDIAN;
		#endif
		if (!(tif->tif_flags&TIFF_BIGTIFF))
		{
			tif->tif_header.common.tiff_version = TIFF_VERSION_CLASSIC;
			tif->tif_header.classic.tiff_diroff = 0;
			tif->tif_header_size = sizeof(TIFFHeaderClassic);
		}
		else
		{
			tif->tif_header.common.tiff_version = TIFF_VERSION_BIG;
			tif->tif_header.big.tiff_offsetsize = 8;
			tif->tif_header.big.tiff_unused = 0;
			tif->tif_header.big.tiff_diroff = 0;
			tif->tif_header_size = sizeof (TIFFHeaderBig);
		}
		/*
		 * The doc for "fopen" for some STD_C_LIBs says that if you
		 * open a file for modify ("+"), then you must fseek (or
		 * fflush?) between any freads and fwrites.  This is not
		 * necessary on most systems, but has been shown to be needed
		 * on Solaris.
		 */
		/*
		 * Setup the byte order handling.
		 */
		if (tif->tif_header.common.tiff_magic == TIFF_BIGENDIAN) {
			#ifndef WORDS_BIGENDIAN
			tif->tif_flags |= TIFF_SWAB;
			#endif
		} else {
			#ifdef WORDS_BIGENDIAN
			tif->tif_flags |= TIFF_SWAB;
			#endif
		}
		/*
		 * Setup default directory.
		 */
		if (!TIFFDefaultDirectory(tif))
			goto bad;
		tif->tif_diroff = 0;
		tif->tif_dirlist = NULL;
		tif->tif_dirlistsize = 0;
		tif->tif_dirnumber = 0;
		return (tif);
	}
	/*
	 * Setup the byte order handling.
	 */
	if (tif->tif_header.common.tiff_magic != TIFF_BIGENDIAN &&
	    tif->tif_header.common.tiff_magic != TIFF_LITTLEENDIAN
	    ) {
		TIFFErrorExt(tif->tif_clientdata, name,
		    "Not a TIFF file, bad magic number %d (0x%x)",
	    //#endif
		    tif->tif_header.common.tiff_magic,
		    tif->tif_header.common.tiff_magic);
		goto bad;
	}
	if ((tif->tif_header.common.tiff_version != TIFF_VERSION_CLASSIC)&&
	    (tif->tif_header.common.tiff_version != TIFF_VERSION_BIG)) {
		TIFFErrorExt(tif->tif_clientdata, name,
		    "Not a TIFF file, bad version number %d (0x%x)",
		    tif->tif_header.common.tiff_version,
		    tif->tif_header.common.tiff_version);
		goto bad;
	}
	if (tif->tif_header.common.tiff_version == TIFF_VERSION_CLASSIC)
	{
		tif->tif_header_size = sizeof(TIFFHeaderClassic);
		tif->tif_diroff = tif->tif_header.classic.tiff_diroff ;  //my patch
	}
	else
	{ //"big tiff"
		if (!ReadProc_ARM32(tif, ((uint8*)(&tif->tif_header) + sizeof(TIFFHeaderClassic)), (sizeof(TIFFHeaderBig)-sizeof(TIFFHeaderClassic))))
		{
			TIFFErrorExt(tif->tif_clientdata, name,
			    "Cannot read TIFF header");
			goto bad;
		}
		if (tif->tif_header.big.tiff_offsetsize != 8)
		{
			TIFFErrorExt(tif->tif_clientdata, name,
			    "Not a TIFF file, bad BigTIFF offsetsize %d (0x%x)",
			    tif->tif_header.big.tiff_offsetsize,
			    tif->tif_header.big.tiff_offsetsize);
			goto bad;
		}
		if (tif->tif_header.big.tiff_unused != 0)
		{
			TIFFErrorExt(tif->tif_clientdata, name,
			    "Not a TIFF file, bad BigTIFF unused %d (0x%x)",
			    tif->tif_header.big.tiff_unused,
			    tif->tif_header.big.tiff_unused);
			goto bad;
		}
		tif->tif_header_size = sizeof(TIFFHeaderBig);
		tif->tif_diroff = tif->tif_header.big.tiff_diroff ;  //my patch
		tif->tif_flags |= TIFF_BIGTIFF;
	}
	tif->tif_flags |= TIFF_MYBUFFER;
	tif->tif_rawcp = tif->tif_rawdata = 0;
	tif->tif_rawdatasize = 0;
        tif->tif_rawdataoff = 0;
        tif->tif_rawdataloaded = 0;

			if (!(tif->tif_flags&TIFF_BIGTIFF))
				tif->tif_nextdiroff = tif->tif_header.classic.tiff_diroff;
			else
				tif->tif_nextdiroff = tif->tif_header.big.tiff_diroff;
			//
			 // Try to use a memory-mapped file if the client
			 // has not explicitly suppressed usage with the
			 // 'm' flag in the open mode (see above).
			 //
			if (tif->tif_flags & TIFF_MAPPED)
			{
				toff_t n;
				//if (TIFFMapFileContents(tif,(void**)(&tif->tif_base),&n))
				if(1)
				{	n=tiff_file_size;
					tif->tif_size=(tmsize_t)n;
					assert((toff_t)tif->tif_size==n);
				}
				else
					tif->tif_flags &= ~TIFF_MAPPED;
			}
			//
			 // Sometimes we do not want to read the first directory (for example,
			 // it may be broken) and want to proceed to other directories. I this
			 // case we use the TIFF_HEADERONLY flag to open file and return
			 // immediately after reading TIFF header.
			 //
			if (tif->tif_flags & TIFF_HEADERONLY)
				return (tif);

			//
			 // Setup initial directory.
			 //
//			if (TIFFReadDirectory(tif)) {
			if (TIFFReadDirectory_my(tif) == 0) {
				tif->tif_rawcc = (tmsize_t)-1;
				tif->tif_flags |= TIFF_BUFFERSETUP;
				
				return (tif);
			}		
bad:
	tif->tif_mode = O_RDONLY;	/* XXX avoid flush */
        TIFFCleanup(tif);
bad2:
	return ((TIFF*)0);
}


/**********************************************************************************/
int TIFFReadScanline_ARM32(TIFF* tif, void* buf, uint32 row, uint16 sample)
{
	int e;
	
	if (!TIFFCheckRead_ARM32(tif, 0))
		return (-1);
	if( (e = TIFFSeek_ARM32(tif, row)) != 0) {
		/*
		 * Decompress desired row into user buffer.
		 */
		e = Fax4Decode(tif, (uint8*) buf, tif->tif_scanlinesize, sample);
		/* we are now poised at the beginning of the next row */
		tif->tif_row = row + 1;		
	}
	return (e > 0 ? 1 : -1);
}

TIFF* TIFFOpen_ARM32(uint8* tiff_file_buffer)
{

	tif = TIFFClientOpen_ARM32_my(tiff_file_buffer); 
	return(tif);
}

/************************************************************************/
/*                            TIFFCleanup()                             */
/************************************************************************/

/**
 * Auxiliary function to free the TIFF structure. Given structure will be
 * completetly freed, so you should save opened file handle and pointer
 * to the close procedure in external variables before calling
 * _TIFFCleanup(), if you will need these ones to close the file.
 * 
 * @param tif A TIFF pointer.
 */

void
TIFFCleanup_ARM32(TIFF* tif)
{

	(*tif->tif_cleanup)(tif);
	TIFFFreeDirectory(tif);

	if (tif->tif_dirlist)
		_TIFFfree(tif->tif_dirlist);

	/*
         * Clean up client info links.
         */
	while( tif->tif_clientinfo )
	{
		TIFFClientInfoLink *link = tif->tif_clientinfo;

		tif->tif_clientinfo = link->next;
		_TIFFfree( link->name );
		_TIFFfree( link );
	}

	if (tif->tif_rawdata && (tif->tif_flags&TIFF_MYBUFFER))
		_TIFFfree(tif->tif_rawdata);

	/*
         * Clean up custom fields.
         */
	if (tif->tif_fields && tif->tif_nfields > 0) {
		uint32 i;

		for (i = 0; i < tif->tif_nfields; i++) {
			TIFFField *fld = tif->tif_fields[i];
			if (fld->field_bit == FIELD_CUSTOM &&
			    strncmp("Tag ", fld->field_name, 4) == 0) {
				_TIFFfree(fld->field_name);
				_TIFFfree(fld);
			}
		}

		_TIFFfree(tif->tif_fields);
	}

        if (tif->tif_nfieldscompat > 0) {
                uint32 i;

                for (i = 0; i < tif->tif_nfieldscompat; i++) {
                        if (tif->tif_fieldscompat[i].allocated_size)
                                _TIFFfree(tif->tif_fieldscompat[i].fields);
                }
                _TIFFfree(tif->tif_fieldscompat);
        }

}

/************************************************************************/
/*                            TIFFClose()                               */
/************************************************************************/

/**
 * Close a previously opened TIFF file.
 *
 * TIFFClose closes a file that was previously opened with TIFFOpen().
 * Any buffered data are flushed to the file, including the contents of
 * the current directory (if modified); and all resources are reclaimed.
 * 
 * @param tif A TIFF pointer.
 */

void
TIFFClose_ARM32(TIFF* tif)
{
	TIFFCloseProc closeproc = tif->tif_closeproc;
	thandle_t fd = tif->tif_clientdata;
}


