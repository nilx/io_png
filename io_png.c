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
#define _IO_PNG_ABORT(MSG) do {                                 \
    fprintf(stderr, "%s:%04u : %s\n", __FILE__, __LINE__, MSG); \
    fflush(stderr);                                             \
    abort();                                                    \
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
 * @brief convert png_byte array to float
 *
 * @param png_data array to convert
 * @param size array size
 * @return converted array
 *
 * @todo use lookup table instead of division?
 */
static float *_io_png_byte2flt(const png_byte * png_data, size_t size)
{
    size_t i;
    float *data;

    if (NULL == png_data || 0 == size)
        _IO_PNG_ABORT("bad parameters");

    data = _IO_PNG_SAFE_MALLOC(size, float);
    for (i = 0; i < size; i++)
        /* png_byte is 8bit unsigned, [0..255] */
        data[i] = (float) (png_data[i]) / 255.;

    return data;
}

/**
 * @brief convert float array to png_byte
 *
 * @param data array to convert
 * @param size array size
 * @return converted array
 *
 * @todo bit twiddling instead of (?:) branching?
 */
static png_byte *_io_png_flt2byte(const float *data, size_t size)
{
    size_t i;
    png_byte *png_data;
    float tmp;

    if (NULL == data || 0 == size)
        _IO_PNG_ABORT("bad parameters");

    png_data = _IO_PNG_SAFE_MALLOC(size, png_byte);
    for (i = 0; i < size; i++) {
        /* png_byte is 8bit data unsigned, [0..255] */
        tmp = data[i] * 255. + .5;
        png_data[i] = (png_byte) (tmp < 0. ? 0. : (tmp > 255. ? 255. : tmp));
    }

    return png_data;
}

/**
 * @brief convert unsigned char array to float
 *
 * @param uchar_data array to convert
 * @param size array size
 * @return converted array
 *
 * @todo use lookup table instead of division?
 */
static float *_io_png_uchar2flt(const unsigned char *uchar_data, size_t size)
{
    size_t i;
    float *data;

    if (NULL == uchar_data || 0 == size)
        _IO_PNG_ABORT("bad parameters");

    data = _IO_PNG_SAFE_MALLOC(size, float);
    for (i = 0; i < size; i++)
        data[i] = (float) (uchar_data[i]) / (float) (UCHAR_MAX);

    return data;
}

/**
 * @brief convert float array to unsigned char
 *
 * @param data array to convert
 * @param size array size
 * @return converted array
 *
 * @todo bit twiddling instead of (?:) branching?
 */
static unsigned char *_io_png_flt2uchar(const float *data, size_t size)
{
    size_t i;
    unsigned char *uchar_data;
    float tmp;

    if (NULL == data || 0 == size)
        _IO_PNG_ABORT("bad parameters");

    uchar_data = _IO_PNG_SAFE_MALLOC(size, unsigned char);
    for (i = 0; i < size; i++) {
        tmp = data[i] * UCHAR_MAX + .5;
        uchar_data[i] =
            (unsigned char) (tmp < 0. ? 0. : (tmp > 255. ? 255. : tmp));
    }

    return uchar_data;
}

/**
 * @brief convert raw png gray to rgb
 *
 * @param data array to convert
 * @param size array size
 * @return converted array (via realloc())
 */
static float *_io_png_gray2rgb(float *data, size_t size)
{
    if (NULL == data || 0 == size)
        _IO_PNG_ABORT("bad parameters");

    data = _IO_PNG_SAFE_REALLOC(data, 3 * size, float);
    memcpy(data + size, data, size * sizeof(float));
    memcpy(data + 2 * size, data, size * sizeof(float));

    return data;
}

/**
 * @brief convert raw png rgb to gray
 *
 * Y = Cr* R + Cg * G + Cb * B
 * with
 * Cr = 0.212639005871510
 * Cg = 0.715168678767756
 * Cb = 0.072192315360734
 * derived from ITU BT.709-5 (Rec 709) sRGB and D65 definitions
 * http://www.itu.int/rec/R-REC-BT.709/en
 *
 * @param data array to convert
 * @param size array size
 * @return converted array (via realloc())
 *
 * @todo restrict keyword?
 */
static float *_io_png_rgb2gray(float *data, size_t size)
{
    size_t i;
    float *r, *g, *b;

    if (NULL == data || 0 == size || 0 != (size % 3))
        _IO_PNG_ABORT("bad parameters");

    size /= 3;
    r = data;
    g = data + size;
    b = data + 2 * size;

    for (i = 0; i < size; i++)
        data[i] = 0.212639005871510 * r[i]
            + 0.715168678767756 * g[i]
            + 0.072192315360734 * b[i];

    data = _IO_PNG_SAFE_REALLOC(data, size, float);

    return data;
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
 * @return pointer to an array of float pixels, abort() on error
 */
static float *_io_png_read(const char *fname,
                           size_t * nxp, size_t * nyp, size_t * ncp)
{
    png_byte png_sig[PNG_SIG_LEN];
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytepp row_pointers;
    png_byte *png_data;
    float *data;
    int png_transform;
    /* volatile: because of setjmp/longjmp */
    FILE *volatile fp = NULL;
    size_t nx, ny, nc;
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
    nx = (size_t) png_get_image_width(png_ptr, info_ptr);
    ny = (size_t) png_get_image_height(png_ptr, info_ptr);
    nc = (size_t) png_get_channels(png_ptr, info_ptr);
    size = nx * ny * nc;
    row_pointers = png_get_rows(png_ptr, info_ptr);

    /* dump the rows in a continuous array */
    /* todo: first check if the data is continuous via
     * row_pointers */
    /* todo: check type width */
    png_data = _IO_PNG_SAFE_MALLOC(size, png_byte);
    for (i = 0; i < ny; i++)
        memcpy((void *) (png_data + i * nx * nc),
               (void *) row_pointers[i], nx * nc * sizeof(png_byte));

    /* deinterlace RGBA RGBA RGBA to RRR GGG BBB AAA */
    png_data = _io_png_deinterlace(png_data, nx * ny, nc);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    if (stdin != fp)
        (void) fclose(fp);

    /* convert to float */
    /* todo: at the row step */
    data = _io_png_byte2flt(png_data, nx * ny * nc);
    free(png_data);

    *nxp = nx;
    *nyp = ny;
    *ncp = nc;
    return data;
}

/**
 * @brief read a PNG file into a float array
 *
 * The array contains the deinterlaced channels, with values in [0,1].
 *
 * @param fname PNG file name
 * @param nxp, nyp, ncp pointers to variables to be filled with the number of
 *        columns, lines and channels of the image
 * @return pointer to an array of float pixels, abort() on error
 */
float *io_png_read_flt(const char *fname,
                       size_t * nxp, size_t * nyp, size_t * ncp)
{
    float *data;
    size_t nx, ny, nc;

    /* read the image */
    data = _io_png_read(fname, &nx, &ny, &nc);

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
    float *data;
    size_t nx, ny, nc;

    /* read the image */
    data = _io_png_read(fname, &nx, &ny, &nc);
    /* strip alpha channel ... */
    if (4 == nc || 2 == nc) {
        data = _IO_PNG_SAFE_REALLOC(data, nx * ny * (nc - 1), float);
        nc = (nc - 1);
    }
    /* gray->rgb */
    if (1 == nc) {
        data = _io_png_gray2rgb(data, nx * ny);
        nc = 3;
    }

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
    float *data;
    size_t nx, ny, nc;

    /* read the image */
    data = _io_png_read(fname, &nx, &ny, &nc);
    /* strip alpha channel ... */
    if (4 == nc || 2 == nc) {
        data = _IO_PNG_SAFE_REALLOC(data, nx * ny * (nc - 1), float);
        nc = nc - 1;
    }
    /* rgb->gray */
    if (3 == nc) {
        data = _io_png_rgb2gray(data, nx * ny * nc);
        nc = 1;
    }

    *nxp = nx;
    *nyp = ny;
    return data;
}

/**
 * @brief read a PNG file into an unsigned char array
 *
 * The array contains the de-interlaced channels, with values in [0,UCHAR_MAX].
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
    float *data;
    unsigned char *uchar_data;
    size_t nx, ny, nc;

    /* read the image */
    data = _io_png_read(fname, &nx, &ny, &nc);
    /* convert to uchar */
    uchar_data = _io_png_flt2uchar(data, nx * ny * nc);
    free(data);

    *nxp = nx;
    *nyp = ny;
    *ncp = nc;
    return uchar_data;
}

/**
 * @brief read a PNG file into an unsigned char array, converted to RGB
 *
 * See io_png_read_uchar() for details.
 */
unsigned char *io_png_read_uchar_rgb(const char *fname, size_t * nxp,
                                     size_t * nyp)
{
    float *data;
    unsigned char *uchar_data;
    size_t nx, ny, nc;

    /* read the image */
    data = _io_png_read(fname, &nx, &ny, &nc);
    /* strip alpha channel ... */
    if (4 == nc || 2 == nc) {
        data = _IO_PNG_SAFE_REALLOC(data, nx * ny * (nc - 1), float);
        nc = nc - 1;
    }
    /* gray->rgb */
    if (1 == nc) {
        data = _io_png_gray2rgb(data, nx * ny);
        nc = 3;
    }
    /* convert to uchar */
    uchar_data = _io_png_flt2uchar(data, nx * ny * nc);
    free(data);

    *nxp = nx;
    *nyp = ny;
    return uchar_data;
}

/**
 * @brief read a PNG file into an unsigned char array, converted to gray
 *
 * See io_png_read_uchar() for details.
 */
unsigned char *io_png_read_uchar_gray(const char *fname,
                                      size_t * nxp, size_t * nyp)
{
    float *data;
    unsigned char *uchar_data;
    size_t nx, ny, nc;

    /* read the image */
    data = _io_png_read(fname, &nx, &ny, &nc);
    /* strip alpha channel ... */
    if (4 == nc || 2 == nc) {
        data = _IO_PNG_SAFE_REALLOC(data, nx * ny * (nc - 1), float);
        nc = (nc - 1);
    }
    /* rgb->gray */
    if (3 == nc) {
        data = _io_png_rgb2gray(data, nx * ny * nc);
        nc = 1;
    }
    /* convert to uchar */
    uchar_data = _io_png_flt2uchar(data, nx * ny * nc);
    free(data);

    *nxp = nx;
    *nyp = ny;
    return uchar_data;
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
 * @param data non interlaced (RRRGGGBBBAAA) float image array
 * @param nx, ny, nc number of columns, lines and channels
 * @return void, abort() on error
 */
static void _io_png_write(const char *fname, const float *data,
                          size_t nx, size_t ny, size_t nc)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;
    png_byte *png_data;
    png_byte bit_depth;
    /* volatile: because of setjmp/longjmp */
    FILE *volatile fp;
    int color_type, interlace, compression, filter;
    size_t i;
    /* error structure */
    _io_png_err_t err;

    /* convert to png_byte */
    png_data = _io_png_flt2byte(data, nx * ny * nc);

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
 * @brief write a float array into a PNG file
 *
 * The array values are taken from the [0,1] interval and converted to
 * 8bit data.
 *
 * @todo save as 16bit images
 *
 * @param fname PNG file name
 * @param data deinterlaced (RRR.GGG.BBB.AAA.) array to write
 * @param nx, ny, nc number of columns, lines and channels of the image
 * @return void, abort() on error
 */
void io_png_write_flt(const char *fname, const float *data,
                      size_t nx, size_t ny, size_t nc)
{
    /* parameters check */
    if (0 >= nx || 0 >= ny || 0 >= nc || NULL == fname || NULL == data)
        _IO_PNG_ABORT("bad parameters");

    _io_png_write(fname, data, nx, ny, nc);
    return;
}

/**
 * @brief write an unsigned char array into a 8bit PNG file
 *
 * The array values are taken from the [0,UCHAR_MAX] interval and
 * converted to float in the [0,1] interval before being saved as 8bit
 * fixed-point data.
 *
 * @param fname PNG file name
 * @param data deinterlaced (RRR.GGG.BBB.AAA.) array to write
 * @param nx, ny, nc number of columns, lines and channels of the image
 * @return void, abort() on error
 *
 * @todo add type width checks
 */
void io_png_write_uchar(const char *fname, const unsigned char *data,
                        size_t nx, size_t ny, size_t nc)
{
    float *flt_data;

    /* parameters check */
    if (0 >= nx || 0 >= ny || 0 >= nc || NULL == fname || NULL == data)
        _IO_PNG_ABORT("bad parameters");

    /* convert from unsigned char to float */
    flt_data = _io_png_uchar2flt(data, nx * ny * nc);

    _io_png_write(fname, flt_data, nx, ny, nc);
    free(flt_data);
    return;
}
