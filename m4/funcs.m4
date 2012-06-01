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

# ---------------------------------------------------------------------------
# Macro: FCNTL_FLAGS
# ---------------------------------------------------------------------------

AC_DEFUN([AC_FCNTL_FLAGS],
[
  AC_CACHE_CHECK([for O_CLOEXEC], [ac_cv_o_cloexec], [
    AC_LANG_PUSH([C])
    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -I${srcdir}"

    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <fcntl.h>], [ int flags= O_CLOEXEC])], [ac_cv_o_cloexec="yes"], [ac_cv_o_cloexec="no"])
    AC_LANG_POP
  ])

  AS_IF([test "x$ac_cv_o_cloexec" = "xyes"],[ AC_DEFINE(HAVE_O_CLOEXEC, 1, [Define to 1 if you have O_CLOEXEC defined])])
])

# ---------------------------------------------------------------------------
# End Macro: FCNTL_FLAGS
# ---------------------------------------------------------------------------
