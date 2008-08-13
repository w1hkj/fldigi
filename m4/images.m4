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

  AC_ARG_WITH([jpeg],
              AC_HELP_STRING([--with-jpeg], [enable libjpeg support @<:@autodetect@:>@]),
              [case "${withval}" in
                yes|no) ac_cv_want_libjpeg="${withval}" ;;
                *)      AC_MSG_ERROR([bad value "${withval}" for --with-jpeg]) ;;
               esac],
               [ac_cv_want_libjpeg=check])

  if test "x$ac_cv_want_libjpeg" = "xno"; then
      ac_cv_libjpeg=no
  else
      AC_FLDIGI_JPEG_HDR([$LIBJPEG_CFLAGS])
      if test "x$ac_cv_header_jpeglib_h" != "xyes"; then
          ac_cv_libjpeg=no
      else
          AC_MSG_CHECKING([for libjpeg])
          if test "x$LIBJPEG_CFLAGS" != "x" || test "x$LIBJPEG_LIBS" != "x"; then
              AC_FLDIGI_JPEG_LIB([$LIBJPEG_CFLAGS], [$LIBJPEG_LIBS])
          else
              AC_FLDIGI_JPEG_LIB([$FLTK_CFLAGS], [$FLTK_LIBS])
              if test "x$ac_cv_libjpeg" != "xyes"; then
                  LIBJPEG_LIBS="-ljpeg"
                  AC_FLDIGI_JPEG_LIB([$LIBJPEG_CFLAGS], [$LIBJPEG_LIBS])
              fi
          fi
          AC_MSG_RESULT([$ac_cv_libjpeg])
      fi
  fi

  if test "x$ac_cv_libjpeg" = "xyes"; then
      AC_DEFINE([USE_LIBJPEG], 1, [Define to 1 if we are using libjpeg])
  else
      AC_DEFINE([USE_LIBJPEG], 0, [Define to 1 if we are using libjpeg])
  fi
])


AC_DEFUN([AC_FLDIGI_PNG_HDR], [
  CXXFLAGS_saved="$CXXFLAGS"
  m4_ifval([$1], [CXXFLAGS="$CXXFLAGS $1"], [:])
  AC_CHECK_HEADER([png.h])
  CXXFLAGS="$CXXFLAGS_saved"
])

AC_DEFUN([AC_FLDIGI_PNG_LIB], [
  CXXFLAGS_saved="$CXXFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  m4_ifval([$1], [CXXFLAGS="$CXXFLAGS $1"], [:])
  m4_ifval([$2], [LDFLAGS="$LDFLAGS $2"], [:])
  AC_LANG_PUSH(C++)
  AC_LINK_IFELSE( [AC_LANG_PROGRAM( [[#include <png.h>
                                      png_structp png = png_create_write_struct(0, 0, 0, 0);]],
                                    [] )
                  ],
                  [ac_cv_libpng=yes], [ac_cv_libpng=no] )
  AC_LANG_POP(C++)
  CXXFLAGS="$CXXFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
])

AC_DEFUN([AC_FLDIGI_PNG], [
  AC_ARG_VAR([LIBPNG_CFLAGS], [C compiler flags for PNG])
  AC_ARG_VAR([LIBPNG_LIBS], [linker flags for PNG])

  AC_ARG_WITH([png],
              AC_HELP_STRING([--with-png], [enable libpng support @<:@autodetect@:>@]),
              [case "${withval}" in
                yes|no) ac_cv_want_libpng="${withval}" ;;
                *)      AC_MSG_ERROR([bad value "${withval}" for --with-png]) ;;
               esac],
               [ac_cv_want_libpng=check])

  if test "x$ac_cv_want_libpng" = "xno"; then
      ac_cv_libpng=no
  else
      if test "x$LIBPNG_CFLAGS" != "x" || test "x$LIBPNG_LIBS" != "x"; then
          AC_FLDIGI_PKG_CHECK([libpng], [libpng >= 1.2.8], [no], [yes])
      else
          AC_FLDIGI_PNG_HDR([$LIBPNG_CFLAGS])
          if test "x$ac_cv_header_png_h" != "xyes"; then
              AC_FLDIGI_PKG_CHECK([libpng], [libpng >= 1.2.8], [no], [yes])
          else
              AC_MSG_CHECKING([for libpng in fltk libs])
              AC_FLDIGI_PNG_LIB([$FLTK_CFLAGS], [$FLTK_LIBS])
              AC_MSG_RESULT([$ac_cv_libpng])
              if test "x$ac_cv_libpng" != "xyes"; then
                  AC_FLDIGI_PKG_CHECK([libpng], [libpng >= 1.2.8], [no], [yes])
              fi
          fi
      fi
  fi

  if test "x$ac_cv_libpng" = "xyes"; then
      AC_DEFINE([USE_LIBPNG], 1, [Define to 1 if we are using libpng])
  else
      AC_DEFINE([USE_LIBPNG], 0, [Define to 1 if we are using libpng])
  fi
])


AC_DEFUN([AC_FLDIGI_IMAGES], [
  AC_REQUIRE([AC_FLDIGI_FLTK])

  AC_FLDIGI_JPEG
  if test "x$ac_cv_libjpeg" = "xyes"; then
      IMAGE_CFLAGS="$IMAGE_CFLAGS $LIBJPEG_CFLAGS"
      IMAGE_LIBS="$IMAGE_LIBS $LIBJPEG_LIBS"
  fi

  AC_FLDIGI_PNG
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
