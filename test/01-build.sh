#!/bin/sh
#
# Test the code compilation and execution.

# simple code execution
_test_run() {
    example/readpng data/lena_g.png /tmp/out.png
}

################################################

_log_init

echo "* default build, clean, rebuild"
_log make
_log _test_run
_log make
_log make clean
_log make
_log make distclean
_log make
_log make distclean

echo "* build and run with embedded libraries"
_log make scrub
_log make LOCAL_LIBS=1
_log _test_run
_log test "0" = "$( nm -u example/readpng | grep -c png_ )"
_log make scrub

echo "* standard cc compiler, without options"
_log make CC=cc CFLAGS=
_log _test_run
_log make distclean

echo "* tcc compiler, without options"
_log make CC=tcc CFLAGS=
_log _test_run
_log make distclean

echo "* standard c++ compiler, with and without options"
_log make CC=c++
_log _test_run
_log make distclean
_log make CC=c++ CCFLAGS=
_log _test_run
_log make distclean


_log_clean
