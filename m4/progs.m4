AC_DEFUN([AC_FLDIGI_PROGRAMS], [
  AC_ARG_ENABLE([fldigi],
                AC_HELP_STRING([--disable-fldigi], [do not build fldigi]),
                [case "${enableval}" in
                  yes|no) ac_cv_want_fldigi="${enableval}" ;;
                  *)      AC_MSG_ERROR([bad value ${enableval} for --disable-fldigi]) ;;
                 esac],
                 [ac_cv_want_fldigi=yes])

  AM_CONDITIONAL([WANT_FLDIGI], [test "x$ac_cv_want_fldigi" = "xyes"])

  AC_ARG_ENABLE([flarq],
                AC_HELP_STRING([--disable-flarq], [do not build flarq]),
                [case "${enableval}" in
                  yes|no) ac_cv_want_flarq="${enableval}" ;;
                  *)      AC_MSG_ERROR([bad value ${enableval} for --disable-flarq]) ;;
                 esac],
                 [ac_cv_want_flarq=yes])

  AM_CONDITIONAL([WANT_FLARQ], [test "x$ac_cv_want_flarq" = "xyes"])
])
