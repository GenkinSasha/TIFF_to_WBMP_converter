#include "..\TIFF_codec\tiff_conf.h"
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef NEED_LIBPORT
# include "libport.h"
#endif

#include "..\TIFF_codec\tiffio.h"
#include "..\TIFF_codec\tiffiop.h"
#include "..\TIFF_codec\tif_fax3.h"
//#include "..\TIFF_codec\t4.h"

#define	strneq(a,b,n)	(strncmp(a,b,n) == 0)

void to_VLQ(uint16 x, uint8 *out)
{
	int i, j;
	for (i = 3; i > 0; i--) {
		if (x & 127ULL << i * 7) break;
	}
	for (j = 0; j <= i; j++)
		out[j] = ((x >> ((i - j) * 7)) & 127) | 128;
 
	out[i] ^= 128;
}

void SystemInit()
{
	return; 
}
/*
My TIFF file: (by IrfanViewer)
- little endian
- no SWAB
- TIFF_VERSION_CLASSIC
- no TIFF_BIGTIFF
*/
extern uint32 tiff_file_size;
int main()
{
	clock_t start, end;
	double cpu_time_used_ms;
	long cpu_time_used_clk, tiff_size;
	static int result;
	int TagSize;
	TIFF *tif;
	uint8 buffer[80], *tiff_file_buffer;
	uint32 *rgba_buffer, row;
	uint16 s, sample; 
	uint16 width, height;
	uint16 samplesperpixel;
	uint16 bitspersample;
	uint16 config;
	uint16 photometric;
	uint16* red, i;
	uint16* green;
	uint16* blue;
	//int row;
	FILE *wbmp_file, *tiff_file;
	tsize_t rowsize;
	tmsize_t ScanlineSize;
	tmsize_t occ;
	start=clock();
	// read .tiff file to memory buffer
	tiff_file = fopen("test_CCITT_Fax4.tif", "rb");
	if (tiff_file == NULL)
	{
		printf("TIFF file opening error!\n");
		return (-1);
	}
	fseek(tiff_file, 0, SEEK_END); // seek to end of file
	tiff_file_size = ftell(tiff_file); // get current file pointer
	fseek(tiff_file, 0, SEEK_SET); // seek back to beginning of file
	printf("TIFF file size: %u\n", tiff_file_size);
	tiff_file_buffer = (uint8 *)malloc(tiff_file_size);
	if(tiff_file_buffer==NULL)
	{
		printf("malloc() error!\n");
		return (-1);
	}	
	fread(tiff_file_buffer, tiff_file_size, 1, tiff_file);
	fclose(tiff_file);
/*	//printout TIFF file in HEX
	for(i=0;i<tiff_file_size;i++)
	{	
		printf("%2X ", *(tiff_file_buffer+i));
		if (i%16 == 0)
			puts("");
	}*/
	//check .tiff file to be suitable for printing:
	//read header and tags
	tif = TIFFOpen_ARM32(tiff_file_buffer);
	
	//TIFFGetField(tif, TIFFTAG_COMPRESSION, &photometric);
	TiffFindAndReadTag(tif, TIFFTAG_COMPRESSION, &TagSize, &tif->tif_dir.td_compression);
	if (tif->tif_dir.td_compression != COMPRESSION_CCITTFAX4 ) {
		fprintf(stderr,  "Bad compression algorytm; can only handle CCIT-Fax4 images.\n");
		return (-1);
	}
	
	//TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
	TiffFindAndReadTag(tif, TIFFTAG_PHOTOMETRIC, &TagSize, &tif->tif_dir.td_photometric);
	if (tif->tif_dir.td_photometric != PHOTOMETRIC_MINISWHITE && photometric != PHOTOMETRIC_MINISBLACK ) {
		fprintf(stderr,  "Bad photometric; can only handle bitonial(BW) images.\n");
		return (-1);
	}
	//TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
	TiffFindAndReadTag(tif, TIFFTAG_SAMPLESPERPIXEL, &TagSize, &tif->tif_dir.td_samplesperpixel);
	if (tif->tif_dir.td_samplesperpixel != 1) {
		fprintf(stderr, "Bad samples pixel %u.\n", samplesperpixel);
		return (-1);
	}
	//TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	TiffFindAndReadTag(tif, TIFFTAG_BITSPERSAMPLE, &TagSize, &tif->tif_dir.td_bitspersample);
	if (tif->tif_dir.td_bitspersample != 1) {
		fprintf(stderr,
		    " Sorry, only handle 1 bpp samples.\n");
		return (-1);
	}

/*	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height); */
	TiffFindAndReadTag(tif, TIFFTAG_IMAGEWIDTH, &TagSize, &width);
	TiffFindAndReadTag(tif, TIFFTAG_IMAGELENGTH, &TagSize, &height);
	ScanlineSize = TIFFScanlineSize(tif);
	printf("width=%d, heigth=%d,ScanlineSize=%d, photometric=%d\n",width, height, ScanlineSize, photometric);
	wbmp_file=fopen("test_CCITT_Fax4.wbmp","w");
	//write WBMP header
	buffer[0]=0;
	buffer[1]=0;
	to_VLQ(width, &buffer[2]);  
	to_VLQ(height, &buffer[4]);	
	fwrite (buffer, 1, 6, wbmp_file);
	for(row=0; row<height; row++ )
	{	
					
		result=TIFFReadScanline_ARM32(tif, buffer, row, 0);
		
		/*if(photometric == PHOTOMETRIC_MINISWHITE)		
		{*/
			for(i=0;i<width/8;i++)
				buffer[i]^=0xFF;
		/*}*/
		fwrite (buffer, 1, width/8, wbmp_file);
		fflush(wbmp_file);		
	}
	end=clock();
	cpu_time_used_clk=end - start;
	cpu_time_used_ms = ((double) ((cpu_time_used_clk))*1000) / CLOCKS_PER_SEC;
		
	printf("decoding %d lines: %ld clocks, %d mS\n", height, cpu_time_used_clk, cpu_time_used_ms);

	fclose(wbmp_file);

	TIFFClose_ARM32(tif);
	exit(1);
}
