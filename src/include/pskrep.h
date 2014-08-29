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

#ifndef PSKREP_H_
#define PSKREP_H_

bool pskrep_start(void);
void pskrep_stop();
const char* pskrep_error(void);
unsigned pskrep_count(void);

// The regular expression that matches the spotter's buffer when it calls us.
// It must define at least two capturing groups, the second of which is the
// spotted callsign.
#define CALLSIGN_RE "[[:alnum:]]?[[:alpha:]/]+[[:digit:]]+[[:alnum:]/]+"
#define PSKREP_RE "(de|cq|qrz)[^[:alnum:]/\n]+"  "(" CALLSIGN_RE ")"  " +(.* +)?\\2[^[:alnum:]]+$"
#define PSKREP_RE_INDEX 2

#endif // PSKREP_H_
