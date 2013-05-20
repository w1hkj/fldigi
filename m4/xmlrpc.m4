AC_DEFUN([AC_FLDIGI_XMLRPC], [
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
      ac_cv_xmlrpc=yes
      AC_DEFINE(USE_XMLRPC, 1, [Define to 1 if we are using xmlrpc])
  fi

  if test "x$ac_cv_xmlrpc" = "xyes"; then
      AC_DEFINE_UNQUOTED([XMLRPC_BUILD_VERSION], ["Builtin"], [XMLRPC-C version])
  else
      AC_DEFINE_UNQUOTED([XMLRPC_BUILD_VERSION], [""], [XMLRPC-C version])
  fi
  AM_CONDITIONAL([ENABLE_XMLRPC], [test "x$ac_cv_xmlrpc" = "xyes"])
])
