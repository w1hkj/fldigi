AC_DEFUN([AC_FLDIGI_SH_DQ], [
  ac_sh_dq="\"`$1 | sed 's/"/\\\\"/g'`\""
])

AC_DEFUN([AC_FLDIGI_BUILD_INFO], [
# define build flags and substitute in Makefile.in
  BUILD_CPPFLAGS="$BUILD_CPPFLAGS -I$srcdir -I$srcdir/include -I$srcdir/irrxml \
-I$srcdir/fileselector $BOOST_CPPFLAGS"
  AC_SUBST([BUILD_CPPFLAGS])

  BUILD_CXXFLAGS="$PORTAUDIO_CFLAGS $FLTK_CFLAGS $SNDFILE_CFLAGS $SAMPLERATE_CFLAGS \
$PULSEAUDIO_CFLAGS $HAMLIB_CFLAGS $IMAGE_CFLAGS $XMLRPC_CFLAGS $MAC_UNIVERSAL_CFLAGS \
$INTL_CFLAGS $PTW32_CFLAGS $BFD_CFLAGS -pipe -Wall -fexceptions $OPT_CFLAGS $DEBUG_CFLAGS"
  BUILD_LDFLAGS="$MAC_UNIVERSAL_LDFLAGS"
  if test "x$ac_cv_debug" != "x"; then
      BUILD_CXXFLAGS="$BUILD_CXXFLAGS -UNDEBUG"
      BUILD_LDFLAGS="$BUILD_LDFLAGS $RDYNAMIC"
  else
      BUILD_CXXFLAGS="$BUILD_CXXFLAGS -DNDEBUG"
  fi
  AC_SUBST([BUILD_CXXFLAGS])
  AC_SUBST([BUILD_LDFLAGS])

  BUILD_LDADD="$PORTAUDIO_LIBS $BOOST_LDFLAGS $FLTK_LIBS $SNDFILE_LIBS $SAMPLERATE_LIBS \
$PULSEAUDIO_LIBS $HAMLIB_LIBS $IMAGE_LIBS $XMLRPC_LIBS $INTL_LIBS $PTW32_LIBS $BFD_LIBS $EXTRA_LIBS"
  AC_SUBST([BUILD_LDADD])

#define build variables for config.h
  AC_DEFINE_UNQUOTED([BUILD_BUILD_PLATFORM], ["$build"], [Build platform])
  AC_DEFINE_UNQUOTED([BUILD_HOST_PLATFORM], ["$host"], [Host platform])
  AC_DEFINE_UNQUOTED([BUILD_TARGET_PLATFORM], ["$target"], [Target platform])

  test "x$LC_ALL" != "x" && LC_ALL_saved="$LC_ALL"
  LC_ALL=C
  export LC_ALL

  AC_FLDIGI_SH_DQ([echo $ac_configure_args])
  AC_DEFINE_UNQUOTED([BUILD_CONFIGURE_ARGS], [$ac_sh_dq], [Configure arguments])

  AC_FLDIGI_SH_DQ([date])
  AC_DEFINE_UNQUOTED([BUILD_DATE], [$ac_sh_dq], [Build date])

  AC_FLDIGI_SH_DQ([whoami])
  AC_DEFINE_UNQUOTED([BUILD_USER], [$ac_sh_dq], [Build user])

  AC_FLDIGI_SH_DQ([hostname])
  AC_DEFINE_UNQUOTED([BUILD_HOST], [$ac_sh_dq], [Build host])

  AC_FLDIGI_SH_DQ([$CXX -v 2>&1 | tail -1])
  AC_DEFINE_UNQUOTED([BUILD_COMPILER], [$ac_sh_dq], [Compiler])

  AC_FLDIGI_SH_DQ([echo $BUILD_CPPFLAGS $BUILD_CXXFLAGS])
  AC_DEFINE_UNQUOTED([BUILD_CXXFLAGS], [$ac_sh_dq], [Compiler flags])

  AC_FLDIGI_SH_DQ([echo $BUILD_LDFLAGS $BUILD_LDADD])
  AC_DEFINE_UNQUOTED([BUILD_LDFLAGS], [$ac_sh_dq], [Linker flags])

  if test "x$LC_ALL_saved" != "x"; then
      LC_ALL="$LC_ALL_saved"
      export LC_ALL
  fi
])
