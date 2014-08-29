dnl Look for a working std::hash or std::tr1::hash.

AC_DEFUN([AC_FLDIGI_HASH], [
  AC_LANG_PUSH(C++)
  AC_MSG_CHECKING([for std::hash in <unordered_map>])
  AC_COMPILE_IFELSE( [AC_LANG_PROGRAM([[#include <unordered_map>
                                        void f(void) { }]],
                                      [[std::hash<int>()(1);]])],
                     [ac_cv_std_hash=yes], [ac_cv_std_hash=no] )
  AC_MSG_RESULT([$ac_cv_std_hash])
  if test "x$ac_cv_std_hash" = "xyes"; then
      AC_DEFINE(HAVE_STD_HASH, 1, [Define to 1 if we have std::hash in <unordered_map>])
  else
      AC_DEFINE(HAVE_STD_HASH, 0, [Define to 0 if we do not have std::hash in <unordered_map>])
  fi

  if test "x$ac_cv_std_hash" = "xno"; then
      AC_MSG_CHECKING([for std::tr1::hash in <tr1/unordered_map>])
      AC_COMPILE_IFELSE( [AC_LANG_PROGRAM([[#include <tr1/unordered_map>
                                            void f(void) { }]],
                                          [[std::tr1::hash<int>()(1);]])],
                         [ac_cv_std_tr1_hash=yes], [ac_cv_std_tr1_hash=no] )
      AC_MSG_RESULT([$ac_cv_std_tr1_hash])
      if test "x$ac_cv_std_tr1_hash" = "xyes"; then
          AC_DEFINE(HAVE_STD_TR1_HASH, 1, [Define to 1 if we have std::tr1::hash in <tr1/unordered_map>])
      else
          AC_DEFINE(HAVE_STD_TR1_HASH, 0, [Define to 1 if we do not have std::tr1::hash in <tr1/unordered_map>])
      fi
  fi
  AC_LANG_POP(C++)

  if test "x$ac_cv_std_hash" = "xno" && test "x$ac_cv_std_tr1_hash" = "xno"; then
      AC_MSG_ERROR([Could not find std::hash or std::tr1::hash])
  fi
])
