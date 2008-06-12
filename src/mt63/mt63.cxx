//=============================================================================
//
//    mt63.cxx  --  MT63 modem for fldigi
//
//    Copyright (C) 2007, 2008
//      Dave Freese, W1HKJ
//
//    This file is part of fldigi
//
//    fldigi is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    fldigi is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with gMFSK; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//=============================================================================

#include <config.h>

#include <string>
#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "mt63.h"
#include "status.h"

using namespace std;

static int IntegLen = 32;	// integration period for sync./data tracking

void mt63::tx_init(SoundBase *sb)
{
	scard = sb;
	Tx->Preset((int)bandwidth, Interleave == 64 ? 1 : 0);
	set_freq(500.0 + bandwidth / 2.0);
	flush = Tx->DataInterleave;
	videoText();
}

void mt63::rx_init()
{
	Rx->Preset((int)bandwidth, Interleave == 64 ? 1 : 0, IntegLen);
	set_freq(500.0 + bandwidth / 2.0);
	InpLevel->Preset(64.0, 0.75);
	escape = 0;
}

double peak = 0.0;

int mt63::tx_process()
{
	int c;

	c = get_tx_char();
	if (c == 0x03)  {
		stopflag = true;
		flush = Tx->DataInterleave;
		c = 0;
	}

	if (c == -1 || stopflag == true) c = 0;
	
	if (stopflag && flush-- == 0) {
		stopflag = false;
		Tx->SendJam();
		for (int i = 0; i < Tx->Comb.Output.Len; i++)
			Tx->Comb.Output.Data[i] /= 0.8;
		ModulateXmtr((Tx->Comb.Output.Data), Tx->Comb.Output.Len);
		cwid();
		return -1;	/* we're done */
	}

	if ((progdefaults.mt63_8bit && c > 255) || (!progdefaults.mt63_8bit && c > 127))
		c = '.';

	put_echo_char(c);
	
	if (c > 127) {
		c &= 127;
		Tx->SendChar(127);
		for (int i = 0; i < Tx->Comb.Output.Len; i++)
			Tx->Comb.Output.Data[i] /= 0.8;
		ModulateXmtr((Tx->Comb.Output.Data), Tx->Comb.Output.Len);
	}

	Tx->SendChar(c);
		for (int i = 0; i < Tx->Comb.Output.Len; i++)
			Tx->Comb.Output.Data[i] /= 0.8;
	ModulateXmtr((Tx->Comb.Output.Data), Tx->Comb.Output.Len);

	return 0;
}

int mt63::rx_process(const double *buf, int len)
{
	double snr;
	unsigned int c;
	int i;

	if (Interleave != progdefaults.mt63_interleave) {
		Interleave = progdefaults.mt63_interleave;
		restart();
	}
	
	if (InpBuff->EnsureSpace(len) == -1) {
		fprintf(stderr, "mt63_rxprocess: buffer error\n");
		return -1;
	}
	for (i = 0; i < len; i++)
		InpBuff->Data[i] = buf[i];
	InpBuff->Len = len;

	InpLevel->Process(InpBuff);
	
	Rx->Process(InpBuff);

	snr = Rx->FEC_SNR();
	if (snr > 99.9)
		snr = 99.9;
	display_metric(snr);

//	static char msg1[15];
//	double s2n = 10.0*log10( snr );
//	snprintf(msg1, sizeof(msg1), "s/n %2d dB", (int)(floor(s2n))); 
//  put_Status1(msg1);

	if (progStatus.sqlonoff && snr < progStatus.sldrSquelchValue)
		return 0;

	for (i = 0; i < Rx->Output.Len; i++) {
		c = Rx->Output.Data[i];

		if (!progdefaults.mt63_8bit) {
			put_rx_char(c);
			continue;
		}

		if ((c < 8) && (escape == 0))
			continue;

		if (c == 127) {
			escape = 1;
			continue;
		}

		if (escape) {
			c += 128;
			escape = 0;
		}

		put_rx_char(c);
	}

	return 0;
}

void mt63::restart()
{
	int err;

	put_MODEstatus(mode);
	set_scope_mode(Digiscope::BLANK);
	set_freq(500.0 + bandwidth / 2.0);

	err = Tx->Preset((int)bandwidth, Interleave == 64 ? 1 : 0);
	if (err)
		fprintf(stderr, "mt63_txinit: init failed\n");
	flush = Tx->DataInterleave;

	err = Rx->Preset((int)bandwidth, Interleave == 64 ? 1 : 0, IntegLen);
	if (err)
		fprintf(stderr, "mt63_rxinit: init failed\n");
	InpLevel->Preset(64.0, 0.75);
}

void mt63::init()
{
	modem::init();
	restart();
}

mt63::mt63 (trx_mode mt63_mode) : modem()
{
	mode = mt63_mode;
	switch (mode) {
	case MODE_MT63_500:
		bandwidth = 500;
		break;
	case MODE_MT63_1000:
		bandwidth = 1000;
		break;
	case MODE_MT63_2000:
		bandwidth = 2000;
		break;
	}
	Interleave = progdefaults.mt63_interleave;

	Tx = new MT63tx;
	Rx = new MT63rx;

	InpLevel = new dspLevelMonitor;
	InpBuff = new double_buff;

	samplerate = 8000;
	fragmentsize = 1024;

	init();
}

mt63::~mt63()
{
	if (Tx) delete Tx;
	if (Rx) delete Rx;

	if (InpLevel) delete InpLevel;
	if (InpBuff) delete InpBuff;
}
