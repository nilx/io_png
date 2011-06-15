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
 * @file axpb_lib.c
 * @brief dummy library example
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 */

#include <stdlib.h>
#include <stdio.h>

/* ensure consistency */
#include "axpb_lib.h"

/**
 * @brief transform an array by f(x) = ax + b
 *
 * @param data input array
 * @param size array size
 * @param a, b numerical parameters
 */
void axpb(float *data, size_t size, double a, double b)
{
    size_t i;

    for (i = 0; i < size; i++)
    {
        data[i] *= a;
        data[i] += b;
    }

    return;
}
