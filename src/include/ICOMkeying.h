// ----------------------------------------------------------------------------
// ICOMkeying.h   serial string CW interface to Elecraft transceivers
//
// Copyright (C) 2020
//		Dave Freese, W1HKJ
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

#ifndef _ICOMKEYING_H
#define _ICOMKEYING_H

extern bool use_ICOMkeyer;
extern int ICOMwpm;

extern void set_ICOMkeyer();
extern void ICOMkeyer_send_char(int);

#endif
