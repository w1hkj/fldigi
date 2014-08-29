// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

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
