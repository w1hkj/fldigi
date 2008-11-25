AC_DEFUN([AC_FLDIGI_INTL], [
  AC_ARG_VAR([INTL_CFLAGS], [C compiler flags for libintl, overriding gettext macros])
  AC_ARG_VAR([INTL_LIBS], [linker flags for libintl, overriding gettext macros])

  if test "x$INTL_LIBS" = "x"; then
      INTL_LIBS="$LIBINTL"
  fi

  AC_SUBST([INTL_CFLAGS])
  AC_SUBST([INTL_LIBS])
])
