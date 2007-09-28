// ----------------------------------------------------------------------------
//
//    ptt.cxx --  PTT control
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

#include "ptt.h"
#include "configuration.h"
#include "rigMEM.h"
#include "rigio.h"
#ifndef NOHAMLIB
	#include "hamlib.h"
#endif

#include <FL/fl_ask.H>


// dev may be one of
// "none" - 0
// "hamlib" - 1
// "memmap" - 2
// "rigcat" - 3
// "tty" - 4 (SPECIFIED by progdefaults.PTTdev)

PTT::PTT(int dev, int mode, bool inverted)
{
	pttfd = -1;
	reset_(dev, mode, inverted);
}

PTT::~PTT()
{
	if (pttdev < 4)
		return;

	if (pttfd != -1) {
		tcsetattr (pttfd, TCSANOW, &oldtio);
		close(pttfd);
		pttfd = -1;
	}
}

void PTT::reset_(int dev, int mode, bool inverted)
{
	pttdev = dev;
	pttinv = inverted;
	pttmode = mode;

	if (pttdev < 4) {
		set(0);
		return;
	}

	pttdevName = progdefaults.PTTdev;
	
	openptt();
	if (pttfd == -1) {
		string msg = "Cannot open ";
		msg = msg + pttdevName;
		fl_message( msg.c_str() );
		return;
	}
	set(false);
}


void PTT::reset(int dev, int mode, bool inverted)
{
	if (pttfd != -1) {
		tcsetattr (pttfd, TCSANOW, &oldtio);
		close(pttfd);
		pttfd = -1;
	}
	reset_(dev, mode, inverted);
}

void PTT::set(bool ptt)
{
	if (pttdev == 0) return;

	if (active_modem == cw_modem && 
			((progdefaults.useCWkeylineRTS == true) || 
			 (progdefaults.useCWkeylineDTR == true) ) )
		return;

// Hamlib ptt
#ifndef NOHAMLIB
	if (pttdev == 1) {
		hamlib_set_ptt(ptt);
		return;
	}
#endif
// Memory mapped i/o
	if (pttdev == 2) { 
		setrigMEM_PTT (ptt);
		return;
	}
	if (pttdev == 3){
		rigCAT_set_ptt (ptt);
		return;
	}
	if (pttfd == -1)
		return;
	
	int status;
	ioctl(pttfd, TIOCMGET, &status);

	if (ptt) {
		if (progdefaults.RTSptt == true && progdefaults.RTSplus == false)
			status |= TIOCM_RTS;
		if (progdefaults.RTSptt == true && progdefaults.RTSplus == true)
			status &= ~TIOCM_RTS;
		if (progdefaults.DTRptt == true && progdefaults.DTRplus == false)
			status |= TIOCM_DTR;
		if (progdefaults.DTRptt == true && progdefaults.DTRplus == true)
			status &= ~TIOCM_DTR;
	} else {
		if (progdefaults.RTSptt == true && progdefaults.RTSplus == false)
			status &= ~TIOCM_RTS;
		if (progdefaults.RTSptt == true && progdefaults.RTSplus == true)
			status |= TIOCM_RTS;
		if (progdefaults.DTRptt == true && progdefaults.DTRplus == false)
			status &= ~TIOCM_DTR;
		if (progdefaults.DTRptt == true && progdefaults.DTRplus == true)
			status |= TIOCM_DTR;
	}

	ioctl(pttfd, TIOCMSET, &status);

}


void PTT::openptt()
{
//	unsigned int arg = pttarg;
	int status;
	if ((pttfd = open(pttdevName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
		return;

	tcgetattr (pttfd, &oldtio);

	ioctl(pttfd, TIOCMGET, &status);

	if (progdefaults.RTSplus)
		status |= TIOCM_RTS;		// set RTS bit
	else
		status &= ~TIOCM_RTS;		// clear RTS bit
	if (progdefaults.DTRplus)
		status |= TIOCM_DTR;		// set DTR bit
	else
		status &= ~TIOCM_DTR;		// clear DTR bit

	ioctl(pttfd, TIOCMSET, &status);

	return;
}


