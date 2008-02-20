AC_DEFUN([AC_FLDIGI_SAMPLERATE], [
  PKG_CHECK_MODULES(SAMPLERATE, samplerate >= 0.1.1, ac_cv_samplerate=yes, ac_cv_samplerate=no)
  if test "x$ac_cv_samplerate" = "xno"; then
      AC_MSG_WARN([using bundled libsamplerate])
  fi
  AC_SUBST([SAMPLERATE_CFLAGS])
  AC_SUBST([SAMPLERATE_LIBS])
  AM_CONDITIONAL([NO_SAMPLERATE], [test "x$ac_cv_samplerate" = "xno"])
])
