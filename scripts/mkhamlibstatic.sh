#!/bin/sh

set -e

if [ $# -ne 1 ]; then
    echo "Syntax: $0 obj-dir" >&2
    exit 1
fi
tmp="$1"
binary="fldigi"

if ! test -r "$binary"; then
    echo "E: $binary not found"
    exit 1
fi

if test "x$PKG_CONFIG" != "x"; then
    hamlib_dir="$($PKG_CONFIG --variable=libdir hamlib)"
    if test "x$hamlib_dir" = "x"; then
	echo "E: Could not determine hamlib \$libdir"
	exit 1
    fi
else
    hamlib_dir="${HAMLIB_LIBS#*-L}"
    hamlib_dir="${HAMLIB_LIBS%% *}"
    if test "x$hamlib_dir" = "x"; then
	hamlib_dir=/usr/lib
    fi
fi

rm -rf $tmp
mkdir -p $tmp
cd $tmp
for i in "$hamlib_dir"/hamlib-*.a; do
    ar x $i
done
cd ..

case "$target_os" in
     *cygwin*)
	AM_LDFLAGS="$AM_LDFLAGS -Wl,--export-all-symbols"
	;;
esac

$CXX -o ${binary}${EXEEXT} $AM_CXXFLAGS $CXXFLAGS $AM_LDFLAGS $LDFLAGS $fldigi_OBJECTS $tmp/*.${OBJEXT} $fldigi_LDADD

rm -rf $tmp
