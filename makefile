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

# C compiler optimization options
COPT	= -O2
# complete C compiler options
CFLAGS	= $(COPT)
# preprocessot options
CPPFLAGS	= -I. -DNDEBUG
# linker options
LDFLAGS	=
# libraries
LDLIBS	= -lpng -lm

# library build dependencies (none)
LIBDEPS =

# default target: the example programs
default: $(BIN)

# partial C compilation xxx.c -> xxx.o
%.o	: %.c $(LIBDEPS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

# final link of an example program
example/%	: $(LIBDEPS)
example/%	: example/%.o io_png.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# cleanup
.PHONY	: clean distclean
clean	:
	$(RM) $(OBJ)
	$(RM) *.flag
distclean	: clean
	$(RM) $(BIN)
	$(RM) -r srcdoc

################################################
# dev tasks

PROJECT	= io_png
-include	makefile.dev
