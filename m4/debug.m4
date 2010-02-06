AC_DEFUN([AC_FLDIGI_RDYNAMIC], [
  LDFLAGS_saved="$LDFLAGS"
  LDFLAGS="$LDFLAGS -rdynamic"

  AC_MSG_CHECKING([whether $CC supports -rdynamic])
  # don't try this on win32; it succeeds but emits a warning at link time
  if test "x$target_win32" != "xyes"; then
      AC_TRY_LINK([], [], [ac_cv_rdynamic=yes], [ac_cv_rdynamic=no])
  else
      ac_cv_rdynamic=no
  fi
  AC_MSG_RESULT([$ac_cv_rdynamic])

  AC_LANG_PUSH(C++)
    AC_MSG_CHECKING([whether $CXX supports -rdynamic])
    if test "x$target_win32" != "xyes"; then
        AC_TRY_LINK([], [], [ac_cv_rdynamic=yes], [ac_cv_rdynamic=no])
    else
        ac_cv_rdynamic=no
    fi
  AC_LANG_POP(C++)
  AC_MSG_RESULT([$ac_cv_rdynamic])

  LDFLAGS="$LDFLAGS_saved"
])


AC_DEFUN([AC_FLDIGI_DEBUG], [
  AC_REQUIRE([AC_FLDIGI_OPT])
  AC_ARG_ENABLE([debug],
                AC_HELP_STRING([--enable-debug], [turn on debugging]),
                [case "${enableval}" in
                  yes|no) ac_cv_debug="${enableval}" ;;
                  *)      AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
                 esac],
                 [ac_cv_debug=no])

  AC_ARG_VAR([BFD_CFLAGS], [C compiler flags for libbfd])
  AC_ARG_VAR([BFD_LIBS], [linker flags for libbfd])
  AC_ARG_WITH([bfd],
              AC_HELP_STRING([--with-bfd@<:@=DIR@:>@],
                             [search for libbfd in DIR/include and DIR/lib @<:@mingw32 only@:>@]),
              [ac_cv_want_bfd="$withval"],
              [ac_cv_want_bfd=yes])

  if test "x$ac_cv_debug" = "xyes" && test "x$ac_cv_want_bfd" != "xno" && \
     test "x$target_mingw32" = "xyes"; then
      if test "x$ac_cv_want_bfd" != "xyes"; then # set -I and -L switches
          bfd_default_cflags="-I${ac_cv_want_bfd}/include"
          bfd_default_libs="-L${ac_cv_want_bfd}/lib"
      fi
      # don't override the user-specified vars
      BFD_CFLAGS="${BFD_CFLAGS:-$bfd_default_cflags}"
      BFD_LIBS="${BFD_LIBS:-$bfd_default_libs}"
      ac_cv_want_bfd="yes"

      CPPFLAGS_saved="$CPPFLAGS"
      LDFLAGS_saved="$LDFLAGS"
      CPPFLAGS="$CPPFLAGS $BFD_CFLAGS"
      LDFLAGS="$LDFLAGS $BFD_LIBS"

      AC_CHECK_HEADER([bfd.h], [ac_cv_have_bfd="yes"], [ac_cv_have_bfd="no"])
      if test "x$ac_cv_have_bfd" = "xyes"; then
          bfd_other_libs="-liberty -lpsapi -limagehlp"
          AC_CHECK_LIB([bfd], [bfd_set_format], [ac_cv_have_bfd="yes"], [ac_cv_have_bfd="no"], [$bfd_other_libs])
      fi
      CPPFLAGS="$CPPFLAGS_saved"
      LDFLAGS="$LDFLAGS_saved"
      if test "x$ac_cv_have_bfd" = "xyes"; then
          BFD_LIBS="$BFD_LIBS -lbfd $bfd_other_libs"
      fi
  fi

  if test "x$ac_cv_debug" = "xyes"; then
      AC_MSG_CHECKING([for debug info flag])
      if test "x$GXX" = "xyes"; then
          if test "x$target_mingw32" = "xyes"; then
              gflag="-gstabs"
          else
              gflag="-ggdb3"
          fi
      else
          gflag="-g"
      fi
      AC_MSG_RESULT([$gflag])
      DEBUG_CFLAGS="-O0 -fno-inline-functions $gflag -Wall"

      AC_FLDIGI_RDYNAMIC
      if test "x$ac_cv_rdynamic" = "xyes"; then
          RDYNAMIC=-rdynamic
      fi

      CXXFLAGS=""
      CFLAGS="$CXXFLAGS"
      AC_MSG_NOTICE([debugging enabled; overriding CXXFLAGS])

      if test "x$target_darwin" = "xyes"; then
          ac_cv_compat_stack=yes
	  AC_DEFINE([HAVE_DBG_STACK], 1, [Define to 1 if we have dbg::stack])
      elif test "x$target_mingw32" = "xyes" && test "x$ac_cv_have_bfd" = "xyes"; then
          ac_cv_compat_stack=yes
	  AC_DEFINE([HAVE_DBG_STACK], 1, [Define to 1 if we have dbg::stack])
      else
          ac_cv_compat_stack=no
	  AC_DEFINE([HAVE_DBG_STACK], 0, [Define to 0 if not using dbg::stack])
      fi
  fi
  AM_CONDITIONAL([ENABLE_DEBUG], [test "x$ac_cv_debug" = "xyes"])
  AC_SUBST([DEBUG_CFLAGS])
  AC_SUBST([RDYNAMIC])

  AM_CONDITIONAL([COMPAT_STACK], [test "x$ac_cv_compat_stack" = "xyes"])
])
