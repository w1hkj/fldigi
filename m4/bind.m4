dnl Look for a working std::bind or std::tr1::bind.

AC_DEFUN([AC_FLDIGI_BIND], [
  AC_LANG_PUSH(C++)
  AC_MSG_CHECKING([for std::bind in <functional>])
  AC_COMPILE_IFELSE( [AC_LANG_PROGRAM([[#include <functional>
                                        void f(void) { }]],
                                      [[std::bind(f)();]])],
                     [ac_cv_std_bind=yes], [ac_cv_std_bind=no] )
  AC_MSG_RESULT([$ac_cv_std_bind])
  if test "x$ac_cv_std_bind" = "xyes"; then
      AC_DEFINE(HAVE_STD_BIND, 1, [Define to 1 if we have std::bind in <functional>])
  else
      AC_DEFINE(HAVE_STD_BIND, 0, [Define to 1 if we have std::bind in <functional>])
  fi

  if test "x$ac_cv_std_bind" = "xno"; then
      AC_MSG_CHECKING([for std::tr1::bind in <tr1/functional>])
      AC_COMPILE_IFELSE( [AC_LANG_PROGRAM([[#include <tr1/functional>
                                            void f(void) { }]],
                                          [[std::tr1::bind(f)();]])],
                         [ac_cv_std_tr1_bind=yes], [ac_cv_std_tr1_bind=no] )
      AC_MSG_RESULT([$ac_cv_std_tr1_bind])
      if test "x$ac_cv_std_tr1_bind" = "xyes"; then
          AC_DEFINE(HAVE_STD_TR1_BIND, 1, [Define to 1 if we have std::tr1::bind in <tr1/functional>])
      else
          AC_DEFINE(HAVE_STD_TR1_BIND, 0, [Define to 1 if we have std::tr1::bind in <tr1/functional>])
      fi
  fi
  AC_LANG_POP(C++)

  if test "x$ac_cv_std_bind" = "xno" && test "x$ac_cv_std_tr1_bind" = "xno"; then
      AC_MSG_ERROR([Could not find std::bind or std::tr1::bind])
  fi
])
