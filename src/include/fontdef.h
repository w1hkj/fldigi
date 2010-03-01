// ------------------------------------------------------------------------------
//
//    fontdef.h  --  FELDHELL modem
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
//  Copyright (C) 2004
//  Lawrence Glaister (ve7it@shaw.ca)
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


#ifndef _FONTDEF_H
#define _FONTDEF_H

struct fntchr { char c; int byte[14]; };

extern fntchr feld7x7_14[];
extern fntchr feld7x7n_14[];
extern fntchr feldDx_14[];
extern fntchr feldfat_14[];
extern fntchr feldhell_12[];
extern fntchr feldlittle_12[];
extern fntchr feldlo8_14[];
extern fntchr feldlow_14[];
extern fntchr feldmodern_14[];
extern fntchr feldmodern8_14[];
extern fntchr feldnarr_14[];
extern fntchr feldreal_14[];
extern fntchr feldstyl_14[];
extern fntchr feldvert_14[];
extern fntchr feldwide_14[];

extern char szFeldFonts[];


#endif
