# Copyright 2010 Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
#
# Copying and distribution of this file, with or without
# modification, are permitted in any medium without royalty provided
# the copyright notice and this notice are preserved.  This file is
# offered as-is, without any warranty.

# source code
SRC	= io_png.c example/readpng.c example/mmms.c example/axpb.c
# object files (partial compilation)
OBJ	= $(SRC:.c=.o)
# binary executable programs
BIN	= $(filter example/%, $(SRC:.c=))

# standard C compiler optimization options
COPT	= -O3 -DNDEBUG
# complete C compiler options
CFLAGS	= -ansi -pedantic -Wall -Wextra -Werror -pipe $(COPT)
# linker options
LDFLAGS	= -lpng -lm
# library build dependencies (none)
LIBDEPS =

# use local embedded libraries
ifdef LOCAL_LIBS
# library location
LIBDIR = ./libs/build/lib
INCDIR = ./libs/build/include
# libpng is required
LIBDEPS += libpng
# compile options to use the local libpng header
CFLAGS 	+= -I$(INCDIR) -DIO_PNG_LOCAL_LIBPNG
# link options to use the local libraries
LDFLAGS = $(LIBDIR)/libpng.a $(LIBDIR)/libz.a -lm
endif

# default target: the example programs
default: $(BIN)

# build the png library
.PHONY	: libpng
libpng	:
	$(MAKE) -C libs libpng

# partial C compilation xxx.c -> xxx.o
%.o	: %.c $(LIBDEPS)
	$(CC) $< -c $(CFLAGS) -I. -o $@

# final link of an example program
example/%	: example/%.o io_png.o $(LIBDEPS)
	$(CC) $< io_png.o $(LDFLAGS) -o $@

# cleanup
.PHONY	: clean distclean scrub
clean	:
	$(RM) $(OBJ)
	$(RM) *.flag
	$(MAKE) -C libs $@
distclean	: clean
	$(RM) $(BIN)
	$(RM) -r srcdoc
	$(MAKE) -C libs $@
scrub	: distclean
	$(MAKE) -C libs $@

################################################
# dev tasks

PROJECT	= io_png
-include	makefile.dev
