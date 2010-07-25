/*
 * This file shows how to use io_png.c. It is released in the public
 * domain and as such comes with no copyright requirement.
 *
 * compile with : cc io_png_example.c io_png.c -lpng
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* include the io_png prototypes */
#include "io_png.h"

int main()
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

    /* read the image */
    img = read_png_f32("in.png", &nx, &ny, &nc);

    /* if img == NULL, there was an error while reading */
    if (NULL == img)
    {
        fprintf(stderr, "failed to read the image in.png\n");
        abort();
    }

    /* nx, ny and nc hols the image sizes */
    printf("image size : %i x %i, %i channels\n",
           (int) nx, (int) ny, (int) nc);

    /*
     * from now on we suppose the image has RGB channels
     * this can be forced by using
     * read_png_f32_rgb() instead of read_png_f32()
     */

    if (3 <= nc)
    {
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
    }

    /* write the image */
    if (0 != write_png_f32("out.png", img, nx, ny, nc))
    {
        fprintf(stderr, "failed to write the image out.png\n");
        abort();
    }

    return EXIT_SUCCESS;
}
