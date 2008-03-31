AC_DEFUN([AC_FLDIGI_FLTK_JPEG], [
  AC_REQUIRE([AC_FLDIGI_FLTK])
  if test "x$ac_cv_header_jpeglib_h" != "xyes"; then
      AC_MSG_ERROR([
  *** The jpeglib.h header could not be found. Please install the development
  *** headers and libraries for JPEG.])
  fi

  AC_MSG_CHECKING([whether we have a working libjpeg])
  CXXFLAGS_saved="$CXXFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  CXXFLAGS="$CXXFLAGS $FLTK_CFLAGS"
  LDFLAGS="$LDFLAGS $FLTK_LIBS"
  AC_LANG_PUSH(C++)
  AC_LINK_IFELSE( [AC_LANG_PROGRAM( [[#include <stdio.h>
                                      #include <jpeglib.h>
                                      struct jpeg_compress_struct cinfo;]],
                                    [] )
                  ],
                  [ac_cv_have_jpeg=yes], [ac_cv_have_jpeg=no] )
  AC_LANG_POP(C++)
  CXXFLAGS="$CXXFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  AC_MSG_RESULT([$ac_cv_have_jpeg])

  if test "x$ac_cv_have_jpeg" != "xyes"; then
      AC_MSG_FAILURE([
  *** A test jpeg program could not be linked using the FLTK linker flags,
  *** even though jpeglib.h is present. Please report this error to
  *** ${PACKAGE_BUGREPORT}, including any relevant output from config.log.])
  fi
])
