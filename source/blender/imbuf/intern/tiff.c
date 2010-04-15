/*
 * tiff.c
 *
 * $Id$
 * 
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor(s): Jonathan Merritt.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/**
 * Provides TIFF file loading and saving for Blender, via libtiff.
 *
 * The task of loading is complicated somewhat by the fact that Blender has
 * already loaded the file into a memory buffer.  libtiff is not well
 * configured to handle files in memory, so a client wrapper is written to
 * surround the memory and turn it into a virtual file.  Currently, reading
 * of TIFF files is done using libtiff's RGBAImage support.  This is a 
 * high-level routine that loads all images as 32-bit RGBA, handling all the
 * required conversions between many different TIFF types internally.
 * 
 * Saving supports RGB, RGBA and BW (greyscale) images correctly, with
 * 8 bits per channel in all cases.  The "deflate" compression algorithm is
 * used to compress images.
 */

#include <string.h>

#include "imbuf.h"

#include "BKE_global.h"

#include "IMB_imbuf_types.h"
#include "IMB_imbuf.h"

#include "IMB_allocimbuf.h"
#include "IMB_filetype.h"

#include "dynlibtiff.h"



/***********************
 * Local declarations. *
 ***********************/
/* Reading and writing of an in-memory TIFF file. */
static tsize_t imb_tiff_ReadProc(thandle_t handle, tdata_t data, tsize_t n);
static tsize_t imb_tiff_WriteProc(thandle_t handle, tdata_t data, tsize_t n);
static toff_t  imb_tiff_SeekProc(thandle_t handle, toff_t ofs, int whence);
static int     imb_tiff_CloseProc(thandle_t handle);
static toff_t  imb_tiff_SizeProc(thandle_t handle);
static int     imb_tiff_DummyMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize);
static void    imb_tiff_DummyUnmapProc(thandle_t fd, tdata_t base, toff_t size);


/* Structure for in-memory TIFF file. */
struct ImbTIFFMemFile {
	unsigned char *mem;	/* Location of first byte of TIFF file. */
	toff_t offset;		/* Current offset within the file.      */
	tsize_t size;		/* Size of the TIFF file.               */
};
#define IMB_TIFF_GET_MEMFILE(x) ((struct ImbTIFFMemFile*)(x));



/*****************************
 * Function implementations. *
 *****************************/


static void imb_tiff_DummyUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
}

static int imb_tiff_DummyMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize) 
{
			return (0);
}

/**
 * Reads data from an in-memory TIFF file.
 *
 * @param handle: Handle of the TIFF file (pointer to ImbTIFFMemFile).
 * @param data:   Buffer to contain data (treat as void*).
 * @param n:      Number of bytes to read.
 *
 * @return: Number of bytes actually read.
 * 	 0 = EOF.
 */
static tsize_t imb_tiff_ReadProc(thandle_t handle, tdata_t data, tsize_t n)
{
	tsize_t nRemaining, nCopy;
	struct ImbTIFFMemFile* mfile;
	void *srcAddr;

	/* get the pointer to the in-memory file */
	mfile = IMB_TIFF_GET_MEMFILE(handle);
	if(!mfile || !mfile->mem) {
		fprintf(stderr, "imb_tiff_ReadProc: !mfile || !mfile->mem!\n");
		return 0;
	}

	/* find the actual number of bytes to read (copy) */
	nCopy = n;
	if((tsize_t)mfile->offset >= mfile->size)
		nRemaining = 0;
	else
		nRemaining = mfile->size - mfile->offset;
	
	if(nCopy > nRemaining)
		nCopy = nRemaining;
	
	/* on EOF, return immediately and read (copy) nothing */
	if(nCopy <= 0)
		return (0);

	/* all set -> do the read (copy) */
	srcAddr = (void*)(&(mfile->mem[mfile->offset]));
	memcpy((void*)data, srcAddr, nCopy);
	mfile->offset += nCopy;		/* advance file ptr by copied bytes */
	return nCopy;
}



/**
 * Writes data to an in-memory TIFF file.
 *
 * NOTE: The current Blender implementation should not need this function.  It
 *       is simply a stub.
 */
static tsize_t imb_tiff_WriteProc(thandle_t handle, tdata_t data, tsize_t n)
{
	printf("imb_tiff_WriteProc: this function should not be called.\n");
	return (-1);
}



/**
 * Seeks to a new location in an in-memory TIFF file.
 *
 * @param handle: Handle of the TIFF file (pointer to ImbTIFFMemFile).
 * @param ofs:    Offset value (interpreted according to whence below).
 * @param whence: This can be one of three values:
 * 	SEEK_SET - The offset is set to ofs bytes.
 * 	SEEK_CUR - The offset is set to its current location plus ofs bytes.
 * 	SEEK_END - (This is unsupported and will return -1, indicating an
 * 	            error).
 *
 * @return: Resulting offset location within the file, measured in bytes from
 *          the beginning of the file.  (-1) indicates an error.
 */
static toff_t imb_tiff_SeekProc(thandle_t handle, toff_t ofs, int whence)
{
	struct ImbTIFFMemFile *mfile;
	toff_t new_offset;

	/* get the pointer to the in-memory file */
	mfile = IMB_TIFF_GET_MEMFILE(handle);
	if(!mfile || !mfile->mem) {
		fprintf(stderr, "imb_tiff_SeekProc: !mfile || !mfile->mem!\n");
		return (-1);
	}

	/* find the location we plan to seek to */
	switch (whence) {
		case SEEK_SET:
			new_offset = ofs;
			break;
		case SEEK_CUR:
			new_offset = mfile->offset + ofs;
			break;
		default:
			/* no other types are supported - return an error */
			fprintf(stderr, 
				"imb_tiff_SeekProc: "
				"Unsupported TIFF SEEK type.\n");
			return (-1);
	}

	/* set the new location */
	mfile->offset = new_offset;
	return mfile->offset;
}



/**
 * Closes (virtually) an in-memory TIFF file.
 *
 * NOTE: All this function actually does is sets the data pointer within the
 *       TIFF file to NULL.  That should trigger assertion errors if attempts
 *       are made to access the file after that point.  However, no such
 *       attempts should ever be made (in theory).
 *
 * @param handle: Handle of the TIFF file (pointer to ImbTIFFMemFile).
 *
 * @return: 0
 */
static int imb_tiff_CloseProc(thandle_t handle)
{
	struct ImbTIFFMemFile *mfile;

	/* get the pointer to the in-memory file */
	mfile = IMB_TIFF_GET_MEMFILE(handle);
	if(!mfile || !mfile->mem) {
		fprintf(stderr,"imb_tiff_CloseProc: !mfile || !mfile->mem!\n");
		return (0);
	}
	
	/* virtually close the file */
	mfile->mem    = NULL;
	mfile->offset = 0;
	mfile->size   = 0;
	
	return (0);
}



/**
 * Returns the size of an in-memory TIFF file in bytes.
 *
 * @return: Size of file (in bytes).
 */
static toff_t imb_tiff_SizeProc(thandle_t handle)
{
	struct ImbTIFFMemFile* mfile;

	/* get the pointer to the in-memory file */
	mfile = IMB_TIFF_GET_MEMFILE(handle);
	if(!mfile || !mfile->mem) {
		fprintf(stderr,"imb_tiff_SizeProc: !mfile || !mfile->mem!\n");
		return (0);
	}

	/* return the size */
	return (toff_t)(mfile->size);
}



/**
 * Checks whether a given memory buffer contains a TIFF file.
 *
 * This method uses the format identifiers from:
 *     http://www.faqs.org/faqs/graphics/fileformats-faq/part4/section-9.html
 * The first four bytes of big-endian and little-endian TIFF files
 * respectively are (hex):
 * 	4d 4d 00 2a
 * 	49 49 2a 00
 * Note that TIFF files on *any* platform can be either big- or little-endian;
 * it's not platform-specific.
 *
 * AFAICT, libtiff doesn't provide a method to do this automatically, and
 * hence my manual comparison. - Jonathan Merritt (lancelet) 4th Sept 2005.
 */
#define IMB_TIFF_NCB 4		/* number of comparison bytes used */
int imb_is_a_tiff(unsigned char *mem)
{
	char big_endian[IMB_TIFF_NCB] = { 0x4d, 0x4d, 0x00, 0x2a };
	char lil_endian[IMB_TIFF_NCB] = { 0x49, 0x49, 0x2a, 0x00 };

	return ( (memcmp(big_endian, mem, IMB_TIFF_NCB) == 0) ||
		 (memcmp(lil_endian, mem, IMB_TIFF_NCB) == 0) );
}

static int imb_read_tiff_pixels(ImBuf *ibuf, TIFF *image)
{
	ImBuf *tmpibuf;
	int success;

	tmpibuf= IMB_allocImBuf(ibuf->x, ibuf->y, 32, IB_rect, 0);
	success = libtiff_TIFFReadRGBAImage(image, ibuf->x, ibuf->y, tmpibuf->rect, 0);

	if (ENDIAN_ORDER == B_ENDIAN) IMB_convert_rgba_to_abgr(tmpibuf);

	/* assign rect last */
	ibuf->rect= tmpibuf->rect;
	ibuf->mall |= IB_rect;
	ibuf->flags |= IB_rect;

	tmpibuf->mall &= ~IB_rect;
	IMB_freeImBuf(tmpibuf);

	return success;
}

/**
 * Loads a TIFF file.
 *
 * This function uses the "RGBA Image" support from libtiff, which enables
 * it to load most commonly-encountered TIFF formats.  libtiff handles format
 * conversion, color depth conversion, etc.
 *
 * @param mem:   Memory containing the TIFF file.
 * @param size:  Size of the mem buffer.
 * @param flags: If flags has IB_test set then the file is not actually loaded,
 *                but all other operations take place.
 *
 * @return: A newly allocated ImBuf structure if successful, otherwise NULL.
 */
struct ImBuf *imb_loadtiff(unsigned char *mem, int size, int flags)
{
	TIFF *image = NULL;
	struct ImBuf *ibuf = NULL;
	struct ImbTIFFMemFile memFile;
	uint32 width, height;
	char *format = NULL;

	if(!G.have_libtiff)
		return NULL;

	memFile.mem = mem;
	memFile.offset = 0;
	memFile.size = size;

	/* check whether or not we have a TIFF file */
	if(size < IMB_TIFF_NCB) {
		fprintf(stderr, "imb_loadtiff: size < IMB_TIFF_NCB\n");
		return NULL;
	}
	if(imb_is_a_tiff(mem) == 0)
		return NULL;

	/* open the TIFF client layer interface to the in-memory file */
	image = libtiff_TIFFClientOpen("(Blender TIFF Interface Layer)", 
		"r", (thandle_t)(&memFile),
		imb_tiff_ReadProc, imb_tiff_WriteProc,
		imb_tiff_SeekProc, imb_tiff_CloseProc,
		imb_tiff_SizeProc, imb_tiff_DummyMapProc, imb_tiff_DummyUnmapProc);
	if(image == NULL) {
		printf("imb_loadtiff: could not open TIFF IO layer.\n");
		return NULL;
	}

	/* allocate the image buffer */
	libtiff_TIFFGetField(image, TIFFTAG_IMAGEWIDTH,  &width);
	libtiff_TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height);
	ibuf = IMB_allocImBuf(width, height, 32, 0, 0);
	if(ibuf) {
		ibuf->ftype = TIF;
		ibuf->profile = IB_PROFILE_SRGB;
	}
	else {
		fprintf(stderr, 
			"imb_loadtiff: could not allocate memory for TIFF " \
			"image.\n");
		libtiff_TIFFClose(image);
		return NULL;
	}

	/* if testing, we're done */
	if(flags & IB_test) {
		libtiff_TIFFClose(image);
		return (ibuf);
	}

	/* detect if we are reading a tiled/mipmapped texture, in that case
	   we don't read pixels but leave it to the cache to load tiles */
	if(flags & IB_usecache) {
		format= NULL;
		libtiff_TIFFGetField(image, TIFFTAG_PIXAR_TEXTUREFORMAT, &format);

		if(format && strcmp(format, "Plain Texture")==0) {
			ibuf->flags |= IB_usecache;
			ibuf->miplevels = libtiff_TIFFNumberOfDirectories(image) + 1;
		}
	}

	/* read pixels */
	if(!(ibuf->flags & IB_usecache) && !imb_read_tiff_pixels(ibuf, image)) {
		fprintf(stderr, "imb_loadtiff: Failed to read tiff image.\n");
		libtiff_TIFFClose(image);
		return NULL;
	}

	/* close the client layer interface to the in-memory file */
	libtiff_TIFFClose(image);

	/* return successfully */
	return (ibuf);
}

void imb_loadmiptiff(struct ImBuf *ibuf, unsigned char *mem, int size, int level)
{
	TIFF *image = NULL;
	uint32 width, height;
	int mipx, mipy;
	struct ImbTIFFMemFile memFile;

	memFile.mem = mem;
	memFile.offset = 0;
	memFile.size = size;

	/* open the TIFF client layer interface to the in-memory file */
	image = libtiff_TIFFClientOpen("(Blender TIFF Interface Layer)", 
		"r", (thandle_t)(&memFile),
		imb_tiff_ReadProc, imb_tiff_WriteProc,
		imb_tiff_SeekProc, imb_tiff_CloseProc,
		imb_tiff_SizeProc, imb_tiff_DummyMapProc, imb_tiff_DummyUnmapProc);

	if(image == NULL) {
		printf("imb_loadtiff: could not open TIFF IO layer for loading mipmap level.\n");
		return;
	}

   	if(libtiff_TIFFSetDirectory(image, level)) {
		IMB_getmipmaplevel_size(ibuf, level, &mipx, &mipy);

		/* allocate the image buffer */
		libtiff_TIFFGetField(image, TIFFTAG_IMAGEWIDTH,  &width);
		libtiff_TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height);

		if(width == mipx && height == mipy) {
			if(level == 0) {
				imb_read_tiff_pixels(ibuf, image);
			}
			else {
				ibuf->mipmap[level-1] = IMB_allocImBuf(width, height, 32, 0, 0);
				imb_read_tiff_pixels(ibuf->mipmap[level-1], image);
			}
		}
		else
			printf("imb_loadtiff: mipmap level %d has unexpected size\n", level);
	}
	else
		printf("imb_loadtiff: could not find mipmap level %d\n", level);

	/* close the client layer interface to the in-memory file */
	libtiff_TIFFClose(image);
}

/**
 * Saves a TIFF file.
 *
 * ImBuf structures with 1, 3 or 4 bytes per pixel (GRAY, RGB, RGBA 
 * respectively) are accepted, and interpreted correctly.  Note that the TIFF
 * convention is to use pre-multiplied alpha, which can be achieved within
 * Blender by setting "Premul" alpha handling.  Other alpha conventions are
 * not strictly correct, but are permitted anyhow.
 *
 * @param ibuf:  Image buffer.
 * @param name:  Name of the TIFF file to create.
 * @param flags: Currently largely ignored.
 *
 * @return: 1 if the function is successful, 0 on failure.
 */

#define FTOUSHORT(val) ((val >= 1.0f-0.5f/65535)? 65535: (val <= 0.0f)? 0: (unsigned short)(val*65535.0f + 0.5f))

int imb_savetiff(struct ImBuf *ibuf, char *name, int flags)
{
	TIFF *image = NULL;
	uint16 samplesperpixel, bitspersample;
	size_t npixels;
	unsigned char *pixels = NULL;
	unsigned char *from = NULL, *to = NULL;
	unsigned short *pixels16 = NULL, *to16 = NULL;
	float *fromf = NULL;
	int x, y, from_i, to_i, i;
	int extraSampleTypes[1] = { EXTRASAMPLE_ASSOCALPHA };
	
	if(!G.have_libtiff) {
		fprintf(stderr, "imb_savetiff: no tiff library available.\n");
		return (0);
	}

	/* check for a valid number of bytes per pixel.  Like the PNG writer,
	 * the TIFF writer supports 1, 3 or 4 bytes per pixel, corresponding
	 * to gray, RGB, RGBA respectively. */
	samplesperpixel = (uint16)((ibuf->depth + 7) >> 3);
	if((samplesperpixel > 4) || (samplesperpixel == 2)) {
		fprintf(stderr,
			"imb_savetiff: unsupported number of bytes per " 
			"pixel: %d\n", samplesperpixel);
		return (0);
	}

	if((ibuf->ftype & TIF_16BIT) && ibuf->rect_float)
		bitspersample = 16;
	else
		bitspersample = 8;

	/* open TIFF file for writing */
	if(flags & IB_mem) {
		/* bork at the creation of a TIFF in memory */
		fprintf(stderr,
			"imb_savetiff: creation of in-memory TIFF files is " 
			"not yet supported.\n");
		return (0);
	}
	else {
		/* create image as a file */
		image = libtiff_TIFFOpen(name, "w");
	}
	if(image == NULL) {
		fprintf(stderr,
			"imb_savetiff: could not open TIFF for writing.\n");
		return (0);
	}

	/* allocate array for pixel data */
	npixels = ibuf->x * ibuf->y;
	if(bitspersample == 16)
		pixels16 = (unsigned short*)libtiff__TIFFmalloc(npixels *
			samplesperpixel * sizeof(unsigned short));
	else
		pixels = (unsigned char*)libtiff__TIFFmalloc(npixels *
			samplesperpixel * sizeof(unsigned char));

	if(pixels == NULL && pixels16 == NULL) {
		fprintf(stderr,
			"imb_savetiff: could not allocate pixels array.\n");
		libtiff_TIFFClose(image);
		return (0);
	}

	/* setup pointers */
	if(bitspersample == 16) {
		fromf = ibuf->rect_float;
		to16   = pixels16;
	}
	else {
		from = (unsigned char*)ibuf->rect;
		to   = pixels;
	}

	/* setup samples per pixel */
	libtiff_TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, bitspersample);
	libtiff_TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);

	if(samplesperpixel == 4) {
		/* RGBA images */
		libtiff_TIFFSetField(image, TIFFTAG_EXTRASAMPLES, 1,
				extraSampleTypes);
		libtiff_TIFFSetField(image, TIFFTAG_PHOTOMETRIC, 
				PHOTOMETRIC_RGB);
	}
	else if(samplesperpixel == 3) {
		/* RGB images */
		libtiff_TIFFSetField(image, TIFFTAG_PHOTOMETRIC,
				PHOTOMETRIC_RGB);
	}
	else if(samplesperpixel == 1) {
		/* greyscale images, 1 channel */
		libtiff_TIFFSetField(image, TIFFTAG_PHOTOMETRIC,
				PHOTOMETRIC_MINISBLACK);
	}

	/* copy pixel data.  While copying, we flip the image vertically. */
	for(x = 0; x < ibuf->x; x++) {
		for(y = 0; y < ibuf->y; y++) {
			from_i = 4*(y*ibuf->x+x);
			to_i   = samplesperpixel*((ibuf->y-y-1)*ibuf->x+x);

			if(pixels16) {
				for(i = 0; i < samplesperpixel; i++, to_i++, from_i++)
					to16[to_i] = FTOUSHORT(fromf[from_i]);
			}
			else {
				for(i = 0; i < samplesperpixel; i++, to_i++, from_i++)
					to[to_i] = from[from_i];
			}
		}
	}

	/* write the actual TIFF file */
	libtiff_TIFFSetField(image, TIFFTAG_IMAGEWIDTH,      ibuf->x);
	libtiff_TIFFSetField(image, TIFFTAG_IMAGELENGTH,     ibuf->y);
	libtiff_TIFFSetField(image, TIFFTAG_ROWSPERSTRIP,    ibuf->y);
	libtiff_TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
	libtiff_TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
	libtiff_TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	libtiff_TIFFSetField(image, TIFFTAG_XRESOLUTION,     150.0);
	libtiff_TIFFSetField(image, TIFFTAG_YRESOLUTION,     150.0);
	libtiff_TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT,  RESUNIT_INCH);
	if(libtiff_TIFFWriteEncodedStrip(image, 0,
			(bitspersample == 16)? (unsigned char*)pixels16: pixels,
			ibuf->x*ibuf->y*samplesperpixel*bitspersample/8) == -1) {
		fprintf(stderr,
			"imb_savetiff: Could not write encoded TIFF.\n");
		libtiff_TIFFClose(image);
		if(pixels) libtiff__TIFFfree(pixels);
		if(pixels16) libtiff__TIFFfree(pixels16);
		return (1);
	}

	/* close the TIFF file */
	libtiff_TIFFClose(image);
	if(pixels) libtiff__TIFFfree(pixels);
	if(pixels16) libtiff__TIFFfree(pixels16);
	return (1);
}

