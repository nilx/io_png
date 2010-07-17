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
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <png.h>

#include "io_png.h"

#define PNG_SIG_LEN 4

/**
 * @brief internal function used to read a PNG file into a byte array
 *
 * @todo don't loose 16bit info, and use
 * - bit_depth = png_get_bit_depth(png_ptr, info_ptr);
 *
 * @param fname PNG file name 
 * @param nx, ny, nc pointers to variables to be fill with the number of
 *        columns, lines and channels of the image
 * @return pointer to an allocated array of pixels,
 *         or NULL if an error happens
 */
static png_byte *read_png_byte(const char *fname,
			       size_t *nx, size_t *ny, unsigned char *nc)
{
    png_byte png_sig[PNG_SIG_LEN];
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    png_byte channels;
    png_bytepp row_pointers;
    png_bytep row_ptr;
    int png_transform = 0;
    FILE *fp;
    png_byte *data=NULL;
    png_byte *data_ptr;
    size_t i, j, k;

    /* parameters check */
    if (NULL == fname
	|| NULL == nx
	|| NULL == ny
	|| NULL == nc)
	return NULL;

    /* open the PNG file */
    if (NULL == (fp = fopen(fname, "rb")))
	return NULL;
    
    /* read in some of the signature bytes and check this signature */
    if ((PNG_SIG_LEN != fread(png_sig, 1, PNG_SIG_LEN, fp))
	|| png_sig_cmp(png_sig, (png_size_t) 0, PNG_SIG_LEN))
    {
	fclose(fp);
	return NULL;
    }
    
    /* 
     * create and initialize the png_struct
     * with the default stderr and error handling
     */
    if (NULL == (png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
						  NULL, NULL, NULL)))
    {
	fclose(fp);
	return NULL;
    }
    
    /* allocate/initialize the memory for image information */
    if (NULL == (info_ptr = png_create_info_struct(png_ptr)))
    {
	png_destroy_read_struct(&png_ptr, NULL, NULL);
	fclose(fp);
	return NULL;
    }
    
    /* set error handling */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
	/* free all of the memory associated with the png_ptr and info_ptr */
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
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
    png_transform = (PNG_TRANSFORM_STRIP_16
		     | PNG_TRANSFORM_PACKING);

    /* read in the entire image at once */
    png_read_png(png_ptr, info_ptr, png_transform, NULL);

    /* get image informations */
    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    channels = png_get_channels(png_ptr, info_ptr);
    row_pointers = png_get_rows(png_ptr, info_ptr);

    /* allocate the data RGB array */
    if (NULL == (data = (png_byte *)malloc(width * height * channels
						* sizeof(png_byte))))
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
    for (k=0; k<channels; k++)
    {
	/* channel loop */
	data_ptr = data + (size_t) (width * height * k);
	for (j=0; j<height; j++)
	{
	    /* row loop */
	    row_ptr = row_pointers[j] + k;
	    for (i=0; i<width; i++)
	    {
		/* pixel loop */
		*data_ptr++ = *row_ptr;
		row_ptr += channels;
	    }
	}
    }

    /* clean up after the read, and free any memory allocated */
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    
    /* close the file */
    fclose(fp);
    
    /* update the size pointers and return the data pointer */
    *nx = (size_t) width;
    *ny = (size_t) height;
    *nc = (size_t) channels;
    
    return data;
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
unsigned char *read_png_u8(const char *fname,
			   size_t *nx, size_t *ny, unsigned char *nc)
{
    /* 
     * png_bytes are same as unsigned chars,
     * so a simple cast is enough.
     * but check, just in case...
     */
    assert (sizeof(png_byte) == sizeof(unsigned char));
    return (unsigned char *)read_png_byte(fname, nx, ny, nc);
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
float *read_png_f32(const char *fname,
		    size_t *nx, size_t *ny, unsigned char *nc)
{
    png_byte *data_byte, *byte_ptr, *byte_end;
    float *data_flt, *flt_ptr;
    size_t size;

    /* read the image as bytes */
    if (NULL == (data_byte = read_png_byte(fname, nx, ny, nc)))
	return NULL;

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

    return data_flt;
}













#if 0










/* Write a png file */
void write_png(char *file_name /* , ... other image information ... */)
{
   FILE *fp;
   png_structp png_ptr;
   png_infop info_ptr;
   png_colorp palette;

   /* Open the file */
   fp = fopen(file_name, "wb");
   if (fp == NULL)
      return (ERROR);

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also check that
    * the library version is compatible with the one used at compile time,
    * in case we are using dynamically linked libraries.  REQUIRED.
    */
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
      png_voidp user_error_ptr, user_error_fn, user_warning_fn);

   if (png_ptr == NULL)
   {
      fclose(fp);
      return (ERROR);
   }

   /* Allocate/initialize the image information data.  REQUIRED */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  NULL);
      return (ERROR);
   }

   /* Set error handling.  REQUIRED if you aren't supplying your own
    * error handling functions in the png_create_write_struct() call.
    */
   if (setjmp(png_jmpbuf(png_ptr)))
   {
      /* If we get here, we had a problem writing the file */
      fclose(fp);
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return (ERROR);
   }

   /* Set up the output control if you are using standard C streams */
   png_init_io(png_ptr, fp);

   /* This is the easy way.  Use it if you already have all the
    * image info living in the structure.  You could "|" many
    * PNG_TRANSFORM flags into the png_transforms integer here.
    */
   png_write_png(png_ptr, info_ptr, png_transforms, NULL);


   /* If you png_malloced a palette, free it here (don't free info_ptr->palette,
    * as recommended in versions 1.0.5m and earlier of this example; if
    * libpng mallocs info_ptr->palette, libpng will free it).  If you
    * allocated it with malloc() instead of png_malloc(), use free() instead
    * of png_free().
    */
   png_free(png_ptr, palette);
   palette = NULL;

   /* Similarly, if you png_malloced any data that you passed in with
    * png_set_something(), such as a hist or trans array, free it here,
    * when you can be sure that libpng is through with it.
    */
   png_free(png_ptr, trans);
   trans = NULL;
   /* Whenever you use png_free() it is a good idea to set the pointer to
    * NULL in case your application inadvertently tries to png_free() it
    * again.  When png_free() sees a NULL it returns without action, thus
    * avoiding the double-free security problem.
    */

   /* Clean up after the write, and free any memory allocated */
   png_destroy_write_struct(&png_ptr, &info_ptr);

   /* Close the file */
   fclose(fp);

   /* That's it */
   return (OK);
}

#endif
