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

#ifndef LOOKUPCALL_H
#define LOOKUPCALL_H

#include <cstring>

extern std::string lookup_latd;
extern std::string lookup_lond;
extern std::string lookup_addr1;
extern std::string lookup_addr2;
extern std::string lookup_qth;
extern std::string lookup_state;
extern std::string lookup_province;
extern std::string lookup_zip;
extern std::string lookup_country;

extern void clear_Lookup();

extern void CALLSIGNquery();

enum qrz_xmlquery_t { 
QRZXML_EXIT = -1, 
QRZXMLNONE, 
QRZNET, QRZCD, 
HAMCALLNET,
CALLOOK, 
HAMQTH };

enum qrz_webquery_t { 
QRZWEB_EXIT = -1, 
QRZWEBNONE, 
QRZHTML, HAMCALLHTML, HAMQTHHTML };

extern void sendEQSL(const char *url);
extern void makeEQSL(const char *msg);

#endif
