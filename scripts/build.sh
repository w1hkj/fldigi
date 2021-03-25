#!/bin/bash

function config32 {

CROSSTARGET=i686-w64-mingw32.static

# Optimizations compatible with all i686 cpus
export CFLAGS=" -march=i686 -mtune=i686 -O2 -ffast-math -mno-3dnow -mmmx -msse -mfpmath=sse "
export CXXFLAGS=$CFLAGS
 
# Configure script options
CONFIGUREFLAGS=" --enable-optimizations=sse --enable-static"

export PREFIX="$HOME/.fldigiMXE/mxe/usr"
export LINKCFG="--enable-static --disable-shared"
export CROSSHST="--host=$CROSSTARGET"
export PKGCFG="PKG_CONFIG=$PREFIX/bin/$CROSSTARGET-pkg-config"
export PKG_CONFIG_PATH=$PREFIX/$CROSSTARGET/lib/pkgconfig
export PATH=$HOME/.fldigiMXE/mxe/usr/bin/:$PATH

echo "Building 32 bit mxe target"

}

function config64 {

CROSSTARGET=x86_64-w64-mingw32.static

# Optimizations compatible with all X86_64 CPUS
export CFLAGS=" -march=x86-64 -mtune=k8 -O2 -ffast-math -mno-3dnow -mmmx -msse -msse2 -mfpmath=sse "
export CXXFLAGS=$CFLAGS
 
# Configure script options
CONFIGUREFLAGS=" --enable-optimizations=sse2 --enable-static" 	# 64-bit

export PREFIX="$HOME/.fldigiMXE/mxe/usr"
export LINKCFG="--enable-static --disable-shared"
export CROSSHST="--host=$CROSSTARGET"
export PKGCFG="PKG_CONFIG=$PREFIX/bin/$CROSSTARGET-pkg-config"
export PKG_CONFIG_PATH=$PREFIX/$CROSSTARGET/lib/pkgconfig
export PATH=$HOME/.fldigiMXE/mxe/usr/bin/:$PATH

echo "Building 64 bit mxe target"

}

# Switch to the correct directory to start building
cd $( dirname ${BASH_SOURCE[0]} ) # cd to directory where this script is
cd ..	# go up one level to correct directory

#autoreconf
#automake --add-missing

rm -fv *.exe
rm -fv src/*.exe

echo "=================================="
if [[ $1 == "32" ]]; then
	config32
else
	config64
fi

### Extra-Security compile-flags copied from Hardened Gentoo
#HARDEN=false		# set true to compile the entire Fldigi MXE environment with extra security
#HARD_CFLAGS=" -fPIE -fstack-protector-all -D_FORTIFY_SOURCE=2 " 
#HARD_LDFLAGS=" -Wl,-z,now -Wl,-z,relro "
#if $HARDEN; then
#	CFLAGS+=$HARD_CFLAGS
#	LDFLAGS+=$HARD_LDFLAGS
#fi

echo "=================================="
echo CROSSTARGET = $CROSSTARGET
echo PREFIX = $PREFIX
echo LINKCFG = $LINKCFG
echo CROSSHST = $CROSSHST
echo PKGCFG = $PKGCFG
echo PKG_CONFIG_PATH = $PKG_CONFIG_PATH
echo PATH = $PATH
echo "=================================="
echo "pkg config files:"
ls $PKG_CONFIG_PATH
echo "=================================="

./configure \
  $CONFIGUREFLAGS \
  $PKGCFG \
  $CROSSHST \
  --without-asciidoc \
  --with-ptw32=$PREFIX/$CROSSTARGET \
  --with-libiconv-prefix=$PREFIX/iconv \
  --with-libintl-prefix=$PREFIX/gettext \
  PTW32_LIBS="-lpthread -lpcreposix -lpcre -lregex" \
  FLTK_CONFIG=$PREFIX/$CROSSTARGET/bin/fltk-config \
  || exit 1

make clean

time make -j 8 || exit 1 

$PREFIX/bin/$CROSSTARGET-strip src/fldigi.exe
$PREFIX/bin/$CROSSTARGET-strip src/flarq.exe

make nsisinst
mv src/*setup.exe .
ls -l *setup.exe

exit 0
