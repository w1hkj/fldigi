#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_COPYRIGHT([Copyright (C) 2007, 2008, 2009 Stelios Bounanos, M0GLD (m0gld AT enotty DOT net)])

AC_PREREQ(2.61)

dnl major and minor must be integers; patch may
dnl contain other characters or be empty
m4_define(FLDIGI_MAJOR, [3])
m4_define(FLDIGI_MINOR, [11])
m4_define(FLDIGI_PATCH, [.6])

AC_INIT([fldigi], FLDIGI_MAJOR.FLDIGI_MINOR[FLDIGI_PATCH], [w1hkj AT w1hkj DOT com])

# substitute in Makefiles
AC_SUBST([FLDIGI_VERSION_MAJOR], [FLDIGI_MAJOR])
AC_SUBST([FLDIGI_VERSION_MINOR], [FLDIGI_MINOR])
AC_SUBST([FLDIGI_VERSION_PATCH], [FLDIGI_PATCH])
# define in config.h
AC_DEFINE([FLDIGI_VERSION_MAJOR], [FLDIGI_MAJOR], [Major version number])
AC_DEFINE([FLDIGI_VERSION_MINOR], [FLDIGI_MINOR], [Minor version number])
AC_DEFINE([FLDIGI_VERSION_PATCH], ["FLDIGI_PATCH"], [Patch/alpha version string])

AC_CONFIG_AUX_DIR([build-aux])

# define build, build_cpu, build_vendor, build_os
AC_CANONICAL_BUILD
# define host, host_cpu, host_vendor, host_os
AC_CANONICAL_HOST
# define target, target_cpu, target_vendor, target_os
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([-Wall foreign std-options 1.9.6])
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([src/main.cxx])
AC_CONFIG_HEADER([src/config.h])
AC_CONFIG_MACRO_DIR([m4])


AC_DEFINE([PACKAGE_AUTHORS], ["Dave Freese, Stelios Bounanos, Leigh Klotz, and others"], [Authors])
AC_DEFINE([PACKAGE_HOME], ["http://www.w1hkj.com/Fldigi.html"], [Home page])
AC_DEFINE([PACKAGE_DL], ["http://www.w1hkj.com/Downloads.html"], [Download page])
AC_DEFINE([PACKAGE_PROJ], ["http://developer.berlios.de/project/showfiles.php?group_id=9149"], [BerliOS page])
AC_DEFINE([PACKAGE_DOCS], ["http://www.w1hkj.com/FldigiHelp/index.html"], [Docs index])


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADERS([arpa/inet.h execinfo.h fcntl.h limits.h memory.h netdb.h netinet/in.h regex.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h termios.h unistd.h values.h linux/ppdev.h dev/ppbus/ppi.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_CHECK_FUNCS([getaddrinfo gethostbyname localtime_r memmove memset mkdir select snprintf socket strcasecmp strchr strdup strerror strncasecmp strcasestr strrchr strstr strtol uname vsnprintf])

AC_FLDIGI_SEARCH_LIBS([dlopen], [dl])
AC_FLDIGI_SEARCH_LIBS([clock_gettime], [rt])
AC_FLDIGI_SEARCH_LIBS([sem_unlink], [rt])
AC_FLDIGI_SEARCH_LIBS([sem_timedwait], [rt])
AC_SUBST([EXTRA_LIBS])


AC_PRESERVE_HELP_ORDER

### Internationalization
AM_GNU_GETTEXT([external])

###### OS support
### OSX
# Set ac_cv_mac_universal to yes/no
# Set DARWIN Makefile conditional
# Substitute MAC_UNIVERSAL_CFLAGS and MAC_UNIVERSAL_LDFLAGS in Makefile
AC_FLDIGI_MACOSX
### win32
# Set WIN32 Makefile conditional
# Set HAVE_WINDRES Makefile conditional
# Substitute WINDRES in Makefile
AC_FLDIGI_WIN32

### static flag
# Set ac_cv_static to yes/no
# Substitute RTLIB in Makefile
AC_FLDIGI_STATIC

### optimizations
# Set ac_cv_opt to arg
# Substitute OPT_FLAGS in Makefile
AC_FLDIGI_OPT

### debug flag
# Set ac_cv_debug to yes/no
# Override CXXFLAGS
# Set ENABLE_DEBUG Makefile conditional
# Substitute RDYNAMIC in Makefile
AC_FLDIGI_DEBUG

### benchmark mode
# Set ac_cv_benchmark to yes/no
# Define BENCHMARK_MODE in config.h
# Set ENABLE_BENCHMARK Makefile conditional
AC_FLDIGI_BENCHMARK

### TLS flag
# Set ac_cv_tls to yes/no
# Define USE_TLS in config.h
AC_FLDIGI_TLS

### std::bind or alternatives
# Set ac_cv_std_bind or ac_cv_std_tr1_bind to yes/no
# Define HAVE_STD_BIND or HAVE_STD_TR1_BIND in config.h
# Substitute BOOST_CPPFLAGS and BOOST_LDFLAGS in Makefile
# if falling back to Boost
AC_FLDIGI_BIND

### FLTK
# Substitute FLTK_CFLAGS and FLTK_LIBS in Makefile
# Set FLUID variable
# Set HAVE_FLUID Makefile conditional
AC_FLDIGI_FLTK

### Image libraries
# JPEG:
#   Test if we can compile a program that includes jpeglib.h
#   and link it either using the FLTK flags set above, or
#   with -ljpeg.
#   Set ac_cv_libjpeg to yes/no
#   Define LIBJPEG_CFLAGS and LIBJPEG_CFLAGS and add them to
#   IMAGE_CFLAGS and IMAGE_LIBS.
#   Define USE_LIBJPEG in config.h
# PNG:
#   Check with the AC_FLDIGI_PKG_CHECK macro
#   Set ac_cv_libpng to yes/no
#   Define LIBJPEG_CFLAGS and LIBJPEG_CFLAGS and add them to
#   IMAGE_CFLAGS and IMAGE_LIBS.
#   Define USE_LIBPNG in config.h
# Substitute IMAGE_CFLAGS and IMAGE_LIBS in Makefile
AC_FLDIGI_IMAGES

### XML-RPC library
# Set ac_cv_xmlrpc to yes/no
# Substitute XMLRPC_CFLAGS and XMLRPC_LIBS in Makefile
# Define USE_XMLRPC in config.h
# Set ENABLE_XMLRPC Makefile conditional
AC_FLDIGI_XMLRPC

### OSS
# Set ac_cv_oss to yes/no
# Define USE_OSS in config.h
AC_FLDIGI_OSS

### libsamplerate
# Set ac_cv_samplerate to yes/no    (not used because we require samplerate)
# Define USE_SAMPLERATE in config.h (as above)
# Substitute SAMPLERATE_CFLAGS and SAMPLERATE_LIBS in Makefile
AC_FLDIGI_PKG_CHECK([samplerate], [samplerate >= 0.1.1], [no], [no])

### libsndfile
# Set ac_cv_sndfile to yes/no
# Define USE_SNDFILE in config.h
# Substitute SNDFILE_CFLAGS and SNDFILE_LIBS in Makefile
AC_FLDIGI_PKG_CHECK([sndfile], [sndfile >= 1.0.10], [yes], [yes],
                    [enable reading/writing of audio files via libsndfile @<:@autodetect@:>@] )

### portaudio
# Set ac_cv_portaudio to yes/no
# Define USE_PORTAUDIO in config.h
# Substitute PORTAUDIO_CFLAGS and PORTAUDIO_LIBS in Makefile
AC_FLDIGI_PKG_CHECK([portaudio], [portaudio-2.0 >= 19], [yes], [yes],
                    [enable support for PortAudio @<:@autodetect@:>@] )

### pulseaudio
# Set ac_cv_pulseaudio to yes/no
# Define USE_PULSEAUDIO in config.h
# Substitute PULSEAUDIO_CFLAGS and PULSEAUDIO_LIBS in Makefile
AC_FLDIGI_PKG_CHECK([pulseaudio], [libpulse-simple >= 0.9.7], [yes], [yes],
                    [enable support for PulseAudio @<:@autodetect@:>@] )

if test "x$ac_cv_oss" = "xno" && \
   test "x$ac_cv_portaudio" = "xno" && \
   test "x$ac_cv_pulseaudio" = "xno"; then
    AC_MSG_WARN([*** $PACKAGE will be compiled without audio device support ***])
fi

### hamlib
# Set ac_cv_hamlib to yes/no
# Define USE_HAMLIB in config.h
# Substitute HAMLIB_CFLAGS and HAMLIB_LIBS in Makefile
# Set ENABLE_HAMLIB Makefile conditional
AC_FLDIGI_PKG_CHECK([hamlib], [hamlib >= 1.2.4], [yes], [yes],
                    [use hamradio control libraries @<:@autodetect@:>@],
                    [ENABLE_HAMLIB])
# We compile our own copy of hamlib's locator.c if hamlib is not available
# or if it is version <= 1.2.9, which has a broken azimuth_long_path function
AX_COMPARE_VERSION([$pkg_hamlib_version], [le], [1.2.9], [need_locator_c=yes], [need_locator_c=no])
AM_CONDITIONAL([NEED_HAMLIB_LOCATOR], [test "$need_locator_c" = "yes"], [:])

### libintl
# Substitute INTL_CFLAGS in Makefile
# Substitute INTL_LIBS in Makefile
# (default to LIBINTL)
AC_FLDIGI_INTL

### asciidoc
# substitute ASCIIDOC and A2X in doc/Makefile
# set HAVE_ASCIIDOC Makefile conditional
AC_FLDIGI_DOCS


### output
AH_TOP([
#ifndef CONFIG_H_
#define CONFIG_H_
])
AH_BOTTOM([
#include "util.h"
#endif /* CONFIG_H_ */
])
AC_CONFIG_FILES([Makefile src/Makefile po/Makefile.in doc/Makefile])
AC_OUTPUT


### summary
AC_MSG_RESULT([
Configuration summary:

  Version ..................... $VERSION

  Static linking .............. $ac_cv_static
  CPU optimizations ........... $ac_cv_opt
  Debugging ................... $ac_cv_debug

  png ......................... $ac_cv_libpng
  jpeg ........................ $ac_cv_libjpeg

  sndfile ..................... $ac_cv_sndfile
  oss ......................... $ac_cv_oss
  portaudio ................... $ac_cv_portaudio
  pulseaudio .................. $ac_cv_pulseaudio

  hamlib ...................... $ac_cv_hamlib
  xmlrpc ...................... $ac_cv_xmlrpc

  i18n ........................ $USE_NLS
])
