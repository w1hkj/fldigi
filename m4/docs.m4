AC_DEFUN([AC_FLDIGI_DOCS], [
  AC_ARG_WITH([asciidoc],
              AC_HELP_STRING([--with-asciidoc], [build documentation using asciidoc @<:@autodetect@:>@]),
              [case "${withval}" in
                yes|no) ac_cv_want_asciidoc="${withval}" ;;
                *)      AC_MSG_ERROR([bad value "${withval}" for --with-asciidoc]) ;;
               esac],
               [ac_cv_want_asciidoc=check])

  if test "x$ac_cv_want_asciidoc" != "xno"; then
      AC_PATH_PROG([ASCIIDOC], [asciidoc])
      asciidoc_min=8.2.0
      if test "x$ASCIIDOC" != "x"; then
          AC_MSG_CHECKING([for asciidoc >= $asciidoc_min])
          asciidoc_ver=$($ASCIIDOC --version | sed -n '1 s/.* //; p')
          AX_COMPARE_VERSION([$asciidoc_ver], [ge], [$asciidoc_min],
                             [AC_MSG_RESULT([yes ($asciidoc_ver)])],
                             [AC_MSG_RESULT([no ($asciidoc_ver)]); ASCIIDOC=""])
      fi
      if test "x$ASCIIDOC" != "x"; then
          AC_PATH_PROG([A2X], [a2x])
          AX_COMPARE_VERSION([$asciidoc_ver], [ge], [8.3.0], [no_xmllint=yes], [no_xmllint=no])
      fi
      if test "x$ac_cv_want_asciidoc" = "xyes"; then
          if test "x$ASCIIDOC" = "x" -o "x$A2X" = "x"; then
              AC_MSG_FAILURE([--with-asciidoc was given, but check for asciidoc failed])
          elif test "x$A2X" = "x"; then
              AC_MSG_FAILURE([--with-asciidoc was given, but check for a2x failed])
          fi
      fi
  fi

  AC_SUBST([ASCIIDOC])
  AC_SUBST([A2X])
  AM_CONDITIONAL([HAVE_ASCIIDOC], [test "x$ASCIIDOC" != "x" -a "x$A2X" != "x"])
  AM_CONDITIONAL([HAVE_ASCIIDOC_NO_XMLLINT], [test "$no_xmllint" = "yes"])
])
