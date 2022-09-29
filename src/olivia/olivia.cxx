// ----------------------------------------------------------------------------
// olivia.cxx  --  OLIVIA modem
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

#include <sstream>

#include <FL/Fl.H>

#include "olivia.h"
#include "modem.h"
#include "fl_digi.h"

#include "misc.h"
#include "confdialog.h"
#include "status.h"
#include "debug.h"
#include "qrunner.h"

//------------------------------------------------------------------------------
#include "threads.h"

static pthread_mutex_t olivia_mutex = PTHREAD_MUTEX_INITIALIZER;
//------------------------------------------------------------------------------

LOG_FILE_SOURCE(debug::LOG_MODEM);

double olivia::nco(double freq)
{
    preamblephase += 2.0 * M_PI * freq / samplerate;

	if (preamblephase > M_PI)
		preamblephase -= 2.0 * M_PI;

	return cos(preamblephase);
}

void olivia::tx_init()
{
	phaseacc = 0;
	prevsymbol = cmplx (1.0, 0.0);
	preamble = 32;
	shreg = 0;

	preamblesent = 0;
	postamblesent = 0;
	txbasefreq = get_txfreq_woffset();

	rx_flush();

{ // critical section
	guard_lock dsp_lock(&olivia_mutex);

	double fc_offset = Tx->Bandwidth*(1.0 - 0.5/Tx->Tones)/2.0;
	if (reverse) { 
		Tx->FirstCarrierMultiplier = (txbasefreq + fc_offset)/500.0; 
		Tx->Reverse = 1; 
	} else {
		Tx->FirstCarrierMultiplier = (txbasefreq - fc_offset)/500.0;
		Tx->Reverse = 0; 
	}
	Tx->Preset();
	Tx->Start();
} // end critical section

	videoText();

	escape = 0;
}

void olivia::rx_flush()
{
	guard_lock dsp_lock(&olivia_mutex);

	unsigned char c;
	Rx->Flush();
	while (Rx->GetChar(c) > 0)
		put_rx_char(c);
}

void olivia::send_tones()
{
	if (!progdefaults.olivia_start_tones) return;

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

void olivia::rx_init()
{
	guard_lock dsp_lock(&olivia_mutex);

	Rx->Reset();
	escape = 0;
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
	modem::tx_process();

	int c = 0, len = 0;

	if ((mode == MODE_OLIVIA && 
		(tones	!= progdefaults.oliviatones ||
		bw 		!= progdefaults.oliviabw)) ||
		smargin != progdefaults.oliviasmargin ||
		sinteg	!= progdefaults.oliviasinteg )
			restart();

{ // critical section
	guard_lock dsp_lock(&olivia_mutex);

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
			if (c > 127) {
				if (progdefaults.olivia8bit && c <= 255) {
					Tx->PutChar(127);
					Tx->PutChar(c & 127);
				}
				else {
					c = '.';
					Tx->PutChar(c);
				}
			}
			else
				Tx->PutChar(c);
		}
	}

	if (c > 0)
		put_echo_char(c);

	if ((len = Tx->Output(txfbuffer)) > 0)
		ModulateXmtr(txfbuffer, len);

	if (stopflag && Tx->DoPostambleYet() == 1 && postamblesent != 1) {
		postamblesent = 1; 
		send_tones();
		memset(tonebuff, 0, sizeof(*tonebuff) * SCBLOCKSIZE);
		ModulateXmtr(tonebuff, SCBLOCKSIZE);
	}

	if (!Tx->Running()) {
		stopflag = false;
		return -1;
	}
} // end critical section 
	return 0;
}


int olivia::rx_process(const double *buf, int len)
{
	int c;
	unsigned char ch = 0;
	static double snr = 1e-3;
	static char msg1[20];
	static char msg2[20];
	double rxf_offset = 0;
	double rx_bw = 0;
	double rx_tones = 0;
	double rx_snr = 0;
	int fc_offset = 0;
	bool gotchar = false;

	if ((mode == MODE_OLIVIA && 
		(tones	!= progdefaults.oliviatones ||
		bw 		!= progdefaults.oliviabw)) ||
		smargin != progdefaults.oliviasmargin ||
		sinteg	!= progdefaults.oliviasinteg )
			restart();

{ // critical section
	guard_lock dsp_lock(&olivia_mutex);

	fc_offset = Tx->Bandwidth*(1.0 - 0.5/Tx->Tones)/2.0;

	if ((lastfreq != frequency || Rx->Reverse) && !reverse) {
		Rx->FirstCarrierMultiplier = (frequency - fc_offset)/500.0; 
		Rx->Reverse = 0;
		lastfreq = frequency;
		Rx->Preset();
	}
	else if ((lastfreq != frequency || !Rx->Reverse) && reverse) {
		Rx->FirstCarrierMultiplier = (frequency + fc_offset)/500.0; 
		Rx->Reverse = 1;
		lastfreq = frequency;
		Rx->Preset();
	}

	Rx->SyncThreshold = progStatus.sqlonoff ? 
		clamp(progStatus.sldrSquelchValue / 5.0 + 3.0, 0, 90.0) : 0.0;

	Rx->Process(buf, len);

	while (Rx->GetChar(ch) > 0) {
		if ((c = unescape(ch)) != -1 && c > 7) {
			put_rx_char(c);
			gotchar = true;
		}
    }

	rxf_offset = Rx->FrequencyOffset();
	rx_bw = Rx->Bandwidth;
	rx_tones = Rx->Tones;
	rx_snr = Rx->SignalToNoiseRatio();
} // end critical section

	sp = 0;
	for (int i = frequency - fc_offset; i < frequency + fc_offset; i++)
		if (wf->Pwr(i) > sp)
			sp = wf->Pwr(i);

	np = wf->Pwr(static_cast<int>(frequency + rx_bw/2 + 2*rx_bw/rx_tones));

	if (np == 0) np = sp + 1e-8;

	sigpwr = decayavg( sigpwr, sp, 10);
	noisepwr = decayavg( noisepwr, np, 50);
	snr = CLAMP(sigpwr / noisepwr, 0.001, 100000);

	metric = clamp( 5.0 * (rx_snr - 3.0), 0, 100);
	display_metric(metric);

	if (gotchar) {
		snprintf(msg1, sizeof(msg1), "s/n: %4.1f dB", 10*log10(snr) - 20);
		put_Status1(msg1, 5, STATUS_CLEAR);
		snprintf(msg2, sizeof(msg2), "f/o %+4.1f Hz", rxf_offset);
		put_Status2(msg2, 5, STATUS_CLEAR);
	}

	return 0;
}

void olivia::restart()
{
	if (mode == MODE_OLIVIA) {
		tones	= progdefaults.oliviatones;
		bw 		= progdefaults.oliviabw;
	}
	smargin = progdefaults.oliviasmargin;
	sinteg	= progdefaults.oliviasinteg;
	
	samplerate = 8000;
	bandwidth = 125 * (1 << bw);

	Tx->Tones = 2 * (1 << tones);
	Tx->Bandwidth = bandwidth;
	Tx->SampleRate = samplerate;
	Tx->OutputSampleRate = samplerate;
	txbasefreq = get_txfreq_woffset();

{ // critical section
	guard_lock dsp_lock(&olivia_mutex);

	int fc_offset = Tx->Bandwidth * (1.0 - 0.5/Tx->Tones) / 2.0;
	if (reverse) { 
		Tx->FirstCarrierMultiplier = (txbasefreq + fc_offset)/500.0; 
		Tx->Reverse = 1; 
	} else { 
		Tx->FirstCarrierMultiplier = (txbasefreq - fc_offset)/500.0; 
		Tx->Reverse = 0; 
	}

	if (Tx->Preset() < 0) {
		LOG_ERROR("olivia: transmitter preset failed!");
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

	fc_offset = Rx->Bandwidth * (1.0 - 0.5/Rx->Tones) / 2.0;
	if (reverse) { 
		Rx->FirstCarrierMultiplier = (frequency + fc_offset)/500.0; 
		Rx->Reverse = 1; 
	} else { 
		Rx->FirstCarrierMultiplier = (frequency - fc_offset)/500.0; 
		Rx->Reverse = 0; 
	}

	if (Rx->Preset() < 0) {
		LOG_ERROR("olivia: receiver preset failed!");
		return;
	}
	fragmentsize = 1024;
	set_bandwidth(Tx->Bandwidth - Tx->Bandwidth / Tx->Tones);

	std::stringstream info;
	info << mode_info[mode].sname;
	put_MODEstatus("%s", info.str().c_str());

	metric = 0;

	sigpwr = 1e-10; noisepwr = 1e-8;
	LOG_DEBUG("\nOlivia Rx parameters:\n%s", Rx->PrintParameters());
} // end critical section
}

void olivia::init()
{
	restart();
	modem::init();
	set_scope_mode(Digiscope::BLANK);

	if (progdefaults.StartAtSweetSpot)
		set_freq(progdefaults.PSKsweetspot);
	else if (progStatus.carrier != 0) {
		set_freq(progStatus.carrier);
#if !BENCHMARK_MODE
		progStatus.carrier = 0;
#endif
	} else
		set_freq(wf->Carrier());

}

olivia::olivia(trx_mode omode)
{
	mode = omode;
	cap |= CAP_REV;

	txfbuffer = 0;
	samplerate = 8000;

	switch (mode) {
		case MODE_OLIVIA_4_125:
			progdefaults.oliviatones = tones = 1;
			progdefaults.oliviabw = bw = 0;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_4_250:
			progdefaults.oliviatones = tones = 1;
			progdefaults.oliviabw = bw = 1;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_4_500:
			progdefaults.oliviatones = tones = 1;
			progdefaults.oliviabw = bw = 2;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_4_1000:
			progdefaults.oliviatones = tones = 1;
			progdefaults.oliviabw = bw = 3;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_4_2000:
			progdefaults.oliviatones = tones = 1;
			progdefaults.oliviabw = bw = 4;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_8_125:
			progdefaults.oliviatones = tones = 2;
			progdefaults.oliviabw = bw = 0;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_8_250:
			progdefaults.oliviatones = tones = 2;
			progdefaults.oliviabw = bw = 1;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_8_500:
			progdefaults.oliviatones = tones = 2;
			progdefaults.oliviabw = bw = 2;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_8_1000:
			progdefaults.oliviatones = tones = 2;
			progdefaults.oliviabw = bw = 3;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_8_2000:
			progdefaults.oliviatones = tones = 2;
			progdefaults.oliviabw = bw = 4;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_16_500:
			progdefaults.oliviatones = tones = 3;
			progdefaults.oliviabw = bw = 2;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_16_1000:
			progdefaults.oliviatones = tones = 3;
			progdefaults.oliviabw = bw = 3;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_16_2000:
			progdefaults.oliviatones = tones = 3;
			progdefaults.oliviabw = bw = 4;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_32_1000:
			progdefaults.oliviatones = tones = 4;
			progdefaults.oliviabw = bw = 3;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_32_2000:
			progdefaults.oliviatones = tones = 4;
			progdefaults.oliviabw = bw = 4;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_64_500:
			progdefaults.oliviatones = tones = 5;
			progdefaults.oliviabw = bw = 2;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_64_1000:
			progdefaults.oliviatones = tones = 5;
			progdefaults.oliviabw = bw = 3;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA_64_2000:
			progdefaults.oliviatones = tones = 5;
			progdefaults.oliviabw = bw = 4;
			REQ(set_olivia_tab_widgets);
			break;
		case MODE_OLIVIA:
		default:
			tones = progdefaults.oliviatones;
			bw    = progdefaults.oliviabw;
			REQ(set_olivia_tab_widgets);
			break;
	}

	Tx = new MFSK_Transmitter< double >;
	Rx = new MFSK_Receiver< double >;

	lastfreq = 0;

	for (int i = 0; i < SR4; i++) ampshape[i] = 1.0;
	for (int i = 0; i < SR4 / 8; i++)
		ampshape[i] = ampshape[SR4 - 1 - i] = 0.5 * (1.0 - cos(M_PI * i / (SR4/8)));

	for (int i = 0; i < TONE_DURATION; i++) tonebuff[i] = 0;

	tone_bw = -1;
	tone_midfreq = -1;
}

olivia::~olivia()
{
	guard_lock dsp_lock(&olivia_mutex);

	if (Tx) delete Tx;
	if (Rx) delete Rx;
	if (txfbuffer) delete [] txfbuffer;
}

