#!/bin/bash
#===============================================================================
# linux binaries no longer created
#===============================================================================

#-------------------------------------------------------------------------------
# make the mxe-mingw32 executable
#-------------------------------------------------------------------------------

autoreconf

rm -f fldigi*$1*

make clean

./configure \
  $PKGCFG \
  $CROSSCFG \
  --without-asciidoc \
  --with-ptw32=$PREFIX/i686-w64-mingw32.static \
  --with-libiconv-prefix=$PREFIX/iconv \
  --enable-static \
  --with-libintl-prefix=$PREFIX/gettext \
  PTW32_LIBS="-lpthread -lpcreposix -lpcre -lregex" \
  FLTK_CONFIG=$PREFIX/bin/i686-w64-mingw32.static-fltk-config \

make

$PREFIX/bin/i686-w64-mingw32.static-strip src/fldigi.exe
$PREFIX/bin/i686-w64-mingw32.static-strip src/flarq.exe

make nsisinst

mv src/*setup.exe .
ls -l *setup.exe

make clean

#-------------------------------------------------------------------------------
# build the distribution tarball
#-------------------------------------------------------------------------------

./configure --without-asciidoc

make distcheck

make clean

git co po/de.po
git co po/es.po
git co po/fr.po
git co po/it.po
git co po/pl.po
git co po/nl.po
git co po/fldigi.pot
git co src/dialogs/guide.cxx
