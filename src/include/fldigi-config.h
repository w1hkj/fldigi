// ----------------------------------------------------------------------------
// fldigi-config.h  --  user configuration items for fldigi
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted in part from code contained in gmfsk 
// source code distribution.
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

#ifndef FLDIGI_CONFIG_H
#define FLDIGI_CONFIG_H

// You can change the x1 width of the waterfall / spectrum display by modifying this
// constant.
// Suggest that you make the value a multiple of 100.
// DO NOT EXCEED 4000
// The larger the number the greater the cpu load will be for creating the
// waterfall display
#define DEFAULT_IMAGE_WIDTH 3000
#define DEFAULT_HWFALL 140
#define DEFAULT_HNOM 570
#define DEFAULT_WNOM (DEFAULT_IMAGE_WIDTH / 4 + DEFAULT_HWFALL - BTN_HEIGHT)

#define EMC_HWFALL 140
#define EMC_HNOM 500
#define EMC_WNOM (EMC_HWFALL + 420)

extern int IMAGE_WIDTH;
extern int Hwfall;
extern int HNOM;
extern int WNOM;

// widget sizes internal to the waterfall widget
#define BEZEL		 2
#define WFTEXT		10
#define WFSCALE     10
#define WFMARKER     6
#define BTN_HEIGHT	20

#define bwColor		25
#define bwFFT		30
#define bwX1		20
#define bwMov		20
#define cwCnt		100
#define cwRef		50
#define cwMode		85
#define bwQsy		35
#define bwRate		45
#define bwXmtLock	40
#define bwRev		40
#define bwMem		37
#define bwXmtRcv	45
#define wSpace		2

#define Hmenu		22
#define Hqsoframe	48
#define Hnotes		22
#define Hstatus		22
#define Hmacros		22

#define Htext		(HNOM - 4 - Hwfall - Hmenu - Hstatus - Hmacros - Hqsoframe - Hnotes)
// Htext = HNOM - 140 - Hwfall
#define Hrcvtxt		(Htext) / 2
#define Hxmttxt		(Htext - (Hrcvtxt))

#define Wwfall		(WNOM - Hwfall + BTN_HEIGHT + 2 * BEZEL)
#define Wmode 		80
#define Ws2n  		100
#define Wimd  		100
#define Wwarn 		16
#define bwAfcOnOff	(Hwfall -22)/2
#define bwSqlOnOff	(Hwfall -22)/2

#define Wstatus (WNOM - Wmode - Ws2n - Wimd - bwAfcOnOff - bwSqlOnOff - Wwarn)

//remove the comment delimiter to enable experimental psk250 and qpsk250 modes

#define USE250

//remove the comment delimiter to enable both old and new text widget class
//switching from one to the other requires a restart of the program before
//the newly selected widget becomes effective.
//
//default is to use the new text widget class that is a fully enabled editor
//widget
//#define USE_BOTH_TEXT_WIDGETS


#endif // FLDIGI_CONFIG_H
