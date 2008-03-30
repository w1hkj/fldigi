AC_DEFUN([AC_FLDIGI_WIN32], [
case "$target_os" in
     *cygwin*|*mingw*|*win32*|*w32*)
        target_win32="yes"
        ;;
     *)
        target_win32="no"
        ;;
esac

if test "x$target_win32" = "xyes"; then
    AC_CHECK_PROG([WINDRES], [windres], [windres])
    if [ test "x$WINDRES" = "x" ]; then
        AC_MSG_WARN([The windres utility could not be found])
    fi
fi

AC_SUBST([WINDRES])
AM_CONDITIONAL([HAVE_WINDRES], [test "x$WINDRES" != "x"])
AM_CONDITIONAL([WIN32], [test "x$target_win32" = "xyes"])
])