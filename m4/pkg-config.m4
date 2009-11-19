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
m4_if([$4], [no], [ac_cv_want_[]$1=yes])

test "x$ac_cv_want_[]$1" = "x" && ac_cv_want_[]$1="check"
case "x$ac_cv_want_[]$1" in
    "xno")
            ac_cv_[]$1=no
            ;;
    "xcheck")
            PKG_CHECK_MODULES(PKG_NAME_UC, [$2], [ac_cv_[]$1=yes], [ac_cv_[]$1=no])
            ;;
    "xyes")
            PKG_CHECK_MODULES(PKG_NAME_UC, [$2])
	    # if we get here the test has succeeded
            ac_cv_[]$1=yes
            ;;
esac

if test "x$ac_cv_[]$1" = "xyes"; then
    AC_DEFINE([USE_]PKG_NAME_UC, 1, [Define to 1 if we are using $1])
    pkg_[]$1_version=`$PKG_CONFIG --modversion "$2" 2>/dev/null`
else
    AC_DEFINE([USE_]PKG_NAME_UC, 0, [Define to 1 if we are using $1])
    pkg_[]$1_version=""
fi

AC_SUBST(PKG_NAME_UC[_CFLAGS])
AC_SUBST(PKG_NAME_UC[_LIBS])
AC_DEFINE_UNQUOTED(PKG_NAME_UC[_BUILD_VERSION], ["$pkg_[]$1_version"], [$1 version])

m4_ifval([$6], [ AM_CONDITIONAL([$6], [test "x$ac_cv_[]$1" = "xyes"]) ], [:])
])
