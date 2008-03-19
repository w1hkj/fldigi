# name, version, optional?, opt-prefix, subst-var-prefix, help-text, [am-cond]
AC_DEFUN([AC_FLDIGI_PKG_CHECK], [

if test "x$3" = "xyes"; then
    AC_ARG_WITH($1, AC_HELP_STRING([--[]$4-[]$1], [$6]),
                    [case "${withval}" in
                       yes|no) ac_cv_want_[]$1="${withval}" ;;
                       *)      AC_MSG_ERROR([bad value "${withval}" for --[]$4-[]$1]) ;;
                     esac],
                [ac_cv_want_[]$1=check])
else
    ac_cv_want_[]$1=yes
fi

if test "x$ac_cv_want_[]$1" = "xno"; then
    AC_DEFINE([USE_][$5], 0)
    ac_cv_[]$1=no
else
    PKG_CHECK_EXISTS([$2], ac_cv_[]$1=yes, ac_cv_[]$1=no)
    if test "x$ac_cv_want_[]$1" = "xcheck"; then
        PKG_CHECK_MODULES([$5], [$2], [:], [:])
        if test "x$ac_cv_[]$1" = "xyes"; then
            AC_DEFINE([USE_][$5], 1, [Define to 1 if we are using [$1]])
        else
            AC_DEFINE([USE_][$5], 0, [Define to 1 if we are using [$1]])
        fi
    else # $ac_cv_want_[]$1 is yes
        if test "x$ac_cv_[]$1" = "xno"; then
            if test "x$3" = "xyes"; then
                AC_MSG_NOTICE([--[]$4-[]$1 was given, but test for [$1] failed])
            fi
        else
            AC_DEFINE([USE_][$5], 1, [Define to 1 if we are using [$1]])
        fi
        PKG_CHECK_MODULES([$5], [$2]) # for the error message
    fi
fi
AC_SUBST([$5][_CFLAGS])
AC_SUBST([$5][_LIBS])

m4_ifval([$7], [ AM_CONDITIONAL([$7], [test "x$ac_cv_[]$1" = "xyes"]) ], [:])
])
