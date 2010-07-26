% io_png : simplified front-end to libpng

# OVERVIEW

io_png.c contains high-level routines to read and write PNG images
using libpng. On only handles common use cases, and provides a
simplified interface.

# LICENSE

io_png.c is distributed under a BSD licence. See the included
copyright notice, conditions and disclaimer for details.

# REQUIREMENTS

libpng is required, version >= 1.2.41. The source code and binaries
can be found at http://www.libpng.org/pub/png/libpng.html.

io_png.c is ANSI C, and should compile on any system with any ANSI C
compiler.

# USAGE

Compile io_png.c with your program, and include io_png.h to get the
function declarations.

## READ

A PNG image is read into a single array. For multiple channel images,
the output array is deinterlaced and successively contains contains
each channel. For example, a color image with 30 rows and 40 columns
is read into a single array of 3600 cells, with:
* the first 1200 cells (30 x 40) containing the red channel
* the next 1200 cells containing the green channel
* the last 1200 cells containing the blue channel
In each channel, the image is stores row after row. Thus, the
resulting array contains.

No image structure is needed, and the image size information is
collected via pointer parameters.

The two main front-end functions are, depending of the data type you want
to use
* read_png_u8() : read a PNG image as an unsigned char array
  - 16bit images are converted to 8bit with precision loss
  - 1, 2 and 4bit images are converted to 8bit without precision loss
* read_png_f32() : read a PNG image as a float array
  - 16bit images are first converted to 8bit with precision loss
  - integer values are then converted to float

These functions have the same syntax:
    read_png_xxx(fname, &nx, &ny, &nc)
    - fname: file name
    - nx, ny, nc: variables to fill with the image size

Four secondary read functions can be used to force a color model:
* read_png_u8_rgb() : convert gray images to RGB and strip the alpha channel
* read_png_u8_gray() : convert RGB images to gray and strip the alpha channel
* read_png_f32_rgb() : convert gray images to RGB and strip the alpha channel
* read_png_f32_gray() : convert RGB images to gray and strip the alpha channel

These functions have the same syntax as the previous ones, except that
they don't need the &nc parameter.

## WRITE

A PNG image is written from a single array, with the same layout as
the one received from the read functions.

2 front-end functions are available; they all have the same syntax:

    write_png_xxx(fname, data, nx, ny, nc)
    - fname: file name
    - data: image array
    - nx, ny, nc: image size

* write_png_u8() : write a PNG image from an unsigned char array
* write_png_f32() : write a PNG image from a float array
  - the array values are first rounded, then limited to [0..255],
    with values lower than 0 set to 0 and values higher than 255 set to 255

## EXAMPLE

see io_png_example.c

# TODO

* handle read from stdin and write to stdout
* handle 16bit data
* add a test suite
* internally handle gray conversion (libpng low-level interface)

# COPYRIGHT

Copyright 2010 Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>

Copying and distribution of this README file, with or without
modification, are permitted in any medium without royalty provided
the copyright notice and this notice are preserved.  This file is
offered as-is, without any warranty.
