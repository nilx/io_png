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
     * size_t is the safe type,
     * because the image size is used as an array index
     */

    size_t nx, ny, nc;

    /*
     * image array,
     * initially set to NULL for safety
     */

    float *img = NULL;

    /*
     * the file to read is given as the first command-line argument,
     * the one to write is the second
     */

    if (3 > argc) {
        fprintf(stderr, "syntax: %s in.png out.png\n", argv[0]);
        abort();
    }

    /* read the image */
    img = io_png_read_flt(argv[1], &nx, &ny, &nc);

    /* nx, ny and nc hold the image sizes */
    printf("image file: %s\n", argv[1]);
    printf("image size: %i x %i, %i channels\n",
           (int) nx, (int) ny, (int) nc);

    /*
     * from now on we suppose the image has RGB channels
     * this can be forced by using
     * read_png_pp_flt(..., "rgb") instead of read_png_flt()
     */

    if (3 <= nc) {
        /*
         * the img array layout is RRRR...GGGG...BBBB...
         * pointers can be set for direct access to each channel
         */

        float *img_r = img;     /* red channel pointer */
        float *img_g = img + nx * ny;   /* green channel pointer */
        float *img_b = img + 2 * nx * ny;       /* blue channel pointer */

        /* let's access the 3 components of the pixel (27, 42) */
        size_t x = 27;
        size_t y = 42;
        printf("the RGB components of the pixel (%i, %i) are ",
               (int) x, (int) y);
        printf("R: %f G: %f B: %f\n",
               img_r[x + nx * y], img_g[x + nx * y], img_b[x + nx * y]);

        /*
         * you can also copy/split the channels into 3 arrays
         */

        /* allocate the memory space */
        img_r = (float *) malloc(nx * ny * sizeof(float));
        img_g = (float *) malloc(nx * ny * sizeof(float));
        img_b = (float *) malloc(nx * ny * sizeof(float));

        /* copy each channel */
        memcpy(img_r, img, nx * ny * sizeof(float));
        memcpy(img_g, img + nx * ny, nx * ny * sizeof(float));
        memcpy(img_b, img + 2 * nx * ny, nx * ny * sizeof(float));

        /* and do stuff with on the image arrays... */
        free(img_r);
        free(img_g);
        free(img_b);
    }

    /* write the image */
    io_png_write_flt(argv[2], img, nx, ny, nc);
    free(img);

    /*
     * you can also use special read functions
     * to handle colorspace conversion
     */
    /* read as RGB, and save */
    img = io_png_read_pp_flt(argv[1], &nx, &ny, NULL, "rgb");
    io_png_write_flt(argv[2], img, nx, ny, 3);
    free(img);
    /* read as gray, and save */
    img = io_png_read_pp_flt(argv[1], &nx, &ny, NULL, "gray");
    io_png_write_flt(argv[2], img, nx, ny, 1);
    free(img);

    return EXIT_SUCCESS;
}
