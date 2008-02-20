AC_DEFUN([AC_FLDIGI_RDYNAMIC], [
  LDFLAGS_saved="$LDFLAGS"
  LDFLAGS="$LDFLAGS -rdynamic"

  AC_MSG_CHECKING([whether $CC supports -rdynamic])
  AC_TRY_LINK([], [], [ac_cv_rdynamic=yes], [ac_cv_rdynamic=no])
  AC_MSG_RESULT([$ac_cv_rdynamic])

  AC_LANG_PUSH(C++)
    AC_MSG_CHECKING([whether $CXX supports -rdynamic])
    AC_TRY_LINK([], [], [ac_cv_rdynamic=yes], [ac_cv_rdynamic=no])
    AC_MSG_RESULT([$ac_cv_rdynamic])
  AC_LANG_POP(C++)

  LDFLAGS="$LDFLAGS_saved"
])


AC_DEFUN([AC_FLDIGI_DEBUG], [
  AC_ARG_ENABLE([debug],
                AC_HELP_STRING([--enable-debug], [turn on debugging]),
                [case "${enableval}" in
                  yes|no) ac_cv_debug="${enableval}" ;;
                  *)      AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
                 esac],
                 [ac_cv_debug=no])
  if test "x$ac_cv_debug" = "xyes"; then
      if test "x$GXX" = "xyes"; then
          CXXFLAGS="-O0 -ggdb3 -Wall"
      else
          CXXFLAGS="-O0 -g -Wall"
      fi

      AC_FLDIGI_RDYNAMIC()
      if test "x$ac_cv_rdynamic" = "xyes"; then
          RDYNAMIC=-rdynamic
      fi

      CFLAGS="$CXXFLAGS"
      AC_MSG_NOTICE([debugging enabled; overriding CXXFLAGS])
  fi
  AM_CONDITIONAL([ENABLE_DEBUG], [test "x$ac_cv_debug" = "xyes"])
  AC_SUBST([RDYNAMIC])
])
