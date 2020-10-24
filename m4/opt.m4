AC_DEFUN([AC_FLDIGI_OPT], [
  AC_ARG_ENABLE([optimizations],
		AC_HELP_STRING([--enable-optimizations],
		               [use CPU optimizations (none|sse|sse2|sse3|sse4|i486|i686|avx|avx2|x86-64|rpi1|rpi2|rpi3|rpi4|bbb|cyclone5|nativeARM|native) @<:@none@:>@]),
                [case "${enableval}" in
                  none|sse|sse2|sse3|sse4|i486|i686|avx|avx2|x86-64|rpi1|rpi2|rpi3|rpi4|bbb|cyclone5|nativeARM|native)
	                  ac_cv_opt="${enableval}"
	                  ;;
                  *)
	                  AC_MSG_WARN([possible values: none|sse|sse2|sse3|sse4|avx|avx2|i486|i686|x86-64|native])
	                  AC_MSG_WARN([possible values: none|rpi1|rpi2|rpi3|rpi4|bbb|cyclone5|nativeARM])
	                  AC_MSG_ERROR([bad value ${enableval} for --enable-optimizations])
	                  ;;
                 esac],
                 [ac_cv_opt=none])
  
  OPT_CFLAGS="-O2"
  OPT_CFLAGS_X86="-O2 -mno-3dnow"
  OPT_CFLAGS_ARM="-O2 -mfloat-abi=hard"
  
  case "$ac_cv_opt" in
      sse)
          OPT_CFLAGS="$OPT_CFLAGS_x86 -mmmx -msse -mfpmath=sse"
		  ;;
      sse2)
          OPT_CFLAGS="$OPT_CFLAGS_x86 -mmmx -msse -msse2 -mfpmath=sse"
		  ;;
      sse3)
          OPT_CFLAGS="$OPT_CFLAGS_x86 -mmmx -msse -msse2 -msse3 -mfpmath=sse"
		  ;;
      sse4)
          OPT_CFLAGS="$OPT_CFLAGS_x86 -mmmx -msse -msse2 -msse3 -msse4 -mfpmath=sse"
		  ;;
      avx)
          OPT_CFLAGS="$OPT_CFLAGS_x86 -mavx -mfpmath=sse"
		  ;;
      avx2)
          OPT_CFLAGS="$OPT_CFLAGS_x86 -mavx2 -mfpmath=sse"
		  ;;
      i486)
          OPT_CFLAGS="$OPT_CFLAGS_x86 -march=i486 -mtune=i486 -mno-mmx -mno-sse -mfpmath=387"
		  ;;
      i686)
          OPT_CFLAGS="$OPT_CFLAGS_x86 -march=i686 -mtune=i686 -mmmx -msse -mfpmath=sse"
		  ;;
      x86-64)
          OPT_CFLAGS="$OPT_CFLAGS_x86 -march=x86-64 -mtune=k8 -mmmx -msse -msse2 -mfpmath=sse"
		  ;;
      rpi1)
          OPT_CFLAGS="$OPT_CFLAGS_ARM -mcpu=arm1176jzf-s -mfpu=vfp"
		  ;;
      rpi2)
          OPT_CFLAGS="$OPT_CFLAGS_ARM -mcpu=cortex-a7 -mfpu=neon-vfpv4"
		  ;;
	  rpi3)
          OPT_CFLAGS="$OPT_CFLAGS_ARM -mcpu=cortex-a7 -mfpu=neon-vfpv4"
		  ;;
	  rpi4)
          OPT_CFLAGS="$OPT_CFLAGS_ARM -mcpu=cortex-a7 -mfpu=neon-vfpv4"
		  ;;
	  bbb)
          OPT_CFLAGS="$OPT_CFLAGS_ARM -mcpu=cortex-a8 -mfpu=neon"
		  ;;
	  cyclone5)
          OPT_CFLAGS="$OPT_CFLAGS_ARM -mcpu=cortex-a9 -mfpu=neon"
		  ;;
	  nativeARM)
          OPT_CFLAGS="$OPT_CFLAGS_ARM -mcpu=native -mfpu=auto" # GCC for ARM uses -mcpu only for best-results
		  ;;
      native)
          OPT_CFLAGS="$OPT_CFLAGS_X86 -march=native -mtune=native -mfpmath=sse"
		  ;;
      none)
	      ;;
  esac

  AC_SUBST([OPT_CFLAGS])
])
