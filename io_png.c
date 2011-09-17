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
 * @todo no more pointer loops
 * @todo npre/post-processing timing
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

#define PNG_SIG_LEN 4

/* internal only data type identifiers */
#define IO_PNG_UCHAR  0x0001    /* unsigned char */
#define IO_PNG_FLT 0x0002       /* float */

/*
 * INFO
 */

/* string tag inserted into the binary */
static char _io_png_tag[] = "using io_png " IO_PNG_VERSION;
/**
 * @brief helps tracking versions, via the string tag inserted into
 * the library
 *
 * This function is not expected to be used in real-world programs.
 *
 * @return a pointer to a version info string
 */
char *io_png_info(void)
{
    return _io_png_tag;
}

/*
 * UTILS
 */

/**
 * @brief abort() wrapper macro with an error message
 *
 * @todo file/line info
 */
#define _IO_PNG_ABORT(MSG) do {                                         \
        fputs(MSG, stderr);                                             \
    fflush(stderr);                                                     \
    abort();                                                            \
    } while (0);

/**
 * @brief safe malloc wrapper
 */
static void *_io_png_safe_malloc(size_t size)
{
    void *memptr;

    if (NULL == (memptr = malloc(size)))
        _IO_PNG_ABORT("not enough memory");
    return memptr;
}

/**
 * @brief safe malloc wrapper macro with safe casting
 */
#define _IO_PNG_SAFE_MALLOC(NB, TYPE)                                   \
    ((TYPE *) _io_png_safe_malloc((size_t) (NB) * sizeof(TYPE)))

/**
 * @brief safe realloc wrapper
 */
static void *_io_png_safe_realloc(void *memptr, size_t size)
{
    void *newptr;

    if (NULL == (newptr = realloc(memptr, size)))
        _IO_PNG_ABORT("not enough memory");
    return newptr;
}

/**
 * @brief safe realloc wrapper macro with safe casting
 */
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

/*
 * READ
 */

/**
 * @brief internal function used to read a PNG file into an array
 *
 * @todo don't loose 16bit info
 *
 * @param fname PNG file name, "-" means stdin
 * @param nxp, nyp, ncp pointers to variables to be filled
 *        with the number of columns, lines and channels of the image
 * @param png_transform a PNG_TRANSFORM flag to be added to the
 *        default libpng read transforms
 * @param dtype identifier for the data type to be used for output
 * @return pointer to an allocated array of pixels, abort() on error
 */
static void *io_png_read_raw(const char *fname,
                             size_t * nxp, size_t * nyp, size_t * ncp,
                             int png_transform, int dtype)
{
    png_byte png_sig[PNG_SIG_LEN];
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytepp row_pointers;
    /* volatile: because of setjmp/longjmp */
    FILE *volatile fp = NULL;
    size_t size;
    /* local error structure */
    _io_png_err_t err;

    /* parameters check */
    if (NULL == fname || NULL == nxp || NULL == nyp || NULL == ncp)
        _IO_PNG_ABORT("bad parameters");
    if (IO_PNG_UCHAR != dtype && IO_PNG_FLT != dtype)
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
    png_transform |= (PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING);

    /*
     * read in the entire image at once
     * then collect the image informations
     */
    png_read_png(png_ptr, info_ptr, png_transform, NULL);
    *nxp = (size_t) png_get_image_width(png_ptr, info_ptr);
    *nyp = (size_t) png_get_image_height(png_ptr, info_ptr);
    *ncp = (size_t) png_get_channels(png_ptr, info_ptr);
    row_pointers = png_get_rows(png_ptr, info_ptr);

    /*
     * allocate the output data RGB array
     * de-interlace and convert png RGB RGB RGB 8bit to RRR GGG BBB
     * the image is de-interlaced layer after layer
     * this loop order is not the fastest but it is generic and also
     * works for one single channel
     */
    size = *nxp * *nyp * *ncp;
    switch (dtype) {
    case IO_PNG_UCHAR:
        {
            unsigned char *data = NULL, *data_ptr = NULL;
            png_bytep row_ptr;
            size_t i, j, k;
            data = _IO_PNG_SAFE_MALLOC(size, unsigned char);
            for (k = 0; k < *ncp; k++) {
                /* channel loop */
                data_ptr = data + (size_t) (*nxp * *nyp * k);
                for (j = 0; j < *nyp; j++) {
                    /* row loop */
                    row_ptr = row_pointers[j] + k;
                    for (i = 0; i < *nxp; i++) {
                        /* pixel loop */
                        *data_ptr++ = (unsigned char) *row_ptr;
                        row_ptr += *ncp;
                    }
                }
            }
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            if (stdin != fp)
                (void) fclose(fp);
            return (void *) data;
        }
        break;                  /* useless */
    case IO_PNG_FLT:
        {
            float *data = NULL, *data_ptr = NULL;
            png_bytep row_ptr;
            size_t i, j, k;
            data = _IO_PNG_SAFE_MALLOC(size, float);
            for (k = 0; k < *ncp; k++) {
                /* channel loop */
                data_ptr = data + (size_t) (*nxp * *nyp * k);
                for (j = 0; j < *nyp; j++) {
                    /* row loop */
                    row_ptr = row_pointers[j] + k;
                    for (i = 0; i < *nxp; i++) {
                        /* pixel loop */
                        *data_ptr++ = (float) *row_ptr;
                        row_ptr += *ncp;
                    }
                }
            }
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            if (stdin != fp)
                (void) fclose(fp);
            return (void *) data;
        }
        break;                  /* useless */
    default:
        _IO_PNG_ABORT("bad parameter");
    }
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
    /* read the image as unsigned char */
    return (unsigned char *) io_png_read_raw(fname, nxp, nyp, ncp,
                                             PNG_TRANSFORM_IDENTITY,
                                             IO_PNG_UCHAR);
}

/**
 * @brief read a PNG file into an unsigned char array, converted to RGB
 *
 * See io_png_read_uchar() for details.
 */
unsigned char *io_png_read_uchar_rgb(const char *fname, size_t * nxp,
                                     size_t * nyp)
{
    size_t nc;
    unsigned char *img;

    /* read the image */
    img = (unsigned char *) io_png_read_raw(fname, nxp, nyp, &nc,
                                            PNG_TRANSFORM_STRIP_ALPHA,
                                            IO_PNG_UCHAR);
    if (NULL == img)
        _IO_PNG_ABORT("bad parameters");
    if (3 == nc)
        /* already RGB */
        return img;
    else {
        /* convert to RGB */
        size_t i, size;
        unsigned char *img_r, *img_g, *img_b;

        /* resize the image */
        size = *nxp * *nyp;
        img = _IO_PNG_SAFE_REALLOC(img, 3 * size, unsigned char);
        img_r = img;
        img_g = img + size;
        img_b = img + 2 * size;

        /* gray->RGB conversion */
        for (i = 0; i < size; i++) {
            img_g[i] = img_r[i];
            img_b[i] = img_r[i];
        }
        return img;
    }
}

/**
 * @brief read a PNG file into an unsigned char array, converted to gray
 *
 * See io_png_read_uchar() for details.
 */
unsigned char *io_png_read_uchar_gray(const char *fname,
                                      size_t * nxp, size_t * nyp)
{
    size_t nc;
    unsigned char *img;

    /* read the image */
    img = (unsigned char *) io_png_read_raw(fname, nxp, nyp, &nc,
                                            PNG_TRANSFORM_STRIP_ALPHA,
                                            IO_PNG_UCHAR);
    if (NULL == img)
        _IO_PNG_ABORT("bad parameters");
    if (1 == nc)
        /* already gray */
        return img;
    else {
        /* convert to gray */
        size_t i, size;
        unsigned char *img_r, *img_g, *img_b;

        /*
         * RGB->gray conversion
         * Y = (6968 * R + 23434 * G + 2366 * B) / 32768
         * integer approximation of
         * Y = Cr* R + Cg * G + Cb * B
         * with
         * Cr = 0.212639005871510
         * Cg = 0.715168678767756
         * Cb = 0.072192315360734
         * derived from ITU BT.709-5 (Rec 709) sRGB and D65 definitions
         * http://www.itu.int/rec/R-REC-BT.709/en
         */
        size = *nxp * *nyp;
        img_r = img;
        img_g = img + size;
        img_b = img + 2 * size;
        for (i = 0; i < size; i++)
            /*
             * if int type is less than 24 bits, we use long ints,
             * guaranteed to be >=32 bit
             */
#if (UINT_MAX>>24 == 0)
#define CR 6968ul
#define CG 23434ul
#define CB 2366ul
#else
#define CR 6968u
#define CG 23434u
#define CB 2366u
#endif
            /* (1 << 14) is added for rounding instead of truncation */
            img[i] = (unsigned char) ((CR * img_r[i] + CG * img_g[i]
                                       + CB * img_b[i] + (1 << 14)) >> 15);
#undef CR
#undef CG
#undef CB
        /* resize and return the image */
        img = _IO_PNG_SAFE_REALLOC(img, size, unsigned char);
        return img;
    }
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
    /* read the image as float */
    return (float *) io_png_read_raw(fname, nxp, nyp, ncp,
                                     PNG_TRANSFORM_IDENTITY, IO_PNG_FLT);
}

/**
 * @brief read a PNG file into a float array, converted to RGB
 *
 * See io_png_read_flt() for details.
 */
float *io_png_read_flt_rgb(const char *fname, size_t * nxp, size_t * nyp)
{
    size_t nc;
    float *img;

    /* read the image */
    img = (float *) io_png_read_raw(fname, nxp, nyp, &nc,
                                    PNG_TRANSFORM_STRIP_ALPHA, IO_PNG_FLT);
    if (NULL == img)
        _IO_PNG_ABORT("bad parameters");
    if (3 == nc)
        /* already RGB */
        return img;
    else {
        /* convert to RGB */
        size_t i, size;
        float *img_r, *img_g, *img_b;

        /* resize the image */
        size = *nxp * *nyp;
        img = _IO_PNG_SAFE_REALLOC(img, 3 * size, float);
        img_r = img;
        img_g = img + size;
        img_b = img + 2 * size;

        /* gray->RGB conversion */
        for (i = 0; i < size; i++) {
            img_g[i] = img_r[i];
            img_b[i] = img_r[i];
        }
        return img;
    }
}

/**
 * @brief read a PNG file into a float array, converted to gray
 *
 * See io_png_read_flt() for details.
 */
float *io_png_read_flt_gray(const char *fname, size_t * nxp, size_t * nyp)
{
    size_t nc;
    float *img;

    /* read the image */
    img = (float *) io_png_read_raw(fname, nxp, nyp, &nc,
                                    PNG_TRANSFORM_STRIP_ALPHA, IO_PNG_FLT);
    if (NULL == img)
        _IO_PNG_ABORT("bad parameters");
    if (1 == nc)
        /* already gray */
        return img;
    else {
        /* convert to gray */
        size_t i, size;
        float *img_r, *img_g, *img_b;

        /*
         * RGB->gray conversion
         * Y = Cr* R + Cg * G + Cb * B
         * with
         * Cr = 0.212639005871510
         * Cg = 0.715168678767756
         * Cb = 0.072192315360734
         * derived from ITU BT.709-5 (Rec 709) sRGB and D65 definitions
         * http://www.itu.int/rec/R-REC-BT.709/en
         */
        size = *nxp * *nyp;
        img_r = img;
        img_g = img + size;
        img_b = img + 2 * size;
        for (i = 0; i < size; i++)
            img[i] = (float) (0.212639005871510 * img_r[i]
                              + 0.715168678767756 * img_g[i]
                              + 0.072192315360734 * img_b[i]);
        /* resize and return the image */
        img = _IO_PNG_SAFE_REALLOC(img, size, float);
        return img;
    }
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
 * @param png_data interlaced (RGBARGBARGBA...) byte image array
 * @param nx, ny, nc number of columns, lines and channels
 * @return void, abort() on error
 */
static void io_png_write_raw(const char *fname, const png_byte * png_data,
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
    const unsigned char *data_ptr;
    png_byte *png_data, *png_data_ptr;
    size_t size;
    size_t i, j, k;

    /* parameters check */
    if (0 >= nx || 0 >= ny || 0 >= nc)
        _IO_PNG_ABORT("bad parameters");
    if (NULL == fname || NULL == data)
        _IO_PNG_ABORT("bad parameters");

    /* allocate png_data */
    size = nx * ny * nc;
    png_data = _IO_PNG_SAFE_MALLOC(size, png_byte);

    /*
     * interlace RRR GGG BBB AAA to RGBA RGBA RGBA
     * the image is interlaced layer after layer
     * this loop order is not the fastest but it is generic and also
     * works for one single channel
     */
    for (k = 0; k < nc; k++) {
        /* channel loop */
        data_ptr = data + (size_t) (nx * ny * k);
        png_data_ptr = png_data + (size_t) k;
        for (j = 0; j < ny; j++) {
            /* row loop */
            for (i = 0; i < nx; i++) {
                /* pixel loop */
                *png_data_ptr = (png_byte) * data_ptr++;
                png_data_ptr += nc;
            }
        }
    }

    io_png_write_raw(fname, png_data,
                     (png_uint_32) nx, (png_uint_32) ny, (png_byte) nc);
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
    const float *data_ptr;
    png_byte *png_data, *png_data_ptr;
    size_t size;
    size_t i, j, k;
    float tmp;

    /* parameters check */
    if (0 >= nx || 0 >= ny || 0 >= nc)
        _IO_PNG_ABORT("bad parameters");
    if (NULL == fname || NULL == data)
        _IO_PNG_ABORT("bad parameters");

    /* allocate png_data */
    size = nx * ny * nc;
    png_data = _IO_PNG_SAFE_MALLOC(size, png_byte);

    /*
     * interlace RRR GGG BBB AAA to RGBA RGBA RGBA
     * the image is interlaced layer after layer
     * this loop order is not the fastest but it is generic and also
     * works for one single channel
     */
    for (k = 0; k < nc; k++) {
        /* channel loop */
        data_ptr = data + (size_t) (nx * ny * k);
        png_data_ptr = png_data + (size_t) k;
        for (j = 0; j < ny; j++) {
            /* row loop */
            for (i = 0; i < nx; i++) {
                /* pixel loop */
                tmp = floor(*data_ptr++ + .5);
                *png_data_ptr = (png_byte) (tmp < 0. ? 0. :
                                            (tmp > 255. ? 255. : tmp));
                png_data_ptr += nc;
            }
        }
    }

    io_png_write_raw(fname, png_data,
                     (png_uint_32) nx, (png_uint_32) ny, (png_byte) nc);
    free(png_data);
    return;
}
