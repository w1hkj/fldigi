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

#ifndef MULTIPSK_H
#define MULTIPSK_H

#include "qso_db.h"

class cTextFile {
private:
  char header[120];
  void makeHeader();
  char *adif_to_date( char *s);
  char *adif_to_time( char *s);
public:
  cTextFile () {};
  ~cTextFile () {};
  void writeCSVHeader(FILE *);
  int writeCSVFile (const char *, cQsoDb *);
  void writeTXTHeader(FILE *);
  int writeTXTFile (const char *, cQsoDb *);
};

#endif
