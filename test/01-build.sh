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
_log make distclean
_log make
_log _test_run
_log make
_log make clean
_log make

echo "* build and run with embedded libraries"
_log make scrub
_log make LOCAL_LIBS=1
_log _test_run
_log test "0" = "$( nm -u example/readpng | grep -c png_ )"

echo "* compiler support"
for CC in cc c++ gcc g++ tcc clang icc; do
    if which $CC; then
	echo "* $CC compiler"
	_log make distclean
	_log make CC=$CC CFLAGS=
	_log _test_run
    fi
done
for CC in gcc g++ clang; do
    if which $CC; then
	echo "* $CC compiler with flags"
	_log make distclean
	_log make CC=$CC
	_log _test_run
    fi
done

_log make distclean


_log_clean
