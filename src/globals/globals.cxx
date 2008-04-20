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

#include <config.h>
#include <iosfwd>
#include <iomanip>
#include <sstream>

#include "globals.h"
#include "modem.h"

const char *state_names[] = {
	"PAUSED",
	"RECEIVE",
	"TRANSMIT",
	"TUNING",
	"ABORTED",
	"FLUSHING"
};

// Elements are in enum trx_mode order. Mode name video-id uses the
// first string (sname), so its length should be a multiple of 2.
const struct mode_info_t mode_info[NUM_MODES] = {
	{ MODE_CW, &cw_modem, "CW", "CW", "CW" },

	{ MODE_DOMINOEX4, &dominoex4_modem, "DomEX4", "DominoEX 4", "DOMINOEX4", "DOMINO" }, // These aren't Domino FEC, right?
	{ MODE_DOMINOEX5, &dominoex5_modem, "DomEX5", "DominoEX 5", "DOMINOEX5", "DOMINO" },
	{ MODE_DOMINOEX8, &dominoex8_modem, "DomEX8", "DominoEX 8", "DOMINOEX8", "DOMINO" },
	{ MODE_DOMINOEX11, &dominoex11_modem, "DomX11", "DominoEX 11", "DOMINOEX11", "DOMINO" },
	{ MODE_DOMINOEX16, &dominoex16_modem, "DomX16", "DominoEX 16", "DOMINOEX16", "DOMINO" },
	{ MODE_DOMINOEX22, &dominoex22_modem, "DomX22", "DominoEX 22", "DOMINOEX22", "DOMINO" },

	{ MODE_FELDHELL, &feld_modem, "FELDHELL", "Feld Hell", "", "HELL" },
	{ MODE_SLOWHELL, &feld_slowmodem, "SLOWHELL", "Slow Hell", "", "HELL" }, // Not sure about these after the first one
	{ MODE_HELLX5, &feld_x5modem, "HELLX5", "Feld Hell X5", "", "HELL" },
	{ MODE_HELLX9, &feld_x9modem, "HELLX9", "Feld Hell X9", "", "HELL"},	
	{ MODE_FSKHELL, &feld_FMmodem, "FSK-HELL", "FSK Hell", "", "FMHELL" },
	{ MODE_FSKH105, &feld_FM105modem, "FSK-H105", "FSK Hell-105", "", "FMHELL" },
	{ MODE_HELL80, &feld_80modem, "HELL80", "Hell 80", "", "HELL80" },

	{ MODE_MFSK8, &mfsk8_modem, "MFSK-8", "MFSK-8", "MFSK8", "MFSK8" }, // !! These were swapped
	{ MODE_MFSK16, &mfsk16_modem, "MFSK16", "MFSK-16", "MFSK16", "MFSK16" },

	{ MODE_MT63_500, &mt63_500_modem, "MT63-500", "MT63-500", "", "MT63" },
	{ MODE_MT63_1000, &mt63_1000_modem, "MT63-1XX", "MT63-1000", "", "MT63" },
	{ MODE_MT63_2000, &mt63_2000_modem, "MT63-2XX", "MT63-2000", "", "MT63" },

	{ MODE_BPSK31, &psk31_modem, "BPSK31", "BPSK-31", "PSK31", "PSK31" },
	{ MODE_QPSK31, &qpsk31_modem, "QPSK31", "QPSK-31", "QPSK31", "QPSK31" },
	{ MODE_PSK63, &psk63_modem, "PSK-63", "BPSK-63", "PSK63", "PSK63" },
	{ MODE_QPSK63, &qpsk63_modem, "QPSK63", "QPSK-63", "QPSK63", "QPSK63" },
	{ MODE_PSK125, &psk125_modem, "PSK125", "BPSK-125", "PSK125", "PSK125" },
	{ MODE_QPSK125, &qpsk125_modem, "QPSK-125", "QPSK-125", "QPSK125", "PSK125" },
	{ MODE_PSK250, &psk250_modem, "PSK-250", "BPSK-250", "PSK250", "PSK250" }, // PSK250 not supported in 2.1.9
	{ MODE_QPSK250, &qpsk250_modem, "QPSK-250", "QPSK-250", "QPSK250", "QPSK250" }, // PSK250 not supported in 2.1.9

	{ MODE_OLIVIA, &olivia_modem, "OLIVIA", "Olivia", "", "OLIVIA" },

	{ MODE_RTTY, &rtty_modem, "RTTY", "RTTY", "RTTY", "RTTY" },

	{ MODE_THROB1, &throb1_modem, "THROB1", "Throb 1", "", "THRB" },
	{ MODE_THROB2, &throb2_modem, "THROB2", "Throb 2", "", "THRB" },
	{ MODE_THROB4, &throb4_modem, "THROB4", "Throb 4", "", "THRB" },
	{ MODE_THROBX1, &throbx1_modem, "THRBX1", "ThrobX 1", "", "THRBX" },
	{ MODE_THROBX2, &throbx2_modem, "THRBX2", "ThrobX 2", "", "THRBX" },
	{ MODE_THROBX4, &throbx4_modem, "THRBX4", "ThrobX 4", "", "THRBX" },

	{ MODE_WWV, &wwv_modem, "WWV", "WWV", "", "" },

	{ MODE_ANALYSIS, &anal_modem, "ANALYSIS", "Freq Analysis", "", "" }
};

std::ostream& operator<<(std::ostream& s, const qrg_mode_t& m)
{
	return s << m.rfcarrier << ' ' << m.rmode << ' ' << m.carrier << ' ' << mode_info[m.mode].sname;
}

std::istream& operator>>(std::istream& s, qrg_mode_t& m)
{
	string sMode;
	int mnbr;
	s >> m.rfcarrier >> m.rmode >> m.carrier >> sMode;
// handle case for reading older type of specification string
	if (sscanf(sMode.c_str(), "%d", &mnbr)) {
		m.mode = mnbr;
		return s;
	}
	m.mode = MODE_BPSK31;
	for (mnbr = MODE_CW; mnbr < NUM_MODES; mnbr++)
		if (sMode == mode_info[mnbr].sname) {
			m.mode = mnbr;
			break;
		}
	return s;
}

std::string qrg_mode_t::str(void)
{
	ostringstream s;
	s << setiosflags(ios::fixed) << setprecision(3) << rfcarrier/1000.0 << '\t'
          << rmode << '\t'
          << (mode < NUM_MODES ? mode_info[mode].sname : "NONE") << '\t'
          << carrier;
	return s.str();


	// This an example of how we would do things if we were not using
	// Fl_Browser and had to format the fields manually and add the string
	// to a menu

	// static unsigned max_mode_sname = 0;

	// if (max_mode_sname == 0)
	//	   for (size_t i = 0; i < NUM_MODES; i++)
	//		   if (max_mode_sname < strlen(mode_info[i].sname))
	//			   max_mode_sname = strlen(mode_info[i].sname);

	// ostringstream s;

	// s << setw(11) << setiosflags(ios::right) << setiosflags(ios::fixed) << setprecision(3)
	//   << rfcarrier/1000.0 << ' '
	//   << setw(max_mode_sname) << resetiosflags(ios::right) << setiosflags(ios::left)
	//   << (mode < NUM_MODES ? mode_info[mode].sname : "NONE") << ' '
	//   << setw(4) << resetiosflags(ios::left) << setiosflags(ios::right)
	//   << carrier << ' ' << rmode;

	// return s.str();
}
