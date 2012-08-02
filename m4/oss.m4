AC_DEFUN([AC_FLDIGI_OSS], [
  AC_REQUIRE([AC_FLDIGI_MACOSX])
  AC_REQUIRE([AC_FLDIGI_WIN32])
  if test "x$target_darwin" = "xno" && test "x$target_win32" = "xno"; then
      AC_ARG_ENABLE([oss],
                    AC_HELP_STRING([--disable-oss], [disable support for OSS @<:@autodetect@:>@]),
                    [case "${enableval}" in
                      yes|no) ac_cv_want_oss="${enableval}" ;;
                      *)      AC_MSG_ERROR([bad value "${enableval}" for --disable-oss]) ;;
                     esac],
                     [ac_cv_want_oss=check])
  else
      AC_MSG_NOTICE([disabling OSS driver on $target_os])
      ac_cv_want_oss=no
  fi

  ac_cv_oss=no
  if test "x$ac_cv_want_oss" = "xno"; then
      AC_DEFINE(USE_OSS, 0, [Defined if we are using OSS])
  else
      AC_CHECK_HEADER( [sys/soundcard.h], [ac_cv_oss=yes],
                       [AC_CHECK_HEADER([machine/soundcard.h], [ac_cv_oss=yes],
		       [AC_CHECK_HEADER([soundcard.h], [ac_cv_oss=yes], [])])])
      if test "x$ac_cv_want_oss" = "xcheck"; then
          if test "x$ac_cv_oss" = "xyes"; then
              AC_DEFINE(USE_OSS, 1, [Defined if we are using OSS])
          else
              AC_MSG_NOTICE([disabling OSS driver])
              AC_DEFINE(USE_OSS, 0, [Defined if we are using OSS])
          fi
      else # $ac_cv_want_oss is yes
          if test "x$ac_cv_oss" = "xno"; then
              AC_MSG_FAILURE([--enable-oss was given, but test for OSS failed])
          else
              AC_DEFINE(USE_OSS, 1, [Defined if we are using OSS])
          fi
      fi
  fi
])

