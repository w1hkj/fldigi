AC_DEFUN([AC_FLDIGI_XMLRPC_CONFIG], [
  ac_cv_xmlrpc=no

  if test "x$XMLRPC_CFLAGS" != "x" && test "x$XMLRPC_LIBS" != "x"; then
      ac_cv_xmlrpc=yes
  else
      if test "x$XMLRPC_C_CONFIG" = "x"; then
          AC_PATH_PROG([XMLRPC_C_CONFIG], [xmlrpc-c-config], [no])
      fi
      if test "x$XMLRPC_C_CONFIG" != "xno" && $XMLRPC_C_CONFIG c++2 abyss-server; then
          ac_cv_xmlrpc=yes

          test "x$XMLRPC_CFLAGS" = "x" && XMLRPC_CFLAGS=`$XMLRPC_C_CONFIG c++2 abyss-server --cflags`
          if test "x$XMLRPC_LIBS" = "x"; then
              XMLRPC_LIBS=`$XMLRPC_C_CONFIG c++2 abyss-server --ldadd`
              test "$ac_cv_static" = "yes" && XMLRPC_LIBS="-Wl,-Bstatic $XMLRPC_LIBS -Wl,-Bdynamic"
          fi
      fi
  fi
])

AC_DEFUN([AC_FLDIGI_XMLRPC], [
  AC_ARG_VAR([XMLRPC_C_CONFIG], [Path to xmlrpc-c-config utility])
  AC_ARG_VAR([XMLRPC_CFLAGS], [C compiler flags for libxmlrpc-c, overrriding xmlrpc-c-config])
  AC_ARG_VAR([XMLRPC_LIBS], [linker flags for libxmlrpc-c, overrriding xmlrpc-c-config])

  AC_ARG_WITH([xmlrpc],
              AC_HELP_STRING([--with-xmlrpc], [enable xmlrpc server support @<:@autodetect@:>@]),
              [case "${withval}" in
                yes|no) ac_cv_want_xmlrpc="${withval}" ;;
                *)      AC_MSG_ERROR([bad value "${withval}" for --with-xmlrpc]) ;;
               esac],
               [ac_cv_want_xmlrpc=check])

  if test "x$ac_cv_want_xmlrpc" = "xno"; then
      AC_DEFINE(USE_XMLRPC, 0, [Define to 1 if we are using xmlrpc])
      ac_cv_xmlrpc=no
  else
      AC_FLDIGI_XMLRPC_CONFIG
      if test "x$ac_cv_want_xmlrpc" = "xcheck"; then
          if test "x$ac_cv_xmlrpc" = "xyes"; then
              AC_DEFINE(USE_XMLRPC, 1, [Define to 1 if we are using xmlrpc])
          else
              AC_DEFINE(USE_XMLRPC, 0, [Define to 1 if we are using xmlrpc])
          fi
      else # $ac_cv_want_xmlrpc is yes
          if test "x$ac_cv_xmlrpc" = "xno"; then
              AC_MSG_FAILURE([--with-xmlrpc was given, but check for libxmlrpc-c failed])
          else
              AC_DEFINE(USE_XMLRPC, 1, [Define to 1 if we are using xmlrpc])
          fi
      fi
  fi

  if test "x$ac_cv_xmlrpc" = "xyes"; then
      AC_DEFINE_UNQUOTED([XMLRPC_BUILD_VERSION], ["`$XMLRPC_C_CONFIG --version`"], [XMLRPC-C version])
  else
      AC_DEFINE_UNQUOTED([XMLRPC_BUILD_VERSION], [""], [XMLRPC-C version])
  fi
  AM_CONDITIONAL([ENABLE_XMLRPC], [test "x$ac_cv_xmlrpc" = "xyes"])
])
