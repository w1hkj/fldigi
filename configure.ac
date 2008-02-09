#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_COPYRIGHT([Copyright (C) 2007 Stelios Bounanos, M0GLD (m0gld AT enotty DOT net)])

AC_PREREQ(2.61)
AC_INIT([fldigi], [2.09], [w1hkj AT w1hkj DOT com])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall foreign std-options 1.9.6])
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([src/main.cxx])
AC_CONFIG_HEADER([src/config.h])

AC_DEFINE(PACKAGE_HOME, ["http://www.w1hkj.com/Fldigi-2.x.html"], [Home page])
AC_DEFINE(PACKAGE_DOCS, ["http://www.w1hkj.com/FldigiHelp-2.0/index.html"], [Docs index])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADERS([arpa/inet.h execinfo.h fcntl.h limits.h memory.h netdb.h netinet/in.h regex.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h termios.h unistd.h values.h])

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
AC_CHECK_FUNCS([gethostbyname localtime_r memmove memset mkdir select snprintf socket strcasecmp strchr strdup strerror strncasecmp strcasestr strrchr strstr strtol uname vsnprintf])


AC_PRESERVE_HELP_ORDER

dnl blank line
AC_ARG_WITH([], [], [], [])

###########################
# static flag
###########################
AC_ARG_ENABLE([static],
              AC_HELP_STRING([--enable-static], [enable static linking]),
              [case "${enableval}" in
                  yes|no) ac_cv_static="${enableval}" ;;
                  *)      AC_MSG_ERROR([bad value ${enableval} for --enable-static]) ;;
               esac],
               [ac_cv_static=no])
if test "x$ac_cv_static" = "xyes"; then
    AC_CHECK_LIB([rt], [clock_gettime], [RTLIB=-lrt])
fi
AC_SUBST([RTLIB])

###########################
# debug flag
###########################
AC_ARG_ENABLE([debug],
              AC_HELP_STRING([--enable-debug], [turn on debugging]),
              [case "${enableval}" in
                yes|no) ac_cv_debug="${enableval}" ;;
                *)      AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
               esac],
               [ac_cv_debug=no])
if test "x$ac_cv_debug" = "xyes"; then
    if test "x$GXX" = "xyes"; then
        RDYNAMIC=-rdynamic
        CXXFLAGS="-O0 -ggdb3 -Wall"
    else
        CXXFLAGS="-O0 -g -Wall"
    fi
    CFLAGS="$CXXFLAGS"
    AC_MSG_NOTICE([debugging enabled; overriding CXXFLAGS])
fi
AM_CONDITIONAL([ENABLE_DEBUG], [test "x$ac_cv_debug" = "xyes"])
AC_SUBST([RDYNAMIC])

###########################
# TLS flag
###########################
AC_ARG_ENABLE([tls],
              AC_HELP_STRING([--enable-tls], [enable use of TLS @<:@autodetect@:>@]),
              [case "${enableval}" in
                yes|no) ac_cv_want_tls="${enableval}" ;;
                *)      AC_MSG_ERROR([bad value "${enableval}" for --enable-tls]) ;;
               esac],
               [ac_cv_want_tls=check])

if test "x$ac_cv_want_tls" = "xno"; then
    AC_DEFINE(USE_TLS, 0, [Defined if we are using TLS])
    ac_cv_have_tls=no
else
    CHECK_TLS()
    if test "x$ac_cv_want_tls" = "xcheck"; then
        if test "x$ac_cv_have_tls" = "xyes"; then
            AC_DEFINE(USE_TLS, 1, [Defined if we are using TLS])
        else
            AC_DEFINE(USE_TLS, 0, [Defined if we are using TLS])
        fi
    else # $ac_cv_want_tls is yes
        if test "x$ac_cv_have_tls" = "xno"; then
            AC_MSG_FAILURE([--enable-tls was given, but TLS is not supported])
        else
            AC_DEFINE(USE_TLS, 1, [Defined if we are using TLS])
        fi
    fi
fi

###########################
# bind
###########################
# Look for a working std::bind or std::tr1::bind. If neither is present
# we will need Boost >= 1.32.0, which provides boost::bind.
AC_LANG_PUSH(C++)
AC_MSG_CHECKING([for std::bind in <functional>])
AC_COMPILE_IFELSE( [AC_LANG_PROGRAM([[#include <functional>
                                      void f(void) { }]],
                                    [[std::bind(f)();]])],
                   [ac_cv_std_bind=yes], [ac_cv_std_bind=no] )
AC_MSG_RESULT([$ac_cv_std_bind])
if test "x$ac_cv_std_bind" = "xyes"; then
    AC_DEFINE(HAVE_STD_BIND, 1, [Define to 1 if we have std::bind in <functional>])
else
    AC_DEFINE(HAVE_STD_BIND, 0, [Define to 1 if we have std::bind in <functional>])
fi

if test "x$ac_cv_std_bind" = "xno"; then
    AC_MSG_CHECKING([for std::tr1::bind in <tr1/functional>])
    AC_COMPILE_IFELSE( [AC_LANG_PROGRAM([[#include <tr1/functional>
                                          void f(void) { }]],
                                        [[std::tr1::bind(f)();]])],
                       [ac_cv_std_tr1_bind=yes], [ac_cv_std_tr1_bind=no] )
    AC_MSG_RESULT([$ac_cv_std_tr1_bind])
    if test "x$ac_cv_std_tr1_bind" = "xyes"; then
        AC_DEFINE(HAVE_STD_TR1_BIND, 1, [Define to 1 if we have std::tr1::bind in <tr1/functional>])
    else
        AC_DEFINE(HAVE_STD_TR1_BIND, 0, [Define to 1 if we have std::tr1::bind in <tr1/functional>])
    fi
fi
AC_LANG_POP(C++)

if test "x$ac_cv_std_bind" = "xno" && test "x$ac_cv_std_tr1_bind" = "xno"; then
    AX_BOOST_BASE(1.32.0)
    if test "x$want_boost" = "xno"; then
        AC_MSG_ERROR([Boost is required])
    fi
fi
AC_SUBST([BOOST_CPPFLAGS])
AC_SUBST([BOOST_LDFLAGS])

###########################
# FLTK
###########################
AC_ARG_VAR([FLTK_CFLAGS], [C compiler flags for FLTK, overriding fltk-config])
AC_ARG_VAR([FLTK_LIBS], [linker flags for FLTK, overriding fltk-config])
if test "x$FLTK_CFLAGS" = "x" && test "x$FLTK_LIBS" = "x"; then
    AC_PATH_PROG([FLTK_CONFIG], [fltk-config], [no])
    if test "$FLTK_CONFIG" = "no"; then
        AC_MSG_ERROR([
*** The fltk-config script could not be found. Please install the development
*** headers and libraries for FLTK 1.1.x, or set PATH to the directory that
*** contains fltk-config.
        ])
    fi
    HAVE_FLTK_API_VERSION=no
    FLTK_API_VERSION=`$FLTK_CONFIG --api-version`
    if test "x$FLTK_API_VERSION" = "x1.1" || "x$FLTK_API_VERSION" = "x1.2"; then
        HAVE_FLTK_API_VERSION=yes
    fi
    if test "${HAVE_FLTK_API_VERSION}" = "no"; then
        AC_MSG_ERROR([
*** The version of FLTK found on your system provides API version $FLTK_API_VERSION.
*** To build $PACKAGE you need a FLTK version that provides API 1.1.
        ])
    fi
    FLTK_CFLAGS=`$FLTK_CONFIG --cxxflags`
    if test "x$ac_cv_static" != "xyes"; then
        FLTK_LIBS=`$FLTK_CONFIG --ldflags --use-images`
    else
        FLTK_LIBS=`$FLTK_CONFIG --ldstaticflags --use-images`
    fi
else
    AC_MSG_NOTICE([not checking for FLTK])
fi
AC_SUBST([FLTK_CFLAGS])
AC_SUBST([FLTK_LIBS])


###########################
# libsamplerate
###########################
PKG_CHECK_MODULES(SAMPLERATE, samplerate >= 0.1.1, ac_cv_samplerate=yes, ac_cv_samplerate=no)
if test "x$ac_cv_samplerate" = "xno"; then
    AC_MSG_NOTICE([using bundled libsamplerate])
fi
AC_SUBST([SAMPLERATE_CFLAGS])
AC_SUBST([SAMPLERATE_LIBS])
AM_CONDITIONAL([NO_SAMPLERATE], [test "x$ac_cv_samplerate" = "xno"])

###########################
# libsndfile
###########################
AC_ARG_WITH([sndfile],
            AC_HELP_STRING([--with-sndfile],
                           [enable reading/writing of audio files via libsndfile @<:@autodetect@:>@]),
            [case "${withval}" in
              yes|no) ac_cv_want_sndfile="${withval}" ;;
              *)      AC_MSG_ERROR([bad value "${withval}" for --with-sndfile]) ;;
             esac],
            [ac_cv_want_sndfile=check])
if test "x$ac_cv_want_sndfile" = "xno"; then
    AC_DEFINE(USE_SNDFILE, 0, [Define to 1 if we are using sndfile])
    ac_cv_sndfile=no
else
    PKG_CHECK_EXISTS(sndfile >= 1.0.10, ac_cv_sndfile=yes, ac_cv_sndfile=no)
    if test "x$ac_cv_want_sndfile" = "xcheck"; then
        PKG_CHECK_MODULES(SNDFILE, sndfile >= 1.0.10, [:], [:])
        if test "x$ac_cv_sndfile" = "xyes"; then
            AC_DEFINE(USE_SNDFILE, 1, [Define to 1 if we are using sndfile])
        else
            AC_DEFINE(USE_SNDFILE, 0, [Define to 1 if we are using sndfile])
        fi
    else # $ac_cv_want_sndfile is yes
        if test "x$ac_cv_sndfile" = "xno"; then
            AC_MSG_NOTICE([--with-sndfile was given, but test for sndfile failed])
        else
            AC_DEFINE(USE_SNDFILE, 1, [Define to 1 if we are using sndfile])
        fi
        PKG_CHECK_MODULES(SNDFILE, sndfile >= 1.0.10) # for the error message
    fi
fi
AC_SUBST([SNDFILE_CFLAGS])
AC_SUBST([SNDFILE_LIBS])


###########################
# portaudio
###########################
AC_ARG_WITH([portaudio],
            AC_HELP_STRING([--without-portaudio],
                           [disable support for PortAudio]),
            [case "${withval}" in
              yes|no) ac_cv_want_portaudio="${withval}" ;;
              *)      AC_MSG_ERROR([bad value "${withval}" for --without-portaudio]) ;;
             esac],
            [ac_cv_want_portaudio=yes])
if test "x$ac_cv_want_portaudio" = "xyes"; then
    PKG_CHECK_EXISTS(portaudio-2.0 >= 19, ac_cv_portaudio=yes, ac_cv_portaudio=no)
    if test "x$ac_cv_portaudio" = "xyes"; then
        AC_DEFINE(USE_PORTAUDIO, 1, [Define to 1 if we are using PortAudio])
    else
        AC_MSG_NOTICE([portaudio test failed (use --without-portaudio to disable)])
    fi
    PKG_CHECK_MODULES(PORTAUDIO, portaudio-2.0 >= 19)
else
    AC_DEFINE(USE_PORTAUDIO, 0, [Define to 1 if we are using PortAudio])
    ac_cv_portaudio=no
fi
AC_SUBST([PORTAUDIO_CFLAGS])
AC_SUBST([PORTAUDIO_LIBS])


###########################
# hamlib
###########################
AC_ARG_WITH([hamlib],
            AC_HELP_STRING([--with-hamlib],
                           [use hamradio control libraries @<:@autodetect@:>@]),
            [case "${withval}" in
              yes|no) ac_cv_want_hamlib="${withval}" ;;
              *)      AC_MSG_ERROR([bad value "${withval}" for --with-hamlib]) ;;
             esac],
            [ac_cv_want_hamlib=check])
if test "x$ac_cv_want_hamlib" = "xno"; then
    AC_DEFINE(USE_HAMLIB, 0)
    ac_cv_hamlib=no
else
    PKG_CHECK_EXISTS(hamlib >= 1.2.4, ac_cv_hamlib=yes, ac_cv_hamlib=no)
    if test "x$ac_cv_want_hamlib" = "xcheck"; then
        PKG_CHECK_MODULES(HAMLIB, hamlib >= 1.2.4, [:], [:])
        if test "x$ac_cv_hamlib" = "xyes"; then
            AC_DEFINE(USE_HAMLIB, 1, [Define to 1 if we are using hamlib])
        else
            AC_DEFINE(USE_HAMLIB, 0, [Define to 1 if we are using hamlib])
        fi
    else # $ac_cv_want_hamlib is yes
        if test "x$ac_cv_hamlib" = "xno"; then
            AC_MSG_NOTICE([--with-hamlib was given, but test for hamlib failed])
        else
            AC_DEFINE(USE_HAMLIB, 1, [Define to 1 if we are using hamlib])
        fi
        PKG_CHECK_MODULES(HAMLIB, hamlib >= 1.2.4) # for the error message
    fi
fi
AC_SUBST([HAMLIB_CFLAGS])
AC_SUBST([HAMLIB_LIBS])
AM_CONDITIONAL([ENABLE_HAMLIB], [test "x$ac_cv_hamlib" = "xyes"])

###########################
# output
###########################
AH_TOP([
#ifndef CONFIG_H_
#define CONFIG_H_
])
AH_BOTTOM([
#include "util.h"
#endif /* CONFIG_H_ */
])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

###########################
# summary
###########################
AC_MSG_RESULT([
Configuration summary:

  Version ............................. $VERSION

  Static linking ...................... $ac_cv_static
  Debugging ........................... $ac_cv_debug
  TLS ................................. $ac_cv_have_tls

  sndfile ............................. $ac_cv_sndfile
  PortAudio ........................... $ac_cv_portaudio
  hamlib .............................. $ac_cv_hamlib
])
