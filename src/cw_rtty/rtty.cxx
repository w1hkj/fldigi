// ----------------------------------------------------------------------------
// rtty.cxx  --  RTTY modem
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code
// distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
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
#include <iostream>
using namespace std;

#include "rtty.h"
#include "fl_digi.h"
#include "digiscope.h"
#include "misc.h"
#include "waterfall.h"
#include "confdialog.h"
#include "configuration.h"
#include "status.h"
#include "digiscope.h"

//=====================================================================
// Baudot support
//=====================================================================

static unsigned char letters[32] = {
	'\0',	'E',	'\n',	'A',	' ',	'S',	'I',	'U',
	'\r',	'D',	'R',	'J',	'N',	'F',	'C',	'K',
	'T',	'Z',	'L',	'W',	'H',	'Y',	'P',	'Q',
	'O',	'B',	'G',	'·',	'M',	'X',	'V',	'·'
};

#if 0
/*
 * ITA-2 version of the figures case.
 */
static unsigned char figures[32] = {
	'\0',	'3',	'\n',	'-',	' ',	'\'',	'8',	'7',
	'\r',	'·',	'4',	'\a',	',',	'·',	':',	'(',
	'5',	'+',	')',	'2',	'·',	'6',	'0',	'1',
	'9',	'?',	'·',	'·',	'.',	'/',	'=',	'·'
};
#endif
#if 1
/*
 * U.S. version of the figures case.
 */
static unsigned char figures[32] = {
	'\0',	'3',	'\n',	'-',	' ',	'\a',	'8',	'7',
	'\r',	'$',	'4',	'\'',	',',	'!',	':',	'(',
	'5',	'"',	')',	'2',	'#',	'6',	'0',	'1',
	'9',	'?',	'&',	'·',	'.',	'/',	';',	'·'
};
#endif
#if 0
/*
 * A mix of the two. This is what seems to be what people actually use.
 */
static unsigned char figures[32] = {
	'\0',	'3',	'\n',	'-',	' ',	'\'',	'8',	'7',
	'\r',	'$',	'4',	'\a',	',',	'!',	':',	'(',
	'5',	'+',	')',	'2',	'#',	'6',	'0',	'1',
	'9',	'?',	'&',	'·',	'.',	'/',	'=',	'·'
};
#endif

int dspcnt = 0;


static char msg1[20];

double _SHIFT[] = {23, 85, 160, 170, 182, 200, 240, 350, 425, 850};
double _BAUD[] = {45, 45.45, 50, 56, 75, 100, 110, 150, 200, 300};
int    _BITS[] = {5, 7, 8};

void rtty::tx_init(SoundBase *sc)
{
// start each new transmission 20 bit lengths MARK tone
	scard = sc;
	phaseacc = 0;
	preamble = 20;
	videoText();
}

void rtty::rx_init()
{
	rxstate = RTTY_RX_STATE_IDLE;
	rxmode = LETTERS;
	phaseacc = 0;
	FSKphaseacc = 0;
	for (int i = 0; i < RTTYMaxSymLen; i++ ) {
		bbfilter[i] = 0.0;
	}
	bitfilt->reset();
	poserr = negerr = 0.0;
}

void rtty::init()
{
	bool wfrev = wf->Reverse();
	bool wfsb = wf->USB();
	reverse = wfrev ^ !wfsb;

	if (progdefaults.StartAtSweetSpot)
		set_freq(progdefaults.RTTYsweetspot);
	else if (progStatus.carrier != 0) {
		set_freq(progStatus.carrier);
#if !BENCHMARK_MODE
		progStatus.carrier = 0;
#endif
	} else
		set_freq(wf->Carrier());

	rx_init();
	put_MODEstatus(mode);
	snprintf(msg1, sizeof(msg1), "%-4.1f / %-4.0f", rtty_baud, rtty_shift);
	put_Status1(msg1);
	if (progdefaults.PreferXhairScope)
		set_scope_mode(Digiscope::XHAIRS);
	else
		set_scope_mode(Digiscope::RTTY);
}

rtty::~rtty()
{
	if (hilbert) delete hilbert;
	if (bitfilt) delete bitfilt;
	if (bpfilt) delete bpfilt;
	if (pipe) delete [] pipe;
	if (dsppipe) delete [] dsppipe;
}

void rtty::restart()
{
	double stl;

	rtty_shift = shift = (progdefaults.rtty_shift >= 0 ?
			      _SHIFT[progdefaults.rtty_shift] : progdefaults.rtty_custom_shift);
	rtty_baud = _BAUD[progdefaults.rtty_baud];
	nbits = rtty_bits = _BITS[progdefaults.rtty_bits];
	if (rtty_bits == 5)
		rtty_parity = RTTY_PARITY_NONE;
	else
		switch (progdefaults.rtty_parity) {
			case 0 : rtty_parity = RTTY_PARITY_NONE; break;
			case 1 : rtty_parity = RTTY_PARITY_EVEN; break;
			case 2 : rtty_parity = RTTY_PARITY_ODD; break;
			case 3 : rtty_parity = RTTY_PARITY_ZERO; break;
			case 4 : rtty_parity = RTTY_PARITY_ONE; break;
			default : rtty_parity = RTTY_PARITY_NONE; break;
		}
	rtty_stop = progdefaults.rtty_stop;

	txmode = LETTERS;
	rxmode = LETTERS;
	symbollen = (int) (samplerate / rtty_baud + 0.5);
	set_bandwidth(shift);

	rtty_BW = 1.5 * rtty_baud;
	progdefaults.RTTY_BW = rtty_BW;
	sldrRTTYbandwidth->value(rtty_BW);

	wf->redraw_marker();

	bp_filt_lo = (shift/2.0 - rtty_BW/2.0) / samplerate;
	if (bp_filt_lo < 0) bp_filt_lo = 0;
	bp_filt_hi = (shift/2.0 + rtty_BW/2.0) / samplerate;

	if (bpfilt)
		bpfilt->create_filter(bp_filt_lo, bp_filt_hi);
	else
		bpfilt = new fftfilt(bp_filt_lo, bp_filt_hi, 1024);

	bflen = symbollen/3;
	if (bitfilt)
		bitfilt->setLength(bflen);
	else
		bitfilt = new Cmovavg(bflen);

// stop length = 1, 1.5 or 2 bits
	rtty_stop = progdefaults.rtty_stop;
	if (rtty_stop == 0) stl = 1.0;
	else if (rtty_stop == 1) stl = 1.5;
	else stl = 2.0;
	stoplen = (int) (stl * samplerate / rtty_baud + 0.5);
	freqerr = 0.0;
	filterptr = 0;
	pipeptr = 0;
	poscnt = negcnt = 0;
	posfreq = negfreq = 0.0;

	metric = 0.0;

	snprintf(msg1, sizeof(msg1), "%-4.1f / %-4.0f", rtty_baud, rtty_shift);
	put_Status1(msg1);
	put_MODEstatus(mode);
	for (int i = 0; i < 1024; i++) QI[i].re = QI[i].im = 0.0;
	sigpwr = 0.0;
	noisepwr = 0.0;
	freqerrlo = freqerrhi = 0.0;
	sigsearch = 0;
	dspcnt = 2*(nbits + 2);

	clear_zdata = true;
}

rtty::rtty(trx_mode tty_mode)
{
	cap |= CAP_AFC | CAP_REV;

	mode = tty_mode;

	samplerate = RTTY_SampleRate;

	bpfilt = (fftfilt *)0;

	bitfilt = (Cmovavg *)0;

	hilbert = new C_FIR_filter();
	hilbert->init_hilbert(37, 1);

	pipe = new double[MAXPIPE];
	dsppipe = new double [MAXPIPE];

	samples = new complex[8];

	restart();
}

void rtty::update_syncscope()
{
	int j;
	for (int i = 0; i < symbollen; i++) {
		j = pipeptr - i;
		if (j < 0) j += symbollen;
		dsppipe[i] = pipe[j];
	}
	set_scope(dsppipe, symbollen, false);
}

void rtty::clear_syncscope()
{
	set_scope(0, 0, false);
}

complex rtty::mixer(complex in)
{
	complex z;
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

unsigned char rtty::bitreverse(unsigned char in, int n)
{
	unsigned char out = 0;

	for (int i = 0; i < n; i++)
		out = (out << 1) | ((in >> i) & 1);

	return out;
}

static int rparity(int c)
{
	int w = c;
	int p = 0;
	while (w) {
		p += (w & 1);
		w >>= 1;
	}
	return p & 1;
}

int rtty::rttyparity(unsigned int c)
{
	c &= (1 << nbits) - 1;

	switch (rtty_parity) {
	default:
	case RTTY_PARITY_NONE:
		return 0;

	case RTTY_PARITY_ODD:
		return rparity(c);

	case RTTY_PARITY_EVEN:
		return !rparity(c);

	case RTTY_PARITY_ZERO:
		return 0;

	case RTTY_PARITY_ONE:
		return 1;
	}
}

int rtty::decode_char()
{
	unsigned int parbit, par, data;

	parbit = (rxdata >> nbits) & 1;
	par = rttyparity(rxdata);

	if (rtty_parity != RTTY_PARITY_NONE && parbit != par)
		return 0;

	data = rxdata & ((1 << nbits) - 1);

	if (nbits == 5)
		return baudot_dec(data);

	return data;
}

bool rtty::rx(bool bit)
{
	bool flag = false;
	unsigned char c;

	switch (rxstate) {
	case RTTY_RX_STATE_IDLE:
		if (!bit) {
			rxstate = RTTY_RX_STATE_START;
			counter = symbollen / 2;
		}
		break;

	case RTTY_RX_STATE_START:
		if (--counter == 0) {
			if (!bit) {
				rxstate = RTTY_RX_STATE_DATA;
				counter = symbollen;
				bitcntr = 0;
				rxdata = 0;
			} else {
				rxstate = RTTY_RX_STATE_IDLE;
			}
		} else
			if (bit) rxstate = RTTY_RX_STATE_IDLE;
		break;

	case RTTY_RX_STATE_DATA:
		if (--counter == 0) {
			rxdata |= bit << bitcntr++;
			counter = symbollen;
		}

		if (bitcntr == nbits) {
			if (rtty_parity == RTTY_PARITY_NONE) {
				rxstate = RTTY_RX_STATE_STOP;
			}
			else {
				rxstate = RTTY_RX_STATE_PARITY;
			}
		}
		break;

	case RTTY_RX_STATE_PARITY:
		if (--counter == 0) {
			rxstate = RTTY_RX_STATE_STOP;
			rxdata |= bit << bitcntr++;
			counter = symbollen;
		}
		break;

	case RTTY_RX_STATE_STOP:
		if (--counter == 0) {
			if (bit) {
				if ((metric >= progStatus.sldrSquelchValue && progStatus.sqlonoff)|| !progStatus.sqlonoff) {
					c = decode_char();
					if ( c != 0 )
						put_rx_char(progdefaults.rx_lowercase ? tolower(c) : c);
				}
				flag = true;
			}
			rxstate = RTTY_RX_STATE_STOP2;
			counter = symbollen / 2;
		}
		break;

	case RTTY_RX_STATE_STOP2:
		if (--counter == 0) {
			rxstate = RTTY_RX_STATE_IDLE;
		}
		break;
	}

	return flag;
}

char snrmsg[80];
void rtty::Metric()
{
	double delta = rtty_baud/2.0;
	double np = 
		wf->powerDensity(frequency - shift * 1.5, delta) +
	 	wf->powerDensity(frequency + shift * 1.5, delta) + 1e-10;
	double sp =
		wf->powerDensity(frequency - shift/2, delta) +
		wf->powerDensity(frequency + shift/2, delta) + 1e-10;
	double snr = 0;

	sigpwr = decayavg( sigpwr, sp, sp - sigpwr > 0 ? 2 : 8);
	noisepwr = decayavg( noisepwr, np, 32 );
	snr = 10*log10(sigpwr / ( noisepwr * (1500 / delta))); // 3000 Hz noise bw

	snprintf(snrmsg, sizeof(snrmsg), "s/n %3.0f dB", snr);
	put_Status2(snrmsg);
	metric = CLAMP(sigpwr/noisepwr, 0.0, 100.0);
	display_metric(metric);
}

void rtty::searchDown()
{
	double srchfreq = frequency - shift -100;
	double minfreq = shift * 2 + 100;
	double spwrlo, spwrhi, npwr;
	while (srchfreq > minfreq) {
		spwrlo = wf->powerDensity(srchfreq - shift/2, 2*rtty_baud);
		spwrhi = wf->powerDensity(srchfreq + shift/2, 2*rtty_baud);
		npwr = wf->powerDensity(srchfreq + shift, 2*rtty_baud) + 1e-10;
		if ((spwrlo / npwr > 10.0) && (spwrhi / npwr > 10.0)) {
			frequency = srchfreq;
			set_freq(frequency);
			sigsearch = SIGSEARCH;
			break;
		}
		srchfreq -= 5.0;
	}
}

void rtty::searchUp()
{
	double srchfreq = frequency + shift +100;
	double maxfreq = IMAGE_WIDTH - shift * 2 - 100;
	double spwrhi, spwrlo, npwr;
	while (srchfreq < maxfreq) {
		spwrlo = wf->powerDensity(srchfreq - shift/2, 2*rtty_baud);
		spwrhi = wf->powerDensity(srchfreq + shift/2, 2*rtty_baud);
		npwr = wf->powerDensity(srchfreq - shift, 2*rtty_baud) + 1e-10;
		if ((spwrlo / npwr > 10.0) && (spwrhi / npwr > 10.0)) {
			frequency = srchfreq;
			set_freq(frequency);
			sigsearch = SIGSEARCH;
			break;
		}
		srchfreq += 5.0;
	}
}

int rtty::rx_process(const double *buf, int len)
{
	complex z, *zp;
	double f = 0.0;
	double fin;
	static bool bit = true;
	int n = 0;
	double deadzone = shift/4;
	double rotate;
	double ferr = 0;

	if (progdefaults.RTTY_BW != rtty_BW) {
		rtty_BW = progdefaults.RTTY_BW;
		bp_filt_lo = (shift/2.0 - rtty_BW/2.0) / samplerate;
		if (bp_filt_lo < 0) bp_filt_lo = 0;
		bp_filt_hi = (shift/2.0 + rtty_BW/2.0) / samplerate;
		bpfilt->create_filter(bp_filt_lo, bp_filt_hi);
		wf->redraw_marker();
	}

	Metric();

	while (len-- > 0) {

// create analytic signal from sound card input samples

		z.re = z.im = *buf++;
		hilbert->run(z, z);

// mix it with the audio carrier frequency to create a baseband signal

		z = mixer(z);

// bandpass filter using Windowed Sinc - Overlap-Add convolution filter

		n = bpfilt->run(z, &zp);

		if (n) {
			for (int i = 0; i < n; i++) {

// measure phase difference between successive samples to determine
// the frequency of the baseband signal (+rtty_baud or -rtty_baud)
// see class complex definiton for operator %

				fin = (prevsmpl % zp[i]).arg() * samplerate / TWOPI;
				prevsmpl = zp[i];


// track the + and - frequency excursions separately to derive an afc signal

                rotate = -4.0 * M_PI * freqerr / rtty_shift;
                double xmix = fabs(4.0 * freqerr / rtty_shift);
                if (xmix > 0.5) xmix = 0.5;
                xmix += 0.05;
				if (fin > 0.0) {
					poscnt++;
					posfreq += fin;
					QI[i].re = zp[i].re;
					QI[i].im = xmix * zp[i].im;
				}
				if (fin < 0.0) {
					negcnt++;
					negfreq += fin;
					QI[i].re = xmix * zp[i].im;
					QI[i].im = zp[i].re;
				}
				QI[i] = QI[i] * complex(cos(rotate), sin(rotate));
                avgsig = decayavg(avgsig, 1.25 * zp[i].mag(), 128);//64);

				if (avgsig > 0)
				    QI[i] = QI[i] / avgsig;

				fin = CLAMP(fin, - rtty_shift, rtty_shift);
// filter the result with a moving average filter
				f = bitfilt->run(fin);
//	hysterisis dead zone in frequency discriminator bit detector
				if (f > deadzone )
					bit = true;
				if (f < -deadzone)
					bit = false;

				if (dspcnt && (--dspcnt % (nbits + 2) == 0)) {
					pipe[pipeptr] = f / shift;
					pipeptr = (pipeptr + 1) % symbollen;
				}

				if ( rx( reverse ? !bit : bit ) ) {
					dspcnt = symbollen * (nbits + 2);

					if (poscnt && negcnt) {
						poserr = posfreq/poscnt;
						negerr = negfreq/negcnt;

// compute the frequency error as the median of + and - relative freq's
						if (sigsearch) sigsearch--;

						ferr = -(poserr + negerr)/(2*(SIGSEARCH - sigsearch + 1));

						int fs = progdefaults.rtty_afcspeed;
						int avging;
						if (fs == 0) avging = 8;
						else if (fs == 1) avging = 4;
						else avging = 1;
						freqerr   = decayavg(freqerr, ferr,  avging);

		    			poscnt = 0; posfreq = 0.0;
			    		negcnt = 0; negfreq = 0.0;

					}
				}
			}
		}
	}
	if (progStatus.afconoff) {
        if (metric > progStatus.sldrSquelchValue || !progStatus.sqlonoff) // || sigsearch) {
            set_freq(frequency - ferr);
    }
    if (metric < progStatus.sldrSquelchValue && progStatus.sqlonoff) {
        if (clear_zdata) {
            for (int i = 0; i < MAX_ZLEN; i++) QI[i].re = QI[i].im = 0.0;
            set_zdata(QI, MAX_ZLEN);
            clear_zdata = false;
            clear_syncscope();
        }
    } else {
        clear_zdata = true;
        update_syncscope();
        set_zdata(QI, n);
    }

	return 0;
}

//=====================================================================
// RTTY transmit
//=====================================================================

double rtty::nco(double freq)
{
	phaseacc += TWOPI * freq / samplerate;

	if (phaseacc > M_PI)
		phaseacc -= TWOPI;

	return cos(phaseacc);
}

double rtty::FSKnco()
{
	FSKphaseacc += TWOPI * 1000 / samplerate;

	if (FSKphaseacc > M_PI)

		FSKphaseacc -= TWOPI;

	return sin(FSKphaseacc);

}

void rtty::send_symbol(int symbol)
{
	double freq;

	if (reverse)
		symbol = !symbol;

	if (symbol)
		freq = get_txfreq_woffset() + shift / 2.0;
	else
		freq = get_txfreq_woffset() - shift / 2.0;

	for (int i = 0; i < symbollen; i++) {
		outbuf[i] = nco(freq);
		if (symbol)
			FSKbuf[i] = FSKnco();
		else
			FSKbuf[i] = 0.0 * FSKnco();
	}

	if (progdefaults.PseudoFSK)
		ModulateStereo(outbuf, FSKbuf, symbollen);
	else
		ModulateXmtr(outbuf, symbollen);
}

void rtty::send_stop()
{
	double freq;
	bool invert = reverse;

	if (invert)
		freq = get_txfreq_woffset() - shift / 2.0;
	else
		freq = get_txfreq_woffset() + shift / 2.0;

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

void rtty::send_char(int c)
{
	int i;

	if (nbits == 5) {
		if (c == LETTERS)
			c = 0x1F;
		if (c == FIGURES)
			c = 0x1B;
	}

	send_symbol(0);
// data bits
	for (i = 0; i < nbits; i++) {
		send_symbol((c >> i) & 1);
	}
// parity bit
	if (rtty_parity != RTTY_PARITY_NONE)
		send_symbol(rttyparity(c));
// stop bit(s)
	send_stop();

	if (nbits == 5) {
		if (c == 0x1F || c == 0x1B)
			return;
		if (txmode == LETTERS)
			c = letters[c];
		else
			c = figures[c];
		if (c)
			put_echo_char(progdefaults.rx_lowercase ? tolower(c) : c);
	}
}

void rtty::send_idle()
{

	if (nbits == 5)
		send_char(LETTERS);
	else
		send_char(0);
}

static int line_char_count = 0;

int rtty::tx_process()
{
	int c;

	if (preamble > 0) {
		preamble--;
		send_symbol(1);
		if (preamble == 0 && nbits == 5) {
			send_char(LETTERS);
			txmode = LETTERS;
		}
		return 0;
	}

	c = get_tx_char();

// TX buffer empty
	if (c == 3 || stopflag) {
		stopflag = false;
		line_char_count = 0;
		if (nbits != 5) {
			send_char('\r');
			send_char('\r');
			send_char('\n');
		} else {
			send_char(0x08);
			send_char(0x08);
			send_char(0x02);
		}
//		KeyLine->clearDTR();
		cwid();
		return -1;
	}

// if NOT Baudot
	if (nbits != 5) {
		send_char(c);
		return 0;
	}

// send idle character if c == -1
	if (c == -1) {
		send_idle();
		txmode = LETTERS;
		return 0;
	}

	if (isalpha(c) || isdigit(c) || isblank(c) || ispunct(c)) {
		++line_char_count;
	}

	if (progdefaults.rtty_autocrlf && (c != '\n' && c != '\r') &&
	    (line_char_count == progdefaults.rtty_autocount ||
	     (line_char_count > progdefaults.rtty_autocount - 5 && c == ' '))) {
		line_char_count = 0;
		if (progdefaults.rtty_crcrlf)
			send_char(0x08); // CR-CR-LF triplet
		send_char(0x08);
		send_char(0x02);
		if (c == ' ')
			return 0;
	}
	if (c == '\r') {
		line_char_count = 0;
		send_char(0x08);
		return 0;
	}
	if (c == '\n') {
		line_char_count = 0;
		if (progdefaults.rtty_crcrlf)
			send_char(0x08); // CR-CR-LF triplet
		send_char(0x02);
		return 0;
	}


/* unshift-on-space */
	if (c == ' ') {
		if (progdefaults.UOStx) {
			send_char(LETTERS);
			send_char(0x04); // coded value for a space
			txmode = LETTERS;
		} else
			send_char(0x04);
		return 0;
	}

	if ((c = baudot_enc(c)) < 0)
		return 0;

// switch case if necessary

	if ((c & 0x300) != txmode) {// == 0) {
		if (txmode == FIGURES) {
			send_char(LETTERS);
			txmode = LETTERS;
		} else {
			send_char(FIGURES);
			txmode = FIGURES;
		}
	}

	send_char(c & 0x1F);

	return 0;
}

int rtty::baudot_enc(unsigned char data)
{
	int i, c, mode;

	mode = 0;
	c = -1;

	if (islower(data))
		data = toupper(data);

	for (i = 0; i < 32; i++) {
		if (data == letters[i]) {
			mode |= LETTERS;
			c = i;
		}
		if (data == figures[i]) {
			mode |= FIGURES;
			c = i;
		}
		if (c != -1)
			return (mode | c);
	}

	return -1;
}

char rtty::baudot_dec(unsigned char data)
{
	int out = 0;

	switch (data) {
	case 0x1F:		/* letters */
		rxmode = LETTERS;
		break;
	case 0x1B:		/* figures */
		rxmode = FIGURES;
		break;
	case 0x04:		/* unshift-on-space */
		if (progdefaults.UOSrx)
			rxmode = LETTERS;
		return ' ';
		break;
	default:
		if (rxmode == LETTERS)
			out = letters[data];
		else
			out = figures[data];
		break;
	}

	return out;
}

