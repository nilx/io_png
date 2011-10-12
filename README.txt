% io_png: simplified front-end to libpng

* overview
* license
* download
* requirements
* compilation
* usage
* todo
* copyright

# OVERVIEW

io_png.c contains high-level routines to read and write PNG images
using libpng. It only handles common use cases, and provides a
simplified interface.

# LICENSE

io_png.c is distributed under a GPL3+ or BSD license, at your
option. See the included copyright notice, conditions and disclaimer
for details.

# DOWNLOAD

The latst version of this code is available at
  http://dev.ipol.im/git/?p=nil/io_png.git
with a mirror at
  https://github.com/nilx/io_png

# REQUIREMENTS

io_png.c is ANSI C, and should compile on any system with any ANSI C
compiler.

libpng is required, version >= 1.2.2. The source code and binaries
can be found at http://www.libpng.org/pub/png/libpng.html.

libpng requires zlib for compression. The source code and binaries can
be found at http://www.zlib.net/.

On Linux, these library can be installed by the package manager. On
Mac OSX, you can use Fink: http://www.finkproject.org/. On Windows,
you can use GnuWin32: http://gnuwin32.sourceforge.net/.

# COMPILATION

Compile io_png.c with the same compiler used for the other source files
of your project. C and C++ compilers are possible. io_png has been
tested with gcc, icc, suncc, pathcc, tcc and nwcc.

You can compile the example codes located in the example folder using
the provided makefile, with the `make` command.

## LOCAL LIBRARIES

If libpng is not installed on your system, of if you prefer a local
static build, a mechanism is available to automatically download,
build and include libpng in your program:

1. run `make libpng`;
   this uses the makefiles from the `libs` folder to download and
   compile libpng and zlib, and builds the libraries into `libs/build`;
2. use the "-DIO_PNG_LOCAL_LIBPNG -I./libs/build/include" options to compile
   io_png.c;
3. add ./libs/build/lib/libpng.a ./libs/build/lib/libz.a to the list of
  files being linked into your program

This is automatically handled in the provided makefile for the example
code example/readpng.c; simply use the `make LOCAL_LIBS=1` command
instead of `make`.

# USAGE

Compile io_png.c with your program, and include io_png.h to get the
function declarations. You can use io_png.c with C or C++ code.

## READ

A PNG image is read into a single array. For multiple channel images,
the output array is de-interlaced and successively contains each
channel. For example, a color image with 30 rows and 40 columns is
read into a single array of 3600 cells, with:

* the first 1200 cells (30 x 40) containing the red channel
* the next 1200 cells containing the green channel
* the last 1200 cells containing the blue channel

In each channel, the image is stored row after row.

No image structure is needed, and the image size information is
collected via pointer parameters.

The three main read functions are available, depending on the data
type you want to use:

* io_png_read_flt(fname, &nx, &ny, &nc)
  read a PNG image into a [0,1] float array
  - fname: file name; the standard input stream is used if fname is "-"
  - nx, ny, nc: variables to fill with the image size; NULL pointer
    can also be used to ignore a size
* io_png_read_uchar(fname, &nx, &ny, &nc)
  read a PNG image into a [0,UCHAR_MAX] unsigned char array
* io_png_read_uchar(fname, &nx, &ny, &nc)
  !!UNTESTED!! read a PNG image into a [0,USHRT_MAX] unsigned short array

Three other read functions add a preprocessing step, to ensure you
always receive the same kind of image, whatever the file contains:

* io_png_read_flt_opt(fname, &nx, &ny, &nc, option)
* io_png_read_uchar_opt(fname, &nx, &ny, &nc, option)
* io_png_read_uchar_opt(fname, &nx, &ny, &nc, option)

Thew option parameter is:
- IO_PNG_OPT_NONE do nothing
- IO_PNG_OPT_RGB  strip alpha and convert gray to rgb
- IO_PNG_OPT_GRAY strip alpha and convert rgb to gray

For all these fonctions, the data is read into float, processed as
float, then requantized to the desired precision. 16bit PNG files are
currently downscaled to 8bit before being read.

## WRITE

A PNG image is written from a single array, with the same layout as
the one received from the read functions.

Three write functions are available, depending on the data type you want
to use:

* io_png_write_flt(fname, data, nx, ny, nc)
  write a 8bit PNG image from a [0,1] float array
  - fname: file name, the standard output stream is used if fname is "-"
  - data: image array
  - nx, ny, nc: image size
* io_png_write_uchar(fname, data, nx, ny, nc)
  write a 8bit PNG image from a [0,UCHAR_MAX] unsigned char array
* io_png_write_ushrt(fname, data, nx, ny, nc)
  !!UNTESTED!! write a 8bit PNg image from a [0,USHRT_MAX] unsigned short array

Three other write functions add processing step, to tune the file content:

* io_png_write_flt(fname, data, nx, ny, nc, option)
* io_png_write_uchar(fname, data, nx, ny, nc, option)
* io_png_write_ushrt(fname, data, nx, ny, nc, option)

The option parameter can be a combinaison of:
- IO_PNG_OPT_NONE  do nothing
- IO_PNG_OPT_ADAM7 do a Adam7 pixel interlacing for progressive display
- IO_PNG_OPT_ZMIN  use minimum data compression (fast, large)
- IO_PNG_OPT_ZMIN  use maximum data compression (small, slow)

## EXAMPLE

see example/readpng.c and example/axpb.c

# TODO

* test 16bit data
* cmake support
* C++ wrappers (vector output, merged functions)
* implement proper sRGB/gamma

# COPYRIGHT

Copyright 2011 Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>

Copying and distribution of this README file, with or without
modification, are permitted in any medium without royalty provided
the copyright notice and this notice are preserved.  This file is
offered as-is, without any warranty.
