AC_DEFUN([AC_FLDIGI_JPEG_HDR], [
  CXXFLAGS_saved="$CXXFLAGS"
  m4_ifval([$1], [CXXFLAGS="$CXXFLAGS $1"], [:])
  AC_CHECK_HEADER([jpeglib.h])
  CXXFLAGS="$CXXFLAGS_saved"
])

AC_DEFUN([AC_FLDIGI_JPEG_LIB], [
  CXXFLAGS_saved="$CXXFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  m4_ifval([$1], [CXXFLAGS="$CXXFLAGS $1"], [:])
  m4_ifval([$2], [LDFLAGS="$LDFLAGS $2"], [:])
  AC_LANG_PUSH(C++)
  AC_LINK_IFELSE( [AC_LANG_PROGRAM( [[#include <stdio.h>
                                      #include <jpeglib.h>
                                      struct jpeg_compress_struct cinfo;]],
                                    [] )
                  ],
                  [ac_cv_libjpeg=yes], [ac_cv_libjpeg=no] )
  AC_LANG_POP(C++)
  CXXFLAGS="$CXXFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
])

AC_DEFUN([AC_FLDIGI_JPEG], [
  AC_ARG_VAR([LIBJPEG_CFLAGS], [C compiler flags for JPEG])
  AC_ARG_VAR([LIBJPEG_LIBS], [linker flags for JPEG])

  AC_FLDIGI_JPEG_HDR([$LIBJPEG_CFLAGS])
  if test "x$ac_cv_header_jpeglib_h" != "xyes"; then
      ac_cv_libjpeg=no
  else
      AC_MSG_CHECKING([for libjpeg])
      test "x$LIBJPEG_LIBS" = "x" && LIBJPEG_LIBS="-ljpeg"
      AC_FLDIGI_JPEG_LIB([$LIBJPEG_CFLAGS], [$LIBJPEG_LIBS])
      AC_MSG_RESULT([$ac_cv_libjpeg])
  fi
  if test "x$ac_cv_libjpeg" = "xyes"; then
      AC_DEFINE([USE_LIBJPEG], 1, [Define to 1 if we are using libjpeg])
  else
      AC_DEFINE([USE_LIBJPEG], 0, [Define to 1 if we are using libjpeg])
  fi
])


AC_DEFUN([AC_FLDIGI_IMAGES], [
  AC_FLDIGI_JPEG
  if test "x$ac_cv_libjpeg" = "xyes"; then
      IMAGE_CFLAGS="$IMAGE_CFLAGS $LIBJPEG_CFLAGS"
      IMAGE_LIBS="$IMAGE_LIBS $LIBJPEG_LIBS"
  fi

  AC_FLDIGI_PKG_CHECK([libpng], [libpng >= 1.2.8], [no], [yes],
                      [support saving images in PNG format @<:@autodetect@:>@])
  if test "x$ac_cv_libpng" = "xyes"; then
      IMAGE_CFLAGS="$IMAGE_CFLAGS $LIBPNG_CFLAGS"
      IMAGE_LIBS="$IMAGE_LIBS $LIBPNG_LIBS"
  fi

  if test "x$ac_cv_libjpeg" != "xyes" && test "x$ac_cv_libpng" != "xyes"; then
      AC_MSG_WARN([No image libraries were found])
  fi

  AC_SUBST([IMAGE_CFLAGS])
  AC_SUBST([IMAGE_LIBS])
])
