#!/bin/sh -e
#
# Check there is no memory leak with valgrind/memcheck.

_test_memcheck() {
    test "0" = "$( valgrind --tool=memcheck $* 2>&1 | grep -c 'LEAK' )"
}

################################################

_log_init

echo "* check memory leaks"
_log make distclean
_log make
_log _test_memcheck example/mmms data/lena_g.png
_log _test_memcheck example/mmms data/lena_ga.png
_log _test_memcheck example/mmms data/lena_rgb.png
_log _test_memcheck example/mmms data/lena_rgba.png

_log make distclean

_log_clean
