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
 * @file axpb.c
 * @brief transform an image by f(x) = ax + b
 *
 * Yes, this is trivial. I know.
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io_png.h"
#include "axpb_lib.h"

#define VERSION "0.01"

/**
 * @brief main function call
 */
int main(int argc, char *const *argv)
{
    unsigned char *img, *img_ptr, *img_end;
    float *fimg, *fimg_ptr;
    size_t nx, ny, nc;
    float a, b;
    size_t i;

    /* "-v" option : version info */
    if (2 <= argc && 0 == strcmp("-v", argv[1]))
    {
        fprintf(stdout, "%s version " VERSION \
		", compiled " __DATE__ "\n", argv[0]);
        return EXIT_SUCCESS;
    }
    /* wrong number of parameters : simple help info */
    if (5 != argc)
    {
        fprintf(stderr, "usage  : %s a in.png b out.png\n", argv[0]);
        fprintf(stderr, "         a, b  : numerical parameters\n");
        fprintf(stderr, "result : out = a * in + b\n");
        return EXIT_FAILURE;
    }

    a = atof(argv[1]);
    b = atof(argv[3]);

    /* read the PNG input image */
    img = read_png_any2u8rgb(argv[2], &nx, &ny, &nc);

    /* convert to float */
    img_ptr = img;
    img_end = img_ptr + nx * ny * nc;
    fimg = (float *) malloc(nx * ny * nc * sizeof(float));
    fimg_ptr = fimg;
    while (img_ptr < img_end)
	*fimg_ptr++ = (float) *img_ptr++;

    printf("R : ");
    for (i=0; i<nx*ny; i++)
	printf("%i ", (int) fimg[i]);
    printf("\nG : ");
    for (i=nx*ny; i<2*nx*ny; i++)
	printf("%i ", (int) fimg[i]);
    printf("\nB : ");
    for (i=2*nx*ny; i<3*nx*ny; i++)
	printf("%i ", (int) fimg[i]);
    printf("\n\n");

    /* transform the data */
    axpb(fimg, nx * ny * nc, a, b);

    printf("R : ");
    for (i=0; i<nx*ny; i++)
	printf("%i ", (int) fimg[i]);
    printf("\nG : ");
    for (i=nx*ny; i<2*nx*ny; i++)
	printf("%i ", (int) fimg[i]);
    printf("\nB : ");
    for (i=2*nx*ny; i<3*nx*ny; i++)
	printf("%i ", (int) fimg[i]);
    printf("\n\n");

    /* write the PNG output image */
/*    write_png_u8rgb(argv[4], img, nx, ny, nc); */
    free(img);
    free(fimg);

    return EXIT_SUCCESS;
}
