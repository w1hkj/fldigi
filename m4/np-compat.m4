AC_DEFUN([AC_FLDIGI_NP_COMPAT], [
  AC_REQUIRE([AC_FLDIGI_MACOSX])
  AC_REQUIRE([AC_FLDIGI_WIN32])


  if test "x$target_mingw32" = "xyes"; then
      # Newer versions of mingw32 comes with pthread.
      sem_libs="pthreadGC2 pthread"
     # pretend that the regex.h check failed so that we use the bundled regex code
     ac_cv_header_regex_h=no
     AC_DEFINE([HAVE_REGEX_H], 0, [Define to 1 if you have the <regex.h> header file.])
  else
      sem_libs="pthread rt"
  fi
  AM_CONDITIONAL([COMPAT_REGEX], [test "x$ac_cv_header_regex_h" != "xyes"])


  other_libs=""

  if test "x$ac_cv_want_ptw32" = "xyes"; then
      CFLAGS_saved="$CFLAGS"
      LDFLAGS_saved="$LDFLAGS"
      EXTRA_LIBS_saved="$EXTRA_LIBS"
      CFLAGS="$CFLAGS $PTW32_CFLAGS"
      LDFLAGS="$LDFLAGS $PTW32_LIBS"
      other_libs=-lws2_32
  fi

  AC_FLDIGI_SEARCH_LIBS([dlopen], [dl], [$other_libs])
  AC_FLDIGI_SEARCH_LIBS([clock_gettime], [rt], [$other_libs])
  AC_FLDIGI_SEARCH_LIBS([sem_unlink], [$sem_libs], [$other_libs])
  AC_FLDIGI_SEARCH_LIBS([sem_timedwait], [$sem_libs], [$other_libs])

  if test "x$ac_cv_want_ptw32" = "xyes"; then
      CFLAGS="$CFLAGS_saved"
      LDFLAGS="$LDFLAGS_saved"
      EXTRA_LIBS="$EXTRA_LIBS_saved"
  fi
  AC_SUBST([EXTRA_LIBS])

  AM_CONDITIONAL([COMPAT_STRCASESTR], [test "x$ac_cv_func_strcasestr" != "xyes"])
])
