AC_DEFUN([AC_FLDIGI_STATIC], [
  AC_ARG_ENABLE([static],
                AC_HELP_STRING([--enable-static], [enable static linking for some libraries]),
                [case "${enableval}" in
                    yes|no) ac_cv_static="${enableval}" ;;
                    *)      AC_MSG_ERROR([bad value ${enableval} for --enable-static]) ;;
                 esac],
                 [ac_cv_static=no])
  if test "x$ac_cv_static" = "xyes"; then
      AC_CHECK_LIB([rt], [clock_gettime], [RTLIB=-lrt])
  fi
  AC_SUBST([RTLIB])
])
