# Copyright 2010 IPOL Image Processing On Line http://www.ipol.im/
# Author: Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
#
# Copying and distribution of this file, with or without
# modification, are permitted in any medium without royalty provided
# the copyright notice and this notice are preserved.  This file is
# offered as-is, without any warranty.

CSRC	= io_png.c axpb_lib.c axpb.c

SRC	= $(CSRC)
OBJ	= $(CSRC:.c=.o)
BIN	= axpb

default: $(BIN)

COPT	= -O3 -funroll-loops -fomit-frame-pointer
CFLAGS	+= -ansi -pedantic -Wall -Wextra -Werror $(COPT)
LDFLAGS += -lpng

# local options
-include makefile.local

%.h	: %.c
	cproto $< > $@

%.o	: %.c
	$(CC) $< -c $(CFLAGS) -o $@

axpb	: axpb.o axpb_lib.o io_png.o
	$(CC) $^ $(LDFLAGS) -o $@ 

.PHONY	: clean distclean
clean	:
	$(RM) $(OBJ)
distclean	: clean
	$(RM) $(BIN)
