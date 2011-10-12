/*
 * This file shows how to use io_png.c. It is released in the public
 * domain and as such comes with no copyright requirement.
 *
 * compile with: cc example.c io_png.c -lpng
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* include the io_png prototypes */
#include "io_png.h"

int main(int argc, char **argv)
{
    /*
     * number of columns, lines and channels of the image
     * the image size is used as an array size and index, and int may
     * be a 32bit type on 64bit machines, so the correct type is size_t
     */
    size_t nx, ny, nc;
    /* float array to store an image */
    float *img;
    /* pointer to rgb channelse */
    float *img_r, *img_g, *img_b;
    /* array to store an image as integers */
    unsigned char *img_uchar;
    unsigned short *img_ushrt;
    /* temporary array */
    float *tmp;
    /* loop counter */
    size_t i;

    /* the file to read is given as the first command-line argument */
    if (2 > argc) {
        fprintf(stderr, "syntax: %s in.png\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* read the image info a float array */
    img = io_png_read_flt(argv[1], &nx, &ny, &nc);

    /* nx, ny and nc hold the image sizes */
    printf("image file: %s\n", argv[1]);
    printf("image size: %i x %i, %i channels\n",
           (int) nx, (int) ny, (int) nc);

    /*
     * img contains the float pixels with values in [0,1],
     * with deinterlaced channels: all the red values, then all the
     * green, and so on. But the nimber of channels, from 1 to 4, will
     * depend on file you read. So you may want to force the image to
     * be read as grayscale image via preprocessing with
     * io_png_read_flt_opt().
     */
    /* reread the image in float with grayscale conversion */
    free(img);
    img = io_png_read_flt_opt(argv[1], &nx, &ny, &nc, IO_PNG_OPT_GRAY);

    /* we now have a single channel image */
    assert(nc == 1);

    /* nc is optional, because it has to be 1, so it can be omitted */
    free(img);
    img = io_png_read_flt_opt(argv[1], &nx, &ny, NULL, IO_PNG_OPT_GRAY);

    /* let's read the pixel (27, 42) */
    if (nx >= 27 && ny >= 42) {
        printf("the pixel (27, 42) is %f\n", img[27 + nx * 42]);
    }

    /* or we can decide to read the image converted to RGB */
    free(img);
    img = io_png_read_flt_opt(argv[1], &nx, &ny, &nc, IO_PNG_OPT_RGB);

    /* we now have a 3 channel RGB image */
    assert(nc == 3);

    /*
     * now the image array img contains
     * - the red values from img[0] to img[nx*ny-1]
     * - the green values from img[nx*ny] to img[2*nx*ny-1]
     * - the blue values from img[2*nx*ny] to img[3*nx*ny-1]
     *
     * the pixel (i,j) rgb values are:
     * - red in img[i+nx*j]
     * - red in img[i+nx*j+nx*ny]
     * - red in img[i+nx*j+2*nx*ny]
     */

    /* let's access the 3 components of the pixel (27, 42) */
    if (nx >= 27 && ny >= 42) {
        printf("the RGB components of the pixel (27, 42) are "
               "R: %f G: %f B: %f\n",
               img[27 + nx * 42],
               img[27 + nx * 42 + 27 * 42], img[27 + nx * 42 + 27 * 42 * 2]);
    }

    /* we can also define convenience pointers on each channel */
    img_r = img;                /* red channel */
    img_g = img + nx * ny;      /* green channel */
    img_b = img + 2 * nx * ny;  /* blue channel */

    /*
     * if you manipulate a lot of pixel values, it is faster
     * because you do the "nx * ny" only once
     */
    if (nx >= 27 && ny >= 42) {
        printf("the RGB components of the pixel (27, 42) are "
               "R: %f G: %f B: %f\n",
               img_r[27 + nx * 42], img_g[27 + nx * 42], img_b[27 + nx * 42]);
    }

    /*
     * When you manipulate the whole image, you don't need
     * two loops for the x and y axis, you only nee one global
     * loop. This is faster, you don't need to compute "i + nx * j"
     * for every.
     */
    /* for example, invert an image */
    for (i = 0; i < nx * ny; i++) {
        img_r[i] = 1 - img_r[i];
        img_g[i] = 1 - img_g[i];
        img_b[i] = 1 - img_b[i];
    }

    /* now let's save this image */
    io_png_write_flt("float_rgb.png", img, nx, ny, 3);

    /* or we can save the channels separately */
    io_png_write_flt("float_r.png", img_r, nx, ny, 1);
    io_png_write_flt("float_g.png", img_g, nx, ny, 1);
    io_png_write_flt("float_b.png", img_b, nx, ny, 1);

    /* you can also manipulate each channel array */
    /* allocate a temporary memory space */
    tmp = (float *) malloc(nx * ny * sizeof(float));

    /* swap the red and  blue channels, and save */
    memcpy(tmp, img_r, nx * ny * sizeof(float));
    memcpy(img_r, img_b, nx * ny * sizeof(float));
    memcpy(img_b, tmp, nx * ny * sizeof(float));
    io_png_write_flt("float_bgr.png", img, nx, ny, 3);

    free(tmp);
    free(img);

    /*
     * instead of floats, you can also read and write the image as an
     * unsigned char or unsigned short array, with a similar syntax.
     * Then, after being read as float and maybe preprocessed, the
     * image values are quantized from [0,1] to [0,UCHAR_MAX] or
     * [0,USHRT_MAX].
     */

    img_uchar = io_png_read_uchar(argv[1], &nx, &ny, &nc);
    free(img_uchar);
    img_uchar =
        io_png_read_uchar_opt(argv[1], &nx, &ny, &nc, IO_PNG_OPT_GRAY);
    free(img_uchar);
    img_uchar = io_png_read_uchar_opt(argv[1], &nx, &ny, &nc, IO_PNG_OPT_RGB);
    io_png_write_uchar("from_uchar.png", img_uchar, nx, ny, nc);
    free(img_uchar);

    img_ushrt = io_png_read_ushrt(argv[1], &nx, &ny, &nc);
    free(img_ushrt);
    img_ushrt =
        io_png_read_ushrt_opt(argv[1], &nx, &ny, &nc, IO_PNG_OPT_GRAY);
    free(img_ushrt);
    img_ushrt = io_png_read_ushrt_opt(argv[1], &nx, &ny, &nc, IO_PNG_OPT_RGB);
    io_png_write_ushrt("from_ushrt.png", img_ushrt, nx, ny, nc);
    free(img_ushrt);

    return EXIT_SUCCESS;
}
