// ----------------------------------------------------------------------------
// rtty.cxx  --  RTTY modem
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
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
// ----------------------------------------------------------------------------

#include <config.h>

#include "rtty.h"
#include "waterfall.h"
#include "confdialog.h"
#include "configuration.h"

//static char rttymsg[80];
static char msg1[20];
static char msg2[20];

double _SHIFT[] = {23, 85, 160, 170, 182, 200, 240, 350, 425, 850};
double _BAUD[] = {45, 45.45, 50, 56, 75, 100, 110, 150, 200, 300};
int    _BITS[] = {5, 7, 8};

void rtty::tx_init(cSound *sc)
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
	modem::init();
	rx_init();
	put_MODEstatus(mode);
	snprintf(msg1, sizeof(msg1), "Shft %-4.0f", rtty_shift); 
	put_Status1(msg1);
	snprintf(msg2, sizeof(msg2), "Baud %-4.1f", rtty_baud); 
	put_Status2(msg2);
	if (progdefaults.PreferXhairScope)
		digiscope->mode(Digiscope::XHAIRS);
	else
		digiscope->mode(Digiscope::RTTY);
}

rtty::~rtty()
{
	if (hilbert) delete hilbert;
//	if (wfid) delete wfid;
	if (bitfilt) delete bitfilt;
}

void rtty::restart()
{
	double fhi;
	double flo;
	double stl;
	
	rtty_shift = shift = _SHIFT[progdefaults.rtty_shift];
	rtty_baud = _BAUD[progdefaults.rtty_baud];
	nbits = rtty_bits = _BITS[progdefaults.rtty_bits];
	if (rtty_bits == 5)
		rtty_parity = PARITY_NONE;
	else
		switch (progdefaults.rtty_parity) {
			case 0 : rtty_parity = PARITY_NONE; break;
			case 1 : rtty_parity = PARITY_EVEN; break;
			case 2 : rtty_parity = PARITY_ODD; break;
			case 3 : rtty_parity = PARITY_ZERO; break;
			case 4 : rtty_parity = PARITY_ONE; break;
			default : rtty_parity = PARITY_NONE; break;
		}
	rtty_stop = progdefaults.rtty_stop;

	txmode = LETTERS;
	rxmode = LETTERS;
	symbollen = (int) (samplerate / rtty_baud + 0.5);
	set_bandwidth(shift);

	fhi = (shift / 2 + rtty_baud * 1.1) / samplerate;
	flo = (shift / 2 - rtty_baud * 1.1) / samplerate;

	if (bpfilt) 
		bpfilt->create_filter(flo, fhi);
	else
		bpfilt = new fftfilt(flo, fhi, 1024);

	if (bitfilt)
		bitfilt->setLength(symbollen / 4);//6);
	else
		bitfilt = new Cmovavg(symbollen / 4);//6);

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

	snprintf(msg1, sizeof(msg1), "Shft %-4.0f", rtty_shift); 
	put_Status1(msg1);
	snprintf(msg2, sizeof(msg2), "Baud %-4.1f", rtty_baud); 
	put_Status2(msg2);
	put_MODEstatus(mode);
}

rtty::rtty(trx_mode tty_mode)
{
	mode = tty_mode;

	samplerate = RTTY_SampleRate;
	fragmentsize = 256;
	
	bpfilt = (fftfilt *)0;
	bitfilt = (Cmovavg *)0;
	
	hilbert = new C_FIR_filter();
	hilbert->init_hilbert(37, 1);

	restart();
}

void rtty::update_syncscope()
{
	double data[RTTYMaxSymLen];
	int i;

	for (i = 0; i < symbollen; i++)
		data[i] = pipe[i];
	set_scope(data, symbollen, false);
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

	phaseacc -= twopi * frequency / samplerate;
	if (phaseacc > M_PI) 
		phaseacc -= twopi;
	else if (phaseacc < M_PI) 
		phaseacc += twopi;

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
	case PARITY_NONE:
		return 0;

	case PARITY_ODD:
		return rparity(c);

	case PARITY_EVEN:
		return !rparity(c);

	case PARITY_ZERO:
		return 0;

	case PARITY_ONE:
		return 1;
	}
}

int rtty::decode_char()
{
	unsigned int parbit, par, data;

	parbit = (rxdata >> nbits) & 1;
	par = rttyparity(rxdata);

	if (rtty_parity != PARITY_NONE && parbit != par)
		return 0;

	data = rxdata & ((1 << nbits) - 1);

	if (nbits == 5)
		return baudot_dec(data);

	return data;
}

int rtty::rx(bool bit)
{
	int flag = 0;
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
				flag = 1;
			} else {
				rxstate = RTTY_RX_STATE_IDLE;
			}
		}
		break;

	case RTTY_RX_STATE_DATA:
		if (--counter == 0) {
			rxdata |= bit << bitcntr++;
			counter = symbollen;
			flag = 1;
		}

		if (bitcntr == nbits) {
			if (rtty_parity == PARITY_NONE) {
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
			flag = 1;
		}
		break;

	case RTTY_RX_STATE_STOP:
		if (--counter == 0) {
			if (bit) {
				if ((metric >= squelch && squelchon)|| !squelchon) {				
					c = decode_char();
					if ( c != 0 )
						put_rx_char(c);
				}
				flag = 2;
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

void rtty::Metric()
{
	double delta = rtty_baud/4.0;
	double noisepwr = wf->powerDensity(frequency - shift * 1.5, delta) +
					  wf->powerDensity(frequency + shift * 1.5, delta) + 1e-10;
	double sigpwr = wf->powerDensity(frequency - shift/2, delta) +
					wf->powerDensity(frequency + shift/2, delta) + 1e-10;
	metric = decayavg( metric, 40.0*log10(sigpwr / noisepwr), 8);
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
			sigsearch = 10;
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
			sigsearch = 10;
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
	static bool bit = false;
	int n, rxflag;
	double deadzone = shift/8;
	static int dspcnt = symbollen * nbits;

	while (len-- > 0) {
		
// create analytic signal from sound card input samples
		
		z.re = z.im = *buf++;
		hilbert->run(z, z);
		
// mix it with the audio carrier frequency to create a baseband signal

		z = mixer(z);

// low pass filter using Windowed Sinc - Overlap-Add convolution filter

		n = bpfilt->run(z, &zp);
		for (int i = 0; i < n; i++) {
			
// measure phase difference between successive samples to determine
// the frequency of the baseband signal (+rtty_baud or -rtty_baud)
// see class complex definiton for operator %
			
			fin = (prevsmpl % zp[i]).arg() * samplerate / twopi;
			prevsmpl = zp[i];
			
// filter the result with a moving average filter
			
			f = bitfilt->run(fin);
			
// track the + and - frequency excursions separately to derive an afc signal
// track the sum and sum^2 for derivation of DCD
			
			if (fin >= 0.0) {
				poscnt++;
				posfreq += fin;
			} else {
				negcnt++;
				negfreq += fin;
			}

//	hysterisis dead zone in frequency discriminator bit detector

			if (f > deadzone )  {
				bit = false;
			}
			if (f < -deadzone) {
				bit = true;
			}
			
			if (--dspcnt % (nbits + 2) == 0) {
				pipe[pipeptr] = f / shift; // display filtered signal		
				pipeptr = (pipeptr + 1) % symbollen;
			}

			if (!progdefaults.RTTY_USB)
				bit = !bit;
			
			rxflag = rx (reverse ? bit : !bit);

			if (rxflag == 2 || dspcnt == 0) {
				if ((metric > squelch && squelchon) || !squelchon) {
					set_scope(pipe, symbollen, false);
					pipe.next(); // change buffers
				}
				else
					clear_syncscope();
								
				dspcnt = symbollen * (nbits + 2);
				pipeptr = 0;

				if (poscnt && negcnt) {
					poserr = posfreq/poscnt;
					negerr = negfreq/negcnt;

					Metric();
				
// compute the frequency error as the median of + and - relative freq's
					int fs = progdefaults.rtty_afcspeed;
					if (sigsearch) {
						freqerr = decayavg(freqerr, poserr + negerr, 4);
						sigsearch--;
					} else
						freqerr = decayavg(freqerr, poserr + negerr, 
										   fs == 0 ? 50 : fs = 1 ? 100 : 200 );
					
// display the FSK +/- signals on the digiscope					
					set_rtty( 
						negerr/(rtty_shift/2.0), 
						poserr/(rtty_shift/2.0), 
						1.0 );
				}
				poscnt = 0; posfreq = 0.0;
				negcnt = 0; negfreq = 0.0;

				if (afcon) {
					if (metric > squelch || !squelchon || sigsearch) {
						set_freq(frequency + freqerr);
					}
				}
			}
		}
	}
	return 0;
}

//=====================================================================
// RTTY transmit
//=====================================================================

double rtty::nco(double freq)
{
	phaseacc += twopi * freq / samplerate;

	if (phaseacc > M_PI)
		phaseacc -= twopi;

	return cos(phaseacc);
}

double rtty::FSKnco()
{
	FSKphaseacc += twopi * 1000 / samplerate;

	if (FSKphaseacc > M_PI)
		FSKphaseacc -= twopi;

	return sin(FSKphaseacc);

}

void rtty::send_symbol(int symbol)
{
	double freq;
	
	if (reverse)
		symbol = !symbol;
	if (!progdefaults.RTTY_USB)
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
			FSKbuf[i] = 0.0;
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
	if (!progdefaults.RTTY_USB)
		invert = !invert;
	
	if (invert)
		freq = get_txfreq_woffset() - shift / 2.0;
	else
		freq = get_txfreq_woffset() + shift / 2.0;

	for (int i = 0; i < stoplen; i++) {
		outbuf[i] = nco(freq);
		if (invert)
			FSKbuf[i] = FSKnco();
		else
			FSKbuf[i] = 0.0;
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
	if (rtty_parity != PARITY_NONE)
		send_symbol(rttyparity(c));
// stop bit(s)
	send_stop();

	if (nbits == 5)
		c = baudot_dec(c);
	put_echo_char(c);
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

	if ( (c != '\n' && c != '\r'))
		if (progdefaults.rtty_autocrlf == true) 
			if ( line_char_count == progdefaults.rtty_autocount ||
			     ((line_char_count > progdefaults.rtty_autocount - 5) && c == ' ') ) {
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
		send_char(LETTERS);
		send_char(0x04); // coded value for a space
		txmode = LETTERS;
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

