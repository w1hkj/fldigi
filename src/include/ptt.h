// ----------------------------------------------------------------------------
//
//    ptt.h --  PTT control
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
// Copyright (C) 2008
//		Stelios Bounanos, M0GLD
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

#ifndef PTT_H_
#define PTT_H_

struct termios;

class PTT {
public:
	enum ptt_t { PTT_INVALID = -1, PTT_NONE, PTT_HAMLIB,
		     PTT_MEMMAP, PTT_RIGCAT, PTT_TTY, PTT_UHROUTER };
	PTT(ptt_t dev = PTT_NONE);
	~PTT();
	void set(bool on);
	void reset(ptt_t dev);
private:
	ptt_t pttdev;

	// tty
	int pttfd;
	struct termios* oldtio;

	// uhrouter
	int uhkfd[2]; // keyer
	int uhfd[2];  // ptt

	void close_all(void);
	void open_tty(void);
	void set_tty(bool ptt);
	void close_tty(void);
	void open_uhrouter(void);
	void set_uhrouter(bool ptt);
	void close_uhrouter(void);
};

#define UHROUTER_FIFO_PREFIX "/tmp/microHamRouter"

#endif // PTT_H_
