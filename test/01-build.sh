#!/bin/sh
#
# Test the code compilation and execution.

# Test the code correctness by computing the min/max/mean/std of a
# known image, lena. The expected output is in the data folder.
_test_run() {
    TEMPFILE=$(tempfile)
    example/mmms data/lena_g.png >> $TEMPFILE
    example/mmms data/lena_ga.png >> $TEMPFILE
    example/mmms data/lena_rgb.png >> $TEMPFILE
    example/mmms data/lena_rgba.png >> $TEMPFILE
    test "9edef44b78fca2d0f129d3c5b788292b  $TEMPFILE" \
	= "$(md5sum $TEMPFILE)"
    rm -f $TEMPFILE
    # test all the read-write code variants
    example/readpng data/lena_g.png
}

# Test the need for a dynamic libpng library
_test_ldd() {
    test "0" = "$(ldd example/mmms | grep -c png)"
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
for CC in cc c++ c99 gcc g++ tcc nwcc clang icc pathcc suncc; do
    which $CC || continue
    echo "* $CC compiler"
    _log make distclean
    _log make CC=$CC
    _log _test_run
done

echo "* build with embedded libraries"
_log make scrub
_log make LOCAL_LIBS=1
_log _test_run
_log _test_ldd

_log make distclean

_log_clean
