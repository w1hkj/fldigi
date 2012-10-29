AC_DEFUN([AC_FLDIGI_SH_DQ], [
  ac_sh_dq="\"`$1 | sed 's/"/\\\\"/g'`\""
])

AC_DEFUN([AC_FLDIGI_BUILD_INFO], [
# Define build flags and substitute in Makefile.in
# CPPFLAGS
  FLDIGI_BUILD_CPPFLAGS="-I\$(srcdir) -I\$(srcdir)/include \
-I\$(srcdir)/irrxml \
-I\$(srcdir)/libtiniconv \
-I\$(srcdir)/fileselector \
-I\$(srcdir)/xmlrpcpp"
# CXXFLAGS
  FLDIGI_BUILD_CXXFLAGS="$PORTAUDIO_CFLAGS $FLTK_CFLAGS $X_CFLAGS $SNDFILE_CFLAGS $SAMPLERATE_CFLAGS \
$PULSEAUDIO_CFLAGS $HAMLIB_CFLAGS $PNG_CFLAGS $XMLRPC_CFLAGS $MAC_UNIVERSAL_CFLAGS \
$INTL_CFLAGS $PTW32_CFLAGS $BFD_CFLAGS -pipe -Wall -fexceptions $OPT_CFLAGS $DEBUG_CFLAGS"
  if test "x$target_mingw32" = "xyes"; then
      FLDIGI_BUILD_CXXFLAGS="-mthreads $FLDIGI_BUILD_CXXFLAGS"
  fi
# LDFLAGS
  FLDIGI_BUILD_LDFLAGS="$MAC_UNIVERSAL_LDFLAGS"
  if test "x$target_mingw32" = "xyes"; then
      FLDIGI_BUILD_LDFLAGS="-mthreads $FLDIGI_BUILD_LDFLAGS"
  fi
# LDADD
  FLDIGI_BUILD_LDADD="$PORTAUDIO_LIBS $FLTK_LIBS $X_LIBS $SNDFILE_LIBS $SAMPLERATE_LIBS \
$PULSEAUDIO_LIBS $HAMLIB_LIBS $PNG_LIBS $XMLRPC_LIBS $INTL_LIBS $PTW32_LIBS $BFD_LIBS $EXTRA_LIBS"

# CPPFLAGS
  FLARQ_BUILD_CPPFLAGS="-I\$(srcdir) -I\$(srcdir)/include -I\$(srcdir)/fileselector \
-I\$(srcdir)/flarq-src -I\$(srcdir)/flarq-src/include"
# CXXFLAGS
  FLARQ_BUILD_CXXFLAGS="$FLTK_CFLAGS $X_CFLAGS $MAC_UNIVERSAL_CFLAGS $INTL_CFLAGS $PTW32_CFLAGS \
$BFD_CFLAGS -pipe -Wall -fexceptions $OPT_CFLAGS $DEBUG_CFLAGS"
  if test "x$target_mingw32" = "xyes"; then
      FLARQ_BUILD_CXXFLAGS="-mthreads $FLARQ_BUILD_CXXFLAGS"
  fi
# LDFLAGS
  FLARQ_BUILD_LDFLAGS="$MAC_UNIVERSAL_LDFLAGS"
  if test "x$target_mingw32" = "xyes"; then
      FLARQ_BUILD_LDFLAGS="-mthreads $FLARQ_BUILD_LDFLAGS"
  fi
# LDADD
  FLARQ_BUILD_LDADD="$FLTK_LIBS $X_LIBS $INTL_LIBS $PTW32_LIBS $BFD_LIBS $EXTRA_LIBS"

  if test "x$ac_cv_debug" = "xyes"; then
      FLDIGI_BUILD_CXXFLAGS="$FLDIGI_BUILD_CXXFLAGS -UNDEBUG"
      FLDIGI_BUILD_LDFLAGS="$FLDIGI_BUILD_LDFLAGS $RDYNAMIC"
      FLARQ_BUILD_CXXFLAGS="$FLARQ_BUILD_CXXFLAGS -UNDEBUG"
      FLARQ_BUILD_LDFLAGS="$FLARQ_BUILD_LDFLAGS $RDYNAMIC"
  else
      FLDIGI_BUILD_CXXFLAGS="$FLDIGI_BUILD_CXXFLAGS -DNDEBUG"
      FLARQ_BUILD_CXXFLAGS="$FLARQ_BUILD_CXXFLAGS -DNDEBUG"
  fi

  AC_SUBST([FLDIGI_BUILD_CPPFLAGS])
  AC_SUBST([FLDIGI_BUILD_CXXFLAGS])
  AC_SUBST([FLDIGI_BUILD_LDFLAGS])
  AC_SUBST([FLDIGI_BUILD_LDADD])

  AC_SUBST([FLARQ_BUILD_CPPFLAGS])
  AC_SUBST([FLARQ_BUILD_CXXFLAGS])
  AC_SUBST([FLARQ_BUILD_LDFLAGS])
  AC_SUBST([FLARQ_BUILD_LDADD])

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

  AC_FLDIGI_SH_DQ([echo $FLDIGI_BUILD_CPPFLAGS $FLDIGI_BUILD_CXXFLAGS])
  AC_DEFINE_UNQUOTED([FLDIGI_BUILD_CXXFLAGS], [$ac_sh_dq], [Fldigi compiler flags])
  AC_FLDIGI_SH_DQ([echo $FLDIGI_BUILD_LDFLAGS $FLDIGI_BUILD_LDADD])
  AC_DEFINE_UNQUOTED([FLDIGI_BUILD_LDFLAGS], [$ac_sh_dq], [Fldigi linker flags])

  AC_FLDIGI_SH_DQ([echo $FLARQ_BUILD_CPPFLAGS $FLARQ_BUILD_CXXFLAGS])
  AC_DEFINE_UNQUOTED([FLARQ_BUILD_CXXFLAGS], [$ac_sh_dq], [Flarq compiler flags])
  AC_FLDIGI_SH_DQ([echo $FLARQ_BUILD_LDFLAGS $FLARQ_BUILD_LDADD])
  AC_DEFINE_UNQUOTED([FLARQ_BUILD_LDFLAGS], [$ac_sh_dq], [Flarq linker flags])

  if test "x$LC_ALL_saved" != "x"; then
      LC_ALL="$LC_ALL_saved"
      export LC_ALL
  fi
])


# This macro defines SILENT_CMDS, which is @expanded@ in
# {src,doc}/Makefile.am to define a function that generates custom build
# command output depending on the values of the variables
# $(AM_DEFAULT_VERBOSITY) and $(V).  These variables affect the custom
# command output in the same way as they do for automake's build rules.
AC_DEFUN([AC_FLDIGI_BUILD_RULES_SILENT], [
  m4_ifdef([AM_SUBST_NOTMAKE], [AM_SUBST_NOTMAKE([SILENT_CMDS])])
  AC_SUBST([SILENT_CMDS],
           ['silent_cmd = @echo "  $(1)" $(2);
            ifeq ($(AM_DEFAULT_VERBOSITY),0)
                silent = $(if $(subst 0,,$(V)),,$(silent_cmd))
            else
                ifeq ($(V),0)
                    silent = $(silent_cmd)
                endif
            endif'])
])

AC_DEFUN([AC_FLDIGI_BUILD_RULES],
         [m4_ifdef([AM_SILENT_RULES], [AC_FLDIGI_BUILD_RULES_SILENT],
                   [AC_SUBST([SILENT_CMDS], [])])])
