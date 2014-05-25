#!/bin/sh
#
# Test the code compilation and execution.

# Test the code correctness by computing the min/max/mean/std of a
# known image, lena. The expected output is in the data folder
#
# red:           min 54.000 max 255.000 mean 180.224 std 49.049
# green:         min  3.000 max 248.000 mean  99.051 std 52.878
# blue:          min  8.000 max 225.000 mean 105.410 std 34.058
# rgb->gray,f32  min 18.511 max 246.183 mean 116.771 std 49.417
# rgb->gray,u8:  min 19.000 max 246.000 mean 116.772 std 49.418
# gray:          min 19.000 max 246.000 mean 116.769 std 49.419
# alpha:         min 0.000  max 255.000 mean 127.499 std 74.045
_test_mmms() {
    TEMPFILE=$(tempfile)
    example/mmms data/lena_g.png >> $TEMPFILE
    example/mmms data/lena_ga.png >> $TEMPFILE
    example/mmms data/lena_rgb.png >> $TEMPFILE
    example/mmms data/lena_rgba.png >> $TEMPFILE
    test "e483dc2043fdd8e662f6bc41140cdd4e  $TEMPFILE" \
	= "$(md5sum $TEMPFILE)"
    rm -f $TEMPFILE
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
for CC in cc c++ gcc g++ tcc clang; do
    which $CC || continue
    echo "* $CC compiler"
    _log make distclean
    case $CC in
	"gcc"|"g++")
	    _log make CC=$CC ;;
	*)
	    _log make CC=$CC CFLAGS= ;;
    esac
    _log _test_mmms
done

_log make distclean

_log_clean
