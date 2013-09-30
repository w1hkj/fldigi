// ----------------------------------------------------------------------------
// throb.cxx  --  throb modem
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
// ThrobX additions by Joe Veldhuis, KD8ATU
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code
// distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
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

#include <stdlib.h>
#include <iostream>

#include "throb.h"
#include "ascii.h"
#include "configuration.h"
#include "fl_digi.h"
#include "status.h"

#define MAX_TONES	15

#undef  CLAMP
#define CLAMP(x,low,high)       (((x)>(high))?(high):(((x)<(low))?(low):(x)))

char throbmsg[80];

void  throb::tx_init(SoundBase *sc)
{
	scard = sc;
	preamble = 4;
	reset_syms();
	videoText();
}

void  throb::rx_init()
{
	rxcntr = rxsymlen;
	waitsync = 1;
	deccntr = 0;
	symptr = 0;
	shift = 0;
	lastchar = '\0';
	reset_syms();
	put_MODEstatus(mode);
}

void throb::init()
{
	modem::init();
	rx_init();
	set_scope_mode(Digiscope::SCOPE);
}

throb::~throb()
{
	if (hilbert) delete hilbert;
	if (syncfilt) delete syncfilt;
	if (fftfilter) delete fftfilter;
	if (snfilter) delete snfilter;
	if (scope_data) delete [] scope_data;
	if (txpulse) delete [] txpulse;
	if (outbuf) delete[] outbuf;
	for (int i = 0; i < num_tones; i++)
		if (rxtone[i]) delete [] rxtone[i];
}

void throb::flip_syms() //call this whenever a space or idle is sent or received
{
	switch(mode) {
		case MODE_THROBX1:
		case MODE_THROBX2:
		case MODE_THROBX4:
			if (idlesym == 0) {
				idlesym = 1;
				spacesym = 0;
			} else {
				idlesym = 0;
				spacesym = 1;
			}
			break;
		default:
			//if we're not running a ThrobX mode, do nothing
			break;
	}
}

void throb::reset_syms() //call when switching from TX to RX or vice versa
{
        switch(mode) {
        case MODE_THROBX1:
        case MODE_THROBX2:
        case MODE_THROBX4:
        	idlesym = 0;
		spacesym = 1;
		break;
	default: //paranoia
		idlesym = 0;
		spacesym = 44;
		break;
	}
}

throb::throb(trx_mode throb_mode) : modem()
{
	cap |= CAP_AFC | CAP_REV;

	double bw;
	double *fp = 0;

	mode = throb_mode;

	switch (mode) {
	case MODE_THROB1:
		symlen = SYMLEN_1;
		txpulse = mk_semi_pulse(symlen);
		fp = mk_semi_pulse(symlen / DOWN_SAMPLE);
		num_tones = 9;
		num_chars = 45;
		idlesym = 0;
		spacesym = 44;
		for (int i = 0; i < num_tones; i++)
			freqs[i] = ThrobToneFreqsNar[i];
		bw = 36.0 / THROB_SAMPLE_RATE;
		break;

	case MODE_THROB2:
		symlen = SYMLEN_2;
		txpulse = mk_semi_pulse(symlen);
		fp = mk_semi_pulse(symlen / DOWN_SAMPLE);
		num_tones = 9;
		num_chars = 45;
		idlesym = 0;
		spacesym = 44;
		for (int i = 0; i < num_tones; i++)
			freqs[i] = ThrobToneFreqsNar[i];
		bw = 36.0 / THROB_SAMPLE_RATE;
		break;

	case MODE_THROB4:
	default:
		symlen = SYMLEN_4;
		txpulse = mk_full_pulse(symlen);
		fp = mk_full_pulse(symlen / DOWN_SAMPLE);
		num_tones = 9;
		num_chars = 45;
		idlesym = 0;
		spacesym = 44;
		for (int i = 0; i < num_tones; i++)
			freqs[i] = ThrobToneFreqsWid[i];
		bw = 72.0 / THROB_SAMPLE_RATE;
		break;

        case MODE_THROBX1:
                symlen = SYMLEN_1;
                txpulse = mk_semi_pulse(symlen);
                fp = mk_semi_pulse(symlen / DOWN_SAMPLE);
                num_tones = 11;
                num_chars = 55;
				idlesym = 0;
				spacesym = 1;
                for (int i = 0; i < num_tones; i++)
                        freqs[i] = ThrobXToneFreqsNar[i];
                bw = 47.0 / THROB_SAMPLE_RATE;
		break;

        case MODE_THROBX2:
                symlen = SYMLEN_2;
                txpulse = mk_semi_pulse(symlen);
                fp = mk_semi_pulse(symlen / DOWN_SAMPLE);
                num_tones = 11;
                num_chars = 55;
				idlesym = 0;
				spacesym = 1;
                for (int i = 0; i < num_tones; i++)
					freqs[i] = ThrobXToneFreqsNar[i];
                bw = 47.0 / THROB_SAMPLE_RATE;
                break;

        case MODE_THROBX4: //NONSTANDARD
                symlen = SYMLEN_4;
                txpulse = mk_full_pulse(symlen);
                fp = mk_full_pulse(symlen / DOWN_SAMPLE);
                num_tones = 11;
                num_chars = 55;
				idlesym = 0;
				spacesym = 1;
                for (int i = 0; i < num_tones; i++)
                        freqs[i] = ThrobXToneFreqsWid[i];
                bw = 94.0 / THROB_SAMPLE_RATE;
                break;
	}

	outbuf = new double[symlen];

	rxsymlen = symlen / DOWN_SAMPLE;

	hilbert		= new C_FIR_filter();
	hilbert->init_hilbert(37, 1);

	fftfilter = new fftfilt(0, bw, FilterFFTLen);

	syncfilt = new C_FIR_filter();
	syncfilt->init(symlen / DOWN_SAMPLE, 1, fp, NULL);
	delete [] fp;

	snfilter = new Cmovavg(16);

	for (int i = 0; i < num_tones; i++)
		rxtone[i] = mk_rxtone(freqs[i], txpulse, symlen);

	reverse = 0;
	samplerate = THROB_SAMPLE_RATE;
	fragmentsize = symlen;
	bandwidth = freqs[num_tones - 1] - freqs[0];
	syncpos = 0.5;

	scope_data	= new double [SCOPE_DATA_LEN];

	phaseacc = 0.0;
	metric = 0.0;
	symptr = 0;

	for (int i = 0; i < MAX_RX_SYMLEN; i++)
		syncbuf[i] = dispbuf[i] = 0.0;

//	init();
}

//=====================================================================
// receive processing
//=====================================================================

// Make a 32 times down sampled cmplx prototype tone for rx.

cmplx *throb::mk_rxtone(double freq, double *pulse, int len)
{
	cmplx *tone;
	double x;
	int i;

	tone = new cmplx [len / DOWN_SAMPLE];

	for (i = 0; i < len; i += DOWN_SAMPLE) {
		x = -2.0 * M_PI * freq * i / THROB_SAMPLE_RATE;
		tone[i / DOWN_SAMPLE] =
			cmplx ( pulse[i] * cos(x), pulse[i] * sin(x) );
	}

	return tone;
}

cmplx throb::mixer(cmplx in)
{
	double f;
	cmplx z (cos(phaseacc), sin(phaseacc));

	z = z * in;

	f = frequency;

	phaseacc -= 2.0 * M_PI * f / THROB_SAMPLE_RATE;

	if (phaseacc < -M_PI)
		phaseacc += 2.0 * M_PI;
	if (phaseacc >  M_PI)
		phaseacc -= 2.0 * M_PI;

	return z;
}

int throb::findtones(cmplx *word, int &tone1, int &tone2)
{
	double max1, max2;
	int maxtone, i;

	max1 = 0;
	tone1 = 0;
	for (i = 0; i < num_tones; i++) {
		if ( abs(word[i]) > max1 ) {
			max1 = abs(word[i]);
			tone1 = i;
		}
	}

	maxtone = tone1;

	max2 = 0;
	tone2 = 0;
	for (i = 0; i < num_tones; i++) {
		if (i == tone1)
			continue;
		if ( abs(word[i]) > max2) {
			max2 = abs(word[i]);
			tone2 = i;
		}
	}

//handle single-tone symbols (Throb only)
	if (mode == MODE_THROB1 ||
		mode == MODE_THROB2 ||
		mode == MODE_THROB4)
		if (max1 > max2 * 2)
			tone2 = tone1;

	if (tone1 > tone2) {
		i = tone1;
		tone1 = tone2;
		tone2 = i;
	}

	signal = noise = 0.0;
	for (i = 0; i < num_tones; i++) {
		if ( i == tone1 || i == tone2)
			signal += abs(word[i]) / 2.0;
		else
			noise += abs(word[i]) / (num_tones - 2.0);
	}

	metric = snfilter->run( signal / (noise + 1e-6));

	s2n = CLAMP( 10.0*log10( metric ) - 3.0, 0.0, 100.0);

	return maxtone;
}

void throb::show_char(int c) {
	if (metric > progStatus.sldrSquelchValue || progStatus.sqlonoff == false)
		put_rx_char(progdefaults.rx_lowercase ? tolower(c) : c);
}

void throb::decodechar(int tone1, int tone2)
{
	int i;

	switch(mode) {
	case MODE_THROB1:
	case MODE_THROB2:
	case MODE_THROB4:
		if (shift == true) {
			if (tone1 == 0 && tone2 == 8)
				show_char('?');

			if (tone1 == 1 && tone2 == 7)
				show_char('@');

			if (tone1 == 2 && tone2 == 6)
				show_char('=');

			if (tone1 == 4 && tone2 == 4)
				show_char('\n');

			shift = false;
			return;
		}

		if (tone1 == 3 && tone2 == 5) {
			shift = true;
			return;
		}

		for (i = 0; i < num_chars; i++) {
			if (ThrobTonePairs[i][0] == tone1 + 1 &&
				ThrobTonePairs[i][1] == tone2 + 1) {
				show_char(ThrobCharSet[i]);
				break;
			}
		}
		break;
//ThrobX mode. No shifted case, but idle and space symbols alternate
	default:
		for (i = 0; i < num_chars; i++) {
			if (ThrobXTonePairs[i][0] == tone1 + 1 && ThrobXTonePairs[i][1] == tone2 + 1) {
				if (i == spacesym || i == idlesym) {
					if (lastchar != '\0' && lastchar != ' ') {
						show_char(ThrobXCharSet[1]);
						lastchar = ' ';
					}
					else {
						lastchar = '\0';
					}
					flip_syms();
				} else {
					show_char(ThrobXCharSet[i]);
					lastchar = ThrobXCharSet[i];
				}
			}
		}
	break;
	}
	return;
}

void throb::rx(cmplx in)
{
	cmplx rxword[MAX_TONES];
	int i, tone1, tone2, maxtone;

	symbol[symptr] = in;

	if (rxcntr > 0.0)
		return;

// correlate against all tones
	for (i = 0; i < num_tones; i++)
		rxword[i] = cmac(rxtone[i], symbol, symptr + 1, rxsymlen);

// find the strongest tones
	maxtone = findtones(rxword, tone1, tone2);

// decode
	if (reverse)
		decodechar (num_tones - 1 - tone2, num_tones - 1 - tone1);
	else
		decodechar (tone1, tone2);

	if (progStatus.afconoff == true && (metric >= progStatus.sldrSquelchValue || progStatus.sqlonoff == false)) {
		cmplx z1, z2;
		double f;

		z1 = rxword[maxtone];
		z2 = cmac(rxtone[maxtone], symbol, symptr + 2, rxsymlen);

		f = arg( conj(z1) * z2 ) / (2 * DOWN_SAMPLE * M_PI / THROB_SAMPLE_RATE);

		f -= freqs[maxtone];

		set_freq(frequency + f / (num_tones - 1));
	}

	/* done with this symbol, start over */
	rxcntr = rxsymlen;
	waitsync = 1;

	snprintf(throbmsg, sizeof(throbmsg), "S/N: %3d dB", (int)(floor(s2n)));
	put_Status1(throbmsg);
	display_metric(metric);

}

void throb::sync(cmplx in)
{
	double f, maxval = 0;
	double mag;
	int i, maxpos = 0;

	/* "rectify", filter and store input */
	mag = abs(in);
	syncfilt->Irun( mag, f);
	syncbuf[symptr] = f;

	/* check counter if we are waiting for sync */
	if (waitsync == 0 || rxcntr > (rxsymlen / 2.0))
		return;

	for (i = 0; i < rxsymlen; i++) {
		f = syncbuf[(i + symptr + 1) % rxsymlen];
		dispbuf[i] = f;
	}

	for (i = 0; i < rxsymlen; i++) {
		if (dispbuf[i] > maxval) {
			maxpos = i;
			maxval = dispbuf[i];
		}
	}

	/* correct sync */
	rxcntr += (maxpos - rxsymlen / 2) / (num_tones - 1);
	waitsync = 0;
	if (metric >= progStatus.sldrSquelchValue || progStatus.sqlonoff == false)
		set_scope(dispbuf, rxsymlen);
	else {
		dispbuf[0] = 0.0;
		set_scope(dispbuf, 1);
	}
	dispbuf.next(); // change buffers
}

int throb::rx_process(const double *buf, int len)
{
	cmplx z, *zp;
	int i, n;

	while (len-- > 0) {
		z = cmplx( *buf, *buf );
		buf++;

		hilbert->run(z, z);
		z = mixer(z);
		n = fftfilter->run(z, &zp);

		/* DOWN_SAMPLE by 32 and push to the receiver */
		for (i = 0; i < n; i++) {
			if (++deccntr >= DOWN_SAMPLE) {
				rxcntr -= 1.0;

				/* do symbol sync */
				sync(zp[i]);

				/* decode */
				rx(zp[i]);

				symptr = (symptr + 1) % rxsymlen;
				deccntr = 0;
			}
		}
	}

	return 0;
}

//=====================================================================
// transmit processing
//=====================================================================

double *throb::mk_semi_pulse(int len)
{
	double *pulse, x;
	int i, j;

	pulse = new double [len];

	for (i = 0; i < len; i++) {
		if (i < len / 5) {
			x = M_PI * i / (len / 5.0);
			pulse[i] = 0.5 * (1 - cos(x));
		}

		if (i >= len / 5 && i < len * 4 / 5)
			pulse[i] = 1.0;

		if (i >= len * 4 / 5) {
			j = i - len * 4 / 5;
			x = M_PI * j / (len / 5.0);
			pulse[i] = 0.5 * (1 + cos(x));
		}
	}

	return pulse;
}

double *throb::mk_full_pulse(int len)
{
	double *pulse;
	int i;

	pulse = new double [len];

	for (i = 0; i < len; i++)
		pulse[i] = 0.5 * (1 - cos(2 * M_PI * i / len));

	return pulse;
}


void throb::send(int symbol)
{
	int tone1, tone2;
	double w1, w2;
	int i;

	if (symbol < 0 || symbol >= num_chars)
		return;

	switch(mode) {
	case MODE_THROB1:
	case MODE_THROB2:
	case MODE_THROB4:
		tone1 = ThrobTonePairs[symbol][0] - 1;
		tone2 = ThrobTonePairs[symbol][1] - 1;
		break;
	default:
		tone1 = ThrobXTonePairs[symbol][0] -1;
		tone2 = ThrobXTonePairs[symbol][1] -1;
		break;
	}

	if (reverse) {
		tone1 = (num_tones - 1) - tone1;
		tone2 = (num_tones - 1) - tone2;
	}

	w1 = 2.0 * M_PI * (get_txfreq_woffset() + freqs[tone1]) / THROB_SAMPLE_RATE;
	w2 = 2.0 * M_PI * (get_txfreq_woffset() + freqs[tone2]) / THROB_SAMPLE_RATE;

	for (i = 0; i < symlen; i++)
		outbuf[i] = txpulse[i] *
				 (sin(w1 * i) + sin(w2 * i)) / 2.0;

	ModulateXmtr(outbuf, symlen);
}

int throb::tx_process()
{
	int i, c, sym;

	if (preamble > 0) {
		send(idlesym);	/* send idle throbs */
		flip_syms();
		preamble--;
		return 0;
	}

	c = get_tx_char();

// end of transmission
	if (c == GET_TX_CHAR_ETX || stopflag) {
		send(idlesym);
//		reset_syms(); //prepare RX. idle/space syms always start as 0 and 1, respectively.
		cwid();
		return -1;
	}

// TX buffer empty
	if (c == GET_TX_CHAR_NODATA) {
		send(idlesym);	/* send idle throbs */
		flip_syms();
		return 0;
	}

	switch(mode) {
	case MODE_THROB1:
	case MODE_THROB2:
	case MODE_THROB4:
	/* handle the special cases first, if we're doing regular Throb */
	switch (c) {
	case '?':
		send(5);	/* shift */
		send(20);
		put_echo_char(c);
		return 0;

	case '@':
		send(5);	/* shift */
		send(13);
		put_echo_char(c);
		return 0;

	case '-':
		send(5);	/* shift */
		send(9);
		put_echo_char(c);
		return 0;

	case '\r':
		return 0;
	case '\n':
		send(5);	/* shift */
		send(0);
		put_echo_char(c);
		return 0;

	default:
		break;
	}
	break;
	default:
	//If we're doing ThrobX, no need to handle shifts
	break;
	}

	/* map lower case character to upper case */
	if (islower(c))
		c = toupper(c);

	/* see if the character can be found in our character set */
	switch(mode) {
	case MODE_THROB1:
	case MODE_THROB2:
	case MODE_THROB4:
		for (sym = -1, i = 0; i < num_chars; i++)
			if (c == ThrobCharSet[i])
				sym = i;
		break;
	default:
		for (sym = -1, i = 0; i < num_chars; i++)
                        if (c == ThrobXCharSet[i])
                                sym = i;
                break;
	}

// send a space for unknown chars
	if (sym == -1) c = ' ';
// handle spaces for throbx
	if (c == ' ') {
		sym = spacesym;
		flip_syms();
	}

	send(sym);
	put_echo_char(progdefaults.rx_lowercase ? tolower(c) : c);

	return 0;
}


//=====================================================================
// throb static declarations
//=====================================================================


int throb::ThrobTonePairs[45][2] = {
	{5, 5},			/* idle... no print */
	{4, 5},			/* A */
	{1, 2},			/* B */
	{1, 3},			/* C */
	{1, 4},			/* D */
	{4, 6},			/* SHIFT (was E) */
	{1, 5},			/* F */
	{1, 6},			/* G */
	{1, 7},			/* H */
	{3, 7},			/* I */
	{1, 8},			/* J */
	{2, 3},			/* K */
	{2, 4},			/* L */
	{2, 8},			/* M */
	{2, 5},			/* N */
	{5, 6},			/* O */
	{2, 6},			/* P */
	{2, 9},			/* Q */
	{3, 4},			/* R */
	{3, 5},			/* S */
	{1, 9},			/* T */
	{3, 6},			/* U */
	{8, 9},			/* V */
	{3, 8},			/* W */
	{3, 3},			/* X */
	{2, 2},			/* Y */
	{1, 1},			/* Z */
	{3, 9},			/* 1 */
	{4, 7},			/* 2 */
	{4, 8},			/* 3 */
	{4, 9},			/* 4 */
	{5, 7},			/* 5 */
	{5, 8},			/* 6 */
	{5, 9},			/* 7 */
	{6, 7},			/* 8 */
	{6, 8},			/* 9 */
	{6, 9},			/* 0 */
	{7, 8},			/* , */
	{7, 9},			/* . */
	{8, 8},			/* ' */
	{7, 7},			/* / */
	{6, 6},			/* ) */
	{4, 4},			/* ( */
	{9, 9},			/* E */
	{2, 7}			/* space */
};

int throb::ThrobXTonePairs[55][2] = {
        {6, 11},                /* idle (initially) */
        {1, 6},                 /* space (initially) */
        {2, 6},                 /* A */
        {2, 5},                 /* B */
        {2, 7},                 /* C */
        {2, 8},                 /* D */
        {5, 6},                 /* E */
        {2, 9},                 /* F */
        {2, 10},                /* G */
        {4, 8},                 /* H */
        {4, 6},                 /* I */
        {2, 11},                /* J */
        {3, 4},                 /* K */
        {3, 5},                 /* L */
        {3, 6},                 /* M */
        {6, 9},                 /* N */
        {6, 10},                /* O */
        {3, 7},                 /* P */
        {3, 8},                 /* Q */
        {3, 9},                 /* R */
        {6, 8},                 /* S */
        {6, 7},                 /* T */
        {3, 10},                /* U */
        {3, 11},                /* V */
        {4, 5},                 /* W */
        {4, 7},                 /* X */
        {4, 9},                 /* Y */
        {4, 10},                /* Z */
        {1, 2},                 /* 1 */
        {1, 3},                 /* 2 */
        {1, 4},                 /* 3 */
        {1, 5},                 /* 4 */
        {1, 7},                 /* 5 */
        {1, 8},                 /* 6 */
        {1, 9},                 /* 7 */
        {1, 10},                /* 8 */
        {2, 3},                 /* 9 */
        {2, 4},                 /* 0 */
        {4, 11},                /* , */
        {5, 7},                 /* . */
        {5, 8},                 /* ' */
        {5, 9},                 /* / */
        {5, 10},                /* ) */
        {5, 11},                /* ( */
        {7, 8},                 /* # */
        {7, 9},                 /* " */
        {7, 10},                /* + */
        {7, 11},                /* - */
        {8, 9},                 /* ; */
        {8, 10},                /* : */
        {8, 11},                /* ? */
        {9, 10},                /* ! */
        {9, 11},                /* @ */
        {10, 11},               /* = */
	{1, 11}			/* cr */ //FIXME: !!COMPLETELY NONSTANDARD!!
};

unsigned char throb::ThrobCharSet[45] = {
	'\0',			/* idle */
	'A',
	'B',
	'C',
	'D',
	'\0',			/* shift */
	'F',
	'G',
	'H',
	'I',
	'J',
	'K',
	'L',
	'M',
	'N',
	'O',
	'P',
	'Q',
	'R',
	'S',
	'T',
	'U',
	'V',
	'W',
	'X',
	'Y',
	'Z',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	'0',
	',',
	'.',
	'\'',
	'/',
	')',
	'(',
	'E',
	' '
};

unsigned char throb::ThrobXCharSet[55] = {
        '\0',                   /* idle (initially) */
        ' ',                    /* space (initially) */
        'A',
        'B',
        'C',
        'D',
        'E',
        'F',
        'G',
        'H',
        'I',
        'J',
        'K',
        'L',
        'M',
        'N',
        'O',
        'P',
        'Q',
        'R',
        'S',
        'T',
        'U',
        'V',
        'W',
        'X',
        'Y',
        'Z',
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',
        '0',
        ',',
        '.',
        '\'',
        '/',
        ')',
        '(',
        '#',
        '"',
        '+',
        '-',
        ';',
        ':',
        '?',
        '!',
        '@',
        '=',
	'\n'
};

double throb::ThrobToneFreqsNar[9] = {-32, -24, -16,  -8,  0,  8, 16, 24, 32};
double throb::ThrobToneFreqsWid[9] = {-64, -48, -32, -16,  0, 16, 32, 48, 64};
double throb::ThrobXToneFreqsNar[11] = {-39.0625, -31.25, -23.4375, -15.625, -7.8125, 0, 7.8125, 15.625, 23.4375, 31.25, 39.0625};
double throb::ThrobXToneFreqsWid[11] = {-78.125, -62.5, -46.875, -31.25, -15.625, 0, 15.625, 31.25, 46.875, 62.5, 78.125};
