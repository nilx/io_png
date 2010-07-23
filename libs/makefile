# Copyright 2010 Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
#
# Copying and distribution of this file, with or without
# modification, are permitted in any medium without royalty provided
# the copyright notice and this notice are preserved.  This file is
# offered as-is, without any warranty.
#
# This makefile imports and compiles libraries libraries
# for local use in static form

DEST_DIR = build
LIB_DIR	= $(DEST_DIR)/lib
INC_DIR	= $(DEST_DIR)/include

ZLIB_LIB	= $(LIB_DIR)/libz.a
ZLIB_INC	= $(addprefix $(INC_DIR)/, zlib.h zconf.h)
ZLIB_FILES	= $(ZLIB_LIB) $(ZLIB_INC)
ZLIB_DIR	= zlib-1.2.5
ZLIB_ARC	= zlib-1.2.5.tar.gz
ZLIB_URL	= http://sourceforge.net/projects/libpng/files/zlib/1.2.5/zlib-1.2.5.tar.gz/download

LIBPNG_LIB	= $(LIB_DIR)/libpng.a
LIBPNG_INC	= $(addprefix $(INC_DIR)/, png.h pngconf.h)
LIBPNG_FILES	= $(LIBPNG_LIB) $(LIBPNG_INC)
LIBPNG_DIR	= libpng-1.4.3
LIBPNG_ARC	= libpng-1.4.3.tar.gz
LIBPNG_URL	= http://sourceforge.net/projects/libpng/files/01-libpng-master/1.4.3/libpng-1.4.3.tar.gz/download

LIBJPEG_LIB	= $(LIB_DIR)/libjpeg.a
LIBJPEG_INC	= $(addprefix $(INC_DIR), jconfig.h jerror.h jmorecfg.h jpeglib.h)
LIBJPEG_FILES	= $(LIBJPEG_LIB) $(LIBJPEG_INC)
LIBJPEG_DIR 	= jpeg-8b
LIBJPEG_ARC 	= jpegsrc.v8b.tar.gz
LIBJPEG_URL 	= http://www.ijg.org/files/jpegsrc.v8b.tar.gz

LIBTIFF_LIB	= $(LIB_DIR)/libtiff.a
LIBTIFF_INC	= $(addprefix $(INC_DIR)/, tiffconf.h tiff.h tiffio.h tiffvers.h)
LIBTIFF_FILES	= $(LIBTIFF_LIB) $(LIBTIFF_INC)
LIBTIFF_DIR 	= tiff-3.9.4
LIBTIFF_ARC 	= tiff-3.9.4.tar.gz
LIBTIFF_URL 	= ftp://ftp.remotesensing.org/pub/libtiff/tiff-3.9.4.tar.gz

# LIBFFTW_FILES	=
# LIBFFTW_DIR 	= fftw-3.2.2
# LIBFFTW_ARC 	= fftw-3.2.2.tar.gz
# LIBFFTW_URL 	= http://www.fftw.org/fftw-3.2.2.tar.gz

# ATLAS_FILES	=
# ATLAS_DIR 	= ATLAS
# ATLAS_ARC 	= atlas3.8.3.tar.gz
# ATLAS_URL 	= http://sourceforge.net/projects/math-atlas/files/Stable/3.8.3/atlas3.8.3.tar.gz/download

#
# COMMON
#

default	: all
all	: zlib libpng libtiff libjpeg

$(LIB_DIR) $(INC_DIR)	:
	mkdir -p $@
$(LIB_DIR)/% $(INC_DIR)/%	:
	cp $(filter %.h %.a, $^) $@

#
# ZLIB
#

.PHONY	: zlib
zlib	: $(ZLIB_FILES)

$(ZLIB_FILES)	: $(LIB_DIR) $(INC_DIR)
$(LIB_DIR)/libz.a	: $(ZLIB_DIR)/libz.a
$(INC_DIR)/zlib.h	: $(ZLIB_DIR)/zlib.h
$(INC_DIR)/zconf.h	: $(ZLIB_DIR)/zconf.h

$(ZLIB_DIR)/libz.a	: $(ZLIB_DIR)
	cd ./$(ZLIB_DIR)/; ./configure --static
	$(MAKE) -C $(ZLIB_DIR) libz.a

$(ZLIB_DIR) : $(ZLIB_ARC)
	tar xvzf $<
	touch $@

$(ZLIB_ARC) :
	wget $(ZLIB_URL)

#
# LIBPNG
#

.PHONY	: libpng 
libpng	: $(LIBPNG_FILES)

$(LIBPNG_FILES)	: $(LIB_DIR) $(INC_DIR)
$(LIB_DIR)/libpng.a	: $(LIBPNG_DIR)/lib/libpng.a
$(INC_DIR)/png.h	: $(LIBPNG_DIR)/png.h
$(INC_DIR)/pngconf.h	: $(LIBPNG_DIR)/pngconf.h

$(LIBPNG_DIR)/lib/libpng.a	: $(LIBPNG_DIR) $(ZLIB_FILES)
	cd ./$(LIBPNG_DIR)/; ./configure \
		--enable-static \
		--disable-shared \
		--prefix=$$PWD
	$(MAKE) -C $(LIBPNG_DIR) libpng14.la ZLIBLIB=../ ZLIBINC=../
	$(MAKE) -C $(LIBPNG_DIR) install

$(LIBPNG_DIR) : $(LIBPNG_ARC)
	tar xvzf $<
	touch $@

$(LIBPNG_ARC) :
	wget $(LIBPNG_URL)

#
# LIBJPEG
#

.PHONY	: libjpeg
libjpeg	: $(LIBJPEG_DIR)

$(LIBJPEG_FILES)	: $(LIB_DIR) $(INC_DIR)
$(LIB_DIR)/libjpeg.a	: $(LIBJPEG_DIR)/lib/libjpeg.a
$(INC_DIR)/jconfig.h	: $(LIBJPEG_DIR)/include/jconfig.h
$(INC_DIR)/jerror.h	: $(LIBJPEG_DIR)/include/jerror.h
$(INC_DIR)/jmorecfg.h	: $(LIBJPEG_DIR)/include/jmorecfg.h
$(INC_DIR)/jpeglib.h	: $(LIBJPEG_DIR)/include/jpeglib.h

$(LIBJPEG_DIR)/lib/libjpeg.a	: $(LIBJPEG_DIR)
	cd ./$(LIBJPEG_DIR)/; ./configure \
		--enable-static \
		--disable-shared \
		--prefix=$$PWD
	$(MAKE) -C $(LIBJPEG_DIR) libjpeg.la
	$(MAKE) -C $(LIBJPEG_DIR) install

$(LIBJPEG_DIR) : $(LIBJPEG_ARC)
	tar xvzf $<
	touch $@

$(LIBJPEG_ARC) :
	wget $(LIBJPEG_URL)

#
# LIBTIFF
#

.PHONY	: libtiff
libtiff	: $(LIBTIFF_FILES)

$(LIBTIFF_FILES)	: $(LIB_DIR) $(INC_DIR)
$(LIB_DIR)/libtiff.a	: $(LIBTIFF_DIR)/lib/libtiff.a
$(INC_DIR)/tiff.h	: $(LIBTIFF_DIR)/include/tiff.h
$(INC_DIR)/tiffio.h	: $(LIBTIFF_DIR)/include/tiffio.h
$(INC_DIR)/tiffconf.h	: $(LIBTIFF_DIR)/include/tiffconf.h
$(INC_DIR)/tiffvers.h	: $(LIBTIFF_DIR)/include/tiffvers.h

$(LIBTIFF_DIR)/lib/libtiff.a	: $(LIBTIFF_DIR) $(ZLIB_FILES) $(LIBJPEG_FILES)
	cd ./$(LIBTIFF_DIR)/; ./configure \
		--with-zlib-include-dir=../ \
		--with-zlib-lib-dir=../ \
		--with-jpeg-include-dir=../ \
		--with-jpeg-lib-dir=../ \
		--disable-cxx \
		--enable-static \
		--disable-shared \
		--disable-rpath --prefix=$$PWD
	$(MAKE) -C $(LIBTIFF_DIR)/port libport.la
	$(MAKE) -C $(LIBTIFF_DIR)/libtiff libtiff.la
	$(MAKE) -C $(LIBTIFF_DIR)/libtiff install

$(LIBTIFF_DIR) : $(LIBTIFF_ARC)
	tar xvzf $<
	touch $@

$(LIBTIFF_ARC) :
	wget $(LIBTIFF_URL)

#
# HOUSEKEEPING
#

clean	:
	$(RM) -r $(ZLIB_DIR)	
	$(RM) -r $(LIBPNG_DIR)
	$(RM) -r $(LIBJPEG_DIR)
	$(RM) -r $(LIBTIFF_DIR)
distclean	: clean
	$(RM) $(ZLIB_ARC)
	$(RM) $(LIBPNG_ARC)
	$(RM) $(LIBJPEG_ARC)
	$(RM) $(LIBTIFF_ARC)
	$(RM) -r $(DEST_DIR)
