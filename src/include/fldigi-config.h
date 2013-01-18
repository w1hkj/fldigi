// ----------------------------------------------------------------------------
// fldigi-config.h  --  user configuration items for fldigi
//
// Copyright (C) 2006-2009
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

#ifndef FLDIGI_CONFIG_H
#define FLDIGI_CONFIG_H

//=============================================================================
// You can change the x1 width of the waterfall / spectrum display by modifying this
// constant.
// Suggest that you make the value a multiple of 100.
// DO NOT EXCEED 4000
// The larger the number the greater the cpu load will be for creating the
// waterfall display
//
// Setting the DEFAULT_IMAGE_WIDTH to 3200 will size the x1 waterfall to be
// 800 pixels wide.  The x1 waterfall size is always DEFAULT_IMAGE_WIDTH / 4
// and the minimum width of main display would then be
//
// DEFAULT_IMAGE_WIDTH / 4 + 2 * BEZEL + 2 * DEFAULT_SW
//
// where BEZEL is set to 2 (border around the waterfall), and
// DEFAULT_SW is the width of the signal level and squelch controls

#define DEFAULT_IMAGE_WIDTH 3000
//=============================================================================

// widget sizes internal to the waterfall widget
#define BEZEL		 2
#define WFTEXT		10
#define WFSCALE     10
#define WFMARKER     6
#define BTN_HEIGHT	20

#define DEFAULT_SW 16
//#define DEFAULT_HWFALL 144
#define DEFAULT_HWFALL 124
#define DEFAULT_HNOM 500
#define WMIN 645
#define HMIN 450
//#define Wwfall		(DEFAULT_HNOM + 2 * BEZEL)
#define DEFAULT_WNOM (Wwfall + 2* DEFAULT_SW)

//#define EMC_HWFALL 144
//#define EMC_HNOM 500
//#define EMC_WNOM (500 + 2 * DEFAULT_SW + 2 * BEZEL)

extern int IMAGE_WIDTH;
extern int Hwfall;
extern int HNOM;
extern int WNOM;
extern int Wwfall;
extern int Haqsoframe;
extern int Hmenu;
extern int Hstatus;
extern int Hmacros;

//#define Htext		(DEFAULT_HNOM - 4 - Hwfall - Hmenu - Hstatus - Hmacros - Hqsoframe)
//#define Hrcvtxt		(Htext) / 2
//#define Hxmttxt		(Htext - (Hrcvtxt))

#define Wmode 		80
#define Ws2n  		120
#define Wimd  		120
#define Wwarn 		16
#define bwTxLevel	120
#define bwAfcOnOff	(Hwfall -22)/2
#define bwSqlOnOff	(Hwfall -22)/2

#define Wstatus (WNOM - Wmode - Ws2n - Wimd - bwAfcOnOff - bwSqlOnOff - Wwarn)

#define SCOPEWIN_MIN_WIDTH 48
#define SCOPEWIN_MIN_HEIGHT 48

#endif // FLDIGI_CONFIG_H
