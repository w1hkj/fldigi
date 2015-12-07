// ----------------------------------------------------------------------------
//    mt63.cxx  --  MT63 modem for fldigi
//
// Copyright (C) 1999-2004 Pawel Jalocha, SP9VRC
// Copyright (c) 2007-2011 Dave Freese, W1HKJ
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

#include "configuration.h"
#include "fl_digi.h"
#include "status.h"
#include "mt63.h"

//------------------------------------------------------------------------------
#include "threads.h"

static pthread_mutex_t mt63_mutex = PTHREAD_MUTEX_INITIALIZER;
//------------------------------------------------------------------------------

using namespace std;
bool startflag = true;

void mt63::tx_init(SoundBase *sb)
{
	guard_lock dsp_lock(&mt63_mutex);

	scard = sb;
	Tx->Preset(get_txfreq_woffset(), (int)bandwidth, Interleave == 64 ? 1 : 0);
	flush = Tx->DataInterleave;
	videoText();
	startflag = true;
}

void mt63::rx_init()
{
	guard_lock dsp_lock(&mt63_mutex);

	Rx->Preset( frequency,
				(int)bandwidth,
				Interleave == 64 ? 1 : 0,
				long_integral ? 32 : 16 );
	InpLevel->Preset(64.0, 0.75);
	escape = 0;
}

int mt63::tx_process()
{
	rx_flush();
// do not put above rx_flush()

	guard_lock dsp_lock(&mt63_mutex);

	int c;

	if (startflag == true) {
		startflag = false;
		maxval = 0.0;
		if (progdefaults.mt63_usetones) {
			double w1 = 2.0 * M_PI * (get_txfreq_woffset() - bandwidth / 2.0) / samplerate;
			double w2 = 2.0 * M_PI * (get_txfreq_woffset() + 31.0 * bandwidth / 64.0) / samplerate;
			double phi1 = 0.0;
			double phi2 = 0.0;
			double buff[512];
			int numsmpls = samplerate * progdefaults.mt63_tone_duration / 512;
			for (int i = 0; i < numsmpls; i++) {
				for (int j = 0; j < 512; j++) {
					buff[j] = TONE_AMP * (progdefaults.mt63_twotones ? 0.5 : 1.0) * cos(phi1) +
							  TONE_AMP * (progdefaults.mt63_twotones ? 0.5 : 0.0) * cos(phi2);
					phi1 += w1;
					phi2 += w2;
					if (i == 0) buff[j] *= (1.0 - exp(-1.0 * j / 40.0));
					if (i == progdefaults.mt63_tone_duration - 1)
						buff[j] *= (1.0 - exp(-1.0 * (samplerate - j) / 40.0));
				}
				Fl::awake();
				ModulateXmtr(buff, 512);
			}
		}
		for (int i = 0; i < Tx->DataInterleave; i++) {
			Tx->SendChar(0);
			Fl::awake();
		}
	}

	c = get_tx_char();
	if (c == GET_TX_CHAR_ETX)  {
		stopflag = true;
		flush = Tx->DataInterleave;
	}

	if (c == GET_TX_CHAR_NODATA || stopflag == true) c = 0;

	if (stopflag) {
		stopflag = false;
		while (--flush) {
			Tx->SendChar(0);
			double buff[Tx->Comb.Output.Len];
			for (int i = 0; i < Tx->Comb.Output.Len; i++)
				if (fabs(Tx->Comb.Output.Data[i]) > maxval)
					maxval = fabs(Tx->Comb.Output.Data[i]);
			for (int i = 0; i < Tx->Comb.Output.Len; i++)
				buff[i] = Tx->Comb.Output.Data[i] /= maxval;
			ModulateXmtr(buff,Tx->Comb.Output.Len);
		}
		Tx->SendJam();
		maxval = 0.0;
		double buff[Tx->Comb.Output.Len];
		for (int i = 0; i < Tx->Comb.Output.Len; i++)
			if (fabs(Tx->Comb.Output.Data[i]) > maxval)
				maxval = fabs(Tx->Comb.Output.Data[i]);
		for (int i = 0; i < Tx->Comb.Output.Len; i++)
			buff[i] = Tx->Comb.Output.Data[i] * 1.0 / maxval;
		ModulateXmtr(buff,Tx->Comb.Output.Len);
		cwid();
		return -1;	/* we're done */
	}

	if (c > 255 || (!progdefaults.mt63_8bit && c > 127))
		c = '.';
	
	int sendc = c;
	
	if (sendc > 127) {
		sendc &= 127;
		Tx->SendChar(127);
		double buff[Tx->Comb.Output.Len];
		for (int i = 0; i < Tx->Comb.Output.Len; i++)
			if (fabs(Tx->Comb.Output.Data[i]) > maxval)
				maxval = fabs(Tx->Comb.Output.Data[i]);
		for (int i = 0; i < Tx->Comb.Output.Len; i++)
			buff[i] = Tx->Comb.Output.Data[i] * 1.0 / maxval;
		ModulateXmtr(buff,Tx->Comb.Output.Len);
	}

	Tx->SendChar(sendc);
	double buff[Tx->Comb.Output.Len];
	int len = Tx->Comb.Output.Len;
	for (int i = 0; i < len; i++) {
		buff[i] = Tx->Comb.Output.Data[i];
		if (maxval < fabs(buff[i])) maxval = fabs(buff[i]);
	}
	for (int i = 0; i < len; i++) {
		buff[i] *= 1.0 / maxval;
	}
	ModulateXmtr(buff,len);

	put_echo_char(c);

	return 0;
}

int mt63::rx_process(const double *buf, int len)
{
	double snr;
	unsigned int c;
	int i;
	static char msg1[20];
	static char msg2[20];
	double f_offset;

	if (long_integral != progdefaults.mt63_rx_integration) {
		long_integral = progdefaults.mt63_rx_integration;
		restart();
	}

	if (InpBuff->EnsureSpace(len) == -1) {
		fprintf(stderr, "mt63_rxprocess: buffer error\n");
		return -1;
	}

	for (i = 0; i < len; i++)
		InpBuff->Data[i] = buf[i];

	{ // critical section
		guard_lock dsp_lock(&mt63_mutex);

		InpBuff->Len = len;
		InpLevel->Process(InpBuff);

		Rx->Process(InpBuff);

		snr = Rx->FEC_SNR();
		if (progStatus.sqlonoff && snr < progStatus.sldrSquelchValue) {
			put_Status1("");
			put_Status2("");
			display_metric(0);
			return 0;
		}

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

		f_offset = Rx->TotalFreqOffset();

	} // end critical section

	if (snr > 99.9) snr = 99.9;

	display_metric(snr);

	double s2n = 10.0*log10( snr == 0 ? 0.001 : snr);
	snprintf(msg1, sizeof(msg1), "s/n %2d dB", (int)(floor(s2n)));
	put_Status1(msg1);

	snprintf(msg2, sizeof(msg2), "f/o %+4.1f Hz", f_offset);
	put_Status2(msg2, 5, STATUS_CLEAR);

	flushbuffer = true;

	return 0;
}

void mt63::rx_flush()
{
	guard_lock dsp_lock(&mt63_mutex);

	unsigned int c;
	int len = 512;
	int dlen = 0;

	if (!flushbuffer) return;

	if (emptyBuff->EnsureSpace(len) == -1) {
		flushbuffer = false;
		return;
	}

	for (int j = 0; j < len; j++)
		emptyBuff->Data[j] = 0.0;
	emptyBuff->Len = len;
	InpLevel->Process(emptyBuff);
	Rx->Process(emptyBuff);
	dlen = Rx->Output.Len;

	while (Rx->SYNC_LockStatus()) {
		for (int i = 0; i < dlen; i++) {
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
		for (int j = 0; j < len; j++)
			emptyBuff->Data[j] = 0.0;
		emptyBuff->Len = len;
		InpLevel->Process(emptyBuff);
		Rx->Process(emptyBuff);
		dlen = Rx->Output.Len;
	}
	flushbuffer = false;

	return;
}

void mt63::restart()
{
	int err;

	put_MODEstatus(mode);
	set_scope_mode(Digiscope::BLANK);

	{ // critical section
		guard_lock dsp_lock(&mt63_mutex);

		err = Tx->Preset(get_txfreq_woffset(), (int)bandwidth, Interleave == 64 ? 1 : 0);
		if (err)
			fprintf(stderr, "mt63_txinit: init failed\n");
		flush = Tx->DataInterleave;

		err = Rx->Preset( frequency, (int)bandwidth,
					  Interleave == 64 ? 1 : 0,
					  long_integral ? 32 : 16);
	} // end critical section
	if (err)
		fprintf(stderr, "mt63_rxinit: init failed\n");
	InpLevel->Preset(64.0, 0.75);
	stopflag = false;
}

void mt63::init()
{
	modem::init();
	restart();
	flushbuffer = false;
	maxval = 0.0;
}

mt63::mt63 (trx_mode mt63_mode) : modem()
{
	mode = mt63_mode;
	switch (mode) {
	case MODE_MT63_500S:
		Interleave = 32;
		bandwidth = 500;
		break;
	case MODE_MT63_500L:
		Interleave = 64;
		bandwidth = 500;
		break;
	case MODE_MT63_1000S:
		Interleave = 32;
		bandwidth = 1000;
		break;
	case MODE_MT63_1000L:
		Interleave = 64;
		bandwidth = 1000;
		break;
	case MODE_MT63_2000S:
		Interleave = 32;
		bandwidth = 2000;
		break;
	case MODE_MT63_2000L:
		Interleave = 64;
		bandwidth = 2000;
		break;
	}

	long_integral = progdefaults.mt63_rx_integration;

	Tx = new MT63tx;
	Rx = new MT63rx;

	InpLevel = new dspLevelMonitor;
	InpBuff = new double_buff;
	emptyBuff = new double_buff;

	samplerate = 8000;
	fragmentsize = 1024;

}

mt63::~mt63()
{
	guard_lock dsp_lock(&mt63_mutex);

	if (Tx) delete Tx;
	if (Rx) delete Rx;

	if (InpLevel) delete InpLevel;
	if (InpBuff) delete InpBuff;
}

// W1HKJ
// user can select manual or fixed positioning of the MT63 encoder/decoder
// progdefaults.mt63_at500 TRUE ==> fixed position
void mt63::set_freq(double f)
{
	if (progdefaults.mt63_at500)
		frequency = 500 + bandwidth / 2;
	else
		frequency = f;

	modem::set_freq(frequency);
	rx_init();
}
