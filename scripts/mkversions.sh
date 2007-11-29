#!/bin/sh

[ $# -eq 2 ] || exit 1

in="$1"
out="$2"

export LC_ALL=C

COMMENT="This file is generated at compile time. Do not include in source tarballs."
COMPILE_CFG="FIXME"
COMPILE_DATE=`date`
COMPILE_USER=`whoami`
COMPILE_HOST=`hostname`
COMPILER=`$CXX -v 2>&1 | grep version`
CFLAGS="$AM_CPPFLAGS $AM_CXXFLAGS"
LDFLAGS="$LDADD"

sed -e "s!%%COMMENT%%!${COMMENT}!g; s!%%COMPILE_CFG%%!${COMPILE_CFG}!g;\
        s!%%COMPILE_DATE%%!${COMPILE_DATE}!g; s!%%COMPILE_USER%%!${COMPILE_USER}!g; \
        s!%%COMPILE_HOST%%!${COMPILE_HOST}!g; s!%%COMPILER%%!${COMPILER}!g; \
        s!%%CFLAGS%%!${CFLAGS}!g; s!%%LDFLAGS%%!${LDFLAGS}!g; s!%%HAVE_VERSIONS_H%%!1!g" < "$in" > "$out"
