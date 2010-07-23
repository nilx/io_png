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

#define VERSION "0.20100723"

/**
 * @brief main function call
 */
int main(int argc, char *const *argv)
{
    float *img;
    size_t nx = 0, ny = 0, nc = 0;
    double a, b;

    /* "-v" option : version info */
    if (2 <= argc && 0 == strcmp("-v", argv[1]))
    {
        fprintf(stdout, "%s version " VERSION
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
    img = read_png_f32(argv[2], &nx, &ny, &nc);

    /* transform the data */
    axpb(img, nx * ny * nc, a, b);

    /* write the PNG output image */
    write_png_f32(argv[4], img, nx, ny, nc);

    /* free the memory */
    free(img);

    return EXIT_SUCCESS;
}
