/*
 * Copyright 2010 Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file io_png.c
 * @brief PNG read-write routines
 *
 * @todo handle stdin/stdout as "-"
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#ifdef _LOCAL_LIBS
#include "png.h"
#else
#include <png.h>
#endif

/* ensure consistency */
#include "io_png.h"

#define PNG_SIG_LEN 4

/*
 * READ
 */

/**
 * @brief internal function used to read a PNG file into a byte array
 *
 * @todo don't loose 16bit info
 *
 * @param fname PNG file name 
 * @param width, height, channels pointers to variables to be filled
 *        with the number of columns, lines and channels of the image
 * @return pointer to an allocated array of pixels,
 *         or NULL if an error happens
 */
static png_byte *read_png_byte(const char *fname,
			       png_uint_32 *width, png_uint_32 *height,
			       png_byte *channels)
{
    png_byte png_sig[PNG_SIG_LEN];
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytepp row_pointers;
    png_bytep row_ptr;
    int transform;
    FILE *fp = NULL;
    png_byte *data = NULL;
    png_byte *data_ptr;
    size_t size;
    size_t i, j, k;

    /* parameters check */
    if (NULL == fname
	|| NULL == width
	|| NULL == height
	|| NULL == channels)
	return NULL;

    /* open the PNG file */
    if (NULL == (fp = fopen(fname, "rb")))
	return NULL;

    /* read in some of the signature bytes and check this signature */
    if ((PNG_SIG_LEN != fread(png_sig, 1, PNG_SIG_LEN, fp))
	|| 0 != png_sig_cmp(png_sig, (png_size_t) 0, PNG_SIG_LEN))
    {
	(void) fclose(fp);
	return NULL;
    }
    
    /* 
     * create and initialize the png_struct
     * with the default stderr and error handling
     */
    if (NULL == (png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
						  NULL, NULL, NULL)))
    {
	(void) fclose(fp);
	return NULL;
    }
    
    /* allocate/initialize the memory for image information */
    if (NULL == (info_ptr = png_create_info_struct(png_ptr)))
    {
	png_destroy_read_struct(&png_ptr, NULL, NULL);
	(void) fclose(fp);
	return NULL;
    }
    
    /* set error handling */
    if (0 != setjmp(png_jmpbuf(png_ptr)))
    {
	/* free all of the memory associated with the png_ptr and info_ptr */
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	(void) fclose(fp);
	/* If we get here, we had a problem reading the file */
	return NULL;
    }
    
    /* set up the input control using standard C streams */
    png_init_io(png_ptr, fp);
    
    /* let libpng know that some bytes have been read */
    png_set_sig_bytes(png_ptr, PNG_SIG_LEN);
    
    /*
     * set the read filter transforms, to get 8bit RGB whatever the
     * original file may contain:
     * PNG_TRANSFORM_STRIP_16      strip 16-bit samples to 8 bits
     * PNG_TRANSFORM_PACKING       expand 1, 2 and 4-bit
     *                             samples to bytes
     */
    transform = (PNG_TRANSFORM_STRIP_16
		 | PNG_TRANSFORM_PACKING);

    /* read in the entire image at once */
    png_read_png(png_ptr, info_ptr, transform, NULL);

    /* get image informations */
    *width = png_get_image_width(png_ptr, info_ptr);
    *height = png_get_image_height(png_ptr, info_ptr);
    *channels = png_get_channels(png_ptr, info_ptr);
    row_pointers = png_get_rows(png_ptr, info_ptr);

    /* allocate the data RGB array */
    size = *width * *height * *channels;
    if (NULL == (data = (png_byte *)malloc(size * sizeof(png_byte))))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
        return NULL;
    }

    /*
     * deinterlace and convert png RGB RGB RGB 8bit to RRR GGG BBB
     * the image is deinterlaced layer after layer
     * this involved more memory exchange, but allows a generic loop
     */
    for (k=0; k<*channels; k++)
    {
	/* channel loop */
	data_ptr = data + (size_t) (*width * *height * k);
	for (j=0; j<*height; j++)
	{
	    /* row loop */
	    row_ptr = row_pointers[j] + k;
	    for (i=0; i<*width; i++)
	    {
		/* pixel loop */
		*data_ptr++ = *row_ptr;
		row_ptr += *channels;
	    }
	}
    }

    /* clean up and free any memory allocated, close the file */
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    
    return data;
}

/**
 * @brief read a PNG file into a 8bit integer array
 *
 * The array contains the deinterlaced channels.
 * 1, 2 and 4bit images are converted to 8bit.
 * 16bit images are previously downscaled to 8bit.
 *
 * @todo don't downscale 16bit images.
 *
 * @param fname PNG file name 
 * @param nx, ny, nc pointers to variables to be filled with the number of
 *        columns, lines and channels of the image
 * @return pointer to an allocated unsigned char array of pixels,
 *         or NULL if an error happens
 */
unsigned char *read_png_u8(const char *fname,
			   size_t *nx, size_t *ny, size_t *nc)
{
    png_byte *data_byte=NULL;
    png_uint_32 width, height;
    png_byte channels;

    /* read the image as bytes */
    if (NULL == (data_byte = read_png_byte(fname, &width, &height, &channels)))
	return NULL;

    *nx = (size_t)width;
    *ny = (size_t)height;
    *nc = (size_t)channels;
    
    /* 
     * png_bytes are same as unsigned chars,
     * so a simple cast is enough.
     * but check, just in case...
     */
    assert (sizeof(png_byte) == sizeof(unsigned char));

    return (unsigned char *)data_byte;
}

/**
 * @brief read a PNG file into a 32bit float array
 *
 * The array contains the deinterlaced channels.
 * 1, 2, 4 and 8bit images are converted to float values
 * between 0. and 1., 3., 15. or 255.
 * 16bit images are also downscaled to 8bit before conversion.
 *
 * @param fname PNG file name
 * @param nx, ny, nc pointers to variables to be filled with the number of
 *        columns, lines and channels of the image
 * @return pointer to an allocated unsigned char array of pixels,
 *         or NULL if an error happens
 */
float *read_png_f32(const char *fname,
		    size_t *nx, size_t *ny, size_t *nc)
{
    png_byte *data_byte=NULL, *byte_ptr=NULL, *byte_end=NULL;
    float *data_flt=NULL, *flt_ptr=NULL;
    png_uint_32 width, height;
    png_byte channels;
    size_t size;

    /* read the image as bytes */
    if (NULL == (data_byte = read_png_byte(fname, &width, &height, &channels)))
	return NULL;

    *nx = (size_t)width;
    *ny = (size_t)height;
    *nc = (size_t)channels;

    /* allocate the float array */
    size = *nx * *ny * *nc;
    if (NULL == (data_flt = (float *)malloc(size * sizeof(float))))
    {
	free(data_byte);
	return NULL;
    }

    /* convert */
    byte_ptr = data_byte;
    byte_end = byte_ptr + size;
    flt_ptr = data_flt;
    while (byte_ptr < byte_end)
	*flt_ptr++ = (float) *byte_ptr++;

    free(data_byte);
    return data_flt;
}

/*
 * WRITE
 */

/**
 * @brief internal function used to write a byte array as a PNG file
 *
 * The PNG file is written as a 8bit image file, interlaced,
 * truecolor. Depending on the number of channels, the color model is
 * gray, gray+alpha, rgb, rgb+alpha.
 *
 * @todo handle 16bit
 *
 * @param fname PNG file name 
 * @param data deinterlaced (RRRGGGBBBAAA) image byte array
 * @param width, height, channels number of columns, lines and channels
 * @return 0 if everything OK, -1 if an error occured
 */
static int write_png_byte(const char *fname, const png_byte *data,
			  png_uint_32 width, png_uint_32 height,
			  png_byte channels)
{
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
    const png_byte *data_ptr=NULL;
    png_byte *idata=NULL, *idata_ptr=NULL;
    png_bytep *row_pointers=NULL;
    png_byte bit_depth;
    int color_type, interlace, compression, filter;
    size_t size;
    size_t i, j, k;

    /* parameters check */
    if (NULL == fname
	|| NULL == data)
	return -1;

    /* open the PNG file */
    if (NULL == (fp = fopen(fname, "wb")))
	return -1;

    /* allocate the interlaced array and row pointers */
    size = width * height * channels;
    if (NULL == (idata = (png_byte *)malloc(size * sizeof(png_byte))))
    {
	fclose(fp);
        return -1;
    }

    if (NULL == (row_pointers = (png_bytep *)malloc(height * sizeof(png_bytep))))
    {
	free(idata);
	fclose(fp);
        return -1;
    }

    /* 
     * create and initialize the png_struct
     * with the default stderr and error handling
     */
    if (NULL == (png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
						  NULL, NULL, NULL)))
    {
	free(row_pointers);
	free(idata);
	(void) fclose(fp);
	return -1;
    }
    
    /* allocate/initialize the memory for image information */
    if (NULL == (info_ptr = png_create_info_struct(png_ptr)))
    {
	png_destroy_read_struct(&png_ptr, NULL, NULL);
	free(row_pointers);
	free(idata);
	(void) fclose(fp);
	return -1;
    }
    
    /* set error handling */
    if (0 != setjmp(png_jmpbuf(png_ptr)))
    {
	/* free all of the memory associated with the png_ptr and info_ptr */
	png_destroy_write_struct(&png_ptr, &info_ptr);
	free(row_pointers);
	free(idata);
	(void) fclose(fp);
	/* If we get here, we had a problem reading the file */
	return -1;
    }
    
    /* set up the input control using standard C streams */
    png_init_io(png_ptr, fp);
    
    /* set image informations */
    bit_depth = 8;
    switch (channels)
    {
    case 1:
	color_type = PNG_COLOR_TYPE_GRAY;
	break;
    case 2:
	color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
	break;
    case 3:
	color_type = PNG_COLOR_TYPE_RGB;
	break;
    case 4:
	color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	break;
    default:
	png_destroy_read_struct(&png_ptr, NULL, NULL);
	free(row_pointers);
	free(idata);
	(void) fclose(fp);
	return -1;
    }
    interlace = PNG_INTERLACE_ADAM7;
    compression = PNG_COMPRESSION_TYPE_BASE;
    filter = PNG_FILTER_TYPE_BASE;

    /* set image header */
    png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type,
		 interlace, compression, filter);
    /* TODO : significant bit (sBIT), gamma (gAMA), comments (text) chunks */
    png_write_info(png_ptr, info_ptr);

    /*
     * interlace and convert RRR GGG BBB to RGB RGB RGB
     * the image is interlaced layer after layer
     * this involved more memory exchange, but allows a generic loop
     */
    for (k=0; k<channels; k++)
    {
	/* channel loop */
	data_ptr = data + (size_t) (width * height * k);
	idata_ptr = idata + (size_t) k;
	for (j=0; j<height; j++)
	{
	    /* row loop */
	    for (i=0; i<width; i++)
	    {
		/* pixel loop */
		*idata_ptr = *data_ptr++;
		idata_ptr += channels;
	    }
	}
    }
    /* set row pointers */
    for (j=0; j<height; j++)
	row_pointers[j] = idata + (size_t) (channels * width * j);

    /* write out the entire image and end it */
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);

    /* clean up and free any memory allocated, close the file */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    free(row_pointers);
    free(idata);
    fclose(fp);

    return 0;
}

/**
 * @brief write a 8bit unsigned integer array into a PNG file
 *
 * @param fname PNG file name 
 * @param data array to write
 * @param nx, ny, nc number of columns, lines and channels of the image
 * @return 0 if everything OK, -1 if an error occured
 */
int write_png_u8(const char *fname, const unsigned char *data,
		 size_t nx, size_t ny, size_t nc)
{
    /* 
     * png_bytes are same as unsigned chars,
     * so a simple cast is enough.
     * but check, just in case...
     */
    assert (sizeof(png_byte) == sizeof(unsigned char));

    return write_png_byte(fname, (png_byte*)data,
			  (png_uint_32)nx, (png_uint_32)ny, (png_byte)nc);
}

/**
 * @brief write a float array into a PNG file
 *
 * The float values are rounded to 8bit integers, and bounded to [0, 255].
 * 
 * @todo handle 16bit images and flexible min/max
 *
 * @param fname PNG file name 
 * @param data array to write
 * @param nx, ny, nc number of columns, lines and channels of the image
 * @return 0 if everything OK, -1 if an error occured
 */
int write_png_f32(const char *fname, const float *data,
		 size_t nx, size_t ny, size_t nc)
{
    png_byte *data_byte=NULL, *byte_ptr=NULL;
    const float *flt_ptr=NULL, *flt_end=NULL;
    size_t size;
    double tmp;
    int retval;

    /* allocate the byte array */
    size = nx * ny * nc;
    if (NULL == (data_byte = (png_byte *)malloc(size * sizeof(png_byte))))
	return -1;

    /* convert */
    flt_ptr = data;
    flt_end = flt_ptr + size;
    byte_ptr = data_byte;
    while (flt_ptr < flt_end)
    {
	tmp = floor(*flt_ptr++ + .5);
	*byte_ptr++ = (png_byte) (tmp < 0. ? 0. :
				  (tmp > 255. ? 255. :
				   tmp));
    }

    retval = write_png_byte(fname, data_byte,
			    (png_uint_32)nx, (png_uint_32)ny, (png_byte)nc);
    free(data_byte);
    return retval;
}

