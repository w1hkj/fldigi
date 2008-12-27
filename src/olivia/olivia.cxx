//
// olivia.cc  --  OLIVIA modem
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
//	Copyright (C) 2005
//	Tomi Manninen (oh2bns@sral.fi)
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
//
//

#include <config.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "olivia.h"
#include "sound.h"

#include "confdialog.h"

#include "status.h"

using namespace std;

double olivia::nco(double freq)
{
    preamblephase += 2.0 * M_PI * freq / samplerate;

	if (preamblephase > M_PI)
		preamblephase -= 2.0 * M_PI;

	return cos(preamblephase);
}

void olivia::tx_init(SoundBase *sc)
{
	unsigned char c;

	scard = sc;
	phaseacc = 0;
	prevsymbol = complex (1.0, 0.0);
	preamble = 32;
	shreg = 0;

	preamblesent = 0;
	postamblesent = 0;
	txbasefreq = get_txfreq_woffset();

	Rx->Flush();

	while (Rx->GetChar(c) > 0)
		put_rx_char(c);

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

void olivia::send_preamble()
{
	double freqa, freqb;
    int i, sr4 = samplerate / 4;

	if (reverse) { 
		freqa = txbasefreq + (bandwidth / 2.0); 
		freqb = txbasefreq - (bandwidth / 2.0); 
	} else { 
		freqa = txbasefreq - (bandwidth / 2.0); 
		freqb = txbasefreq + (bandwidth / 2.0); 
	}

	for (i = 0; i < sr4; i++)
		outbuf[i] = nco(freqa);
	for (i = sr4; i < 2*sr4; i++)
		outbuf[i] = nco(freqb);
	for (i = 2*sr4; i < 3*sr4; i++)
		outbuf[i] = nco(freqa);
	for (i = 3*sr4; i < samplerate; i++)
		outbuf[i] = nco(freqb);
	ModulateXmtr(outbuf, samplerate);
}

void olivia::send_postamble()
{
	double freqa, freqb;
	int i, sr4 = samplerate / 4;

	if (reverse) { 
		freqa = txbasefreq + (bandwidth / 2.0); 
		freqb = txbasefreq - (bandwidth / 2.0); 
	} else { 
		freqa = txbasefreq - (bandwidth / 2.0); 
		freqb = txbasefreq + (bandwidth / 2.0); 
	}

	for (i = 0; i < sr4; i++)
		outbuf[i] = nco(freqa);
	for (i = sr4; i < 2*sr4; i++)
		outbuf[i] = nco(freqb);
	for (i = 2*sr4; i < 3*sr4; i++)
		outbuf[i] = nco(freqa);
	for (i = 3*sr4; i < samplerate; i++)
		outbuf[i] = nco(freqb);
	ModulateXmtr(outbuf, samplerate);
}

void olivia::rx_init()
{
	Rx->Reset();
	escape = 0;
	set_AFCind(0.0);
}

int olivia::unescape(int c)
{
	if (progdefaults.olivia8bit == 0)
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

int olivia::tx_process()
{
	int c, i, len;
	unsigned char ch;

	if (tones	!= progdefaults.oliviatones ||
		bw 		!= progdefaults.oliviabw ||
		smargin != progdefaults.oliviasmargin ||
		sinteg	!= progdefaults.oliviasinteg )
			restart();

	if (preamblesent != 1) { 
		send_preamble(); 
		preamblesent = 1; 
	}

// The encoder works with BitsPerSymbol length blocks. If the
// modem already has that many characters buffered, don't try
// to read any more. If stopflag is set, we will always read 
// whatever there is.

	if (stopflag || (Tx->GetReadReady() < Tx->BitsPerSymbol)) {
		if ((c = get_tx_char()) == 0x03 || stopflag ) {
			stopflag = true;
			Tx->Stop();
		} else {
			/* Replace un-representable characters with a dot */
			if (c == -1)
                                c = 0;
			if (c > (progdefaults.olivia8bit ? 255 : 127))
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
			put_echo_char(c);

	if ((len = Tx->Output(txbuffer)) > 0) {
		for (i = 0; i < len; i++) {
			txfbuffer[i] = (double) (txbuffer[i] / 24000.0);
		}

		ModulateXmtr(txfbuffer, len);
	}

	if (stopflag && Tx->DoPostambleYet() == 1 && postamblesent != 1) {
		postamblesent = 1; 
		preamblephase = Tx->GetSymbolPhase(); 
		send_postamble();
	}

	if (!Tx->Running()) {
		cwid();
		return -1;
	}

	return 0;
}

int olivia::rx_process(const double *buf, int len)
{
	int i, c;
	unsigned char ch = 0;
	double snr;
	static char msg1[20];
	static char msg2[20];
//	static char msg3[60];

	if (tones	!= progdefaults.oliviatones ||
		bw 		!= progdefaults.oliviabw ||
		smargin != progdefaults.oliviasmargin ||
		sinteg	!= progdefaults.oliviasinteg )
			restart();

	if ((lastfreq != frequency || Rx->Reverse != 0) && !reverse) {
		Rx->FirstCarrierMultiplier = (frequency - (Rx->Bandwidth / 2)) / 500; 
		Rx->Reverse = 0;
		lastfreq = frequency;
		Rx->Preset();
	}
	else if ((lastfreq != frequency || Rx->Reverse != 1) && reverse) {
		Rx->FirstCarrierMultiplier = (frequency + (Rx->Bandwidth / 2)) / 500; 
		Rx->Reverse = 1;
		lastfreq = frequency;
		Rx->Preset();
	}

	if (len > rxbufferlen) {
		delete [] rxbuffer;
		rxbuffer = new short int[len];
		rxbufferlen = len;
	}

	for (i = 0; i < len; i++)
		rxbuffer[i] = (short int) (buf[i] * 32767.0);

	Rx->SyncThreshold = progStatus.sqlonoff ? progStatus.sldrSquelchValue / 2.0 : 0.0;

	Rx->Process(rxbuffer, len);

	snr = Rx->SignalToNoiseRatio();

	set_metric(snr);
	display_metric(snr > 50.0 ? 100.0 : snr * 2.0);
	
	double s2n = 20.0 * log10(snr < 0.1 ? 0.1 : snr);

	snprintf(msg1, sizeof(msg1), "s/n %4.1f dB", s2n);
	put_Status1(msg1);
	snprintf(msg2, sizeof(msg2), "f/o %+4.1f Hz", Rx->FrequencyOffset());
	put_Status2(msg2);

	while (Rx->GetChar(ch) > 0)
		if ((c = unescape(ch)) != -1 && c > 7)
			put_rx_char(c);

	return 0;
}

void olivia::restart()
{
	tones	= progdefaults.oliviatones;
	bw 		= progdefaults.oliviabw;
	smargin = progdefaults.oliviasmargin;
	sinteg	= progdefaults.oliviasinteg;
	
	samplerate = 8000;
	
	Tx->Tones = 2 * (1 << tones);
	Tx->Bandwidth = 125 * (1 << bw);
	Tx->SampleRate = samplerate;
	Tx->OutputSampleRate = samplerate;
    txbasefreq = get_txfreq_woffset();

	if (reverse) { 
		Tx->FirstCarrierMultiplier = (txbasefreq + (Tx->Bandwidth / 2)) / 500; 
		Tx->Reverse = 1; 
	} else { 
		Tx->FirstCarrierMultiplier = (txbasefreq - (Tx->Bandwidth / 2)) / 500; 
		Tx->Reverse = 0; 
	}

	if (Tx->Preset() < 0) {
		fl_message("olivia: transmitter preset failed!");
		return;
	}
		
	txbufferlen = Tx->MaxOutputLen;
	if (txbuffer) delete [] txbuffer;
	txbuffer = new short int[txbufferlen];
	
	if (txfbuffer) delete [] txfbuffer;
	txfbuffer = new double[txbufferlen];

	rxbufferlen = 0;
	rxbuffer = 0;
	
	Rx->Tones = Tx->Tones;
	Rx->Bandwidth = Tx->Bandwidth;
	Rx->SyncMargin = smargin;
	Rx->SyncIntegLen = sinteg;
	Rx->SyncThreshold = progStatus.sqlonoff ? progStatus.sldrSquelchValue : 0.0;

	Rx->SampleRate = samplerate;
	Rx->InputSampleRate = samplerate;

	if (reverse) { 
		Rx->FirstCarrierMultiplier = (frequency + (Rx->Bandwidth / 2)) / 500; 
		Rx->Reverse = 1; 
	} else { 
		Rx->FirstCarrierMultiplier = (frequency - (Rx->Bandwidth /2)) / 500; 
		Rx->Reverse = 0; 
	}

	if (Rx->Preset() < 0) {
		fl_message("olivia: receiver preset failed!");
		return;
	}
	fragmentsize = 1024;
	set_bandwidth(Tx->Bandwidth);

	put_MODEstatus("%s %d/%d", get_mode_name(), Tx->Tones, Tx->Bandwidth);
}

void olivia::init()
{
	modem::init();
	restart();
	set_scope_mode(Digiscope::BLANK);
}

olivia::olivia()
{
	cap = CAP_REV;

	txbuffer = 0;
	txfbuffer = 0;
	rxbuffer = 0;
	samplerate = 8000;

	Tx = new MFSK_Transmitter< float >;
	Rx = new MFSK_Receiver< float >;

	mode = MODE_OLIVIA;
	lastfreq = 0;
	init();
}

olivia::~olivia()
{
	if (Tx) delete Tx;
	if (Rx) delete Rx;
	if (txbuffer) delete [] txbuffer;
	if (txfbuffer) delete [] txfbuffer;
	if (rxbuffer) delete [] rxbuffer;
}

