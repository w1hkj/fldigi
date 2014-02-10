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

#include <string>

#include "irrXML.h"

using namespace irr;
using namespace io;

class IIrrXMLStringReader: public IFileReadCallBack {
  const char *s;
  int len;
  int p;

public:

  IIrrXMLStringReader(const char *szStr) {
    s = szStr;
    len = strlen(s);
    p=0;
  }

  IIrrXMLStringReader(const std::string &str) {
    s=str.c_str();
    len = strlen(s);
    p=0;
  }

  int read(void * buffer, int sizeToRead) {
    char *sss = (char *)buffer;
    if (p >= len) return 0;
    int j = 0;
    for (int i = p; i < len && j < sizeToRead; ) {
      sss[j++] = s[i++];
    }
    return 1;
  }
    
  int getSize() {
    return len-p;
  }

};

