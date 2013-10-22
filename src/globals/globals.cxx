// ----------------------------------------------------------------------------
// globals.cxx  --  constants, variables, arrays & functions that need to be
//                  outside of any thread
//
// Copyright (C) 2006-2009
//		Dave Freese, W1HKJ
// Copyright (C) 2007-2009
//		Stelios Bounanos, M0GLD
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

#include <config.h>
#include <iosfwd>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cerrno>
#include <cstdio>

#include "gettext.h"
#include "globals.h"
#include "modem.h"

using namespace std;

// Elements are in enum trx_mode order.
// N.B. it is not valid to use an _("NLS") string in this table!!
// ... doing so will break the Fl_menu_item table 'menu_'.  -Kamal

const struct mode_info_t mode_info[NUM_MODES] = {
	{ MODE_NULL, &null_modem, "NULL", "NULL", "", "NULL", "" },

    { MODE_CW, &cw_modem, "CW", "CW", "CW", "CW", "CW" },

	{ MODE_CONTESTIA, &contestia_modem, "CTSTIA", "Contestia", "", "CONTESTI", "CT" },

	{ MODE_DOMINOEX4, &dominoex4_modem, "DOMEX4", "DominoEX 4", "DOMINOEX4", "DOMINO", "DM 4" },
	{ MODE_DOMINOEX5, &dominoex5_modem, "DOMEX5", "DominoEX 5", "DOMINOEX5", "DOMINO", "DM 5" },
	{ MODE_DOMINOEX8, &dominoex8_modem, "DOMEX8", "DominoEX 8", "DOMINOEX8", "DOMINO", "DM 8" },
	{ MODE_DOMINOEX11, &dominoex11_modem, "DOMX11", "DominoEX 11", "DOMINOEX11", "DOMINO", "DM11" },
	{ MODE_DOMINOEX16, &dominoex16_modem, "DOMX16", "DominoEX 16", "DOMINOEX16", "DOMINO", "DM16" },
	{ MODE_DOMINOEX22, &dominoex22_modem, "DOMX22", "DominoEX 22", "DOMINOEX22", "DOMINO", "DM22" },
	{ MODE_DOMINOEX44, &dominoex44_modem, "DOMX44", "DominoEX 44", "DOMINOEX44", "DOMINO", "DM44" },
	{ MODE_DOMINOEX88, &dominoex88_modem, "DOMX88", "DominoEX 88", "DOMINOEX88", "DOMINO", "DM88" },

	{ MODE_FELDHELL, &feld_modem, "FELDHELL", "Feld Hell", "", "HELL", "HELL" },
	{ MODE_SLOWHELL, &feld_slowmodem, "SLOWHELL", "Slow Hell", "", "HELL", "SHLL" },
	{ MODE_HELLX5, &feld_x5modem, "HELLX5", "Feld Hell X5", "", "HELL", "HLX5" },
	{ MODE_HELLX9, &feld_x9modem, "HELLX9", "Feld Hell X9", "", "HELL", "HLX9"},
	{ MODE_FSKHELL, &feld_FMmodem, "FSKHELL", "FSK Hell", "", "FMHELL", "FMHL"  },
	{ MODE_FSKH105, &feld_FM105modem, "FSKH105", "FSK Hell-105", "", "FMHELL", "H105" },
	{ MODE_HELL80, &feld_80modem, "HELL80", "Hell 80", "", "HELL80", "HL80" },

	{ MODE_MFSK8, &mfsk8_modem, "MFSK8", "MFSK-8", "MFSK8", "MFSK8", "MK 8" },
	{ MODE_MFSK16, &mfsk16_modem, "MFSK16", "MFSK-16", "MFSK16", "MFSK16", "MK16" },
	{ MODE_MFSK32, &mfsk32_modem, "MFSK32", "MFSK-32", "MFSK32", "MFSK32", "MK32" },

	{ MODE_MFSK4, &mfsk4_modem, "MFSK4", "MFSK-4", "MFSK4", "MFSK4", "MK 4" },
	{ MODE_MFSK11, &mfsk11_modem, "MFSK11", "MFSK-11", "MFSK11", "MFSK11", "MK11" },
	{ MODE_MFSK22, &mfsk22_modem, "MFSK22", "MFSK-22", "MFSK22", "MFSK22", "MK22" },
	{ MODE_MFSK31, &mfsk31_modem, "MFSK31", "MFSK-31", "MFSK31", "MFSK31", "MK31" },
	{ MODE_MFSK64, &mfsk64_modem, "MFSK64", "MFSK-64", "MFSK64", "MFSK64", "MK64" },
	{ MODE_MFSK128, &mfsk128_modem, "MFSK128", "MFSK-128", "MFSK128", "MFSK128", "MK128" },
	{ MODE_MFSK64L, &mfsk64l_modem, "MFSK64L", "MFSK-64L", "MFSK64L", "MFSK64L", "MK64L" },
	{ MODE_MFSK128L, &mfsk128l_modem, "MFSK128L", "MFSK-128L", "MFSK128L", "MFSK128L", "MK128L" },
 
	{ MODE_WEFAX_576, &wefax576_modem, "WEFAX576", "WEFAX-IOC576", "WEFAXIOC576", "FAX", "FX576" },
	{ MODE_WEFAX_288, &wefax288_modem, "WEFAX288", "WEFAX-IOC288", "WEFAXIOC288", "FAX", "FX288" },

	{ MODE_NAVTEX, &navtex_modem, "NAVTEX", "NAVTEX", "NAVTEX", "TOR", "NAVTEX" },
	{ MODE_SITORB, &sitorb_modem, "SITORB", "SITORB", "SITORB", "TOR", "SITORB" },

	{ MODE_MT63_500S, &mt63_500S_modem, "MT63-500S", "MT63-500S", "MT63-500S", "MT63", "MT63-500S" },
	{ MODE_MT63_500L, &mt63_500L_modem, "MT63-500L", "MT63-500L", "MT63-500L", "MT63", "MT63-500L" },
	{ MODE_MT63_1000S, &mt63_1000S_modem, "MT63-1KS", "MT63-1000S", "MT63-1XXS", "MT63", "MT63 1KS" },
	{ MODE_MT63_1000L, &mt63_1000L_modem, "MT63-1KL", "MT63-1000L", "MT63-1XXL", "MT63", "MT63 1KL" },
	{ MODE_MT63_2000S, &mt63_2000S_modem, "MT63-2KS", "MT63-2000S", "MT63-2XXS", "MT63", "MT63 2KS" },
	{ MODE_MT63_2000L, &mt63_2000L_modem, "MT63-2KL", "MT63-2000L", "MT63-2XXL", "MT63", "MT63 2KL" },

	{ MODE_PSK31, &psk31_modem, "BPSK31", "BPSK-31", "PSK31", "PSK31", "P31" },
	{ MODE_PSK63, &psk63_modem, "BPSK63", "BPSK-63", "PSK63", "PSK63", "P63" },
	{ MODE_PSK63F, &psk63f_modem, "BPSK63F", "BPSK-63F", "PSK63F", "PSK63F", "P63F" },
	{ MODE_PSK125, &psk125_modem, "BPSK125", "BPSK-125", "PSK125", "PSK125", "P125" },
	{ MODE_PSK250, &psk250_modem, "BPSK250", "BPSK-250", "PSK250", "PSK250", "P250" },
	{ MODE_PSK500, &psk500_modem, "BPSK500", "BPSK-500", "PSK500", "PSK500", "P500" },

	{ MODE_QPSK31, &qpsk31_modem, "QPSK31", "QPSK-31", "QPSK31", "QPSK31", "Q31" },
	{ MODE_QPSK63, &qpsk63_modem, "QPSK63", "QPSK-63", "QPSK63", "QPSK63", "Q63" },
	{ MODE_QPSK125, &qpsk125_modem, "QPSK125", "QPSK-125", "QPSK125", "QPSK125", "Q125" },
	{ MODE_QPSK250, &qpsk250_modem, "QPSK250", "QPSK-250", "QPSK250", "QPSK250", "Q250" },
	{ MODE_QPSK500, &qpsk500_modem, "QPSK500", "QPSK-500", "QPSK500", "QPSK500", "Q500" },

	{ MODE_PSK125R, &psk125r_modem, "PSK125R", "PSK-125R", "PSK125R", "PSK125R", "P125R" },
	{ MODE_PSK250R, &psk250r_modem, "PSK250R", "PSK-250R", "PSK250R", "PSK250R", "P250R" },
	{ MODE_PSK500R, &psk500r_modem, "PSK500R", "PSK-500R", "PSK500R", "PSK500R", "P500R" },

	{ MODE_PSK1000, &psk1000_modem, "BPSK1000", "BPSK-1000", "PSK1000", "PSK1000", "P1000" },
	{ MODE_PSK1000R, &psk1000r_modem, "PSK1000R", "PSK-1000R", "PSK1000R", "PSK1000R", "PSK1000R" },

	{ MODE_OLIVIA, &olivia_modem, "OLIVIA", "OLIVIA", "OLIVIA", "OLIVIA", "OL" },
	{ MODE_OLIVIA_4_250, &olivia_4_250_modem, "Olivia-4-250", "OL 4-250", "OLIV 4/250", "OLIVIA", "OL4/250" },
	{ MODE_OLIVIA_8_250, &olivia_8_250_modem, "Olivia-8-250", "OL 8-250", "OLIV 8/250", "OLIVIA", "OL8/250" },
	{ MODE_OLIVIA_4_500, &olivia_4_500_modem, "Olivia-4-500", "OL 4-500", "OLIV 4/500", "OLIVIA", "OL4/500" },
	{ MODE_OLIVIA_8_500, &olivia_8_500_modem, "Olivia-8-500", "OL 8-500", "OLIV 8/500", "OLIVIA", "OL8/500" },
	{ MODE_OLIVIA_16_500, &olivia_16_500_modem, "Olivia-16-500", "OL 16-500", "OLIV 16/500", "OLIVIA", "OL16/500" },
	{ MODE_OLIVIA_8_1000, &olivia_8_1000_modem, "Olivia-8-1K", "OL 8-1K", "OLIV 8/1K", "OLIVIA", "OL8/1K" },
	{ MODE_OLIVIA_16_1000, &olivia_16_1000_modem, "Olivia-16-1K", "OL 16-1K", "OLIV 16/1K", "OLIVIA", "OL16/1K" },
	{ MODE_OLIVIA_32_1000, &olivia_32_1000_modem, "Olivia-32-1K", "OL 32-1K", "OLIV 32/1K", "OLIVIA", "OL32/1K" },
	{ MODE_OLIVIA_64_2000, &olivia_64_2000_modem, "Olivia-64-2K", "OL 64-2K", "OLIV 64/2K", "OLIVIA", "OL64/2K" },

	{ MODE_RTTY, &rtty_modem, "RTTY", "RTTY", "RTTY", "RTTY", "RY" },

	{ MODE_THOR4, &thor4_modem, "THOR4", "THOR 4", "THOR4", "THOR", "TH4" },
	{ MODE_THOR5, &thor5_modem, "THOR5", "THOR 5", "THOR5", "THOR", "TH5" },
	{ MODE_THOR8, &thor8_modem, "THOR8", "THOR 8", "THOR8", "THOR", "TH8" },
	{ MODE_THOR11, &thor11_modem, "THOR11", "THOR 11", "THOR11", "THOR", "TH11" },
	{ MODE_THOR16, &thor16_modem, "THOR16", "THOR 16", "THOR16", "THOR", "TH16" },
	{ MODE_THOR22, &thor22_modem, "THOR22", "THOR 22", "THOR22", "THOR", "TH22" },
	{ MODE_THOR25x4, &thor25x4_modem, "THOR25x4", "THOR 25 x4", "THOR25x4", "THOR", "TH25" },
	{ MODE_THOR50x1, &thor50x1_modem, "THOR50x1", "THOR 50 x1", "THOR50x1", "THOR", "TH51" },
	{ MODE_THOR50x2, &thor50x2_modem, "THOR50x2", "THOR 50 x2", "THOR50x2", "THOR", "TH52" },
	{ MODE_THOR100, &thor100_modem, "THOR100", "THOR 100", "THOR100", "THOR", "TH10" },


	{ MODE_THROB1, &throb1_modem, "THROB1", "Throb 1", "", "THRB", "TB1" },
	{ MODE_THROB2, &throb2_modem, "THROB2", "Throb 2", "", "THRB", "TB2" },
	{ MODE_THROB4, &throb4_modem, "THROB4", "Throb 4", "", "THRB", "TB4" },
	{ MODE_THROBX1, &throbx1_modem, "THRBX1", "ThrobX 1", "", "THRBX", "TX1" },
	{ MODE_THROBX2, &throbx2_modem, "THRBX2", "ThrobX 2", "", "THRBX", "TX2" },
	{ MODE_THROBX4, &throbx4_modem, "THRBX4", "ThrobX 4", "", "THRBX", "TX4" },

	{ MODE_4X_PSK63R, &psk63r_c4_modem, "PSK63RC4", "4xPSK63R", "PSK63RC4", "PSK63RC4", "P63R4" },
	{ MODE_5X_PSK63R, &psk63r_c5_modem, "PSK63RC5", "5xPSK63R", "PSK63RC5", "PSK63RC5", "P63R5" },
	{ MODE_10X_PSK63R, &psk63r_c10_modem, "PSK63RC10", "10xPSK63R", "PSK63RC10", "PSK63RC10", "P63R10" },
	{ MODE_20X_PSK63R, &psk63r_c20_modem, "PSK63RC20", "20xPSK63R", "PSK63RC20", "PSK63RC20", "P63R20" },
	{ MODE_32X_PSK63R, &psk63r_c32_modem, "PSK63RC32", "32xPSK63R", "PSK63RC32", "PSK63RC32", "P63R32" },

	{ MODE_4X_PSK125R, &psk125r_c4_modem, "PSK125RC4", "4xPSK125R", "PSK125RC4", "PSK125RC4", "P125R4" },
	{ MODE_5X_PSK125R, &psk125r_c5_modem, "PSK125RC5", "5xPSK125R", "PSK125RC5", "PSK125RC5", "P125R5" },
	{ MODE_10X_PSK125R, &psk125r_c10_modem, "PSK125RC10", "10xPSK125R", "PSK125RC10", "PSK125RC10", "P125R10" },
	{ MODE_12X_PSK125, &psk125_c12_modem, "PSK125C12", "12xPSK125", "PSK125C12", "PSK125C12", "P125C12" },
	{ MODE_12X_PSK125R, &psk125r_c12_modem, "PSK125RC12", "12xPSK125R", "PSK125RC12", "PSK125RC12", "P125R12" },
	{ MODE_16X_PSK125R, &psk125r_c16_modem, "PSK125RC16", "16xPSK125R", "PSK125RC16", "PSK125RC16", "P125R16" },

	{ MODE_6X_PSK250, &psk250_c6_modem, "PSK250C6", "6xPSK250", "PSK250C6", "PSK250C6", "P2506"},

	{ MODE_2X_PSK250R, &psk250r_c2_modem, "PSK250RC2", "2xPSK250R", "PSK250RC2", "PSK250RC2", "P250R2" },
	{ MODE_3X_PSK250R, &psk250r_c3_modem, "PSK250RC3", "3xPSK250R", "PSK250RC3", "PSK250RC3", "P250R3" },
	{ MODE_5X_PSK250R, &psk250r_c5_modem, "PSK250RC5", "5xPSK250R", "PSK250RC5", "PSK250RC5", "P250R5"},
	{ MODE_6X_PSK250R, &psk250r_c6_modem, "PSK250RC6", "6xPSK250R", "PSK250RC6", "PSK250RC6", "P250R6"},
	{ MODE_7X_PSK250R, &psk250r_c7_modem, "PSK250RC7", "7xPSK250R", "PSK250RC7", "PSK250RC7", "P250R7"},

	{ MODE_2X_PSK500, &psk500_c2_modem, "PSK500C2", "2xPSK500", "PSK500C2", "PSK500C2", "2xP500" },
	{ MODE_4X_PSK500, &psk500_c4_modem, "PSK500C4", "4xPSK500", "PSK500C4", "PSK500C4", "4xP500" },

	{ MODE_2X_PSK500R, &psk500r_c2_modem, "PSK500RC2", "2xPSK500R", "PSK500RC2", "PSK500RC2", "P500R2" },
	{ MODE_3X_PSK500R, &psk500r_c3_modem, "PSK500RC3", "3xPSK500R", "PSK500RC3", "PSK500RC3", "P500R3" },
	{ MODE_4X_PSK500R, &psk500r_c4_modem, "PSK500RC4", "4xPSK500R", "PSK500RC4", "PSK500RC4", "P500R4"},

	{ MODE_2X_PSK800, &psk800_c2_modem, "PSK800C2", "2xPSK800", "PSK800C2", "PSK800C2", "P800RC2" },

	{ MODE_2X_PSK800R, &psk800r_c2_modem, "PSK800RC2", "2xPSK800R", "PSK800RC2", "PSK800RC2", "P800RC2" },

	{ MODE_2X_PSK1000, &psk1000_c2_modem, "PSK1000C2", "2xPSK1000", "PSK1000C2", "PSK1000C2", "P1KC2" },
	{ MODE_2X_PSK1000R, &psk1000r_c2_modem, "PSK1000RC2", "2xPSK1000R", "PSK1000RC2", "PSK1000RC2", "P1KRC2" },

	{ MODE_SSB, &ssb_modem, "SSB", "SSB", "", "SSB", "" },
	{ MODE_WWV, &wwv_modem, "WWV", "WWV", "", "", "" },
	{ MODE_ANALYSIS, &anal_modem, "ANALYSIS", "Freq Analysis", "", "", "" }

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
	m.mode = MODE_PSK31;
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
	s << setiosflags(ios::left | ios::fixed)
	  << setw(12) << setprecision(3) << rfcarrier/1000.0
	  << setw(8) << rmode
	  << setw(10) << (mode < NUM_MODES ? mode_info[mode].sname : "NONE")
	  << carrier;
	return s.str();
}


band_t band(long long freq_hz)
{
	switch (freq_hz / 1000000LL) {
		case 0: case 1: return BAND_160M;
		case 3: return BAND_80M;
		case 4: return BAND_75M;
		case 5: return BAND_60M;
		case 7: return BAND_40M;
		case 10: return BAND_30M;
		case 14: return BAND_20M;
		case 18: return BAND_17M;
		case 21: return BAND_15M;
		case 24: return BAND_12M;
		case 28 ... 29: return BAND_10M;
		case 50 ... 54: return BAND_6M;
		case 70 ... 71: return BAND_4M;
		case 144 ... 148: return BAND_2M;
		case 222 ... 225: return BAND_125CM;
		case 420 ... 450: return BAND_70CM;
		case 902 ... 928: return BAND_33CM;
		case 1240 ... 1325: return BAND_23CM;
		case 2300 ... 2450: return BAND_13CM;
		case 3300 ... 3500: return BAND_9CM;
		case 5650 ... 5925: return BAND_6CM;
		case 10000 ... 10500: return BAND_3CM;
		case 24000 ... 24250: return BAND_125MM;
		case 47000 ... 47200: return BAND_6MM;
		case 75500 ... 81000: return BAND_4MM;
		case 119980 ... 120020: return BAND_2P5MM;
		case 142000 ... 149000: return BAND_2MM;
		case 241000 ... 250000: return BAND_1MM;
	}

	return BAND_OTHER;
}

band_t band(const char* freq_mhz)
{
	errno = 0;
	double d = strtod(freq_mhz, NULL);
	if (d != 0.0 && errno == 0)
		return band((long long)(d * 1e6));
	else
		return BAND_OTHER;
}

struct band_freq_t {
	const char* band;
	const char* freq;
};

static struct band_freq_t band_names[NUM_BANDS] = {
	{ "160m", "1.8" },
	{ "80m", "3.5" },
	{ "75m", "4.0" },
	{ "60m", "5.3" },
	{ "40m", "7.0" },
	{ "30m", "10.0" },
	{ "20m", "14.0" },
	{ "17m", "18.0" },
	{ "15m", "21.0" },
	{ "12m", "24.0" },
	{ "10m", "28.0" },
	{ "6m", "50.0" },
	{ "4m", "70.0" },
	{ "2m", "144.0" },
	{ "1.25m", "222.0" },
	{ "70cm", "420.0" },
	{ "33cm", "902.0" },
	{ "23cm", "1240.0" },
	{ "13cm", "2300.0" },
	{ "9cm", "3300.0" },
	{ "6cm", "5650.0" },
	{ "3cm", "10000.0" },
	{ "1.25cm", "24000.0" },
	{ "6mm", "47000.0" },
	{ "4mm", "75500.0" },
	{ "2.5mm", "119980.0" },
	{ "2mm", "142000.0" },
	{ "1mm", "241000.0" },
	{ "other", "" }
};

const char* band_name(band_t b)
{
	return band_names[CLAMP(b, 0, NUM_BANDS-1)].band;
}

const char* band_name(const char* freq_mhz)
{
	return band_name(band(freq_mhz));
}

const char* band_freq(band_t b)
{
	return band_names[CLAMP(b, 0, NUM_BANDS-1)].freq;
}

const char* band_freq(const char* band_name)
{
	for (size_t i = 0; i < BAND_OTHER; i++)
		if (!strcmp(band_names[i].band, band_name))
			return band_names[i].freq;

	return "";
}
