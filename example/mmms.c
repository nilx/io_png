/*
 * This file computes the min, max, mean and standard deviation of a
 * PNG image, read with the six io_png_read front-ends.
 *
 * compile with: cc mmms.c io_png.c -lpng
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* include the io_png prototypes */
#include "io_png.h"

/* constants used to select the read mode */
#define FLT      0
#define FLT_GRAY 1
#define FLT_RGB  2
#define UCHAR       3
#define UCHAR_GRAY  4
#define UCHAR_RGB   5

static char mode_str[][11] = {
    "flt", "flt_gray", "flt_rgb", "uchar", "uchar_gray", "uchar_rgb"
};

/**
 * compute and print the min/max of a float array
 */
static void mm(float *data, size_t size)
{
    size_t i;
    float pxl, min, max;

    min = data[0];
    max = data[0];
    for (i = 1; i < size; i++) {
        pxl = data[i];
        min = pxl < min ? pxl : min;
        max = pxl > max ? pxl : max;
    }
    printf("   min:\t%0.3f\n", min);
    printf("   max:\t%0.3f\n", max);
    return;
}

/**
 * compute and print the mean/std of a float array
 */
static void ms(float *data, size_t size)
{
    size_t i;
    double mean, std;

    mean = 0.;
    std = 0.;
    for (i = 0; i < size; i++)
        mean += (double) data[i];
    mean /= (double) size;
    for (i = 0; i < size; i++)
        std += ((double) data[i] - mean) * ((double) data[i] - mean);
    std /= (double) size;
    std = sqrt(std);

    printf("   mean:\t%0.3f\n", (float) mean);
    printf("   std:\t%0.3f\n", (float) std);
    return;
}

/**
 * read a png file using one of the six available modes
 */
static float *io_png_read_mode(const char *fname,
                               size_t * nx, size_t * ny, size_t * nc,
                               int mode)
{
    float *img;
    unsigned char *img_uchar;
    size_t sz, i;

    printf(" mode:\t%s\n", mode_str[mode]);

    /* read the image file in one of the eight read modes */
    switch (mode) {
    case FLT:
        img = io_png_read_flt(fname, nx, ny, nc);
        break;
    case FLT_GRAY:
        *nc = 1;
        img = io_png_read_flt_gray(fname, nx, ny);
        break;
    case FLT_RGB:
        *nc = 3;
        img = io_png_read_flt_rgb(fname, nx, ny);
        break;
    case UCHAR:
        img_uchar = io_png_read_uchar(fname, nx, ny, nc);
        break;
    case UCHAR_GRAY:
        *nc = 1;
        img_uchar = io_png_read_uchar_gray(fname, nx, ny);
        break;
    case UCHAR_RGB:
        *nc = 3;
        img_uchar = io_png_read_uchar_rgb(fname, nx, ny);
        break;
    default:
        abort();
    }

    /* convert 8bit integer data to floats */
    switch (mode) {
    case UCHAR:
    case UCHAR_GRAY:
    case UCHAR_RGB:
        sz = *nx * *ny * *nc;
        if (NULL == img_uchar)
            abort();
        img = (float *) malloc(sz * sizeof(float));
        for (i = 0; i < sz; i++)
            img[i] = (float) img_uchar[i];
        free(img_uchar);
        break;
    }

    if (NULL == img)
        abort();

    return img;
}

/**
 * print informations about an image file, read in uchar/flt, gray/rgb mode
 */
static void mmms(const char *fname, int mode)
{
    size_t nx, ny, nc, c;
    float *img;

    img = io_png_read_mode(fname, &nx, &ny, &nc, mode);

    /* print generic info */
    printf("  size:\t%ix%ix%i\n", (int) nx, (int) ny, (int) nc);
    /* print channel info */
    for (c = 0; c < nc; c++) {
        printf("  channel:\t%i\n", (int) c);
        mm(img + c * nx * ny, nx * ny);
        ms(img + c * nx * ny, nx * ny);
    }
    free(img);
}

int main(int argc, const char **argv)
{
    if (2 > argc) {
        fprintf(stderr, "syntax:\t%s in.png\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("file:\t%s\n", argv[1]);
    mmms(argv[1], FLT);
    mmms(argv[1], FLT_GRAY);
    mmms(argv[1], FLT_RGB);
    mmms(argv[1], UCHAR);
    mmms(argv[1], UCHAR_GRAY);
    mmms(argv[1], UCHAR_RGB);

    return EXIT_SUCCESS;
}
