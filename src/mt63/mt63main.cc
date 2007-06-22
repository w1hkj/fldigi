/*
 *    mt63main.cc  --  MT63 modem
 *
 *    Copyright (C) 2001, 2002, 2003
 *      Tomi Manninen (oh2bns@sral.fi)
 *
 *    This file is part of gMFSK.
 *
 *    gMFSK is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    gMFSK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with gMFSK; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "stdio.h"

#include "conf.h"
#include "trx.h"
#include "snd.h"

#include "dsp.h"
#include "mt63.h"

extern "C" void mt63_init(struct trx *trx);

static int IntegLen = 32;	// integration period for sync./data tracking

struct mt63 {
	int Bandwidth;
	int Interleave;
	char *CWID;

	MT63tx *Tx;
	MT63rx *Rx;

	LevelMonitor *InpLevel;
	float_buff *InpBuff;

	int flush;
	int escape;
};

static void mt63_txinit(struct trx *trx)
{
	struct mt63 *s = (struct mt63 *) trx->modem;
	int err;

	err = s->Tx->Preset(s->Bandwidth, s->Interleave, s->CWID);

	if (err)
		fprintf(stderr, "mt63_txinit: init failed\n");

	s->flush = s->Tx->DataInterleave;
}

static void mt63_rxinit(struct trx *trx)
{
	struct mt63 *s = (struct mt63 *) trx->modem;
	int err;

	err = s->Rx->Preset(s->Bandwidth, s->Interleave, IntegLen);

	if (err)
		fprintf(stderr, "mt63_rxinit: init failed\n");

	s->InpLevel->Preset(64.0, 0.75);

	s->escape = 0;
}

static void mt63_free(struct mt63 *s)
{
	if (s) {
		s->Tx->Free();
		s->Rx->Free();

		delete s->Tx;
		delete s->Rx;
		delete s->InpLevel;
		delete s->InpBuff;

		free(s->CWID);

		free(s);
	}
}

static void mt63_destructor(struct trx *trx)
{
	struct mt63 *s = (struct mt63 *) trx->modem;

	mt63_free(s);

	trx->modem = NULL;
	trx->txinit = NULL;
	trx->rxinit = NULL;
	trx->txprocess = NULL;
	trx->rxprocess = NULL;
	trx->destructor = NULL;
}

static int mt63_txprocess(struct trx *trx)
{
	struct mt63 *s = (struct mt63 *) trx->modem;
	int c;

	c = trx_get_tx_char();

	if (c == -1) {
		if (trx->stopflag && s->flush-- == 0)
			return -1;
		c = 0;
	} else
		s->flush = s->Tx->DataInterleave;

	if ((trx->mt63_esc && c > 255) || (!trx->mt63_esc && c > 127))
		c = '.';

	trx_put_echo_char(c);

	if (c > 127) {
		c &= 127;
		s->Tx->SendChar(127);
		sound_write(s->Tx->Comb.Output.Data, s->Tx->Comb.Output.Len);
	}

	s->Tx->SendChar(c);
	sound_write(s->Tx->Comb.Output.Data, s->Tx->Comb.Output.Len);

	return 0;
}

static int mt63_rxprocess(struct trx *trx, float *buf, int len)
{
	struct mt63 *s = (struct mt63 *) trx->modem;
	float snr;
	unsigned int c;
	int i;

	if (s->InpBuff->EnsureSpace(len) == -1) {
		fprintf(stderr, "mt63_rxprocess: buffer error\n");
		return -1;
	}
	for (i = 0; i < len; i++)
		s->InpBuff->Data[i] = buf[i];
	s->InpBuff->Len = len;

	s->InpLevel->Process(s->InpBuff);
	s->Rx->Process(s->InpBuff);

	snr = s->Rx->FEC_SNR();
	if (snr > 99.9)
		snr = 99.9;

	trx_set_metric(snr);

	if (trx->squelchon && snr < trx->mt63_squelch)
		return 0;

	for (i = 0; i < s->Rx->Output.Len; i++) {
		c = s->Rx->Output.Data[i];

		if (!trx->mt63_esc) {
			trx_put_rx_char(c);
			continue;
		}

		if ((c < 8) && (s->escape == 0))
			continue;

		if (c == 127) {
			s->escape = 1;
			continue;
		}

		if (s->escape) {
			c += 128;
			s->escape = 0;
		}

		trx_put_rx_char(c);
	}

	return 0;
}

void mt63_init(struct trx *trx)
{
	struct mt63 *s;

	if ((s = (mt63 *) calloc(1, sizeof(struct mt63))) == NULL)
		return;

	switch (trx->mt63_bandwidth) {
	case 0:
		s->Bandwidth = 500;
		break;
	case 1:
		s->Bandwidth = 1000;
		break;
	case 2:
		s->Bandwidth = 2000;
		break;
	}

	s->Interleave = trx->mt63_interleave;

	if (trx->mt63_cwid)
		s->CWID = g_strdup_printf("%s MT63", conf_get_mycall());
	else
		s->CWID = NULL;

	s->Tx = new MT63tx;
	s->Rx = new MT63rx;

	s->InpLevel = new LevelMonitor;
	s->InpBuff = new float_buff;

	trx->modem = s;

	trx->txinit = mt63_txinit;
	trx->rxinit = mt63_rxinit;

	trx->txprocess = mt63_txprocess;
	trx->rxprocess = mt63_rxprocess;

	trx->destructor = mt63_destructor;

	trx->samplerate = 8000;
	trx->fragmentsize = 1024;

	trx->bandwidth = s->Bandwidth;
	trx->frequency = 500.0 + s->Bandwidth / 2.0;
}
