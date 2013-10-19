// ----------------------------------------------------------------------------
// contestia.cxx  --  CONTESTIA modem
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
//	Copyright (C) 2005
//	Tomi Manninen (oh2bns@sral.fi)
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

#include <FL/Fl.H>

#include "contestia.h"
#include "modem.h"
#include "fl_digi.h"

#include "misc.h"
#include "confdialog.h"
#include "status.h"
#include "debug.h"

LOG_FILE_SOURCE(debug::LOG_MODEM);

using namespace std;

double contestia::nco(double freq)
{
    preamblephase += 2.0 * M_PI * freq / samplerate;

	if (preamblephase > M_PI)
		preamblephase -= 2.0 * M_PI;

	return cos(preamblephase);
}

void contestia::tx_init(SoundBase *sc)
{
	scard = sc;
	phaseacc = 0;
	prevsymbol = cmplx (1.0, 0.0);
	preamble = 32;
	shreg = 0;

	preamblesent = 0;
	postamblesent = 0;
	txbasefreq = get_txfreq_woffset();

	rx_flush();

	if (reverse) { 
		Tx->FirstCarrierMultiplier = (txbasefreq + (Tx->Bandwidth / 2)) / 500; 
		Tx->Reverse = 1; 
	} else {
		Tx->FirstCarrierMultiplier = (txbasefreq - (Tx->Bandwidth / 2)) / 500;
		Tx->Reverse = 0; 
	}

	videoText();

	Tx->Preset();
	Tx->Start();
	escape = 0;
}

void contestia::rx_flush()
{
	unsigned char c;
	Rx->Flush();
	while (Rx->GetChar(c) > 0)
		put_rx_char(c);
}

void contestia::send_tones()
{
	double freqa, freqb;
	tone_bw = bandwidth;
	tone_midfreq = txbasefreq;

	if (reverse) { 
		freqa = tone_midfreq + (tone_bw / 2.0); 
		freqb = tone_midfreq - (tone_bw / 2.0); 
	} else { 
		freqa = tone_midfreq - (tone_bw / 2.0); 
		freqb = tone_midfreq + (tone_bw / 2.0); 
	}

	preamblephase = 0;
	for (int i = 0; i < SR4; i++)
		tonebuff[2*SR4 + i] = tonebuff[i] = nco(freqa) * ampshape[i];

	preamblephase = 0;
	for (int i = 0; i < SR4; i++)
		tonebuff[3*SR4 + i] = tonebuff[SR4 + i] = nco(freqb) * ampshape[i];

	for (int j = 0; j < TONE_DURATION; j += SCBLOCKSIZE)
		ModulateXmtr(&tonebuff[j], SCBLOCKSIZE);

}

void contestia::rx_init()
{
	Rx->Reset();
	escape = 0;
}

int contestia::unescape(int c)
{
	if (progdefaults.contestia8bit == 0)
		return c;

	if (escape) {
		escape = 0;
		return c + 128;
	}

	if (c == 127) {
		escape = 1;
		return -1;
	}

	return c;
}

int contestia::tx_process()
{
	int c = 0, len = 0;
	unsigned char ch;

	if (tones	!= progdefaults.contestiatones ||
		bw 		!= progdefaults.contestiabw ||
		smargin != progdefaults.contestiasmargin ||
		sinteg	!= progdefaults.contestiasinteg )
			restart();

	if (preamblesent != 1) { 
		send_tones(); 
		preamblesent = 1;
		// Olivia Transmitter class requires at least character
		Tx->PutChar(0);
	}

// The encoder works with BitsPerSymbol length blocks. If the
// modem already has that many characters buffered, don't try
// to read any more. If stopflag is set, we will always read 
// whatever there is.
	if (stopflag || (Tx->GetReadReady() < Tx->BitsPerSymbol)) {
		if (!stopflag && (c = get_tx_char()) == GET_TX_CHAR_ETX)
			stopflag = true;
		if (stopflag)
			Tx->Stop();
		else {
			if (c == GET_TX_CHAR_NODATA)
		                c = 0;
			/* Replace un-representable characters with a dot */
			if (c > (progdefaults.contestia8bit ? 255 : 127))
				c = '.';
			if (c > 127) {
				c &= 127;
				Tx->PutChar(127);
			}
			Tx->PutChar(c);
		}
	}

	if (Tx->GetChar(ch) > 0)
		if ((c = unescape(ch)) != -1)
			put_echo_char(progdefaults.rx_lowercase ? tolower(c) : toupper(c));

    if ((len = Tx->Output(txfbuffer)) > 0)
		ModulateXmtr(txfbuffer, len);

	if (stopflag && Tx->DoPostambleYet() == 1 && postamblesent != 1) {
		postamblesent = 1; 
		send_tones();
	}

	if (!Tx->Running()) {
		cwid();
		stopflag = false;
		return -1;
	}
 
	return 0;
}


int contestia::rx_process(const double *buf, int len)
{
	int c;
	unsigned char ch = 0;
	static double snr = 1e-3;
	static char msg1[20];
	static char msg2[20];

	if (tones	!= progdefaults.contestiatones ||
		bw 		!= progdefaults.contestiabw ||
		smargin != progdefaults.contestiasmargin ||
		sinteg	!= progdefaults.contestiasinteg )
			restart();

	if ((lastfreq != frequency || Rx->Reverse) && !reverse) {
		Rx->FirstCarrierMultiplier = (frequency - (Rx->Bandwidth / 2)) / 500; 
		Rx->Reverse = 0;
		lastfreq = frequency;
		Rx->Preset();
	}
	else if ((lastfreq != frequency || !Rx->Reverse) && reverse) {
		Rx->FirstCarrierMultiplier = (frequency + (Rx->Bandwidth / 2)) / 500; 
		Rx->Reverse = 1;
		lastfreq = frequency;
		Rx->Preset();
	}

	Rx->SyncThreshold = progStatus.sqlonoff ? 
		clamp(progStatus.sldrSquelchValue / 5.0 + 3.0, 3.0, 90.0) : 3.0;

    Rx->Process(buf, len);
	sp = 0;
	for (int i = frequency - Rx->Bandwidth/2; i < frequency - 1 + Rx->Bandwidth/2; i++)
		if (wf->Pwr(i) > sp)
			sp = wf->Pwr(i);
	np = wf->Pwr(frequency + Rx->Bandwidth/2 + 2*Rx->Bandwidth/Rx->Tones);
	if (np == 0) np = sp + 1e-8;
	sigpwr = decayavg( sigpwr, sp, 10);
	noisepwr = decayavg( noisepwr, np, 50);
	snr = CLAMP(sigpwr / noisepwr, 0.001, 100000);

	metric = clamp( 5.0 * (Rx->SignalToNoiseRatio() - 3.0), 0, 100);
	display_metric(metric);

	bool gotchar = false;
	while (Rx->GetChar(ch) > 0) {
		if ((c = unescape(ch)) != -1 && c > 7) {
			put_rx_char(progdefaults.rx_lowercase ? tolower(c) : c);
			gotchar = true;
		}
    }
	if (gotchar) {
		snprintf(msg1, sizeof(msg1), "s/n: %4.1f dB", 10*log10(snr) - 20);
		put_Status1(msg1, 5, STATUS_CLEAR);
		snprintf(msg2, sizeof(msg2), "f/o %+4.1f Hz", Rx->FrequencyOffset());
		put_Status2(msg2, 5, STATUS_CLEAR);
	}

	return 0;
}

void contestia::restart()
{
	tones	= progdefaults.contestiatones;
	bw 		= progdefaults.contestiabw;
	smargin = progdefaults.contestiasmargin;
	sinteg	= progdefaults.contestiasinteg;
	
	samplerate = 8000;
	bandwidth = 125 * (1 << bw);

	Tx->Tones = 2 * (1 << tones);
	Tx->Bandwidth = bandwidth;
	Tx->SampleRate = samplerate;
	Tx->OutputSampleRate = samplerate;
    txbasefreq = get_txfreq_woffset();
	Tx->bContestia = true;

	if (reverse) { 
		Tx->FirstCarrierMultiplier = (txbasefreq + (Tx->Bandwidth / 2)) / 500; 
		Tx->Reverse = 1; 
	} else { 
		Tx->FirstCarrierMultiplier = (txbasefreq - (Tx->Bandwidth / 2)) / 500; 
		Tx->Reverse = 0; 
	}

	if (Tx->Preset() < 0) {
		LOG_ERROR("contestia: transmitter preset failed!");
		return;
	}
		
	txbufferlen = Tx->MaxOutputLen;
	
	if (txfbuffer) delete [] txfbuffer;
	txfbuffer = new double[txbufferlen];

	Rx->Tones = Tx->Tones;
	Rx->Bandwidth = bandwidth;
	Rx->SyncMargin = smargin;
	Rx->SyncIntegLen = sinteg;
	Rx->SyncThreshold = progStatus.sqlonoff ? 
		clamp(progStatus.sldrSquelchValue / 5.0 + 3.0, 0, 90.0) : 0.0;

	Rx->SampleRate = samplerate;
	Rx->InputSampleRate = samplerate;
	Rx->bContestia = true;

	if (reverse) { 
		Rx->FirstCarrierMultiplier = (frequency + (Rx->Bandwidth / 2)) / 500; 
		Rx->Reverse = 1; 
	} else { 
		Rx->FirstCarrierMultiplier = (frequency - (Rx->Bandwidth /2)) / 500; 
		Rx->Reverse = 0; 
	}

	if (Rx->Preset() < 0) {
		LOG_ERROR("contestia: receiver preset failed!");
		return;
	}
	fragmentsize = 1024;
	set_bandwidth(Tx->Bandwidth - Tx->Bandwidth / Tx->Tones);

	put_MODEstatus("%s %" PRIuSZ "/%" PRIuSZ "", get_mode_name(), Tx->Tones, Tx->Bandwidth);
	metric = 0;

	sigpwr = 1e-10; noisepwr = 1e-8;
	LOG_DEBUG("\nContestia Rx parameters:\n%s", Rx->PrintParameters());
}

void contestia::init()
{
	restart();
	modem::init();
	set_scope_mode(Digiscope::BLANK);
}

contestia::contestia()
{
	cap |= CAP_REV;

	txfbuffer = 0;
	samplerate = 8000;

	Tx = new MFSK_Transmitter< double >;
	Rx = new MFSK_Receiver< double >;

	Tx->bContestia = true;
	Rx->bContestia = true;

	mode = MODE_CONTESTIA;

	lastfreq = 0;

	for (int i = 0; i < SR4; i++) ampshape[i] = 1.0;
	for (int i = 0; i < SR4 / 8; i++)
		ampshape[i] = ampshape[SR4 - 1 - i] = 0.5 * (1.0 - cos(M_PI * i / (SR4/8)));

	for (int i = 0; i < TONE_DURATION; i++) tonebuff[i] = 0;

	tone_bw = -1;
	tone_midfreq = -1;
}

contestia::~contestia()
{
	if (Tx) delete Tx;
	if (Rx) delete Rx;
	if (txfbuffer) delete [] txfbuffer;
}

