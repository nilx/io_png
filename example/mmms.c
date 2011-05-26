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
#define F32      0
#define F32_GRAY 1
#define F32_RGB  2
#define U8       3
#define U8_GRAY  4
#define U8_RGB   5

static char mode_str[][9] = {
    "f32", "f32_gray", "f32_rgb", "u8", "u8_gray", "u8_rgb"
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
    unsigned char *img_u8;
    size_t sz, i;

    printf(" mode:\t%s\n", mode_str[mode]);

    /* read the image file in one of the eight read modes */
    switch (mode) {
    case F32:
        img = io_png_read_f32(fname, nx, ny, nc);
        break;
    case F32_GRAY:
        *nc = 1;
        img = io_png_read_f32_gray(fname, nx, ny);
        break;
    case F32_RGB:
        *nc = 3;
        img = io_png_read_f32_rgb(fname, nx, ny);
        break;
    case U8:
        img_u8 = io_png_read_u8(fname, nx, ny, nc);
        break;
    case U8_GRAY:
        *nc = 1;
        img_u8 = io_png_read_u8_gray(fname, nx, ny);
        break;
    case U8_RGB:
        *nc = 3;
        img_u8 = io_png_read_u8_rgb(fname, nx, ny);
        break;
    default:
        abort();
    }

    /* convert 8bit integer data to floats */
    switch (mode) {
    case U8:
    case U8_GRAY:
    case U8_RGB:
        sz = *nx * *ny * *nc;
        if (NULL == img_u8)
            abort();
        img = (float *) malloc(sz * sizeof(float));
        for (i = 0; i < sz; i++)
            img[i] = (float) img_u8[i];
        free(img_u8);
        break;
    }

    if (NULL == img)
        abort();

    return img;
}

/**
 * print informations about an image file, read in u8/f32, gray/rgb mode
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
    mmms(argv[1], F32);
    mmms(argv[1], F32_GRAY);
    mmms(argv[1], F32_RGB);
    mmms(argv[1], U8);
    mmms(argv[1], U8_GRAY);
    mmms(argv[1], U8_RGB);

    return EXIT_SUCCESS;
}
