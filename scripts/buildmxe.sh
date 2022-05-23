#!/bin/bash
#=======================================================================
# make the mxe-mingw32 executable

./configure \
  $PKGCFG \
  --host=i686-w64-mingw32.static \
  --without-asciidoc \
  --with-ptw32=$PREFIX/i686-w64-mingw32.static \
  --with-libiconv-prefix=$PREFIX/iconv \
  --enable-static \
  --with-libintl-prefix=$PREFIX/gettext \
  PTW32_LIBS="-lpthread -lpcreposix -lpcre" \
  FLTK_CONFIG=$PREFIX/bin/i686-w64-mingw32.static-fltk-config \

make -j 4

$PREFIX/bin/i686-w64-mingw32.static-strip src/fldigi.exe
$PREFIX/bin/i686-w64-mingw32.static-strip src/flarq.exe

make nsisinst

mv src/*setup.exe .
ls -l *setup.exe
