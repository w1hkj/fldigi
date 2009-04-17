# args: function, search-libs, other-libraries
AC_DEFUN([AC_FLDIGI_SEARCH_LIBS], [

m4_define([FUNC_NAME_UC], m4_translit([$1], [a-z], [A-Z]))

LIBS_search_libs_save="$LIBS"
LIBS=""
AC_SEARCH_LIBS($1, $2, [ac_cv_have_func_[]$1=1], [ac_cv_have_func_[]$1=0], $3)
if test "x$LIBS" != "x"; then
    echo "$EXTRA_LIBS" | grep -q -e "$LIBS" || EXTRA_LIBS="$EXTRA_LIBS $LIBS"
fi
AC_DEFINE_UNQUOTED([HAVE_]FUNC_NAME_UC, $ac_cv_have_func_[]$1, [Define to 1 if we have $1])
LIBS="$LIBS_search_libs_save"

])
