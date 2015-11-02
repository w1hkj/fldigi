// ---------------------------------------------------------------------
// pkt.cxx  --  1200/300/2400 baud AX.25
//
//
// This file is a proposed part of fldigi.  Adapted very liberally from
// rtty.cxx, with many thanks to John Hansen, W2FS, who wrote
// 'dcc.doc' and 'dcc2.doc', GNU Octave, GNU Radio Companion, and finally
// Bartek Kania (bk.gnarf.org) whose 'aprs.c' expository coding style helped
// shape this implementation.
//
// Copyright (C) 2010
//	Dave Freese, W1HKJ
//	Chris Sylvain, KB3CS
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the
//
//  Free Software Foundation, Inc.
//  51 Franklin Street, Fifth Floor
//  Boston, MA  02110-1301 USA.
//
// ---------------------------------------------------------------------


#include <config.h>
#include <iostream>
using namespace std;

#include "pkt.h"

#include "fl_digi.h"
#include "misc.h"
#include "confdialog.h"
#include "configuration.h"
#include "status.h"
#include "qrunner.h"
#include "globals.h"

#include "timeops.h"

/***********************************************************************
 * these are static members of the class pkt in the pkt.h file
***********************************************************************/
/*
 Default values:
   300 -> tones 1600 space and 1800 Hz mark (1700 Hz center)
  1200 -> tones 1200 mark and 2200 Hz space (1700 Hz center)
  2400 -> tones 397.5 mark and 2202.5 Hz space (1400 Hz center)
*/
const double	pkt::CENTER[]	= {1700, 1700};//, 1400};
const double	pkt::SHIFT[] 	= { 200, 1000};//, 1805};
const int		pkt::BAUD[]		= { 300, 1200};//, 2400};
const int		pkt::BITS[]		= {   8,    8};//,    8};
const int		pkt::FLEN[]		= { 512,   64};//,   32};

void pkt::tx_init(SoundBase *sc)
{
	scard = sc;

	int scale_factor = (pkt_baud > 1200 ? 2 : 1); // baud rate proportional

	// start each new transmission with MARK tone
	pretone = PKT_MarkBits * scale_factor;
	// number of flags to begin frame
	preamble = PKT_StartFlags * scale_factor;
	// number of flags to end frame
	postamble = PKT_EndFlags * scale_factor;

	if (!lo_tone)  lo_tone = new NCO();
	if (!hi_tone)  hi_tone = new NCO();

	double xmtfreq = pkt_ctrfreq;
	if (freqlocked())
		xmtfreq = get_txfreq();
	lo_tone->init(xmtfreq - (pkt_shift/2), 0, PKT_SampleRate);
	hi_tone->init(xmtfreq + (pkt_shift/2), 0, PKT_SampleRate);

	tx_cbuf = &txbuf[0];
	nr_ones = 0;
	currbit = nostuff = did_pkt_head = false;

	memset(txbuf, ' ', MAXOCTETS + 4);

	videoText();
}

void pkt::rx_init()
{
	rxstate = PKT_RX_STATE_STOP;
	scounter = 0;

	cbuf = &x25.rxbuf[0]; // init rx buf ptr
}

void pkt::set_freq(double f)
{
	if (f - pkt_shift/2 < 300) f = 300 + pkt_shift/2;
	if (f + pkt_shift/2 > 3600) f = 3600 - pkt_shift/2;

	if (progdefaults.PKT_MANUAL_TUNE)
		pkt_ctrfreq = f;
	else
		pkt_ctrfreq = CENTER[select_val];
	modem::set_freq(pkt_ctrfreq);
	REQ(put_freq, pkt_ctrfreq);

	nco_lo->init(pkt_ctrfreq-(pkt_shift/2), 0, PKT_SampleRate);
	nco_hi->init(pkt_ctrfreq+(pkt_shift/2), 0, PKT_SampleRate);
	nco_mid->init(pkt_ctrfreq, 0, PKT_SampleRate);
	return;
}

void pkt::init()
{
	restart();

	modem::init();

	set_freq(pkt_ctrfreq);

	rx_init();

	if (progdefaults.PKT_PreferXhairScope)
		set_scope_mode(Digiscope::XHAIRS);
	else
		set_scope_mode(Digiscope::RTTY);

	lo_signal_gain = pow(10, progdefaults.PKT_LOSIG_RXGAIN / 10);
	hi_signal_gain = pow(10, progdefaults.PKT_HISIG_RXGAIN / 10);

	lo_txgain = pow(10, progdefaults.PKT_LOSIG_TXGAIN / 10);
	hi_txgain = pow(10, progdefaults.PKT_HISIG_TXGAIN / 10);

	if (hi_txgain > 1.0 || lo_txgain > 1.0) {
	// renormalize output levels
	// [ modem output recording depends on gain =< 1.0 ]
		double inv;

		if (hi_txgain > lo_txgain)
			inv = 1.0 / hi_txgain;
		else
			inv = 1.0 / lo_txgain;

		lo_txgain *= inv;
		hi_txgain *= inv;
	}

	// leave 10% headroom
	lo_txgain *= 0.9;
	hi_txgain *= 0.9;

}

pkt::~pkt()
{
	if (nco_lo) delete nco_lo;
	if (nco_hi) delete nco_hi;
	if (nco_mid) delete nco_mid;

	if (idle_signal_buf)  delete idle_signal_buf;

	if (lo_signal_buf)  delete  lo_signal_buf;
	if (hi_signal_buf)  delete  hi_signal_buf;
	if (mid_signal_buf) delete mid_signal_buf;

	if (signal_buf)  delete signal_buf;

	if (pipe) delete [] pipe;
	if (dsppipe) delete [] dsppipe;

	if (lo_tone) delete lo_tone;
	if (hi_tone) delete hi_tone;

	delete lo_filt;
	delete hi_filt;
	delete mid_filt;
}

void pkt::set_pkt_modem_params(int i)
{
	pkt_baud = BAUD[i];
	pkt_shift = SHIFT[i];
	pkt_nbits = BITS[i];
	filter_len = FLEN[i];

	/**************************************************************
	 SYMBOLLEN is the number of samples in one data bit (aka one symbol)
	 at the current baud rate
	**************************************************************/

	symbollen = (int) floor((double)PKT_SampleRate / pkt_baud + 0.5);

	pkt_startlen =	  4 * symbollen;
	pkt_detectlen =	PKT_DetectLen * symbollen;
	pkt_syncdisplen =	PKT_SyncDispLen * symbollen;
	pkt_idlelen =	PKT_IdleLen * symbollen;
	pkt_startlen =	  2 * pkt_idlelen;

	fragmentsize = symbollen; // modem::fragmentsize -> see modem.h

	// http://users.encs.concordia.ca/~n_goswam/advsg00/advsgtxt/c10digtx_b1_r00.htm
	// BW = Baud + Shift * K  .. K ::= 1.2
	pkt_BW = pkt_baud + pkt_shift * 1.2;

	switch (i) {
		case 0: mode = MODE_PACKET300; break;
		case 1: mode = MODE_PACKET1200; break;
		//case 2: mode = MODEM_PACKET2400; break;
	}
	put_MODEstatus(mode);

}

void pkt::restart()
{
	if (select_val != progdefaults.PKT_BAUD_SELECT) {
		select_val = progdefaults.PKT_BAUD_SELECT;
		set_pkt_modem_params(select_val);
	}
	if (progdefaults.PKT_MANUAL_TUNE)
		pkt_ctrfreq = frequency;
	else
		pkt_ctrfreq = CENTER[select_val];

	float flpf = 1.0 * pkt_shift/samplerate;
	delete lo_filt;
	lo_filt = new fftfilt(flpf, filter_len);
	delete hi_filt;
	hi_filt = new fftfilt(flpf, filter_len);
	delete mid_filt;
	mid_filt = new fftfilt(flpf, filter_len);

	lo_phase = hi_phase = mid_phase = 0.0;

	if (!nco_lo)  nco_lo = new NCO();
	if (!nco_hi)  nco_hi = new NCO();
	if (!nco_mid) nco_mid = new NCO();

	set_freq(pkt_ctrfreq);

	set_bandwidth(pkt_shift); // waterfall tuning box

	wf->redraw_marker();

	if (!idle_signal_buf)
		idle_signal_buf = new double [PKT_IdleLen*PKT_MaxSymbolLen];

	idle_signal_pwr = idle_signal_buf_ptr = 0;
	for (int i = 0; i < pkt_idlelen; i++)
		idle_signal_buf[i] = 0;

	if (!lo_signal_buf)	lo_signal_buf = new cmplx [PKT_MaxSymbolLen];
	if (!hi_signal_buf)	hi_signal_buf = new cmplx [PKT_MaxSymbolLen];
	if (!mid_signal_buf)  mid_signal_buf = new cmplx [PKT_MaxSymbolLen];

	if (!signal_buf)
	signal_buf = new double [PKT_DetectLen*PKT_MaxSymbolLen];

	signal_pwr = signal_buf_ptr = 0;
	signal_gain = 1.0; // 5.0

	for(int i = 0; i < pkt_detectlen; i++)
	signal_buf[i] = 0;

	lo_signal_energy = hi_signal_energy =
	mid_signal_energy = cmplx(0, 0);

	yt_avg = correlate_buf_ptr = 0;

	for(int i = 0; i < symbollen; i++)
	lo_signal_buf[i] = hi_signal_buf[i] =
		mid_signal_buf[i] = cmplx(0, 0);

	if (!pipe)
	pipe = new double [PKT_SyncDispLen*PKT_MaxSymbolLen];
	if (!dsppipe)
	dsppipe = new double [PKT_SyncDispLen*PKT_MaxSymbolLen];

	QIptr = pipeptr = 0;

	// 1024 = 2 * SCBLOCKSIZE ( == MAX_ZLEN )
	for (int i = 0; i < MAX_ZLEN; i++)
		QI[i] = cmplx(0,0);

	metric = 0.0;
	signal_power = noise_power = power_ratio = 0;

	clear_zdata = true;
}

pkt::pkt(trx_mode md)
{
	cap |= CAP_REV;
	cap &= ~CAP_AFC; // modem::cap

	mode = md; // modem::mode

	samplerate = PKT_SampleRate; // modem::samplerate

	nco_lo = nco_hi = nco_mid = (NCO *)0;

	idle_signal_buf = (double *)0;

	lo_signal_buf = hi_signal_buf = mid_signal_buf = (cmplx *)0;

	signal_buf = (double *)0;

	pipe = dsppipe = (double *)0;

	select_val = -1; // force modem param init

	lo_filt = (fftfilt *)0;
	hi_filt = (fftfilt *)0;
	mid_filt = (fftfilt *)0;

//	restart();

	lo_tone = hi_tone = (NCO *)0;
	tx_char_count = MAXOCTETS-3; // leave room for FCS and end-flag

}

#include "pkt_receive.cxx"

#include "pkt_transmit.cxx"

