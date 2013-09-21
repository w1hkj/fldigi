// ----------------------------------------------------------------------------
//  ex_extract.h Remote Log Interface for fldigi
//
// Copyright W1HKJ, Dave Freese 2006
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef _RX_EXTRACT_H
#define _RX_EXTRACT_H

#include <string>

extern const char *txtWrapInfo;
extern void rx_extract_add(int c);
extern void select_flmsg_pathname();
extern std::string select_binary_pathname(std::string deffilename);

extern bool extract_wrap;
extern bool extract_flamp;

#endif
