# Copyright 2010 IPOL Image Processing On Line http://www.ipol.im/
# Author: Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
#
# Copying and distribution of this file, with or without
# modification, are permitted in any medium without royalty provided
# the copyright notice and this notice are preserved.  This file is
# offered as-is, without any warranty.

# source code, C language
CSRC	= io_png.c axpb_lib.c axpb.c

# source code, all languages (only C here)
SRC	= $(CSRC)
# object files (partial compilation)
OBJ	= $(CSRC:.c=.o)
# binary executable program
BIN	= axpb

# default target : the binary executable program
default: $(BIN)

# standard C compiler optimization options
COPT	= -O2 -funroll-loops -fomit-frame-pointer
# complete C compiler options
CFLAGS	= -ansi -pedantic -Wall -Wextra -Werror -pipe $(COPT)
LDFLAGS	= -lpng -lm

# optional makefile config
-include makefile.extra

# partial C compilation xxx.c -> xxx.o
%.o	: %.c
	$(CC) $< -c $(CFLAGS) -o $@

# final link of the partially compiled files
# (LIBDEPS is for optional library dependencies)
axpb	: $(OBJ) $(LIBDEPS)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

# cleanup
.PHONY	: clean distclean
clean	:
	$(RM) $(OBJ)
distclean	: clean
	$(RM) $(BIN)
