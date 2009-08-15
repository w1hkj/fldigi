AC_DEFUN([AC_FLDIGI_MACOSX], [
case "$target_os" in
  darwin*)
      target_darwin="yes"
      ;;
  *)
      target_darwin="no"
      ;;
esac

if test "$target_darwin" = "yes"; then
   AC_MSG_NOTICE([using bundled GNU regex on $target_os])
   # pretend that the regex.h check failed so that we use the bundled regex code
   ac_cv_header_regex_h=no
   AC_DEFINE([HAVE_REGEX_H], 0, [Define to 1 if you have the <regex.h> header file.])
fi

AC_ARG_ENABLE([mac-universal], AC_HELP_STRING([--enable-mac-universal],
                                              [build a universal binary on Mac OS X @<:@no@:>@]),
              [case "${enableval}" in
                 yes|no) ac_cv_mac_universal="${enableval}" ;;
                 *)      AC_MSG_ERROR([bad value "${enableval}" for --enable-mac-universal]) ;;
	       esac],
              [ac_cv_mac_universal=no])

if test "x$target_darwin" = "xyes" && test "x$ac_cv_mac_universal" = "xyes"; then
    mac_minversion="-mmacosx-version-min=10.4"
    case "$target_os" in
      darwin8*)
        mac_arches="-arch i386 -arch ppc"
        mac_sysroot="-isysroot /Developer/SDKs/MacOSX10.4u.sdk"
        ;;
      darwin9*)
        mac_arches="-arch i386 -arch ppc -arch x86_64 -arch ppc64"
        mac_sysroot="-isysroot /Developer/SDKs/MacOSX10.5.sdk"
        ;;
      *)
        mac_arches=""
        mac_sysroot=""
        ;;
    esac
    MAC_UNIVERSAL_CFLAGS="$mac_arches $mac_sysroot $mac_minversion"
    MAC_UNIVERSAL_LDFLAGS="$mac_arches"
fi
AC_SUBST([MAC_UNIVERSAL_CFLAGS])
AC_SUBST([MAC_UNIVERSAL_LDFLAGS])

AM_CONDITIONAL([DARWIN], [test "x$target_darwin" = "xyes"])
])
