% io_png : high-level front-end to libpng

# OVERVIEW

io_png.c contains high-level routines to read and write PNG images
using libpng. On only handles common use cases, and provides a
simplified interface.

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
each array. No custom structure is needed, and the image size
information is collected via pointer parameters.

4 front-end functions are available; they all have the same syntax:

    read_png_xxx(fname, &nx, &ny, &nc)
    - fname: file name
    - nx, ny, nc: variables to fill with the image size

* read_png_u8() : read a PNG image as an unsigned char array
  - 16bit images are converted to 8bit with precision loss
  - 1, 2 and 4bit images are converted to 8bit without precision loss
* read_png_u8_rgb() : same as read_png_u8(), but converts grey images
  to RGB and strip the alpha channel
* read_png_f32() : read a PNG image as a float array
  - 16bit images are first converted to 8bit with precision loss
  - integer values are then converted to float
* read_png_f32_rgb() : same as read_png_f32(), but converts grey images
  to RGB and strip the alpha channel

## WRITE

A PNG image is written from a single array, with the same layout as
the on received from png_read_xxx() functions.

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
* read_png_u8_gray() and read_png_f32_gray()

# COPYRIGHT

Copyright 2010 Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>

Copying and distribution of this README file, with or without
modification, are permitted in any medium without royalty provided
the copyright notice and this notice are preserved.  This file is
offered as-is, without any warranty.
