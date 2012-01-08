AC_DEFUN([AC_FLDIGI_WIN32], [
case "$target_os" in
     *mingw*)
        target_win32="yes"
        target_mingw32="yes"
        ;;
     *cygwin*|*win32*|*w32*)
        target_win32="yes"
        ;;
     *)
        target_win32="no"
        ;;
esac

if test "x$target_win32" = "xyes"; then
    AC_CHECK_PROG([WINDRES], [${ac_tool_prefix}windres], [${ac_tool_prefix}windres])
    if [ test "x$WINDRES" = "x" ]; then
        AC_MSG_WARN([The windres utility could not be found])
    fi
    AC_DEFINE([__WOE32__], 1, [Define to 1 if we are building on cygwin or mingw])
    AC_DEFINE([__MINGW32__], 1, [Define to 1 if we are building on cygwin or mingw])
    AC_DEFINE([_WINDOWS], 1, [Define to 1 if we are building on cygwin or mingw])
fi

if test "x$target_mingw32" = "xyes"; then
    AC_CHECK_PROG([MAKENSIS], [makensis], [makensis])
fi

AC_SUBST([WINDRES])
AM_CONDITIONAL([HAVE_WINDRES], [test "x$WINDRES" != "x"])
AC_SUBST([MAKENSIS])
AM_CONDITIONAL([HAVE_NSIS], [test "x$MAKENSIS" != "x"])
AM_CONDITIONAL([WIN32], [test "x$target_win32" = "xyes"])
AM_CONDITIONAL([MINGW32], [test "x$target_mingw32" = "xyes"])


AC_ARG_VAR([PTW32_CFLAGS], [C compiler flags for pthreads-w32])
AC_ARG_VAR([PTW32_LIBS], [linker flags for pthreads-w32])

AC_ARG_WITH([ptw32],
            AC_HELP_STRING([--with-ptw32@<:@=DIR@:>@],
                           [search for pthreads-w32 in DIR/include and DIR/lib @<:@mingw32 only@:>@]),
            [ac_cv_want_ptw32="$withval"],
            [ac_cv_want_ptw32=no])

if test "x$ac_cv_want_ptw32" != "xno"; then
    if test "x$ac_cv_want_ptw32" != "xyes"; then # set -I and -L switches
        ptw32_default_cflags="-I${ac_cv_want_ptw32}/include"
        ptw32_default_libs="-L${ac_cv_want_ptw32}/lib"
    fi
    ptw32_default_libs="$ptw32_default_libs -lpthreadGC2 -lws2_32"

    # don't override the user-specified vars
    PTW32_CFLAGS="${PTW32_CFLAGS:-$ptw32_default_cflags}"
    PTW32_LIBS="${PTW32_LIBS:-$ptw32_default_libs}"
    ac_cv_want_ptw32="yes"
fi

AC_SUBST([PTW32_CFLAGS])
AC_SUBST([PTW32_LIBS])
])
