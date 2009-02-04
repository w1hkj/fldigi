AC_DEFUN([AC_FLDIGI_BENCHMARK], [
  AC_ARG_ENABLE([benchmark],
                AC_HELP_STRING([--enable-benchmark], [build for benchmark-only operation]),
                [case "${enableval}" in
                  yes|no) ac_cv_benchmark="${enableval}" ;;
                  *)      AC_MSG_ERROR([bad value ${enableval} for --enable-benchmark]) ;;
                 esac],
                 [ac_cv_benchmark=no])

  if test "x$ac_cv_benchmark" = "xyes"; then
      AC_DEFINE(BENCHMARK_MODE, 1, [Defined if we are building for benchmarking])
  else
      AC_DEFINE(BENCHMARK_MODE, 0, [Defined if we are building for benchmarking])
  fi

  AM_CONDITIONAL([ENABLE_BENCHMARK], [test "x$ac_cv_benchmark" = "xyes"])
])
