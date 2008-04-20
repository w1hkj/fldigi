# name, version, show-in-help?, optional?, help-text, [am-cond]
AC_DEFUN([AC_FLDIGI_PKG_CHECK], [

m4_define([PKG_NAME_UC], m4_translit([$1], [a-z], [A-Z]))

m4_if([$3], [yes],
    [ AC_ARG_WITH($1, AC_HELP_STRING([--with-[]$1], [$5]),
                     [case "${withval}" in
                        yes|no) ac_cv_want_[]$1="${withval}" ;;
                        *)      AC_MSG_ERROR([bad value "${withval}" for --with-[]$1]) ;;
                      esac],
                 [ac_cv_want_[]$1=check])
    ])
m4_if([$4], [yes], [ac_cv_want_[]$1=check], [ac_cv_want_[]$1=yes])

if test "x$ac_cv_want_[]$1" = "xno"; then
    AC_DEFINE([USE_]PKG_NAME_UC, 0)
    ac_cv_[]$1=no
else
    PKG_CHECK_EXISTS([$2], ac_cv_[]$1=yes, ac_cv_[]$1=no)
    if test "x$ac_cv_want_[]$1" = "xcheck"; then
        PKG_CHECK_MODULES(PKG_NAME_UC, [$2], [:], [:])
        if test "x$ac_cv_[]$1" = "xyes"; then
            AC_DEFINE([USE_]PKG_NAME_UC, 1, [Define to 1 if we are using $1])
        else
            AC_DEFINE([USE_]PKG_NAME_UC, 0, [Define to 1 if we are using $1])
        fi
    else # $ac_cv_want_[]$1 is yes
        if test "x$ac_cv_[]$1" = "xno"; then
            if test "x$3" = "xyes"; then
                AC_MSG_NOTICE([--with-[]$1 was given, but test for $1 failed])
            fi
        else
            AC_DEFINE([USE_]PKG_NAME_UC, 1, [Define to 1 if we are using $1])
        fi
        PKG_CHECK_MODULES(PKG_NAME_UC, $2) # for the error message
    fi
fi
AC_SUBST(PKG_NAME_UC[_CFLAGS])
AC_SUBST(PKG_NAME_UC[_LIBS])

m4_ifval([$6], [ AM_CONDITIONAL([$6], [test "x$ac_cv_[]$1" = "xyes"]) ], [:])
])
