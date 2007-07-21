// ----------------------------------------------------------------------------
// globals.cxx  --  constants, variables, arrays & functions that need to be
//                  outside of any thread
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

#include "globals.h"

const char *mode_names[] = {
	"MFSK16",
	"MFSK-8",
	"OLIVIA",
	"RTTY",
	"THROB1",
	"THROB2",
	"THROB4",
	"THRBX1",
	"THRBX2",
	"THRBX4",
	"BPSK31",
	"QPSK31",
	"PSK-63",
	"QPSK63",
	"PSK125",
	"QPSK-125",
	"PSK-250",
	"QPSK-250",
	"MT63",
	"FELDHELL",
	"FSK-HELL",
	"FSK-H105",
	"CW",
	"DomEX4",
	"DomEX5",
	"DomEX8",
	"DomX11",
	"DomX16",
	"DomX22",
	"WWV",
	"ANALYSIS"
};

const char *state_names[] = {
	"PAUSED",
	"RECEIVE",
	"TRANSMIT",
	"TUNING",
	"ABORTED",
	"FLUSHING"
};

