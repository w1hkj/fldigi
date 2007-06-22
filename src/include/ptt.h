// ----------------------------------------------------------------------------
//
//    ptt.h --  PTT control
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
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#ifndef _PTT_H
#define _PTT_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string>

using namespace std;

class PTT {
private:
	int pttfd;
	int pttinv;
	int pttmode;
	int pttarg;
	int pttdev;
	string pttdevName;
	struct termios oldtio;
	void  openptt();
	void reset_(int dev, int mode, bool inverted);
public:
	PTT(int dev = 0, int mode = 0, bool inverted = false);
	~PTT();
	void reset(int dev = 0, int mode = 0, bool inverted = false);
	void set(bool on);

};


#endif
