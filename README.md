TIFF to WBMP converter v0.1

 https://github.com/GenkinSasha/TIFF_to_WBMP_converter :: CCIT-Fax4 (.tiff) to .wbmp converter
 Genkin Alexander          Genkin_S@hotmail.com

Overview

CCITT-Fax4 - very effective binary image compression algorythm (10-15:1 compression ratio), commonly used for saving bitonial scanned images as a part of TIFF format. Using this coding scheme in embedded systems that have
strict code size and RAM restrains was problematic. This program propose the way to do it under some restrictions.

This program converts TIFF file contains binary image coded by CCITT-Fax4 algorythm to plain WBMP format. Decoding is size optimized (31KB!!!) to suite embedded system design guidelines. 

The program use 'libtiff' library. I moved out all "esoteric" formats and options support, reduced CPU architectures support (target MCU must be little endian 16/32 bits) and significally reduced tags processing. Actually, only small set of baseline tags is allowed, sorry. tif_fax3sm.c in original library contains Haffman code tables, I replaced them with computation, code size reduction was vast.

Not all TIFF options are supported. I had have to remove some important format's feachures to achieve minimal code size. TIFF file to be decoded must suite several conditions:
1. Only basic tags are supported. (http://www.awaresystems.be/imaging/tiff/tifftags/baseline.html)
Other tags are ignored. Tags contains 'float' or 'rational' values are ignored to.
2. Image must be 1bpp (bitonial), CCITT-Fax4 compressed. 
Tags values:
	TIFFTAG_COMPRESSION = COMPRESSION_CCITTFAX4 
	TIFFTAG_PHOTOMETRIC = PHOTOMETRIC_MINISWHITE/PHOTOMETRIC_MINISBLACK
	TIFFTAG_SAMPLESPERPIXEL = 1
	TIFFTAG_BITSPERSAMPLE =1
3. Only 1 tiff directories level is allowed
4. TIFF_BIGTIFF (http://www.awaresystems.be/imaging/tiff/bigtiff.html) not supported

Warnings
This is a gamma version! Tested under Windows XP for several .tiff files only!

Notes
	
To-do
	Linux version
F.A.Q.

    Q: Why did you make this? A: I wanted to prove that it is possible.

References
1. 'libtiff' library (BSD-like licence): http://www.remotesensing.org/libtiff/
2. CCITT Group 4 compression algorythm: https://en.wikipedia.org/wiki/Group_4_compression
3. TIFF File Format: http://www.awaresystems.be/imaging/tiff.html

Author
     Alexander          Genkin_S@hotmail.com

