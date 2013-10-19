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
// along with fldigi; if not, write to the Free Software
// Foundation, Inc.
// 59 Temple Place
// Suite 330
// Boston, MA  02111-1307  USA
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

#include "timeops.h"

static char msg1[20];
static char msg2[20];

/***********************************************************************
 * these are redefined as static members of the class
 * in the pkt1200.h file
 * increased number of elements in the PKTBITS to prepare for when
 * HF/VHF/DOUBLE selection is available
 * removed leading _ in names.  Leading underscore is reserved in Linux
 * for the OS and library code.
***********************************************************************/
/*
  1200 -> tones 1200 mark and 2200 Hz space (1700 Hz center)
   300 -> tones 1600 space and 1800 Hz mark (1700 Hz center)
  2400 -> tones 297.5 mark and 2102.5 Hz space (1200 Hz center)
*/
const double	pkt::CENTER[] = {1700, 1700, 1200};
const double	pkt::SHIFT[]  = {1000, 200, 1805};
const int	pkt::BAUD[]   = {1200, 300, 2400};
const int	pkt::BITS[]   = {8, 8, 8};

PKT_MicE_field	pkt::MicE_table[][12][5] =  {
	{
	{ Zero,  Zero, South, P0, East },
	{ One,   Zero, South, P0, East },
	{ Two,   Zero, South, P0, East },
	{ Three, Zero, South, P0, East },
	{ Four,  Zero, South, P0, East },
	{ Five,  Zero, South, P0, East },
	{ Six,   Zero, South, P0, East },
	{ Seven, Zero, South, P0, East },
	{ Eight, Zero, South, P0, East },
	{ Nine,  Zero, South, P0, East },
	{ Invalid, Null, Null, Null, Null },
	{ Invalid, Null, Null, Null, Null }
    },
    { // ['A'..'K'] + 'L'
	{ Zero,  One,  Null, Null, Null }, // custom A/B/C msg codes
	{ One,   One,  Null, Null, Null },
	{ Two,   One,  Null, Null, Null },
	{ Three, One,  Null, Null, Null },
	{ Four,  One,  Null, Null, Null },
	{ Five,  One,  Null, Null, Null },
	{ Six,   One,  Null, Null, Null },
	{ Seven, One,  Null, Null, Null },
	{ Eight, One,  Null, Null, Null },
	{ Nine,  One,  Null, Null, Null },
	{ Space, One,  Null, Null, Null },
	{ Space, Zero, South, P0, East }
    },
    { // ['P'..'Z']
	{ Zero,  One,  North, P100, West }, // standard A/B/C msg codes
	{ One,   One,  North, P100, West },
	{ Two,   One,  North, P100, West },
	{ Three, One,  North, P100, West },
	{ Four,  One,  North, P100, West },
	{ Five,  One,  North, P100, West },
	{ Six,   One,  North, P100, West },
	{ Seven, One,  North, P100, West },
	{ Eight, One,  North, P100, West },
	{ Nine,  One,  North, P100, West },
	{ Space, One,  North, P100, West },
	{ Invalid, Null, Null, Null, Null }
    } };

PKT_PHG_table	pkt::PHG_table[] = {
	{ "Omni", 4 },
	{ "NE", 2 },
	{ "E",  1 },
	{ "SE", 2 },
	{ "S",  1 },
	{ "SW", 2 },
	{ "W",  1 },
	{ "NW", 2 },
	{ "N",  1 }
    };

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

    lo_tone->init(pkt_ctrfreq-(pkt_shift/2), 0, PKT_SampleRate);
    hi_tone->init(pkt_ctrfreq+(pkt_shift/2), 0, PKT_SampleRate);

    tx_cbuf = &txbuf[0];
    nr_ones = 0;
    currbit = nostuff = did_pkt_head = false;

    videoText();
}

void pkt::rx_init()
{
    rxstate = PKT_RX_STATE_STOP;
    scounter = 0;

    cbuf = &rxbuf[0]; // init rx buf ptr
}

void pkt::set_freq(double f)
{
    // fixed transmit frequency modem
    if (abs(f - 1700) > 9) {
	if (!modem::freqlocked()) {
	    modem::set_freq(pkt_ctrfreq);
	    modem::freqlock = true;
	}
	modem::set_freq(f);
	nco_lo->init(f-(pkt_shift/2), 0, PKT_SampleRate);
	nco_hi->init(f+(pkt_shift/2), 0, PKT_SampleRate);
	nco_mid->init(f, 0, PKT_SampleRate);
    }
    else {
	if (modem::freqlocked())
	    modem::freqlock = false;

	modem::set_freq(pkt_ctrfreq);
	nco_lo->init(pkt_ctrfreq-(pkt_shift/2), 0, PKT_SampleRate);
	nco_hi->init(pkt_ctrfreq+(pkt_shift/2), 0, PKT_SampleRate);
	nco_mid->init(pkt_ctrfreq, 0, PKT_SampleRate);
    }
}

void pkt::init()
{
    modem::init();
    set_freq(pkt_ctrfreq); // use pkt default center freq. don't use PSKsweetspot.

    rx_init();

    snprintf(msg1, sizeof(msg1), "%4i / %-4.0f", pkt_baud, pkt_shift);
    put_Status1(msg1);
    put_MODEstatus(mode);

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

#ifndef NDEBUG
    static bool do_once = true;

    /* from TAPR AX.25 v2.2 Specification:
       2.2.8 Order of Bit Transmission
       With the exception of the FCS field, all fields of an AX.25 frame shall
       be sent with each octet's least-significant bit first. The FCS shall be
       sent most-significant bit first. 
    */
    // it seems they meant "byte first" in the last sentence above.  sigh.

     /* useful tools for identifying CRCs
	SRP16 CRC16  http://home.netsurf.de/wolfgang.ehrhardt/crchash_en.html
     */

    if (do_once) {
#undef CRCDEBUG
#ifdef CRCDEBUG
	// first two: http://ecee.colorado.edu/~newhallw/TechDepot/AX25CRC/CRC_for_AX25.pdf
	rxbuf[0] = 'A';
	rxbuf[1] = 'B';
	rxbuf[2] = 'C';
	rxbuf[3] = 0x9f;
	rxbuf[4] = 0x2f; // FCS 0x9F2F (CRC16-X25) 0xF9F4 byte-wise reflected
	checkFCS(&rxbuf[3]);

	/* http://www.lammertbies.nl/comm/info/crc-calculation.html

	   8c4ccc2cac6cec1c9c7609 -> 0x1D0F  (each msg byte is flipped)
	   3132333435363738399067 -> 0x0F2E
	*/

	rxbuf[0] = '1';
	rxbuf[1] = '2';
	rxbuf[2] = '3';
	rxbuf[3] = '4';
	rxbuf[4] = '5';
	rxbuf[5] = '6';
	rxbuf[6] = '7';
	rxbuf[7] = '8';
	rxbuf[8] = '9';
	rxbuf[9] = 0x90;
	rxbuf[10] = 0x6e; // FCS 0x906E (CRC16-X25) 0x0976 bytewise reflected
	checkFCS(&rxbuf[9]);

	// http://www.aerospacesoftware.com/winhexcom.html
	rxbuf[0] = 0x11;
	rxbuf[1] = 0x22;
	rxbuf[2] = 0x33;
	rxbuf[3] = 0x44;
	rxbuf[4] = 0x55;
	rxbuf[5] = 0x66;
	rxbuf[6] = 0x77;
	rxbuf[7] = 0x88;
	rxbuf[8] = 0x99;
	rxbuf[9] = 0x14;
	rxbuf[10] = 0x99; // FCS 0x1499 (CRC16-X25) 0x2899 bytewise reflected
	checkFCS(&rxbuf[9]);

	// next two: http://www.lammertbies.nl/forum/viewtopic.php?t=607
	rxbuf[0] = 0x10; // 7e 08 91 87 44 7e  <- 0x4487 HDLC
	rxbuf[1] = 0x89; // == 7e 10 89 e1 22 7e
	rxbuf[2] = 0x83;
	rxbuf[3] = 0x1f; // CRC16-X25 0x831F reflected is 0xC1F8
	checkFCS(&rxbuf[2]);
	rxbuf[2] = 0x1f;
	rxbuf[3] = 0x83; // set FCS (lo,hi)
	rxbuf[4] = 0x0f;
	rxbuf[5] = 0x47; // magic 0x0F47
	checkFCS(&rxbuf[4]);

	rxbuf[0] = 0x10; // 7e 08 b1 85 65 7e  <- 0x6585 HDLC
	rxbuf[1] = 0x8d; // == 7e 10 8d a1 a6 7e
	rxbuf[2] = 0xc5;
	rxbuf[3] = 0x3b; // CRC16-X25 0xC53B reflected Bwise is 0xA3DC
	checkFCS(&rxbuf[2]);
	rxbuf[2] = 0x3b;
	rxbuf[3] = 0xc5; // set FCS (lo,hi)
	rxbuf[4] = 0x0f;
	rxbuf[5] = 0x47; // magic 0x0F47
	checkFCS(&rxbuf[4]);

	// http://cs.nju.edu.cn/yangxc/dcc_teach/fcs-calc.pdf
	/*	rxbuf[0] = 0x72;
	rxbuf[1] = 0xd3;
	rxbuf[2] = 0x4f;
	rxbuf[3] = 0x3c;
	rxbuf[4] = 0x30; // FCS reflected Bwise is 0x3C30
	*/
	rxbuf = { 0x72, 0xd3, 0x4f, 0x3c, 0x0c };
	checkFCS(&rxbuf[3]);

	rxbuf[0] = 0x01; //DF test
	rxbuf[1] = 0x02;
	rxbuf[2] = 0x03;
	rxbuf[3] = 0x04;
	rxbuf[4] = 0x40;
	rxbuf[5] = 0x80;
	rxbuf[6] = 0x61; // (was) 0xb77b
	rxbuf[7] = 0xee; // 0x61EE is reflected Bw 0x8677
	checkFCS(&rxbuf[6]);

	// http://www.avrfreaks.net/index.php?name=PNphpBB2&file=printview&t=75742&start=0
	// (looks like an anonymized AX.25 packet)
	rxbuf[0] = 0x80;
	rxbuf[1] = 0x80;
	rxbuf[2] = 0x80;
	rxbuf[3] = 0x80;
	rxbuf[4] = 0x80;
	rxbuf[5] = 0x80;
	rxbuf[6] = 0xf2;
	rxbuf[7] = 0x9e; // addr char 0x9e>>1 = 0x4f == 'O'
	rxbuf[8] = 0x9e;
	rxbuf[9] = 0x9e;
	rxbuf[10] = 0x9e;
	rxbuf[11] = 0x9e;
	rxbuf[12] = 0x9e;
	rxbuf[13] = 0x61;
	rxbuf[14] = 0x03; // ax.25 flag
	rxbuf[15] = 0xf0; // ax.25 flag
	rxbuf[16] = 0x0b;
	rxbuf[17] = 0x58; // FCS is 0x0B58 (hi,lo) msb->lsb 0xd01a Bwise refl.
	checkFCS(&rxbuf[16]);
	rxbuf[16] = 0x58;
	rxbuf[17] = 0x0b; // set FCS (lo,hi) msb->lsb
	rxbuf[18] = 0x0f;
	rxbuf[19] = 0x47; // magic value 0x0F47 revflipped is 0xF0E2
	// bitflip(0x0F47) = 0xE2F0
	// ~(0xE2F0) = 0x1D0F
	// CRC-CCITT magic value is 0x1D0F
	checkFCS(&rxbuf[18]);
#endif // CRCDEBUG

	if (debug::level == debug::DEBUG_LEVEL) {
	    // same anonymized packet from above plus some trash before 1st flag
	    rxbuf[0]  = 0x66; rxbuf[1]  = 0x77; rxbuf[2]  = 0x7e; rxbuf[3]  = 0x7e;
		rxbuf[4]  = 0x80; rxbuf[5]  = 0x80; rxbuf[6]  = 0x80; rxbuf[7]  = 0x80;
		rxbuf[8]  = 0x80; rxbuf[9]  = 0x80; rxbuf[10] = 0xf2; rxbuf[11] = 0x9e;
		rxbuf[12] = 0x9e; rxbuf[13] = 0x9e; rxbuf[14] = 0x9e; rxbuf[15] = 0x9e;
		rxbuf[16] = 0x9e; rxbuf[17] = 0x61; rxbuf[18] = 0x03; rxbuf[19] = 0xf0;
		rxbuf[20] = 0x58; rxbuf[21] = 0x0b; rxbuf[22] = 0x7e; rxbuf[23] = 0x00;

	    unsigned char *cp, b;

	    for (cp = &rxbuf[0]; *cp ; cp++) {
		for (b = 0; b < 8; b++)
		if (*cp & (1 << b))
		    rx(true);
	        else
		    rx(false);
	    }

	    // "Sun May 24 14:56:03 2009" http://bk.gnarf.org/radio/aprs-20090524-1.txt
	    rxbuf[0]  = 0x7e; rxbuf[1]  = 0x82; rxbuf[2]  = 0xa0; rxbuf[3]  = 0xaa;
	    rxbuf[4]  = 0x64; rxbuf[5]  = 0x6a; rxbuf[6]  = 0x9c; rxbuf[7]  = 0xe0;
		rxbuf[8]  = 0xa6; rxbuf[9]  = 0x9a; rxbuf[10] = 0x6e; rxbuf[11] = 0x8e;
		rxbuf[12] = 0xb2; rxbuf[13] = 0xa8; rxbuf[14] = 0x60; rxbuf[15] = 0xae;
		rxbuf[16] = 0x92; rxbuf[17] = 0x88; rxbuf[18] = 0x8a; rxbuf[19] = 0x66;
		rxbuf[20] = 0x40; rxbuf[21] = 0x65; rxbuf[22] = 0x03; rxbuf[23] = 0xf0;
		rxbuf[24] = 0x3d; rxbuf[25] = 0x35; rxbuf[26] = 0x35; rxbuf[27] = 0x34;
		rxbuf[28] = 0x39; rxbuf[29] = 0x2e; rxbuf[30] = 0x32; rxbuf[31] = 0x36;
		rxbuf[32] = 0x4e; rxbuf[33] = 0x2f; rxbuf[34] = 0x30; rxbuf[35] = 0x31;
		rxbuf[36] = 0x33; rxbuf[37] = 0x30; rxbuf[38] = 0x37; rxbuf[39] = 0x2e;
		rxbuf[40] = 0x30; rxbuf[41] = 0x33; rxbuf[42] = 0x45; rxbuf[43] = 0x2f;
		rxbuf[44] = 0x2a; rxbuf[45] = 0x2a; rxbuf[46] = 0x4b; rxbuf[47] = 0x45;
		rxbuf[48] = 0x56; rxbuf[49] = 0x4c; rxbuf[50] = 0x41; rxbuf[51] = 0x4e;
		rxbuf[52] = 0x47; rxbuf[53] = 0x45; rxbuf[54] = 0x20; rxbuf[55] = 0x48;
		rxbuf[56] = 0x45; rxbuf[57] = 0x4c; rxbuf[58] = 0x49; rxbuf[59] = 0x50;
		rxbuf[60] = 0x4f; rxbuf[61] = 0x52; rxbuf[62] = 0x54; rxbuf[63] = 0x2a;
		rxbuf[64] = 0x2a; rxbuf[65] = 0x20; rxbuf[66] = 0x7b; rxbuf[67] = 0x55;
		rxbuf[68] = 0x49; rxbuf[69] = 0x56; rxbuf[70] = 0x33; rxbuf[71] = 0x32;
		rxbuf[72] = 0x4e; rxbuf[73] = 0x7d; rxbuf[74] = 0x0d; rxbuf[75] = 0xe8;
		rxbuf[76] = 0x03;

	    checkFCS(&rxbuf[75]);
	    do_put_rx_char(&rxbuf[75]);
	}

#undef NCODEBUG
#ifdef NCODEBUG
	NCO *osc = new NCO();
	osc->init(1200, 0, PKT_SampleRate);
	for(int i = 0; i < 30; i++) {
	    cmplx z = osc->cmplx_sample();
	    fprintf(stderr,"  %f    %f\n", z.re, z.im);
	}

	osc->init(2200, 0, PKT_SampleRate);
	for(int i = 0; i < 30; i++) {
	    cmplx z = osc->cmplx_sample();
	    fprintf(stderr,"  %f    %f\n", z.re, z.im);
	}

	delete osc;
#endif // NCODEBUG

	do_once = false;
    }
#endif // NDEBUG
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
}

void pkt::set_pkt_modem_params(int i)
{
    pkt_baud = BAUD[i];
    pkt_shift = SHIFT[i];
    pkt_nbits = BITS[i];
    pkt_ctrfreq = CENTER[i];

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
}

void pkt::restart()
{
    if (select_val != progdefaults.PKT_BAUD_SELECT) {
	select_val = progdefaults.PKT_BAUD_SELECT;
	set_pkt_modem_params(select_val);
    }

    snprintf(msg1, sizeof(msg1), "%4i / %-4.0f", pkt_baud, pkt_shift);
    put_Status1(msg1);
    put_MODEstatus(mode);

    if (!nco_lo)  nco_lo = new NCO();
    if (!nco_hi)  nco_hi = new NCO();
    if (!nco_mid) nco_mid = new NCO();

    nco_lo->init(pkt_ctrfreq-(pkt_shift/2), 0, PKT_SampleRate);
    nco_hi->init(pkt_ctrfreq+(pkt_shift/2), 0, PKT_SampleRate);
    nco_mid->init(pkt_ctrfreq, 0, PKT_SampleRate);

    set_freq(pkt_ctrfreq);

    set_bandwidth(pkt_shift); // waterfall tuning box

    wf->redraw_marker();

    if (!idle_signal_buf)
	idle_signal_buf = new double [PKT_IdleLen*PKT_MaxSymbolLen];

    idle_signal_pwr = idle_signal_buf_ptr = 0;
    for (int i = 0; i < pkt_idlelen; i++)
	idle_signal_buf[i] = 0;

    if (!lo_signal_buf)    lo_signal_buf = new cmplx [PKT_MaxSymbolLen];
    if (!hi_signal_buf)    hi_signal_buf = new cmplx [PKT_MaxSymbolLen];
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
    signal_power = noise_power = power_ratio = snr_avg = 1;

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

	restart();

	lo_tone = hi_tone = (NCO *)0;
	tx_char_count = MAXOCTETS-3; // leave room for FCS and end-flag

//	init_MicE_table();
//	init_PHG_table();
}

void pkt::update_syncscope()
{
    int j, len = pkt_syncdisplen;

    for (int i = 0; i < len; i++) {
	j = pipeptr - i;
	if (j < 0) j += len;
	dsppipe[i] = pipe[j];
    }
    set_scope(dsppipe, len, false);
}

void pkt::clear_syncscope()
{
    set_scope(0, 0, false);
}

/*
cmplx pkt::mixer(cmplx in)
{
	cmplx z;
	z.re = cos(phaseacc);
	z.im = sin(phaseacc);
	z = z * in;

	phaseacc -= TWOPI * frequency / samplerate;
	if (phaseacc > M_PI)
		phaseacc -= TWOPI;
	else if (phaseacc < M_PI)
		phaseacc += TWOPI;

	return z;
}
*/

unsigned char pkt::bitreverse(unsigned char in, int n)
{
	unsigned char out = 0;

	for (int i = 0; i < n; i++)
		out = (out << 1) | ((in >> i) & 1);

	return out;
}

unsigned int pkt::computeFCS(unsigned char *head, unsigned char *tail)
{
    unsigned char *c, b, tc;

    // CRC AX.25 Generator mask
    //             == bitflip(poly(x**16 + x**12 + x**5 + 1))
    //             == bitflip(0x1021) == bitflip(0001000000100001)
    //             == 1000010000001000 == 0x8408

    // http://www.ross.net/crc/download/crc_v3.txt
    
    // we're good as long as unsigned int is at least 16 bits
    unsigned int fcsv = 0xFFFF;

    // as in Ross except: shift right instead of left and reflected mask
    // (mirror image because AX.25 is lsb->msb order)

    for(c = head; c < tail; c++) {

	fcsv ^= *c;

	for(b = 0; b < 8; b++) {

	    if (fcsv & 0x0001)
		fcsv = (fcsv >> 1) ^ 0x8408;
	    else
		fcsv >>= 1;

	    fcsv &= 0xFFFF;
	}
    }
    fcsv ^= 0xFFFF;
    // fcsv is now (lo,hi)

    tc = (fcsv & 0xFF00) >> 8;
    fcsv = ((fcsv & 0x00FF) << 8) | tc;
    // fcsv is now (hi,lo)

    return fcsv;
}

// compare AX.25 Frame CheckSum (FCS) value to computed value

bool pkt::checkFCS(unsigned char *cp)
{
    // HDLC frame must be at least one byte plus FCS
    // AX.25 frame must be at least MINOCTETS plus FCS
    if (cp < &rxbuf[MINOCTETS-1])  return false;

    /* http://www.tapr.org/pub_ax25.html

       transmitted AX.25 FCS is big endian (high byte first)
       and bit-wise reversed for each byte versus the rest of the frame.

       because we traverse each byte from lsb->msb
       while receiving or transmitting
       -- and --
       the FCS is transmitted msb->lsb per specification, therefore:

       >>    the FCS must be in the buffer big endian
       >>    with a byte-wise bit-reversed ("reflected") order.

       ... really.
    */
    /* with more investigating I learn:

       AX.25 FCS is big endian and bit-wise lsb->msb like all the rest
       of the octets.

       so therefore:  the FCS goes in the buffer big endian order.

       ... just that and no more.
    */

    unsigned int fcsv_rcvd = (unsigned int)(cp[0] << 8) | cp[1];
    unsigned int fcsv = computeFCS(&rxbuf[1], cp); // begin after leading flag

    LOG_DEBUG("FCS computed %04X %s received %04X", fcsv,
	     (fcsv_rcvd == fcsv ? "==" : "<>"), fcsv_rcvd);

    if (fcsv_rcvd == fcsv)
	return true;

    return false;
}

inline void put_rx_const(const char s[])
{
    unsigned char *p = (unsigned char *) &s[0];
    for( ; *p; p++)  put_rx_char(*p);
}

inline void put_rx_hex(unsigned char c)
{
    char v[3];

    snprintf(&v[0], 3, "%02x", c);

    put_rx_char(v[0]);
    put_rx_char(v[1]);
}

void pkt::expand_Cmp(unsigned char *cpI)
{
    // APRS Spec 1.0.1 Chapter 9 - Compressed Position Report format

    unsigned char *cp, tc, cc;
    unsigned char Cmpbuf[96], *bp = &Cmpbuf[0];
    unsigned char *tbp = bp;

    double Lat, Lon, td;
    bool sign;

    cp = cpI+1; // skip past Symbol Table ID char

    // Latitude as base91 number
    tc = *cp++ - 33;
    Lat = tc * 91 * 91 * 91;  // fourth digit ==> x * 91^3
    tc = *cp++ - 33;
    Lat += tc * 91 * 91;  // third digit ==> x * 91^2
    tc = *cp++ - 33;
    Lat += tc * 91;  // second digit ==> x * 91^1
    tc = *cp++ - 33;
    Lat += tc;  // units digit ==> x * 91^0

    Lat = 90.0 - Lat / 380926.0;  // - ==> S, + ==> N

    // Longitude as base91 number
    tc = *cp++ - 33;
    Lon = tc * 91 * 91 * 91;  // 4th digit
    tc = *cp++ - 33;
    Lon += tc * 91 * 91;  // 3rd digit
    tc = *cp++ - 33;
    Lon += tc * 91;  // 2nd digit
    tc = *cp++ - 33;
    Lon += tc;  // units digit

    Lon = -180.0 + Lon / 190463.0;  // - ==> W, + ==> E

    if (Lat < 0) {
	sign = 1; // has sign (is negative)
	Lat *= -1;
    }
    else  sign = 0;

    td = Lat - floor(Lat);
    cc = snprintf((char *)bp, 3, "%2.f", (Lat - td)); // DD
    bp += cc;
    cc = snprintf((char *)bp, 6, "%05.2f", td*60); // MM.MM
    bp += cc;

    if (sign)  *bp++ = 'S';
    else  *bp++ = 'N';

    *bp++ = ' ';

    if (Lon < 0) {
	sign = 1;
	Lon *= -1;
    }
    else  sign = 0;

    td = Lon - floor(Lon);
    cc = snprintf((char *)bp, 4, "%03.f", (Lon - td)); // DDD
    bp += cc;
    cc = snprintf((char *)bp, 6, "%5.2f", td*60); // MM.MM
    bp += cc;

    if (sign)  *bp++ = 'W';
    else  *bp++ = 'E';

    cp += 1; // skip past Symbol Code char

    if (*cp != ' ') { // still more
	if ((*(cp + 2) & 0x18) == 0x10) { // NMEA source = GGA sentence
	    // compressed Altitude uses chars in the same range
	    // as CSE/SPD but the Compression Type ID takes precedence
	    // when it indicates the NMEA source is a GGA sentence.
	    // so check on this one first and CSE/SPD last.
	    double Altitude;

	    tc = *cp++ - 33;  // 2nd digit
	    Altitude = tc * 91;
	    tc = *cp++ - 33;
	    Altitude += tc;

	    // this compressed posit field is not very useful as spec'ed,
	    // since it cannot produce a possible negative altitude!
	    // the NMEA GGA sentence is perfectly capable of providing
	    // a negative altitude value.  Mic-E gets this right.

	    // Since the example given in the APRS 1.0.1 Spec uses a value
	    // in excess of 10000, this field should be re-spec'ed as a
	    // value in meters relative to 10km below mean sea level (just
	    // as done in Mic-E).
	    Altitude = pow(1.002, Altitude);

	    if (progdefaults.PKT_unitsSI)
		cc = snprintf((char *)bp, 11, " %-.1fm", Altitude*0.3048);
	    else // units per Spec
		cc = snprintf((char *)bp, 12, " %-.1fft", Altitude);
	    bp += cc;
	}
	else if (*cp == '{') { // pre-calculated radio range
	    double Range;

	    cp += 1; // skip past ID char

	    tc = *cp++ - 33; // range
	    Range = pow(1.08, (double)tc) * 2;

	    if (progdefaults.PKT_unitsSI)
		cc = snprintf((char *)bp, 24, " Est. Range = %-.1fkm", Range*1.609);
	    else // units per Spec
		cc = snprintf((char *)bp, 24, " Est. Range = %-.1fmi", Range);
	    bp += cc;
	}
	else if (*cp >= '!' && *cp <= 'z') { // compressed CSE/SPD
	    int Speed;

	    tc = *cp++ - 33; // course
	    cc = snprintf((char *)bp, 8, " %03ddeg", tc*4);
	    bp += cc;

	    tc = *cp++ - 33; // speed
	    Speed = (int)floor(pow(1.08, (double)tc) - 1); // 1.08^tc - 1 kts

	    if (progdefaults.PKT_unitsSI)
		cc = snprintf((char *)bp, 8, " %03dkph", (int)floor(Speed*1.852+0.5));
	    else if (progdefaults.PKT_unitsEnglish)
		cc = snprintf((char *)bp, 8, " %03dmph", (int)floor(Speed*1.151+0.5));
	    else // units per Spec
		cc = snprintf((char *)bp, 8, " %03dkts", Speed);
	    bp += cc;
	}
    }
    if (progdefaults.PKT_RXTimestamp)
	put_rx_const("           ");

    put_rx_const(" [Cmp] ");

    for(; tbp < bp; tbp++)  put_rx_char(*tbp);

    put_rx_char('\r');

    if (debug::level >= debug::VERBOSE_LEVEL) {
	cp = cpI+12; // skip to Compression Type ID char

	if (*(cp - 2) != ' ') { // Cmp Type ID is valid
	    tbp = bp = &Cmpbuf[0];

	    tc = *cp - 33; // T

	    cc = snprintf((char *)bp, 4, "%02x:", tc);
	    bp += cc;

	    strcpy((char *)bp, " GPS Fix = ");
	    bp += 11;

	    if ((tc & 0x20) == 0x20) {
		strcpy((char *)bp, "old");
		bp += 3;
	    }
	    else {
		strcpy((char *)bp, "current");
		bp += 7;
	    }

	    strcpy((char *)bp, ", NMEA Source = ");
	    bp += 16;

	    switch (tc & 0x18) {
	    case 0x00:
		strcpy((char *)bp, "other");
		bp += 5;
		break;
	    case 0x08:
		strcpy((char *)bp, "GLL");
		bp += 3;
		break;
	    case 0x10:
		strcpy((char *)bp, "GGA");
		bp += 3;
		break;
	    case 0x18:
		strcpy((char *)bp, "RMC");
		bp += 3;
		break;
	    default:
		strcpy((char *)bp, "\?\?");
		bp += 2;
		break;
	    }

	    strcpy((char *)bp, ", Cmp Origin = ");
	    bp += 15;

	    switch (tc & 0x07) {
	    case 0x00:
		strcpy((char *)bp, "Compressed");
		bp += 10;
		break;
	    case 0x01:
		strcpy((char *)bp, "TNC BText");
		bp += 9;
		break;
	    case 0x02:
		strcpy((char *)bp, "Software (DOS/Mac/Win/+SA)");
		bp += 26;
		break;
	    case 0x03:
		strcpy((char *)bp, "[tbd]");
		bp += 5;
		break;
	    case 0x04:
		strcpy((char *)bp, "KPC3");
		bp += 4;
		break;
	    case 0x05:
		strcpy((char *)bp, "Pico");
		bp += 4;
		break;
	    case 0x06:
		strcpy((char *)bp, "Other tracker [tbd]");
		bp += 19;
		break;
	    case 0x07:
		strcpy((char *)bp, "Digipeater conversion");
		bp += 21;
		break;
	    default:
		strcpy((char *)bp, "\?\?");
		bp += 2;
		break;
	    }

	    if (progdefaults.PKT_RXTimestamp)
		put_rx_const("           ");

	    put_rx_const(" [CmpType] ");

	    for(; tbp < bp; tbp++)  put_rx_char(*tbp);

	    put_rx_char('\r');
	}
    }
}

void pkt::expand_PHG(unsigned char *cpI)
{
    // APRS Spec 1.0.1 Chapter 6 - Time and Position format
    // APRS Spec 1.0.1 Chapter 7 - PHG Extension format

    bool hasPHG = false;
    unsigned char *cp, tc, cc;
    unsigned char PHGbuf[64], *bp = &PHGbuf[0];
    unsigned char *tbp = bp;

    switch (*cpI) {
    case '!':
    case '=': // simplest posits
	
	cp = cpI+1; // skip past posit ID char

        if (*cp != '/') { // posit not compressed
	    cp += 19; // skip past posit data
	}
	else { // posit is compressed
	    cp += 1; // skip past compressed posit ID char
	    cp += 12; // skip past compressed posit data
	}

	if (strncmp((const char *)cp, "PHG", 3) == 0) { // strings match
	    unsigned char ndigits;
	    int power, height;
	    double gain, range;

	    cp += 3; // skip past Data Extension ID chars

	    // get span of chars in cp which are only digits
	    ndigits = strspn((const char *)cp, "0123456789");

	    switch (ndigits) {
	    //case 1: H might be larger than '9'. code below will work.
	    //  must also check that P.GD are all '0'-'9'
	    //break;
	    case 4: // APRS Spec 1.0.1 Chapter 7 page 28
	    case 5: // PHGR proposed for APRS Spec 1.2
		hasPHG = true;

		tc = *cp++ - '0'; // P
		power = tc * tc; // tc^2
		cc = snprintf((char *)bp, 5, "%dW,", power);
		bp += cc;

		tc = *cp++ - '0'; // H
		*bp++ = ' ';
		if (tc < 30) { // constrain Height to signed 32bit value
		    height = 10 * (1 << tc); // 10 * 2^tc

		    if (progdefaults.PKT_unitsSI)
			cc = snprintf((char *)bp, 11, "%dm", (int)floor(height*0.3048+0.5));
		    else // units per Spec
			cc = snprintf((char *)bp, 12, "%dft", height);
		    bp += cc;
		}
		else {
		    height = 0;
		    strcpy((char *)bp, "-\?\?-");
		    bp += 4;
		}
		strcpy((char *)bp, " HAAT,");
		bp += 6;

		tc = *cp++; // G
		gain = pow(10, ((double)(tc - '0') / 10));
		cc = snprintf((char *)bp, 6, " %cdB,", tc);
		bp += cc;

		tc = *cp++ - '0'; // D
		*bp++ = ' ';
		if (tc < 9) {
		    strcpy((char *)bp, PHG_table[tc].s);
		    bp += PHG_table[tc].l;
		}
		else {
		    strcpy((char *)bp, "-\?\?-");
		    bp += 4;
		}
		*bp++ = ',';

		range = sqrt(2 * height * sqrt(((double)power / 10) * (gain / 2)));
		if (progdefaults.PKT_unitsSI)
		    cc = snprintf((char *)bp, 24, " Est. Range = %-.1fkm", range*1.609);
		else // units per Spec
		    cc = snprintf((char *)bp, 24, " Est. Range = %-.1fmi", range);
		bp += cc;

		if (ndigits == 5 && *(cp + 1) == '/') {
		    // PHGR: http://www.aprs.org/aprs12/probes.txt
		    // '1'-'9' and 'A'-'Z' are actually permissible.
		    // does anyone send 10 ('A') or more beacons per hour?
		    strcpy((char *)bp, ", ");
		    bp += 2;

		    tc = *cp++; // R
		    cc = snprintf((char *)bp, 14, "%c beacons/hr", tc);
		    bp += cc;
		}
		break;
	    default: // switch(ndigits)
		break;
	    }
	}
	break;
    default: // switch(*cpI)
	break;
    }

    if (hasPHG) {
	if (progdefaults.PKT_RXTimestamp)
	    put_rx_const("           ");

	put_rx_const(" [PHG] ");

	for(; tbp < bp; tbp++)  put_rx_char(*tbp);

	put_rx_char('\r');
    }
}

void pkt::expand_MicE(unsigned char *cpI, unsigned char *cpE)
{
    // APRS Spec 1.0.1 Chapter 10 - Mic-E Data format

    bool isMicE = true;
    bool msgstd = false, msgcustom = false;

    // decoding starts at first AX.25 dest addr
    unsigned char *cp = &rxbuf[1], tc, cc;
    unsigned char MicEbuf[64], *bp = &MicEbuf[0];
    unsigned char *tbp = bp;
    unsigned int msgABC = 0;
    PKT_MicE_field Lat = North, LonOffset = Zero, Lon = West;

    for (int i = 0; i < 3; i++) {
	// remember: AX.25 dest addr chars are shifted left by one
	tc = *cp++ >> 1;

	switch (tc & 0xF0) {
	case 0x30: // MicE_table[0]
	    cc = tc - '0';
	    if (cc < 10) {
		*bp++ = MicE_table[0][cc][0];
	    }
	    else  isMicE = false;
	    break;
	case 0x40: // MicE_table[1]
	    cc = tc - 'A';
	    if (cc < 12) {
		bool t = MicE_table[1][cc][1]-'0';
		if (t)  {
		    msgABC |= t << (2-i);
		    msgcustom = true;
		}
		else  msgABC &= ~(1 << (2-i));
		*bp++ = MicE_table[1][cc][0];
	    }
	    else  isMicE = false;
	    break;
	case 0x50: // MicE_table[2]
	    cc = tc - 'P';
	    if (cc < 11) {
		msgABC |= (MicE_table[2][cc][1]-'0') << (2-i);
		msgstd = true;
		*bp++ = MicE_table[2][cc][0];
	    }
	    else  isMicE = false;
	    break;
	default:   // Invalid
	    isMicE = false;
	    break;
	}
    }

    for (int i = 3; i < 6; i++) {
	// remember: AX.25 dest addr chars are shifted left by one
	tc = *cp++ >> 1;

	switch (i) {
	case 3:
	    switch (tc & 0xF0) {
	    case 0x30: // MicE_table[0]
		cc = tc - '0';
		if (cc < 10) {
		    Lat = MicE_table[0][cc][2];
		    *bp++ = MicE_table[0][cc][0];
		}
		else  isMicE = false;
		break;
	    case 0x40: // MicE_table[1]
		cc = tc - 'A';
		if (cc == 11) {
		    Lat = MicE_table[1][cc][2];
		    *bp++ = MicE_table[1][cc][0];
		}
		else  isMicE = false;
		break;
	    case 0x50: // MicE_table[2]
		cc = tc - 'P';
		if (cc < 11) {
		    Lat = MicE_table[2][cc][2];
		    *bp++ = MicE_table[2][cc][0];
		}
		else  isMicE = false;
		break;
	    default:   // Invalid
		isMicE = false;
		break;
	    }
	    break;
	case 4:
	    switch (tc & 0xF0) {
	    case 0x30: // MicE_table[0]
		cc = tc - '0';
		if (cc < 10) {
		    LonOffset = MicE_table[0][cc][3];
		    *bp++ = MicE_table[0][cc][0];
		}
		else  isMicE = false;
		break;
	    case 0x40: // MicE_table[1]
		cc = tc - 'A';
		if (cc == 11) {
		    LonOffset = MicE_table[1][cc][3];
		    *bp++ = MicE_table[1][cc][0];
		}
		else  isMicE = false;
		break;
	    case 0x50: // MicE_table[2]
		cc = tc - 'P';
		if (cc < 11) {
		    LonOffset = MicE_table[2][cc][3];
		    *bp++ = MicE_table[2][cc][0];
		}
		else  isMicE = false;
		break;
	    default:   // Invalid
		isMicE = false;
		break;
	    }
	    break;
	case 5:
	    switch (tc & 0xF0) {
	    case 0x30: // MicE_table[0]
		cc = tc - '0';
		if (cc < 10) {
		    Lon = MicE_table[0][cc][4];
		    *bp++ = MicE_table[0][cc][0];
		}
		else  isMicE = false;
		break;
	    case 0x40: // MicE_table[1]
		cc = tc - 'A';
		if (cc == 11) {
		    Lon = MicE_table[1][cc][4];
		    *bp++ = MicE_table[1][cc][0];
		}
		else  isMicE = false;
		break;
	    case 0x50: // MicE_table[2]
		cc = tc - 'P';
		if (cc < 11) {
		    Lon = MicE_table[2][cc][4];
		    *bp++ = MicE_table[2][cc][0];
		}
		else  isMicE = false;
		break;
	    default:   // Invalid
		isMicE = false;
		break;
	    }
	    break;
	default:   // Invalid
	    isMicE = false;
	    break;
	}
    }

    if (isMicE) {
	int Speed = 0, Course = 0;

	if (progdefaults.PKT_RXTimestamp)
	    put_rx_const("           ");

	put_rx_const(" [Mic-E] ");

	if (msgstd && msgcustom)
	    put_rx_const("Unknown? ");
	else if (msgcustom) {
	    put_rx_const("Custom-");
	    put_rx_char((7 - msgABC)+'0');
	    put_rx_const(". ");
	}
	else {
	    switch (msgABC) { // APRS Spec 1.0.1 Chapter 10 page 45
	    case 0:
		put_rx_const("Emergency");
		break;
	    case 1:
		put_rx_const("Priority");
		break;
	    case 2:
		put_rx_const("Special");
		break;
	    case 3:
		put_rx_const("Committed");
		break;
	    case 4:
		put_rx_const("Returning");
		break;
	    case 5:
		put_rx_const("In Service");
		break;
	    case 6:
		put_rx_const("En Route");
		break;
	    case 7:
		put_rx_const("Off Duty");
		break;
	    default:
		put_rx_const("-\?\?-");
		break;
	    }
	    if (msgABC)  put_rx_char('.');
	    else  put_rx_char('!'); // Emergency!

	    put_rx_char(' ');
	}

	for (; tbp < bp; tbp++) {
	    put_rx_char(*tbp);
	    if (tbp == (bp - 3))  put_rx_char('.');
	}

	if (Lat == North)  put_rx_char('N');
	else if (Lat == South)  put_rx_char('S');
	else  put_rx_char('\?');

	put_rx_char(' ');

	cp = cpI+1; // one past the Data Type ID char

	// decode Lon degrees - APRS Spec 1.0.1 Chapter 10 page 48
	tc = *cp++ - 28;
	if (LonOffset == P100) tc += 100;
	if (tc > 179 && tc < 190)  tc -= 80;
	else if (tc > 189 && tc < 200)  tc -= 190;

	cc = snprintf((char *)bp, 4, "%03d", tc);
	bp += cc;

	// decode Lon minutes
	tc = *cp++ - 28;
	if (tc > 59)  tc -= 60;

	cc = snprintf((char *)bp, 3, "%02d", tc);
	bp += cc;

	// decode Lon hundredths of a minute
	tc = *cp++ - 28;

	cc = snprintf((char *)bp, 3, "%02d", tc);
	bp += cc;

	for (; tbp < bp; tbp++) {
	    put_rx_char(*tbp);
	    if (tbp == (bp - 3))  put_rx_char('.');
	}

	if (Lon == East)  put_rx_char('E');
	else if (Lon == West)  put_rx_char('W');
	else  put_rx_char('\?');

	// decode Speed and Course - APRS Spec 1.0.1 Chapter 10 page 52
	tc = *cp++ - 28; // speed: hundreds and tens

	if (tc > 79)  tc -= 80;
	Speed = tc * 10;

	tc = *cp++ - 28; // speed: units and course: hundreds

	Course = (tc % 10); // remainder from dividing by 10
	tc -= Course; tc /= 10; // tc is now quotient from dividing by 10
	Speed += tc;

	if (Course > 3)  Course -= 4;
	Course *= 100;

	tc = *cp++ - 28; // course: tens and units

	Course += tc;

	if (progdefaults.PKT_unitsSI)
	    cc = snprintf((char *)bp, 8, " %03dkph", (int)floor(Speed*1.852+0.5));
	else if (progdefaults.PKT_unitsEnglish)
	    cc = snprintf((char *)bp, 8, " %03dmph", (int)floor(Speed*1.151+0.5));
	else // units per Spec
	    cc = snprintf((char *)bp, 8, " %03dkts", Speed);
	bp += cc;

	cc = snprintf((char *)bp, 8, " %03ddeg", Course);
	bp += cc;

	for (; tbp < bp; tbp++) {
	    put_rx_char(*tbp);
	}

	cp += 2; // skip past Symbol and Symbol Table ID chars

	if (cp <= cpE) { // still more

	    if (*cp == '>') {
		cp += 1;
		put_rx_const(" TH-D7");
	    }
	    else if (*cp == ']' && *cpE == '=') {
		cp += 1;
		cpE -= 1;
		put_rx_const(" TM-D710");
	    }
	    else if (*cp == ']') {
		cp += 1;
		put_rx_const(" TM-D700");
	    }
	    else if (*cp == '\'' && *(cpE - 1) == '|' && *cpE == '3') {
		cp += 1;
		cpE -= 2;
		put_rx_const(" TT3");
	    }
	    else if (*cp == '\'' && *(cpE - 1) == '|' && *cpE == '4') {
		cp += 1;
		cpE -= 2;
		put_rx_const(" TT4");
	    }
	    else if (*cp == '`' && *(cpE - 1) == '_' && *cpE == ' ') {
		cp += 1;
		cpE -= 2;
		put_rx_const(" VX-8");
	    }
	    else if (*cp == '`' && *(cpE - 1) == '_' && *cpE == '#') {
		cp += 1;
		cpE -= 2;
		put_rx_const(" VX-8D/G"); // VX-8G for certain. guessing.
	    }
	    else if (*cp == '`' && *(cpE - 1) == '_' && *cpE == '\"') {
		cp += 1;
		cpE -= 2;
		put_rx_const(" FTM-350");
	    }
	    else if ((*cp == '\'' || *cp == '`') && *(cp + 4) == '}') {
		cp += 1;
		// tracker? rig? ID codes are somewhat ad hoc.
		put_rx_const(" MFR\?");
	    }

	    if (cp < cpE) {
		if (*(cp + 3) == '}') { // station altitude as base91 number
		    int Altitude = 0;

		    tc = *cp++ - 33; // third digit ==> x * 91^2
		    Altitude = tc * 91 * 91;
		    tc = *cp++ - 33; // second digit ==> x * 91^1 ==> x * 91
		    Altitude += tc * 91;
		    tc = *cp++ - 33; // unit digit ==> x * 91^0 ==> x * 1
		    Altitude += tc;

		    Altitude -= 10000; // remove offset from datum

		    *bp++ = ' ';
		    if (Altitude >= 0)  *bp++ = '+';

		    if (progdefaults.PKT_unitsEnglish)
			cc = snprintf((char *)bp, 12, "%dft", (int)floor(Altitude*3.281+0.5));
		    else // units per Spec
			cc = snprintf((char *)bp, 11, "%dm", Altitude);
		    bp += cc;

		    for (; tbp < bp; tbp++) {
			put_rx_char(*tbp);
		    }

		    cp += 1; // skip past '}'
		}
	    }

	    if (cp < cpE)  put_rx_char(' ');

	    for (; cp <= cpE; cp++)  put_rx_char(*cp);
	}

	put_rx_char('\r');
    }
}

void pkt::do_put_rx_char(unsigned char *cp)
{
    int i, j;
    unsigned char c;
    bool isMicE = false;
    unsigned char *cpInfo;

    for (i = 8; i < 14; i++) { // src callsign is second in AX.25 frame
	c = rxbuf[i] >> 1;
	if (c != ' ') put_rx_char(c); // skip past padding (if any)
    }

    // bit  7   = command/response bit
    // bits 6,5 = 1
    // bits 4-1 = src SSID
    // bit  0   = last callsign flag
    c = (rxbuf[14] & 0x7f) >> 1;

    if (c > 0x30) {
	put_rx_char('-');
	if (c < 0x3a)
	    put_rx_char(c);
	else {
	    put_rx_char('1');
	    put_rx_char(c-0x0a);
	}
    }

    put_rx_char('>');

    for (i = 1; i < 7; i++) { // dest callsign is first in AX.25 frame
	c = rxbuf[i] >> 1;
	if (c != ' ') put_rx_char(c);
    }

    c = (rxbuf[7] & 0x7f) >> 1;
    if (c > 0x30) {
	put_rx_char('-');
	if (c < 0x3a)
	    put_rx_char(c);
	else {
	    put_rx_char('1');
	    put_rx_char(c-0x0a);
	}
    }

    j=8;
    if ((rxbuf[14] & 0x01) != 1) { // check last callsign flag
	do {
	    put_rx_char(',');

	    j += 7;
	    for (i = j; i < (j+6); i++) {
		c = rxbuf[i] >> 1;
		if (c != ' ') put_rx_char(c);
	    }

	    c = (rxbuf[j+6] & 0x7f) >> 1;
	    if (c > 0x30) {
		put_rx_char('-');
		if (c < 0x3a)
		    put_rx_char(c);
		else {
		    put_rx_char('1');
		    put_rx_char(c-0x0a);
		}
	    }

	} while ((rxbuf[j+6] & 0x01) != 1);

	if (rxbuf[j+6] & 0x80) // packet gets no more hops
	    put_rx_char('*');
    }

    if (debug::level < debug::VERBOSE_LEVEL) {
	// skip past CTRL and PID to INFO bytes when I_FRAME
	// puts buffer pointer in FCS when U_FRAME and S_FRAME
	// (save CTRL byte for possible MicE decoding)
	j += 7;
	c = rxbuf[j];
	j += 2;
    }
    else { // show more frame info when .ge. VERBOSE debug level
	j += 7;
	put_rx_char(';');

	c = rxbuf[j]; // CTRL present in all frames

	if ((c & 0x01) == 0) { // I_FRAME
	    unsigned char p = rxbuf[j+1]; // PID present only in I_FRAME

	    if (debug::level == debug::DEBUG_LEVEL) {
		put_rx_hex(c);
		put_rx_char(' ');
		put_rx_hex(p);
		put_rx_char(';');
	    }

	    put_rx_const("I/");

	    put_rx_hex( (c & 0xE0) >> 5 ); // AX.25 v2.2 para 2.3.2.1
	    if (c & 0x10)  put_rx_char('*'); // P/F bit
	    else  put_rx_char('.');
	    put_rx_hex( (c & 0x0E) >> 1 );

	    put_rx_char('/');

	    switch (p) { // AX.25 v2.2 para 2.2.4
	    case 0x01:
		put_rx_const("X.25PLP");
		break;
	    case 0x06:
		put_rx_const("C-TCPIP");
		break;
	    case 0x07:
		put_rx_const("U-TCPIP");
		break;
	    case 0x08:
		put_rx_const("FRAG");
		break;
	    case 0xC3:
		put_rx_const("TEXNET");
		break;
	    case 0xC4:
		put_rx_const("LQP");
		break;
	    case 0xCA:
		put_rx_const("ATALK");
		break;
	    case 0xCB:
		put_rx_const("ATALK-ARP");
		break;
	    case 0xCC:
		put_rx_const("ARPA-IP");
		break;
	    case 0xCD:
		put_rx_const("ARPA-AR");
		break;
	    case 0xCE:
		put_rx_const("FLEXNET");
		break;
	    case 0xCF:
		put_rx_const("NET/ROM");
		break;
	    case 0xF0:
		put_rx_const("NO-L3");
		break;

	    case 0xFF:
		put_rx_const("L3ESC=");
		put_rx_hex(rxbuf[++j]);
		break;

	    default:
		if ((p & 0x30) == 0x10)  put_rx_const("L3V1");
		else if ((p & 0x30) == 0x20)  put_rx_const("L3V2");
		else  put_rx_const("L3-RSVD");

		put_rx_char('=');
		put_rx_hex(p);
		break;
	    }
	}
	else if ((c & 0x03) == 0x01) { // S_FRAME

	    if (debug::level == debug::DEBUG_LEVEL) {
		put_rx_hex(c);
		put_rx_char(';');
	    }

	    put_rx_const("S/");

	    put_rx_hex( (c & 0xE0) >> 5 );
	    if (c & 0x10)  put_rx_char('*');
	    else  put_rx_char('.');
	    put_rx_char('/');

	    switch (c & 0x0C) { // AX.25 v2.2 para 2.3.4.2
	    case 0x00:
		put_rx_const("RR");
		break;
	    case 0x04:
		put_rx_const("RNR");
		break;
	    case 0x08:
		put_rx_const("REJ");
		break;
	    case 0x0C:
	    default:
		put_rx_const("UNK");
		break;
	    }
	}
	else if ((c & 0x03) == 0x03) { // U_FRAME

	    if (debug::level == debug::DEBUG_LEVEL) {
		put_rx_hex(c);
		put_rx_char(';');
	    }

	    put_rx_char('U');

	    if (c & 0x10)  put_rx_char('*');
	    else  put_rx_char('.');

	    switch (c & 0xEC) { // AX.25 v2.2 para 2.3.4.3
	    case 0x00:
		put_rx_const("UI");
		break;
	    case 0x0E:
		put_rx_const("DM");
		break;
	    case 0x1E:
		put_rx_const("SABM");
		break;
	    case 0x20:
		put_rx_const("DISC");
		break;
	    case 0x30:
		put_rx_const("UA");
		break;
	    case 0x81:
		put_rx_const("FRMR");
		break;
	    default:
		put_rx_const("UNK");
		break;
	    }
	}
	j+=2;
    }
    put_rx_char(':');

    // ptr to first info field char
    cpInfo = &rxbuf[j];

    if ((c & 0x03) == 0x03 && (c & 0xEC) == 0x00
	&& (*cpInfo == '\'' || *cpInfo == '`' 
	    || *cpInfo == 0x1C || *cpInfo == 0x1D)
	&& (cp - cpInfo) > 7) {
	/*
	  Mic-E must have at least 8 info chars + Data Type ID char
	  cp - (cpInfo - 1) > 8 ==> cp - cpInfo > 7
	*/
	// this is very probably a Mic-E encoded packet
	isMicE = true;
    }

    // offset between last info char (not FCS) and bufhead
    i = (cp - &rxbuf[0]); // (cp - &rxbuf[1]) + 1 ==> (cp - &rxbuf[0])

    while (j < i)  put_rx_char(rxbuf[j++]);

    if (*(cp-1) != '\r')
	put_rx_char('\r'); // <cr> only for packets not ending with <cr>

    // cp points to FCS, so (cp-X) is last info field char
    if ((progdefaults.PKT_expandMicE || debug::level >= debug::VERBOSE_LEVEL)
	&& isMicE)
	expand_MicE(cpInfo, (*(cp-1) == '\r' ? cp-2 : cp-1));

    // need to deal with posits having timestamps ('/' and '@' leading char)
    if (*cpInfo == '!' || *cpInfo == '=') {
	if ((progdefaults.PKT_expandCmp || debug::level >= debug::VERBOSE_LEVEL)
	     && (*(cpInfo + 1) == '/' || *(cpInfo + 1) == '\\'))
	    // compressed posit
	    expand_Cmp(cpInfo+1);

	if (progdefaults.PKT_expandPHG
	    || debug::level >= debug::VERBOSE_LEVEL) // look for PHG data
	    expand_PHG(cpInfo);
    }

    if (*(cp-1) == '\r')
	put_rx_char('\r'); // for packets ending with <cr>: show it on-screen
}


void pkt::rx(bool bit)
{
    static unsigned char c = 0, bcounter = 0;

    c >>= 1;
    ++bcounter;

    if (bit == false) {
	c &= ~(1 << (pkt_nbits-1)); // bits are sent lsb first

	if (seq_ones == 6) { // flag byte found
	    bcounter = 0;

	    if (cbuf >= &rxbuf[MINOCTETS]) { // flag at end of frame

		*cbuf = PKT_Flag; // == 0x7e

		if (debug::level == debug::DEBUG_LEVEL)
		    fprintf(stderr,"7e\n");

		//   monitor Metric and Squelch Level
		if ( !progStatus.sqlonoff ||
		     metric >= progStatus.sldrSquelchValue ) { // lazy eval
		    /*
		    // check FCS at end of frame
		    // if FCS is OK - packet is in rxbuffer,
		    //		  put_rx_char() for each byte in rxbuffer
		    */
		    if (checkFCS((cbuf - 2)) == true) {
			if (progdefaults.PKT_RXTimestamp) {
			    unsigned char ts[16], *tc = &ts[0];
			    time_t t = time(NULL);
			    struct tm stm;

			    (void)gmtime_r(&t, &stm);
			    snprintf((char *)ts, sizeof(ts),
				     "[%02d:%02d:%02d] ",
				     stm.tm_hour, stm.tm_min, stm.tm_sec);

			    while (*tc)  put_rx_char(*tc++);
			}
			do_put_rx_char((cbuf - 2));
		    }
		}

		cbuf = &rxbuf[0]; // reset after first end frame flag
	    }
	    else {
		// packet too short if cbuf < &rxbuf[MINOCTETS]

		// put only one beginning flag into buffer
		rxbuf[0] = PKT_Flag;
		cbuf = &rxbuf[1];

		if (debug::level == debug::DEBUG_LEVEL)
		    fprintf(stderr,"7e ");
	    }
	}
	else if (seq_ones == 5) { // need bit un-stuffing
	    c <<= 1;  // shift c back to skip stuffed bit
	    --bcounter;
	}

	seq_ones = 0;
    }
    else {
	c |= (1 << (pkt_nbits-1)); // bits are sent lsb first
	++seq_ones;

	//if (seq_ones > 6) { // something is wrong
	//}
    }

    if (bcounter == pkt_nbits) {
	bcounter = 0;
	if (cbuf < &rxbuf[MAXOCTETS]) {
	    *cbuf++ = c;

	    if (debug::level == debug::DEBUG_LEVEL)
		fprintf(stderr,"%02x ",c);
	}
	else
	    // else complain: cbuf is at MAXOCTETS
	    LOG_WARN("Long input packet, %d octets!",(int)(cbuf - &rxbuf[0]));
    }

    /*
    //   when flag found: keep collecting flags (0x7e) until !(0x7e) found
    //   (first non-0x7e begins DATA)

    //   keep collecting bits in DATA, 8 at-a-time, until 0x7e found
    //   while collecting bits, perform bit-unstuffing so we place in
    //   databuffer exactly 8 bits at-a-time
    //   (at times more than 8 bits in to 8 bits out)
    //   first trailing 0x7e ends DATA and begins STOP
    */
}

void pkt::Metric()
{
    double snr = 0;
    double pr = signal_power / noise_power;

    power_ratio = decayavg(power_ratio, pr, pr-power_ratio > 0 ? 2 : 8);
    snr = 10*log10( power_ratio );

    snprintf(msg2, sizeof(msg2), "s/n %3.0f dB", snr-snr_avg);
    put_Status2(msg2);

    metric = CLAMP(power_ratio, 0.0, 100.0);
    display_metric(metric);

    if (metric < 5.0)
	snr_avg = decayavg(snr_avg, snr, 8);
}

void pkt::idle_signal_power(double sample)
{
    // average of signal energy over PKT_IdleLen duration

    sample *= sample;

    idle_signal_pwr += sample;
    idle_signal_pwr -= idle_signal_buf[idle_signal_buf_ptr];
    idle_signal_buf[idle_signal_buf_ptr] = sample;

    ++idle_signal_buf_ptr %= pkt_idlelen; // circular buffer
}

#ifndef NDEBUG
double pkt::corr_power(cmplx z)
#else
inline double pkt::corr_power(cmplx z)
#endif
{
    // scaled magnitude
    double power = norm(z);
    power /= 2 * symbollen;

    return power;
}

void pkt::correlate(double sample)
{
    cmplx yl, yh, yt;

    yl = nco_lo->cmplx_sample();
    yl *= sample;

    lo_signal_energy += yl;
    lo_signal_energy -= lo_signal_buf[correlate_buf_ptr];
    lo_signal_buf[correlate_buf_ptr] = yl;
    lo_signal_corr = lo_signal_gain * corr_power(lo_signal_energy);

    yh = nco_hi->cmplx_sample();
    yh *= sample;

    hi_signal_energy += yh;
    hi_signal_energy -= hi_signal_buf[correlate_buf_ptr];
    hi_signal_buf[correlate_buf_ptr] = yh;
    hi_signal_corr = hi_signal_gain * corr_power(hi_signal_energy);

    yt = nco_mid->cmplx_sample();
    yt *= sample;

    mid_signal_energy += yt;
    mid_signal_energy -= mid_signal_buf[correlate_buf_ptr];
    mid_signal_buf[correlate_buf_ptr] = yt;
    //mid_signal_corr = corr_power(mid_signal_energy);

    ++correlate_buf_ptr %= symbollen; // SymbolLen correlation window

    yt = mid_signal_energy;

    if (abs(lo_signal_corr - hi_signal_corr) < 0.2) { // mid-symbol or noise
	pipe[pipeptr] = 0.0;

	QI[QIptr] = cmplx(yt.imag(), yt.real());
    }
    else if (lo_signal_corr > hi_signal_corr) {
	pipe[pipeptr] = 0.5;

	QI[QIptr] = cmplx (0.125 * yt.imag(), yt.real());
    }
    else {
	pipe[pipeptr] = -0.5;

	QI[QIptr] = cmplx( yt.real(), 0.125 * yt.imag());
    }
    ++pipeptr %= pkt_syncdisplen;

    yt_avg = decayavg(yt_avg, abs(yt), symbollen/2);

    if (yt_avg > 0)  QI[QIptr] = QI[QIptr] / yt_avg;

    ++QIptr %= MAX_ZLEN;
}

void pkt::detect_signal()
{
    double sig_corr = lo_signal_corr + hi_signal_corr;// - mid_signal_corr;

    signal_pwr += sig_corr;
    signal_pwr -= signal_buf[signal_buf_ptr];
    signal_buf[signal_buf_ptr] = sig_corr;

    ++signal_buf_ptr %= pkt_detectlen; // circular buffer

    signal_power = signal_pwr / pkt_detectlen * signal_gain;
    noise_power = idle_signal_pwr / pkt_idlelen;

    // allow lazy eval
    if (signal_power > PKT_MinSignalPwr && signal_power > noise_power) {
	// lo+hi freq signals are present
	detect_drop = pkt_detectlen; // reset signal drop counter

	if (rxstate == PKT_RX_STATE_DROP) {
	    rxstate = PKT_RX_STATE_START;

	    // init STATE_START
	    signal_gain = 1.0; // 5.0
	    scounter = pkt_detectlen; //(was) 9 * symbollen;//11 instead of 9?
	    lo_sync = 100.0;
	    hi_sync = -100.0;
	}
    }
    else if (--detect_drop < 1 && rxstate == PKT_RX_STATE_DATA) { // lazy eval
	// give up on signals - gone,gone.
	rxstate = PKT_RX_STATE_STOP;
	scounter = 0; // (scounter is signed int)

	if (debug::level == debug::DEBUG_LEVEL)
	    fprintf(stderr,"\n");
    }
}

void pkt::do_sync()
{
    double power_delta = lo_signal_corr - hi_signal_corr;

    lo_sync_ptr--; hi_sync_ptr--;

    if (lo_sync > power_delta && power_delta < 0) {
	lo_sync_ptr = 0;
	lo_sync = power_delta;
    }

    if (hi_sync < power_delta && power_delta > 0) {
	hi_sync_ptr = 0;
	hi_sync = power_delta;
    }

    if (--scounter < 1) {
	int offset;

	if (fabs(hi_sync) > fabs(lo_sync))
	    offset = -hi_sync_ptr;
	else
	    offset = -lo_sync_ptr;

	offset %= symbollen; // clamp offset to +/- one symbol

	mid_symbol = symbollen - offset;
	prev_symbol = pll_symbol = hi_signal_corr < lo_signal_corr;
	
	rxstate = PKT_RX_STATE_DATA;
	seq_ones = 0;     // reset once on transition to STATE_DATA
	cbuf = &rxbuf[0]; // and reset packet buffer ptr
    }
}

void pkt::rx_data()
{
    bool tbit = hi_signal_corr < lo_signal_corr; // detect current symbol

    mid_symbol -= 1;
    int imid_symbol = (int) floor(mid_symbol+0.5);

    // hard-decision symbol decoding.  done at mid-symbol (imid_symbol < 1).
    //
    // might do instead a soft-decision by cumulative voting
    // of more than one mid-symbol sample?  symbollen-2 samples max?
    //
    if (imid_symbol < 1) { // counting down to middle of next symbol

	bool bit = prev_symbol == tbit; // detect symbol change

	prev_symbol = pll_symbol = tbit;
	mid_symbol += symbollen; // advance to middle of next symbol

	// remember to check value of reverse here to determine symbol
	rx(reverse ? !bit : bit);
    }
    else
	if (tbit != pll_symbol) { // update PLL
	    if (mid_symbol < ((double)symbollen/2))
		mid_symbol += PKT_PLLAdjVal;
	    else
		mid_symbol -= PKT_PLLAdjVal;

	    pll_symbol = tbit;
	}
}

int pkt::rx_process(const double *buf, int len)
{
    double x_n;

    if (select_val != progdefaults.PKT_BAUD_SELECT) {
	// need to re-init modem
	restart();
    }

    Metric();

    while (len-- > 0) {

	x_n = *buf++;

	switch (rxstate) {

	case PKT_RX_STATE_STOP:
	default:
	    rxstate = PKT_RX_STATE_IDLE;
	    // fallthrough with next sample (x_(n+1))

	case PKT_RX_STATE_IDLE:
	    idle_signal_power( x_n );

	    if (--scounter < 1) {
		rxstate = PKT_RX_STATE_DROP;

		scounter = pkt_idlelen;
		signal_gain = 1.0;

		signal_pwr = signal_buf_ptr = 0;

		for(int i = 0; i < pkt_detectlen; i++)
		    signal_buf[i] = 0;

		lo_signal_energy = hi_signal_energy =
		    mid_signal_energy = cmplx(0, 0);

		yt_avg = correlate_buf_ptr = 0;

		for(int i = 0; i < symbollen; i++)
		    lo_signal_buf[i] = hi_signal_buf[i] =
			mid_signal_buf[i] = cmplx(0, 0);
	    }
	    break;

	case PKT_RX_STATE_DROP:
	    idle_signal_power( x_n );
	    correlate( x_n );
	    detect_signal();
	    break;

	case PKT_RX_STATE_START:
	    correlate( x_n );
	    do_sync();
	    break;

	case PKT_RX_STATE_DATA:
	    correlate( x_n );
	    rx_data();
	    detect_signal();
	    break;
	}
    } // while(len)

    if (metric < progStatus.sldrSquelchValue && progStatus.sqlonoff) {
	if (clear_zdata) {
	    for (int i = 0; i < MAX_ZLEN; i++) 
			QI[i] = cmplx(0,0);
	    QIptr = 0;
	    set_zdata(QI, MAX_ZLEN);
	    clear_zdata = false;
	    clear_syncscope();
	}
    } else {
	clear_zdata = true;
	update_syncscope();
	set_zdata(QI, MAX_ZLEN);
    }

    return 0;
}

//=====================================================================
// PKT1200 transmit
//=====================================================================

void pkt::send_symbol(bool bit)
{
    if (reverse)  bit = !bit;

    for (int i = 0; i < symbollen; i++) {
	if (bit)
	    outbuf[i] = hi_tone->sample() * hi_txgain; // modem::outbuf[]
	else
	    outbuf[i] = lo_tone->sample() * lo_txgain; // modem::outbuf[]
    }

    // adjust nco phase accumulator to pick up where the other nco left off
    if (bit)
	lo_tone->setphaseacc(hi_tone->getphaseacc());
    else
	hi_tone->setphaseacc(lo_tone->getphaseacc());

    ModulateXmtr(outbuf, symbollen); // ignore retv?
}

/*
void pkt::send_stop()
{
	double freq;
	bool invert = reverse;

	if (invert)
		freq = get_txfreq_woffset() - pkt_shift / 2.0;
	else
		freq = get_txfreq_woffset() + pkt_shift / 2.0;

	//    stoplen = 0;
	// stoplen ::= (int) ((1.0 * 12000 / 1200) + 0.5) = 10

	for (int i = 0; i < stoplen; i++) {
		outbuf[i] = nco(freq);
		if (invert)
			FSKbuf[i] = 0.0 * FSKnco();
		else
			FSKbuf[i] = FSKnco();
	}
	if (progdefaults.PseudoFSK)
		ModulateStereo(outbuf, FSKbuf, stoplen);
	else
		ModulateXmtr(outbuf, stoplen);
}
*/

void pkt::send_char(unsigned char c)
{
    // if nostuff == 1 then { do _no_ bit stuffing }
    // else { keep track of no-transition (one bits) and stuff as needed }

    for (int i = 0; i < pkt_nbits; i++) {

	if ((c & 0x01) == 0) {
	    currbit = !currbit; // transition on zero bits
	    send_symbol(currbit);

	    nr_ones = 0;
	}
	else {
	    send_symbol(currbit);

	    if (++nr_ones == 5 && !nostuff) {
		currbit = !currbit; // stuff zero bit
		send_symbol(currbit);

		nr_ones = 0;
	    }
	}

	c >>= 1; // lsb to msb bit order
    }
}

string upcase(string s)
{
    // this function is also defined in arq::upcase
    // how to make a fldigi::upcase and use it here and in flarq?
    // maybe add it to misc/strutil.cxx ?

    for (size_t i = 0; i < s.length(); i++)
	s[i] = toupper(s[i]);
    return s;
}

void pkt::send_msg(unsigned char c)
{
    // form TX pane incoming chars into a valid UI AX.25 packet
    // in KISS mode, we do not create a packet header.
    if (!did_pkt_head) {
	int i, j;
	// the following is split into two because putting it all together
	// as "upcase(progdefaults.myCall).c_str()" would compile but not
	// work correctly.  Callsign decoded as "******-9" (wrong!).
	string ts = upcase(progdefaults.myCall);
	const char *src = ts.c_str(), // MYCALL
	    dest2[] = "WIDE2 "; // digi
	char dest[7];

	snprintf(dest, 7, "APL%1d%02d", FLDIGI_VERSION_MAJOR,
		 FLDIGI_VERSION_MINOR); // dest[] = "APL321"

	for (i = 7, j = 0; i < 13; i++, j++) { // put src callsign in frame
	    if (src[j] == 0)  break;
	    txbuf[i] = src[j] << 1;
	}
	while (j++ < 6) {
	    txbuf[i++] = ' ' << 1; // padding
	}

	txbuf[13] = '9' << 1; // src + SSID ==> MYCALL-9

	for (i = 0, j = 0; i < 6; i++, j++ ) { // put dest callsign in frame
	    txbuf[i] = dest[j] << 1;
	}
	txbuf[6] = '1' << 1; // dest[] + SSID ==> APL321-1

	if (pkt_baud >= 1200) {
	    for (i = 14, j = 0; i < 20; i++, j++) { // put digi callsign in frame
		txbuf[i] = dest2[j] << 1;
	    }
	    txbuf[20] = '1' << 1; // dest2[] + SSID ==> WIDE1-1
	    txbuf[20] |= 0x01; // add last callsign flag

	    tx_cbuf = &txbuf[21];
	}
	else { // use no digi when baud < 1200. limited bandwidth!
	    txbuf[6] |= 0x01; // add last callsign flag

	    tx_cbuf = &txbuf[14];
	}

	*tx_cbuf++ = 0x03; // add CTRL val for U_FRAME type UI
	*tx_cbuf++ = 0xf0;

	unsigned char *tc = &txbuf[0];

	while (tc < tx_cbuf) {
	    send_char(*tc++);
	    tx_char_count--;  // must count header chars in pkt length
	}

	did_pkt_head = true;
    }

    send_char(c);
    *tx_cbuf++ = c;
}


int pkt::tx_process()
{
    if (pretone > 0) {
	while (pretone-- > 0) {
	    send_symbol(0); // Mark (low) tone
	}
    }

    if (preamble > 0) {
	nostuff = 1;  // turn off bit stuffing here
	while (preamble-- > 0) {
	    send_char(PKT_Flag);
	}
	nostuff = 0;  // send_char() is 8-bit clean
	tx_char_count--; // count only last flag char
    }

    static int c = 0;
    if (tx_char_count >  0)
	c = get_tx_char();

    // TX buffer empty
    //    if (c == 0x03 || stopflag) {
    if (c == GET_TX_CHAR_ETX || stopflag || tx_char_count == 0) {
	if (!stopflag) {
	    // compute FCS and add it to the frame via *tx_cbuf
	    unsigned int fcs = computeFCS(&txbuf[0], tx_cbuf);
	    // (B == Byte, b == bit)
	    unsigned char tc = (fcs & 0xFF00) >> 8; // msB

	    *tx_cbuf++ = tc;
	    send_char(tc);

	    tc = (unsigned char)(fcs & 0x00FF); // lsB

	    *tx_cbuf++ = tc;
	    send_char(tc);

	    nostuff = 1;  // turn off bit stuffing
	    while (postamble-- > 0) {
		send_char(PKT_Flag);
	    }
	    nostuff = 0;

	    stopflag = true;
	    return 0;
	}

	stopflag = false;
	tx_char_count = MAXOCTETS-3; // leave room for FCS and end-flag
	tx_cbuf = &txbuf[0];
	currbit = 0;
	did_pkt_head = false;

	cwid();
	return -1;
    }

    if (tx_char_count-- > 0)
	send_msg((unsigned char)c);
    else {
	// else complain:  maybe auto-segment?
	LOG_WARN("Long output packet, %d octets!",(int)(tx_cbuf - &txbuf[0]));

	return -1;
    }

    return 0;
}
