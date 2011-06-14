#!/bin/sh
#
# Test the code compilation and execution.

# Test the code correctness by computing the min/max/mean/std of a
# known image, lena. The expected output is in the data folder
#
# red:       min 54.000 max 255.000 mean 180.224 std 49.049
# green:     min  3.000 max 248.000 mean  99.051 std 52.878
# blue:      min  8.000 max 225.000 mean 105.410 std 34.058
# rgb->gray: min 18.511 max 246.184 mean 116.773 std 49.417
# gray:      min 19.000 max 246.000 mean 116.768 std 49.419
# alpha:     min 0.000  max 255.000 mean 127.499 std 74.045
_test_mmms() {
    TEMPFILE=$(tempfile)
    example/mmms data/lena_g.png >> $TEMPFILE
    example/mmms data/lena_ga.png >> $TEMPFILE
    example/mmms data/lena_rgb.png >> $TEMPFILE
    example/mmms data/lena_rgba.png >> $TEMPFILE
    test "c3492d9ccbb3707d5c5c3f2f2510317a  $TEMPFILE" \
	= "$(md5sum $TEMPFILE)"
    rm -f $TEMPFILE
}

# Test the need for a dynamic libpng library
_test_ldd() {
    test "0" = "$(ldd example/mmms | grep -c png)"
}

################################################

_log_init

echo "* default build, test, clean, rebuild"
_log make distclean
_log make
_log _test_mmms
_log make
_log make clean
_log make

echo "* compiler support"
for CC in cc c++ gcc g++ tcc clang icc pathcc; do
    if which $CC; then
	echo "* $CC compiler"
	_log make distclean
	_log make CC=$CC CFLAGS=
	_log _test_mmms
    fi
done
for CC in gcc g++ clang; do
    if which $CC; then
	echo "* $CC compiler with flags"
	_log make distclean
	_log make CC=$CC
	_log _test_mmms
    fi
done

echo "* build with embedded libraries"
_log make scrub
_log make LOCAL_LIBS=1
_log _test_mmms
_log _test_ldd

_log make distclean

_log_clean
