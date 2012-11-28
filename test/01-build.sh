#!/bin/sh
#
# Test the code compilation and execution.

# Test the code correctness by computing the min/max/mean/std of a
# known image, lena. The expected output is in the data folder.
_test_run() {
    TEMPFILE=$(tempfile)
    ./example/mmms data/lena_g.png >> $TEMPFILE
    ./example/mmms data/lena_ga.png >> $TEMPFILE
    ./example/mmms data/lena_rgb.png >> $TEMPFILE
    ./example/mmms data/lena_rgba.png >> $TEMPFILE
    dos2unix $TEMPFILE
    test "9edef44b78fca2d0f129d3c5b788292b  $TEMPFILE" \
	= "$(md5sum $TEMPFILE)"
    ./example/axpb 1 - 0 - < data/lena_g.png > $TEMPFILE
    test "b8a0502abf9127666aa093eb346288b7  $TEMPFILE" \
	= "$(md5sum $TEMPFILE)"
    rm -f $TEMPFILE
    # test all the read-write code variants
    ./example/readpng data/lena_g.png
}

################################################

_log_init

echo "* default build, test, clean, rebuild"
_log make -B debug
_log _test_run
_log make -B
_log _test_run
_log make
_log make clean
_log make

echo "* compiler support"
for CC in cc c++ c89 c99 gcc g++ tcc clang icc suncc \
    i586-mingw32msvc-cc; do
    which $CC || continue
    echo "* $CC compiler"
    _log make distclean
    case $CC in
	"i586-mingw32msvc-cc")
	    test -d ./win32/lib || break
	    # default mingw behaviour has extra precision
	    _log make CC=$CC CFLAGS="-O2 -ffloat-store"\
		CPPFLAGS="-DNDEBUG -I. -I./win32/include" \
		LDFLAGS="-L./win32/lib"
	    ln -f -s ../win32/bin/libpng3.dll ../win32/bin/zlib1.dll ./example/
	    ;;
	*)
	    _log make CC=$CC
	    ;;
    esac
    _log _test_run
    rm -f ./example/*.dll
done

_log make distclean

_log_clean
