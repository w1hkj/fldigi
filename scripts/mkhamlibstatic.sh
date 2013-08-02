#!/bin/sh

# This script must be run by make

set -e

if test "x$PKG_CONFIG" != "x"; then
    hamlib_dir="$($PKG_CONFIG --variable=libdir hamlib)"
    if test "x$hamlib_dir" = "x"; then
	echo "E: Could not determine hamlib \$libdir" >&2
	exit 1
    fi
else
    hamlib_dir="${HAMLIB_LIBS#*-L}"
    hamlib_dir="${HAMLIB_LIBS%% *}"
    if test "x$hamlib_dir" = "x"; then\
	hamlib_dir=/usr/lib
    fi
fi
hamlib_libs="$hamlib_dir/hamlib-*.a"

case "$target_os" in
    *linux*)
	AM_LDFLAGS="$AM_LDFLAGS -Wl,--export-dynamic -Wl,--whole-archive $hamlib_libs -Wl,--no-whole-archive"
	;;
    *darwin*)
        # Apple's ld isn't quite up to this task: there is no way to specify -all_load for
        # only a subset of the libraries that we must link with. For this reason we resort
        # to using the "dangerous" -m flag, which turns "multiply defined symbol" errors
        # into warnings. This will probably not work for ppc64 and x86_64 universal binaries.
	AM_LDFLAGS="$AM_LDFLAGS -Wl,-all_load -Wl,-m $hamlib_libs"
	;;
    *cygwin*)
	AM_LDFLAGS="$AM_LDFLAGS -Wl,--export-all-symbols -Wl,--whole-archive $hamlib_libs -Wl,--no-whole-archive"
	;;
    *mingw32*)
	AM_LDFLAGS="$AM_LDFLAGS -Wl,--export-dynamic -Wl,--allow-multiple-definition -Wl,--whole-archive $hamlib_libs -Wl,--no-whole-archive"
	;;
    *)
	echo "E: This script does not support $target_os" >&2
	exit 1
	;;
esac

$CXX -o ${1}${EXEEXT} $AM_CXXFLAGS $fldigi_CXXFLAGS $CXXFLAGS $AM_LDFLAGS $fldigi_LDFLAGS $LDFLAGS $fldigi_OBJECTS $fldigi_LDADD
