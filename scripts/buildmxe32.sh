#!/bin/bash
#=======================================================================
# make the mxe-mingw 32-bit windows executable


# Target environment setting
CROSSTARGET=i686-w64-mingw32.static		# 32-bit 

### Extra-Security compile-flags copied from Hardened Gentoo
HARDEN=false		# set true to compile the entire Fldigi MXE environment with extra security
HARD_CFLAGS=" -fPIE -fstack-protector-all -D_FORTIFY_SOURCE=2 " 
HARD_LDFLAGS=" -Wl,-z,now -Wl,-z,relro "

# Compiler Optimization Options
export CFLAGS=" -march=i686 -mtune=i686 -O2 -ffast-math -mno-3dnow -mmmx -msse -mfpmath=sse "		# Optimizations compatible with all i686 cpus
if $HARDEN; then
	CFLAGS+=$HARD_CFLAGS
	LDFLAGS+=$HARD_LDFLAGS
fi
export CXXFLAGS=$CFLAGS
 
# Configure script options
CONFIGUREFLAGS=" --enable-optimizations=sse " 		# 32-bit

# Set/change the environment variables ONLY if they are unset
function Env_Variables
{
	[ -z "$PREFIX" ] && export PREFIX="$HOME/fldigi.MXE/mxe/usr"
	[ -z "$LINKCFG" ] && export LINKCFG="--enable-static --disable-shared"
	[ -z "$CROSSCFG" ] && export CROSSCFG="--host=$CROSSTARGET"
	[ -z "$PKGCFG" ] && export PKGCFG="PKG_CONFIG=$PREFIX/bin/$CROSSTARGET-pkg-config"
	[ -z "$PKG_CONFIG_PATH" ] && export PKG_CONFIG_PATH=$PREFIX/$CROSSTARGET/lib/pkgconfig

	export PATH=$HOME/fldigi.MXE/mxe/usr/bin/:$PATH
}

Env_Variables
echo "=================================="
echo CROSSTARGET = $CROSSTARGET
echo PREFIX = $PREFIX
echo LINKCFG = $LINKCFG
echo CROSSCFG = $CROSSCFG
echo PKGCFG = $PKGCFG
echo PKG_CONFIG_PATH = $PKG_CONFIG_PATH
echo PATH = $PATH
echo "=================================="
echo "pkg config files:"
ls $PKG_CONFIG_PATH
echo "=================================="


# Switch to the correct directory to start building
cd $( dirname ${BASH_SOURCE[0]} ) # cd to directory where this script is
cd ..	# go up one level to correct directory

make distclean
autoreconf
automake --add-missing

rm -fv *.exe
rm -fv src/*.exe

./configure \
  $CONFIGUREFLAGS \
  $@ \
  $PKGCFG \
  $CROSSCFG \
  --without-asciidoc \
  --with-ptw32=$PREFIX/$CROSSTARGET \
  --with-libiconv-prefix=$PREFIX/iconv \
  --enable-static \
  --with-libintl-prefix=$PREFIX/gettext \
  PTW32_LIBS="-lpthread -lpcreposix -lpcre -lregex" \
  FLTK_CONFIG=$PREFIX/bin/$CROSSTARGET-fltk-config \
  || exit 1

make -j 4 || exit 1 

$PREFIX/bin/$CROSSTARGET-strip src/fldigi.exe
$PREFIX/bin/$CROSSTARGET-strip src/flarq.exe

make nsisinst

mv src/*setup.exe .
ls -l *setup.exe

