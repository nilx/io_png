#!/bin/sh -e
#
# Test the code correctness by computing the min/max/mean/std of a
# known image, lena. The expected values are:
#
# red:
#  min: 54.000
#  max: 255.000
#  mean: 180.224
#  std: 49.049
# green:
#  min: 3.000
#  max: 248.000
#  mean: 99.051
#  std: 52.878
# blue:
#  min: 8.000
#  max: 225.000
#  mean: 105.410
#  std: 34.058
# rgb->gray:
#  min: 18.511
#  max: 246.184
#  mean: 116.773
#  std: 49.417
# gray:
#  min: 19.000
#  max: 246.000
#  mean: 116.768
#  std: 49.419
# alpha:
#  min: 0.000
#  max: 255.000
#  mean: 127.499
#  std: 74.045

# simple code execution
_test_mmms() {
    example/mmms data/lena_$1.png | diff - test/mmms_lena_$1.txt
}

################################################

_log_init

echo "* compute characheristics of the example images"
_log make distclean
_log make
_log _test_mmms g
_log _test_mmms ga
_log _test_mmms rgb
_log _test_mmms rgba

_log make distclean

_log_clean
