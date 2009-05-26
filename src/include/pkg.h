#ifndef PKG_H_
#define PKG_H_

#include <config.h>

#if BUILD_FLDIGI
#  define PACKAGE_AUTHORS FLDIGI_AUTHORS
#else
#  define PACKAGE_AUTHORS FLARQ_AUTHORS
#  undef PACKAGE
#  define PACKAGE "flarq"
#  undef PACKAGE_NAME
#  define PACKAGE_NAME "flarq"
#  undef PACKAGE_TARNAME
#  define PACKAGE_TARNAME "flarq"
#  undef PACKAGE_VERSION
#  define PACKAGE_VERSION FLARQ_VERSION
#  undef PACKAGE_STRING
#  define PACKAGE_STRING PACKAGE_TARNAME " " PACKAGE_VERSION
#  undef VERSION
#  define VERSION PACKAGE_VERSION
#endif

#endif
