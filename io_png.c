/*
 * Copyright (c) 2010-2011, Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under, at your option, the terms of the GNU General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version, or
 * the terms of the simplified BSD license.
 *
 * You should have received a copy of these licenses along this
 * program. If not, see <http://www.gnu.org/licenses/> and
 * <http://www.opensource.org/licenses/bsd-license.html>.
 */

/**
 * @file io_png.c
 * @brief PNG read/write simplified interface
 *
 * This is a front-end to libpng, with routines to:
 * @li read a PNG file into a de-interlaced unsigned char or float array
 * @li write an unsigned char or float array to a PNG file
 *
 * Multi-channel images are handled: gray, gray+alpha, rgb and
 * rgb+alpha, as well as on-the-fly rgb/gray conversion.
 *
 * @todo add type width assertions
 * @todo handle 16bit data
 * @todo replace rgb/gray with sRGB / Y references
 * @todo implement sRGB gamma and better RGBY conversion
 * @todo process the data as float before quantization
 * @todo pre/post-processing timing
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

/* option to use a local version of the libpng */
#ifdef IO_PNG_LOCAL_LIBPNG
#include "png.h"
#else
#include <png.h>
#endif

/* ensure consistency */
#include "io_png.h"

/*
 * INFO
 */

/** @brief string tag inserted into the binary */
static char _io_png_tag[] = "using io_png " IO_PNG_VERSION;
/** @brief helps tracking versions with the string tag */
char *io_png_info(void)
{
    return _io_png_tag;
}

/*
 * UTILS
 */

/** @brief abort() wrapper macro with an error message */
#define _IO_PNG_ABORT(MSG) do {					\
    fprintf(stderr, "%s:%04u : %s\n", __FILE__, __LINE__, MSG); \
    fflush(stderr);						\
    abort();							\
    } while (0);

/** @brief safe malloc wrapper */
static void *_io_png_safe_malloc(size_t size)
{
    void *memptr;

    if (NULL == (memptr = malloc(size)))
        _IO_PNG_ABORT("not enough memory");
    return memptr;
}

/** @brief safe malloc wrapper macro with safe casting */
#define _IO_PNG_SAFE_MALLOC(NB, TYPE)                                   \
    ((TYPE *) _io_png_safe_malloc((size_t) (NB) * sizeof(TYPE)))

/** @brief safe realloc wrapper */
static void *_io_png_safe_realloc(void *memptr, size_t size)
{
    void *newptr;

    if (NULL == (newptr = realloc(memptr, size)))
        _IO_PNG_ABORT("not enough memory");
    return newptr;
}

/** @brief safe realloc wrapper macro with safe casting */
#define _IO_PNG_SAFE_REALLOC(PTR, NB, TYPE)                             \
    ((TYPE *) _io_png_safe_realloc((void *) (PTR), (size_t) (NB) * sizeof(TYPE)))

/**
 * @brief local error structure
 * see http://www.libpng.org/pub/png/book/chapter14.htmlpointer
 */
typedef struct _io_png_err_s {
    jmp_buf jmpbuf;
} _io_png_err_t;

/**
 * @brief local error handler
 * see http://www.libpng.org/pub/png/book/chapter14.htmlpointer
 */
static void _io_png_err_hdl(png_structp png_ptr, png_const_charp msg)
{
    _io_png_err_t *err_ptr;

    fprintf(stderr, "libpng error: %s\n", msg);

    err_ptr = (_io_png_err_t *) png_get_error_ptr(png_ptr);
    if (NULL == png_ptr)
        _IO_PNG_ABORT
            ("fatal unrecoverable error in libpng calls, terminating");

    longjmp(err_ptr->jmpbuf, 1);
}

/**
 * @brief interlace a png_byte array
 *
 * @param png_data array to interlace
 * @param csize array size per channel
 * @param nc number of channels to interlace
 * @return new array
 *
 * @todo real in-place method
 */
static png_byte *_io_png_interlace(png_byte * png_data, size_t csize,
                                   size_t nc)
{
    size_t i, size;
    png_byte *tmp;

    if (NULL == png_data || 0 == csize || 0 == nc)
        _IO_PNG_ABORT("bad parameters");
    if (1 == nc || 1 == csize)
        /* nothing to do */
        return png_data;

    size = nc * csize;
    tmp = _IO_PNG_SAFE_MALLOC(size, png_byte);
    for (i = 0; i < size; i++)
        /*
         * set the i-th element of tmp, interlaced
         * its channel is i % nc
         * its position in this channel is i / nc
         */
        tmp[i] = png_data[i % nc * csize + i / nc];

    free(png_data);

    return tmp;
}

/**
 * @brief deinterlace a png_byte array
 *
 * @param png_data array to deinterlace
 * @param csize array size per channel
 * @param nc number of channels to deinterlace
 * @return new array
 *
 * @todo real in-place method
 */
static png_byte *_io_png_deinterlace(png_byte * png_data, size_t csize,
                                     size_t nc)
{
    size_t i, size;
    png_byte *tmp;

    if (NULL == png_data || 0 == csize || 0 == nc)
        _IO_PNG_ABORT("bad parameters");
    if (1 == nc || 1 == csize)
        /* nothing to do */
        return png_data;

    size = nc * csize;
    tmp = _IO_PNG_SAFE_MALLOC(size, png_byte);
    for (i = 0; i < size; i++)
        /* see _io_png_interlace() */
        tmp[i % nc * csize + i / nc] = png_data[i];

    free(png_data);

    return tmp;
}

/**
 * @brief convert raw png data to unsigned char
 *
 * This function is currently useless, but is inserted to help for the
 * float-based transition.
 *
 * @param png_data array to convert
 * @param size array size
 * @return converted array
 *
 * @todo check types, real conversion
 */
static unsigned char *_io_png_to_uchar(const png_byte * png_data, size_t size)
{
    size_t i;
    unsigned char *data;

    if (NULL == png_data || 0 == size)
        _IO_PNG_ABORT("bad parameters");

    data = _IO_PNG_SAFE_MALLOC(size, unsigned char);
    for (i = 0; i < size; i++)
        data[i] = (unsigned char) png_data[i];

    return data;
}

/**
 * @brief convert to raw png data from unsigned char
 *
 * This function is currently useless, but is inserted to help for the
 * float-based transition.
 *
 * @param data array to convert
 * @param size array size
 * @return converted array
 *
 * @todo check types, real conversion
 */
static png_byte *_io_png_from_uchar(const unsigned char *data, size_t size)
{
    size_t i;
    png_byte *png_data;

    if (NULL == data || 0 == size)
        _IO_PNG_ABORT("bad parameters");

    png_data = _IO_PNG_SAFE_MALLOC(size, png_byte);
    for (i = 0; i < size; i++)
        png_data[i] = (png_byte) data[i];

    return png_data;
}

/**
 * @brief convert raw png data to float
 *
 * @param png_data array to convert
 * @param size array size
 * @return converted array
 *
 * @todo real sampling
 */
static float *_io_png_to_flt(const png_byte * png_data, size_t size)
{
    size_t i;
    float *data;

    if (NULL == png_data || 0 == size)
        _IO_PNG_ABORT("bad parameters");

    data = _IO_PNG_SAFE_MALLOC(size, float);
    for (i = 0; i < size; i++)
        data[i] = (float) png_data[i];

    return data;
}

/**
 * @brief convert to raw png data from float
 *
 * @param data array to convert
 * @param size array size
 * @return converted array
 *
 * @todo real sampling
 */
static png_byte *_io_png_from_flt(const float *data, size_t size)
{
    size_t i;
    png_byte *png_data;
    float tmp;

    if (NULL == data || 0 == size)
        _IO_PNG_ABORT("bad parameters");

    png_data = _IO_PNG_SAFE_MALLOC(size, png_byte);
    for (i = 0; i < size; i++) {
        tmp = floor(data[i] + .5);
        png_data[i] = (png_byte) (tmp < 0. ? 0. : (tmp > 255. ? 255. : tmp));
    }

    return png_data;
}

/**
 * @brief convert raw png gray to rgb
 *
 * @param png_data array to convert
 * @param size array size
 * @return converted array (via realloc())
 *
 * @todo restrict keyword
 */
static png_byte *_io_png_gray_to_rgb(png_byte * png_data, size_t size)
{
    size_t i;
    png_byte *r, *g, *b;

    if (NULL == png_data || 0 == size)
        _IO_PNG_ABORT("bad parameters");

    png_data = _IO_PNG_SAFE_REALLOC(png_data, 3 * size, png_byte);
    r = png_data;
    g = png_data + size;
    b = png_data + 2 * size;
    for (i = 0; i < size; i++) {
        g[i] = r[i];
        b[i] = r[i];
    }

    return png_data;
}

/**
 * @brief convert raw png rgb to gray
 *
 * Y = (6968 * R + 23434 * G + 2366 * B) / 32768
 * integer approximation of
 * Y = Cr* R + Cg * G + Cb * B
 * with
 * Cr = 0.212639005871510
 * Cg = 0.715168678767756
 * Cb = 0.072192315360734
 * derived from ITU BT.709-5 (Rec 709) sRGB and D65 definitions
 * http://www.itu.int/rec/R-REC-BT.709/en
 *
 * @param png_data array to convert
 * @param size array size
 * @return converted array (via realloc())
 *
 * @todo restrict keyword
 */
static png_byte *_io_png_rgb_to_gray(png_byte * png_data, size_t size)
{
    size_t i;
    png_byte *r, *g, *b;

    if (NULL == png_data || 0 == size || 0 != (size % 3))
        _IO_PNG_ABORT("bad parameters");

    size /= 3;
    r = png_data;
    g = png_data + size;
    b = png_data + 2 * size;

    /*
     * if int type is less than 24 bits, we use long ints,
     * guaranteed to be >= 32 bit
     */
#if (UINT_MAX >> 24 == 0)
#define CR 6968UL
#define CG 23434UL
#define CB 2366UL
#else
#define CR 6968U
#define CG 23434U
#define CB 2366U
#endif
    for (i = 0; i < size; i++)
        /* (1 << 14) is added for rounding instead of truncation */
        png_data[i] = (unsigned char) ((CR * r[i] + CG * g[i]
                                        + CB * b[i] + (1 << 14)) >> 15);
#undef CR
#undef CG
#undef CB

    png_data = _IO_PNG_SAFE_REALLOC(png_data, size, png_byte);

    return png_data;
}

/*
 * READ
 */

#define PNG_SIG_LEN 4

/**
 * @brief internal function used to read a PNG file into an array
 *
 * @todo don't loose 16bit info
 *
 * @param fname PNG file name, "-" means stdin
 * @param nxp, nyp, ncp pointers to variables to be filled
 *        with the number of columns, lines and channels of the image
 * @return pointer to an allocated array of pixels, abort() on error
 */
static png_byte *_io_png_read_raw(const char *fname,
                                  size_t * nxp, size_t * nyp, size_t * ncp)
{
    png_byte png_sig[PNG_SIG_LEN];
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytepp row_pointers;
    png_byte *png_data;
    int png_transform;
    /* volatile: because of setjmp/longjmp */
    FILE *volatile fp = NULL;
    size_t size;
    size_t i;
    /* local error structure */
    _io_png_err_t err;

    /* parameters check */
    if (NULL == fname || NULL == nxp || NULL == nyp || NULL == ncp)
        _IO_PNG_ABORT("bad parameters");

    /* open the PNG input file */
    if (0 == strcmp(fname, "-"))
        fp = stdin;
    else if (NULL == (fp = fopen(fname, "rb")))
        _IO_PNG_ABORT("failed to open file");

    /* read in some of the signature bytes and check this signature */
    if ((PNG_SIG_LEN != fread(png_sig, 1, PNG_SIG_LEN, fp))
        || 0 != png_sig_cmp(png_sig, (png_size_t) 0, PNG_SIG_LEN))
        _IO_PNG_ABORT("the file is not a PNG image");

    /*
     * create and initialize the png_struct and png_info structures
     * with local error handling
     */
    if (NULL == (png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                  &err, &_io_png_err_hdl,
                                                  NULL)))
        _IO_PNG_ABORT("libpng initialization error");
    if (NULL == (info_ptr = png_create_info_struct(png_ptr)))
        _IO_PNG_ABORT("libpng initialization error");

    /* if we get here, we had a problem reading from the file */
    if (setjmp(err.jmpbuf))
        _IO_PNG_ABORT("libpng reading error");

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
    png_transform = (PNG_TRANSFORM_IDENTITY
                     | PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING);

    /*
     * read in the entire image at once
     * then collect the image informations
     */
    png_read_png(png_ptr, info_ptr, png_transform, NULL);
    *nxp = (size_t) png_get_image_width(png_ptr, info_ptr);
    *nyp = (size_t) png_get_image_height(png_ptr, info_ptr);
    *ncp = (size_t) png_get_channels(png_ptr, info_ptr);
    row_pointers = png_get_rows(png_ptr, info_ptr);
    size = *nxp * *nyp * *ncp;

    /* dump the rows in a continuous array */
    /* todo: first check if the data is continuous via
     * row_pointers */
    /* todo: check type width */
    png_data = _IO_PNG_SAFE_MALLOC(size, png_byte);
    for (i = 0; i < *nyp; i++)
        memcpy((void *) (png_data + i * *nxp * *ncp),
               (void *) row_pointers[i], *nxp * *ncp * sizeof(png_byte));

    /* deinterlace RGBA RGBA RGBA to RRR GGG BBB AAA */
    png_data = _io_png_deinterlace(png_data, *nxp * *nyp, *ncp);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    if (stdin != fp)
        (void) fclose(fp);

    return png_data;
}

/**
 * @brief read a PNG file into an unsigned char array
 *
 * The array contains the de-interlaced channels.
 * 1, 2 and 4bit images are converted to 8bit.
 * 16bit images are previously downscaled to 8bit.
 *
 * @todo don't downscale 16bit images.
 *
 * @param fname PNG file name
 * @param nxp, nyp, ncp pointers to variables to be filled with the number of
 *        columns, lines and channels of the image
 * @return pointer to an allocated unsigned char array of pixels,
 *         or NULL if an error happens
 */
unsigned char *io_png_read_uchar(const char *fname,
                                 size_t * nxp, size_t * nyp, size_t * ncp)
{
    png_byte *png_data;
    unsigned char *data;
    size_t nx, ny, nc;

    /* read the raw image */
    png_data = _io_png_read_raw(fname, &nx, &ny, &nc);
    /* convert to uchar */
    data = _io_png_to_uchar(png_data, nx * ny * nc);
    free(png_data);

    *nxp = nx;
    *nyp = ny;
    *ncp = nc;
    return data;
}

/**
 * @brief read a PNG file into an unsigned char array, converted to RGB
 *
 * See io_png_read_uchar() for details.
 */
unsigned char *io_png_read_uchar_rgb(const char *fname, size_t * nxp,
                                     size_t * nyp)
{
    png_byte *png_data;
    unsigned char *data;
    size_t nx, ny, nc;

    /* read the raw image */
    png_data = _io_png_read_raw(fname, &nx, &ny, &nc);
    /* strip alpha channel ... */
    if (4 == nc || 2 == nc) {
        png_data =
            _IO_PNG_SAFE_REALLOC(png_data, nx * ny * (nc - 1), png_byte);
        nc = nc - 1;
    }
    /* gray->rgb */
    if (1 == nc) {
        png_data = _io_png_gray_to_rgb(png_data, nx * ny);
        nc = 3;
    }
    /* convert to uchar */
    data = _io_png_to_uchar(png_data, nx * ny * nc);
    free(png_data);

    *nxp = nx;
    *nyp = ny;
    return data;
}

/**
 * @brief read a PNG file into an unsigned char array, converted to gray
 *
 * See io_png_read_uchar() for details.
 */
unsigned char *io_png_read_uchar_gray(const char *fname,
                                      size_t * nxp, size_t * nyp)
{
    png_byte *png_data;
    unsigned char *data;
    size_t nx, ny, nc;

    /* read the raw image */
    png_data = _io_png_read_raw(fname, &nx, &ny, &nc);
    /* strip alpha channel ... */
    if (4 == nc || 2 == nc) {
        png_data =
            _IO_PNG_SAFE_REALLOC(png_data, nx * ny * (nc - 1), png_byte);
        nc = (nc - 1);
    }
    /* rgb->gray */
    if (3 == nc) {
        png_data = _io_png_rgb_to_gray(png_data, nx * ny * nc);
        nc = 1;
    }
    /* convert to uchar */
    data = _io_png_to_uchar(png_data, nx * ny * nc);
    free(png_data);

    *nxp = nx;
    *nyp = ny;
    return data;
}

/**
 * @brief read a PNG file into a float array
 *
 * The array contains the deinterlaced channels.
 * 1, 2, 4 and 8bit images are converted to float values
 * between 0. and 1., 3., 15. or 255.
 * 16bit images are also downscaled to 8bit before conversion.
 *
 * @param fname PNG file name
 * @param nxp, nyp, ncp pointers to variables to be filled with the number of
 *        columns, lines and channels of the image
 * @return pointer to an allocated unsigned char array of pixels,
 *         or NULL if an error happens
 */
float *io_png_read_flt(const char *fname,
                       size_t * nxp, size_t * nyp, size_t * ncp)
{
    png_byte *png_data;
    float *data;
    size_t nx, ny, nc;

    /* read the raw image */
    png_data = _io_png_read_raw(fname, &nx, &ny, &nc);
    /* convert to flt */
    data = _io_png_to_flt(png_data, nx * ny * nc);
    free(png_data);

    *nxp = nx;
    *nyp = ny;
    *ncp = nc;
    return data;
}

/**
 * @brief read a PNG file into a float array, converted to RGB
 *
 * See io_png_read_flt() for details.
 */
float *io_png_read_flt_rgb(const char *fname, size_t * nxp, size_t * nyp)
{
    png_byte *png_data;
    float *data;
    size_t nx, ny, nc;

    /* read the raw image */
    png_data = _io_png_read_raw(fname, &nx, &ny, &nc);
    /* strip alpha channel ... */
    if (4 == nc || 2 == nc) {
        png_data =
            _IO_PNG_SAFE_REALLOC(png_data, nx * ny * (nc - 1), png_byte);
        nc = (nc - 1);
    }
    /* gray->rgb */
    if (1 == nc) {
        png_data = _io_png_gray_to_rgb(png_data, nx * ny);
        nc = 3;
    }
    /* convert to flt */
    data = _io_png_to_flt(png_data, nx * ny * nc);
    free(png_data);

    *nxp = nx;
    *nyp = ny;
    return data;
}

/**
 * @brief read a PNG file into a float array, converted to gray
 *
 * See io_png_read_flt() for details.
 */
float *io_png_read_flt_gray(const char *fname, size_t * nxp, size_t * nyp)
{
    png_byte *png_data;
    float *data;
    size_t nx, ny, nc;

    /* read the raw image */
    png_data = _io_png_read_raw(fname, &nx, &ny, &nc);
    /* strip alpha channel ... */
    if (4 == nc || 2 == nc) {
        png_data =
            _IO_PNG_SAFE_REALLOC(png_data, nx * ny * (nc - 1), png_byte);
        nc = nc - 1;
    }
    /* rgb->gray */
    if (3 == nc) {
        png_data = _io_png_rgb_to_gray(png_data, nx * ny * nc);
        nc = 1;
    }
    /* convert to flt */
    data = _io_png_to_flt(png_data, nx * ny * nc);
    free(png_data);

    *nxp = nx;
    *nyp = ny;
    return data;
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
 * @todo in-place deinterlace
 *
 * @param fname PNG file name, "-" means stdout
 * @param png_data non interlaced (RRRGGGBBBAAA) byte image array
 * @param nx, ny, nc number of columns, lines and channels
 * @return void, abort() on error
 */
static void io_png_write_raw(const char *fname, png_byte * png_data,
                             size_t nx, size_t ny, size_t nc)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;
    png_byte bit_depth;
    /* volatile: because of setjmp/longjmp */
    FILE *volatile fp;
    int color_type, interlace, compression, filter;
    size_t i;
    /* error structure */
    _io_png_err_t err;

    /* interlace RRR GGG BBB AAA to RGBA RGBA RGBA */
    png_data = _io_png_interlace(png_data, nx * ny, nc);

    /* open the PNG output file */
    if (0 == strcmp(fname, "-"))
        fp = stdout;
    else if (NULL == (fp = fopen(fname, "wb")))
        _IO_PNG_ABORT("failed to open file");

    /* allocate the row pointers */
    row_pointers = _IO_PNG_SAFE_MALLOC(ny, png_bytep);

    /*
     * create and initialize the png_struct and png_info structures
     * with local error handling
     */
    if (NULL == (png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                   &err, &_io_png_err_hdl,
                                                   NULL)))
        _IO_PNG_ABORT("libpng initialization error");
    if (NULL == (info_ptr = png_create_info_struct(png_ptr)))
        _IO_PNG_ABORT("libpng initialization error");

    /* if we get here, we had a problem writing to the file */
    if (0 != setjmp(err.jmpbuf))
        _IO_PNG_ABORT("libpng writing error");

    /* set up the input control using standard C streams */
    png_init_io(png_ptr, fp);

    /* set image informations */
    bit_depth = 8;
    switch (nc) {
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
        _IO_PNG_ABORT("bad parameters");
    }
    interlace = PNG_INTERLACE_ADAM7;
    compression = PNG_COMPRESSION_TYPE_BASE;
    filter = PNG_FILTER_TYPE_BASE;

    /* set image header */
    png_set_IHDR(png_ptr, info_ptr, (png_uint_32) nx, (png_uint_32) ny,
                 bit_depth, color_type, interlace, compression, filter);
    /* TODO : significant bit (sBIT), gamma (gAMA) chunks */
    png_write_info(png_ptr, info_ptr);

    /* set row pointers */
    for (i = 0; i < ny; i++)
        row_pointers[i] = (png_bytep) png_data + (size_t) (nc * nx * i);

    /* write out the entire image and end it */
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);

    /* clean up and free any memory allocated, close the file */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    free(row_pointers);
    if (stdout != fp)
        (void) fclose(fp);

    return;
}

/**
 * @brief write an unsigned char array into a PNG file
 *
 * The unsigned char values are casted into png_byte.
 *
 * @param fname PNG file name
 * @param data deinterlaced array (RRRR..GGGG..BBBB..AAAA..) to write
 * @param nx, ny, nc number of columns, lines and channels of the image
 * @return void, abort() on error
 *
 * @todo add type width checks
 */
void io_png_write_uchar(const char *fname, const unsigned char *data,
                        size_t nx, size_t ny, size_t nc)
{
    png_byte *png_data;

    /* parameters check */
    if (0 >= nx || 0 >= ny || 0 >= nc || NULL == fname || NULL == data)
        _IO_PNG_ABORT("bad parameters");

    /* convert from unsigned char to png_byte */
    png_data = _io_png_from_uchar(data, nx * ny * nc);

    io_png_write_raw(fname, png_data, nx, ny, nc);
    free(png_data);
    return;
}

/**
 * @brief write a float array into a PNG file
 *
 * The float values are rounded to 8bit integers, and bounded to [0, 255].
 *
 * @todo handle 16bit images
 *
 * @param fname PNG file name
 * @param data array to write
 * @param nx, ny, nc number of columns, lines and channels of the image
 * @return void, abort() on error
 */
void io_png_write_flt(const char *fname, const float *data,
                      size_t nx, size_t ny, size_t nc)
{
    png_byte *png_data;

    /* parameters check */
    if (0 >= nx || 0 >= ny || 0 >= nc || NULL == fname || NULL == data)
        _IO_PNG_ABORT("bad parameters");

    /* convert from unsigned char to png_byte */
    png_data = _io_png_from_flt(data, nx * ny * nc);

    io_png_write_raw(fname, png_data, nx, ny, nc);
    free(png_data);
    return;
}
