AC_DEFUN([AC_FLDIGI_OPT], [
  AC_ARG_ENABLE([optimizations],
		AC_HELP_STRING([--enable-optimizations],
		               [use x86 optimizations (none|sse|sse2|sse3|native) @<:@none@:>@]),
                [case "${enableval}" in
                  none|sse|sse2|sse3|native) ac_cv_opt="${enableval}" ;;
                  *)                         AC_MSG_ERROR([bad value ${enableval} for --enable-optimizations]) ;;
                 esac],
                 [ac_cv_opt=none])
  OPT_CFLAGS="-O2 -ffast-math -finline-functions"
  case "$ac_cv_opt" in
      sse)
          OPT_CFLAGS="$OPT_CFLAGS -msse -mfpmath=sse"
	  ;;
      sse2)
          OPT_CFLAGS="$OPT_CFLAGS -msse2 -mfpmath=sse"
	  ;;
      sse3)
          OPT_CFLAGS="$OPT_CFLAGS -msse3 -mfpmath=sse"
	  ;;
      native)
          OPT_CFLAGS="$OPT_CFLAGS -march=native -mfpmath=sse"
	  ;;
      none)
          ;;
  esac

  AC_SUBST([OPT_CFLAGS])
])
