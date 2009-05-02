// ----------------------------------------------------------------------------
// mfsk.cxx  --  mfsk modem
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

#include <cstdlib>
#include <cstring>
#include <libgen.h>

#include <FL/Fl.H>

#include "mfsk.h"
#include "modem.h"

#include "misc.h"
#include "main.h"
#include "fl_digi.h"
#include "configuration.h"
#include "status.h"
#include "trx.h"

#include "ascii.h"

#include "fileselect.h"

#include "qrunner.h"

using namespace std;

//=============================================================================
char mfskmsg[80];
//=============================================================================

#include "mfsk-pic.cxx"

void  mfsk::tx_init(SoundBase *sc)
{
	scard = sc;
	txstate = TX_STATE_PREAMBLE;
	bitstate = 0;
	
	videoText();
}

void  mfsk::rx_init()
{
	rxstate = RX_STATE_DATA;
	synccounter = 0;
	symcounter = 0;
	met1 = 0.0;
	met2 = 0.0;
	counter = 0;
	for (int i = 0; i < 2 * symlen; i++) {
		for (int j = 0; j < 32; j++)
			(pipe[i].vector[j]).re = (pipe[i].vector[j]).im = 0.0;
	}
	reset_afc();
	s2n = 0.0;
	memset(picheader, ' ', PICHEADER - 1);
	picheader[PICHEADER -1] = 0;
	put_MODEstatus(mode);
	syncfilter->reset();
	staticburst = false;
}

void mfsk::init()
{
	modem::init();
	rx_init();
	set_scope_mode(Digiscope::SCOPE);
// picture mode init
	setpicture_link(this);
	TXspp = txSPP;
	RXspp = 8;
}

void mfsk::shutdown()
{
}

mfsk::~mfsk()
{
	stopflag = true;
	if (picTxWin)
		picTxWin->hide();
	if (picRxWin)
		picRxWin->hide();
	activate_mfsk_image_item(false);

	if (bpfilt) delete bpfilt;
	if (rxinlv) delete rxinlv;
	if (txinlv) delete txinlv;
	if (dec2) delete dec2;
	if (dec1) delete dec1;
	if (enc) delete enc;
	if (pipe) delete [] pipe;
	if (hbfilt) delete hbfilt;
	if (binsfft) delete binsfft;
	for (int i = 0; i < SCOPESIZE; i++) {
		if (vidfilter[i]) delete vidfilter[i];
	}
	if (syncfilter) delete syncfilter;
}

mfsk::mfsk(trx_mode mfsk_mode) : modem()
{
	cap = CAP_AFC | CAP_REV;

	double bw, cf, flo, fhi;
	mode = mfsk_mode;

	switch (mode) {
		
	case MODE_MFSK8:
		samplerate = 8000;
		symlen =  1024;
		symbits =    5;
		basetone = 128;
		numtones = 32;
		break;
	case MODE_MFSK16:
		samplerate = 8000;
		symlen =  512;
		symbits =   4;
		basetone = 64;
		numtones = 16;
		cap |= CAP_IMG;
		break;
	case MODE_MFSK32:
		samplerate = 8000;
		symlen =  256;
		symbits =   4;
		basetone = 32;
		numtones = 16;
		cap |= CAP_IMG;
		break;

	case MODE_MFSK4:
		samplerate = 8000;
		symlen = 2048;
		symbits = 5;
		basetone = 256;
		numtones = 32;
		break;
	case MODE_MFSK31:
		samplerate = 8000;
		symlen =  256;
		symbits =   3;
		basetone = 32;
		numtones = 8;
		cap |= CAP_IMG;
		break;
	case MODE_MFSK64:
		samplerate = 8000;
		symlen =  128;
		symbits =    4;
		basetone = 16;
		numtones = 16;
		cap |= CAP_IMG;
		break;
	case MODE_MFSK11:
		samplerate = 11025;
		symlen =  1024;
		symbits =   4;
		basetone = 93;
		numtones = 16;
		cap |= CAP_IMG;
		break;
	case MODE_MFSK22:
		samplerate = 11025;
		symlen =  512;
		symbits =    4;
		basetone = 46;
		numtones = 16;
		cap |= CAP_IMG;
		break;
//
	default:
		samplerate = 8000;
		symlen =  512;
		symbits =   4;
		basetone = 64;
		numtones = 16;
        break;
	}

	tonespacing = (double) samplerate / symlen;
	basefreq = 1.0 * samplerate * basetone / symlen;

	binsfft		= new sfft (symlen, basetone, basetone + numtones );
	hbfilt		= new C_FIR_filter();
	hbfilt->init_hilbert(37, 1);

	syncfilter = new Cmovavg(8);
	
	for (int i = 0; i < SCOPESIZE; i++)
		vidfilter[i] = new Cmovavg(16);

	pipe		= new rxpipe[ 2 * symlen ];

	enc			= new encoder (K, POLY1, POLY2);
	dec1		= new viterbi (K, POLY1, POLY2);
	dec2		= new viterbi (K, POLY1, POLY2);

	dec1->settraceback (45);
	dec2->settraceback (45);
	dec1->setchunksize (1);
	dec2->setchunksize (1);

	txinlv = new interleave (symbits, INTERLEAVE_FWD);
	rxinlv = new interleave (symbits, INTERLEAVE_REV);

	bw = (numtones - 1) * tonespacing;
	cf = basefreq + bw / 2.0;

	flo = (cf - bw/2 - 2 * tonespacing) / samplerate;
	fhi = (cf + bw/2 + 2 * tonespacing) / samplerate;

	bpfilt = new C_FIR_filter();
	bpfilt->init_bandpass (127, 1, flo, fhi);

	scopedata.alloc(symlen * 2);

	fragmentsize = symlen;
	bandwidth = (numtones - 1) * tonespacing;
	
	startpic = false;
	abortxmt = false;
	stopflag = false;

	bitshreg = 0;
	bitstate = 0;
	phaseacc = 0;
	pipeptr = 0;
	metric = 0;
	prev1symbol = prev2symbol = 0;
	symbolpair[0] = symbolpair[1] = 0;
	
// picTxWin and picRxWin are created once to support all instances of mfsk
	if (!picTxWin) createTxViewer();
	if (!picRxWin)
		createRxViewer();
	activate_mfsk_image_item(true);
	afcmetric = 0.0;
	datashreg = 1;

	init();

}


//=====================================================================
// receive processing
//=====================================================================


bool mfsk::check_picture_header(char c)
{
	char *p;

	if (c >= ' ' && c <= 'z') {
		memmove(picheader, picheader + 1, PICHEADER - 1);
		picheader[PICHEADER - 2] = c;
	}
	picW = 0;
	picH = 0;
	color = false;

	p = strstr(picheader, "Pic:");
	if (p == NULL)
		return false;

	p += 4;
	
	if (*p == 0) return false;

	while ( *p && isdigit(*p))
		picW = (picW * 10) + (*p++ - '0');

	if (*p++ != 'x')
		return false;

	while ( *p && isdigit(*p))
		picH = (picH * 10) + (*p++ - '0');

	if (*p == 'C') {
		color = true;
		p++;
	}
	if (*p == ';') {
		if (picW == 0 || picH == 0 || picW > 4095 || picH > 4095)
			return false;
		RXspp = 8;
		return true;
	}
	if (*p == 'p')
		p++;
	else
		return false;
	if (!*p) 
		return false;
	RXspp = 8;
	if (*p == '4') RXspp = 4;
	if (*p == '2') RXspp = 2;
	p++;
	if (!*p) 
		return false;
	if (*p != ';')
		return false;
	if (picW == 0 || picH == 0 || picW > 4095 || picH > 4095)
		return false;
	return true;
}

void mfsk::recvpic(complex z)
{
	int byte;
	picf += (prevz % z).arg() * samplerate / TWOPI;
	prevz = z;

	if (RXspp < 8 && progdefaults.slowcpu == true)
		return;
		
	if ((counter % RXspp) == 0) {
		picf = 256 * (picf / RXspp - basefreq) / bandwidth;
		byte = (int)CLAMP(picf, 0.0, 255.0);
		if (reverse)
			byte = 255 - byte;
		
		if (color) {
			pixelnbr = rgb + row + 3*col;
			REQ(updateRxPic, byte, pixelnbr);
			if (++col == picW) {
				col = 0;
				if (++rgb == 3) {
					rgb = 0;
					row += 3 * picW;
				}
			}
		} else {
			for (int i = 0; i < 3; i++)
				REQ(updateRxPic, byte, pixelnbr++);
		}
		picf = 0.0;

		int n = picW * picH * 3;
		if (pixelnbr % (picW * 3) == 0) {
			int s = snprintf(mfskmsg, sizeof(mfskmsg),
					 "Recv picture: %04.1f%% done",
					 (100.0f * pixelnbr) / n);
			print_time_left( (n - pixelnbr ) * 0.000125 * RXspp , 
					mfskmsg + s,
					sizeof(mfskmsg) - s, ", ", " left");
			put_status(mfskmsg);
		}
	}
}

void mfsk::recvchar(int c)
{
	if (c == -1 || c == 0)
		return;

	if (check_picture_header(c) == true) {
// 44 nulls at 8 samples per pixel
// 88 nulls at 4 samples per pixel
// 176 nulls at 2 samples per pixel
		counter = 352; 
		if (symbolbit == symbits) counter += symlen;
		rxstate = RX_STATE_PICTURE_START;
		picturesize = RXspp * picW * picH * (color ? 3 : 1);
		pixelnbr = 0;
		col = 0;
		row = 0;
		rgb = 0;
		memset(picheader, ' ', PICHEADER - 1);
		picheader[PICHEADER -1] = 0;		
	}
	put_rx_char(c);
	
}

void mfsk::recvbit(int bit)
{
	int c;

	datashreg = (datashreg << 1) | !!bit;
	if ((datashreg & 7) == 1) {
		c = varidec(datashreg >> 1);
		recvchar(c);
		datashreg = 1;
	}

}

void mfsk::decodesymbol(unsigned char symbol)
{
	int c, met;

	symbolpair[0] = symbolpair[1];
	symbolpair[1] = symbol;

	symcounter = symcounter ? 0 : 1;

// only modes with odd number of symbits need a vote
	if (symbits == 5 || symbits == 3) { // could use symbits % 2 == 0
		if (symcounter) {
			if ((c = dec1->decode(symbolpair, &met)) == -1)
				return;
			met1 = decayavg(met1, met, 32);
			if (met1 < met2)
				return;
			metric = met1 / 2.0;
		} else {
			if ((c = dec2->decode(symbolpair, &met)) == -1)
				return;
			met2 = decayavg(met2, met, 32);
			if (met2 < met1)
				return;
			metric = met2 / 2.0;
		}
	} else {
		if (symcounter) return;
		if ((c = dec2->decode(symbolpair, &met)) == -1)
			return;
		met2 = decayavg(met2, met, 32);
		metric = met2 / 2.0;
	}
	
	display_metric(metric);
	
	if (progStatus.sqlonoff && metric < progStatus.sldrSquelchValue)
		return;

	recvbit(c);

}

void mfsk::softdecode(complex *bins)
{
	double binmag, sum, b[symbits];
	unsigned char symbols[symbits];
	int i, j, k;

	for (i = 0; i < symbits; i++)
		b[i] = 0.0;

// avoid divide by zero later
	sum = 1e-10;
// gray decode and form soft decision samples
	for (i = 0; i < numtones; i++) {
		j = graydecode(i);

		if (reverse)
			k = (numtones - 1) - i;
		else
			k = i;

		binmag = bins[k].mag();

		for (k = 0; k < symbits; k++)
			b[k] += (j & (1 << (symbits - k - 1))) ? binmag : -binmag;

		sum += binmag;
	}

// shift to range 0...255
	for (i = 0; i < symbits; i++)
		if (staticburst)
			symbols[i] = 0;  // puncturing
		else
			symbols[i] = (unsigned char)clamp(128.0 + (b[i] / sum * 128.0), 0, 255);

	rxinlv->symbols(symbols);

	for (i = 0; i < symbits; i++) {
		symbolbit = i + 1;
		decodesymbol(symbols[i]);
	}
}

complex mfsk::mixer(complex in, double f)
{
	complex z;

// Basetone is a nominal 1000 Hz 
	f -= tonespacing * basetone + bandwidth / 2;	
	
	z = in * complex( cos(phaseacc), sin(phaseacc) );

	phaseacc -= TWOPI * f / samplerate;
	if (phaseacc > TWOPI) phaseacc -= TWOPI;
	if (phaseacc < -TWOPI) phaseacc += TWOPI;
	
	return z;
}

// finds the tone bin with the largest signal level
// assumes that will be the present tone received 
// with NO CW inteference

int mfsk::harddecode(complex *in)
{
	double x, max = 0.0, avg = 0.0;
	int i, symbol = 0;
	int burstcount = 0;

	for (int i = 0; i < numtones; i++)
		avg += in[i].mag();
	avg /= numtones;
			
	if (avg < 1e-20) avg = 1e-20;
	
	for (i = 0; i < numtones; i++) {
		x = in[i].mag();
		if ( x > max) {
			max = x;
			symbol = i;
		}
		if (x > 2.0 * avg) burstcount++;
	}

	staticburst = (burstcount == numtones);

	if (!staticburst)
		afcmetric = 0.95*afcmetric + 0.05 * (2 * max / avg);
	else
		afcmetric = 0.0;

	return symbol;
}

void mfsk::update_syncscope()
{
	int j;
	int pipelen = 2 * symlen;
	memset(scopedata, 0, 2 * symlen * sizeof(double));
	if (!progStatus.sqlonoff || metric >= progStatus.sldrSquelchValue)
		for (unsigned int i = 0; i < SCOPESIZE; i++) {
			j = (pipeptr + i * pipelen / SCOPESIZE + 1) % (pipelen);
			scopedata[i] = vidfilter[i]->run(pipe[j].vector[prev1symbol].mag());
		}
	set_scope(scopedata, SCOPESIZE);

	scopedata.next(); // change buffers
	snprintf(mfskmsg, sizeof(mfskmsg), "s/n %3.0f dB", 20.0 * log10(s2n));
	put_Status1(mfskmsg);
}

void mfsk::synchronize()
{
	int i, j;
	double syn = -1;
	double val, max = 0.0;

	if (currsymbol == prev1symbol)
		return;
	if (prev1symbol == prev2symbol)
		return;

	j = pipeptr;

	for (i = 0; i < 2 * symlen; i++) {
		val = (pipe[j].vector[prev1symbol]).mag();

		if (val > max) {
			max = val;
			syn = i;
		}

		j = (j + 1) % (2 * symlen);
	}

	syn = syncfilter->run(syn);

	synccounter += (int) floor((syn - symlen) / numtones + 0.5);
	
	update_syncscope();
}

void mfsk::reset_afc() {
	freqerr = 0.0;
	syncfilter->reset();
	return;
}

void mfsk::afc()
{
	complex z;
	complex prevvector;
	double f, f1;
	double ts = tonespacing / 4;

	if (sigsearch) {
		reset_afc();
		sigsearch = 0;
	}
	
	if (staticburst || !progStatus.afconoff)
		return;
	if (metric < progStatus.sldrSquelchValue)
		return;
	if (afcmetric < 3.0)
		return;
	if (currsymbol != prev1symbol)
		return;
//	if (prev1symbol != prev2symbol)
//		return;
	
	if (pipeptr == 0)
		prevvector = pipe[2*symlen - 1].vector[currsymbol];
	else
		prevvector = pipe[pipeptr - 1].vector[currsymbol];
	z = prevvector % currvector;

	f = z.arg() * samplerate / TWOPI;
	
	f1 = tonespacing * (basetone + currsymbol);	

	if ( fabs(f1 - f) < ts) {
		freqerr = decayavg(freqerr, (f1 - f), 32);
		set_freq(frequency - freqerr);
	}

}

void mfsk::eval_s2n()
{
	sig = pipe[pipeptr].vector[currsymbol].mag();
	noise = 0.0;
	for (int i = 0; i < numtones; i++) {
		if (i != currsymbol)
			noise += pipe[pipeptr].vector[i].mag();
	}	
	if (noise > 0)
		s2n = decayavg ( s2n, sig / noise, 64 );
}

int mfsk::rx_process(const double *buf, int len)
{
	complex z, *bins = 0;
	int i;

	while (len-- > 0) {
// create analytic signal...
		z.re = z.im = *buf++;
		hbfilt->run ( z, z );
// shift in frequency to the base freq
		z = mixer(z, frequency);
// bandpass filter around the shifted center frequency
// with required bandwidth 
		bpfilt->run ( z, z );
		
		if (rxstate == RX_STATE_PICTURE_START) {
			if (--counter == 0) {
				counter = picturesize;
				rxstate = RX_STATE_PICTURE;
				REQ( showRxViewer, picW, picH );
			}
			continue;
		}
		if (rxstate == RX_STATE_PICTURE) {
			if (--counter == 0) {
				if (btnpicRxAbort) {
					FL_LOCK_E();
					btnpicRxAbort->hide();
					btnpicRxSave->show();
					FL_UNLOCK_E();
				}
				rxstate = RX_STATE_DATA;
				put_status("");

				string autosave_dir = PicsDir;
				picRx->save_png(autosave_dir.c_str());
			} else
				recvpic(z);
			continue;
		}

// binsfft->run(z) returns pointer to first frequency of interest
		bins = binsfft->run (z);

// copy current vector to the pipe
		for (i = 0; i < numtones; i++)
			pipe[pipeptr].vector[i] = bins[i];
			
		if (--synccounter <= 0) {
			
			synccounter = symlen;

			currsymbol = harddecode(bins);
			currvector = bins[currsymbol];			
			softdecode(bins);

// frequency tracking 
//			afc();
//			eval_s2n();
// decode symbol 
//			softdecode(bins);

// symbol sync 
			synchronize();

// frequency tracking 
			afc();
			eval_s2n();

			prev2symbol = prev1symbol;
			prev2vector = prev1vector;
			prev1symbol = currsymbol;
			prev1vector = currvector;
//			prevmaxval = maxval;
		}
		pipeptr = (pipeptr + 1) % (2 * symlen);
	}

	return 0;
}


//=====================================================================
// transmit processing
//=====================================================================


void mfsk::sendsymbol(int sym)
{
	double f, phaseincr;

	f = get_txfreq_woffset() - bandwidth / 2;
	
	sym = grayencode(sym & (numtones - 1));
	if (reverse)
		sym = (numtones - 1) - sym;

	phaseincr = TWOPI * (f + sym*tonespacing) / samplerate;
	
	for (int i = 0; i < symlen; i++) {
		outbuf[i] = cos(phaseacc);
		phaseacc -= phaseincr;
		if (phaseacc > M_PI)
			phaseacc -= TWOPI;
		else if (phaseacc < M_PI)
			phaseacc += TWOPI;
	}
	ModulateXmtr(outbuf, symlen);

}

void mfsk::sendbit(int bit)
{
	int data = enc->encode(bit);
	for (int i = 0; i < 2; i++) {
		bitshreg = (bitshreg << 1) | ((data >> i) & 1);
		bitstate++;

		if (bitstate == symbits) {
			txinlv->bits(&bitshreg);
			sendsymbol(bitshreg);
			bitstate = 0;
			bitshreg = 0;
		}
	}
}

void mfsk::sendchar(unsigned char c)
{
	const char *code = varienc(c);
	while (*code)
		sendbit(*code++ - '0');
	put_echo_char(c);
}

void mfsk::sendidle()
{
	sendchar(0);	// <NUL>
	sendbit(1);

// extended zero bit stream
	for (int i = 0; i < 32; i++)
		sendbit(0);
}

void mfsk::flushtx()
{
// flush the varicode decoder at the other end
	sendbit(1);

// flush the convolutional encoder and interleaver
	for (int i = 0; i < 107; i++)
		sendbit(0);

	bitstate = 0;
}


void mfsk::sendpic(unsigned char *data, int len)
{
	double *ptr;
	double f;
	int i, j;

	ptr = outbuf;

	for (i = 0; i < len; i++) {
		if (txstate == TX_STATE_PICTURE)
		    REQ(updateTxPic, data[i]);
		if (reverse)
			f = get_txfreq_woffset() - bandwidth * (data[i] - 128) / 256.0;
		else
			f = get_txfreq_woffset() + bandwidth * (data[i] - 128) / 256.0;
			
		for (j = 0; j < TXspp; j++) {
			*ptr++ = cos(phaseacc);

			phaseacc += TWOPI * f / samplerate;

			if (phaseacc > M_PI)
				phaseacc -= 2.0 * M_PI;
		}
	}

	ModulateXmtr(outbuf, TXspp * len);
}


void mfsk::clearbits()
{
	int data = enc->encode(0);
	for (int k = 0; k < 100; k++) {
		for (int i = 0; i < 2; i++) {
			bitshreg = (bitshreg << 1) | ((data >> i) & 1);
			bitstate++;

			if (bitstate == symbits) {
				txinlv->bits(&bitshreg);
				bitstate = 0;
				bitshreg = 0;
			}
		}
	}
}


int mfsk::tx_process()
{

	int xmtbyte;

	switch (txstate) {
		case TX_STATE_PREAMBLE:
			clearbits();
			for (int i = 0; i < 32; i++)
				sendbit(0);
			txstate = TX_STATE_START;
			break;

		case TX_STATE_START:
			sendchar('\r');
			sendchar(2);		// STX
			sendchar('\r');
			txstate = TX_STATE_DATA;
			break;

		case TX_STATE_DATA:
			xmtbyte = get_tx_char();

			if (xmtbyte == 0x05 || startpic == true) {
				put_status("Send picture: start");
				int len = (int)strlen(picheader);
				for (int i = 0; i < len; i++)
					sendchar(picheader[i]);
				flushtx();
				startpic = false;
				txstate = TX_STATE_PICTURE_START;
			}
			else if ( xmtbyte == 0x03 || stopflag)
				txstate = TX_STATE_FLUSH;
			else if (xmtbyte == -1)
				sendidle();
			else
				sendchar(xmtbyte);
			break;

		case TX_STATE_FLUSH:
			sendchar('\r');
			sendchar(4);		// EOT
			sendchar('\r');
			flushtx();
			rxstate = RX_STATE_DATA;
			txstate = TX_STATE_PREAMBLE;
			stopflag = false;
			cwid();
			return -1;

		case TX_STATE_PICTURE_START:
// 176 samples
			memset(picprologue, 0, 44 * 8 / TXspp);
			sendpic(picprologue, 44 * 8 / TXspp);
			txstate = TX_STATE_PICTURE;
			break;
	
		case TX_STATE_PICTURE:
			int i = 0;
			int blocklen = 128;
			while (i < xmtbytes) {
				if (stopflag || abortxmt)
					break;
				if (i + blocklen < xmtbytes)
					sendpic( &xmtpicbuff[i], blocklen);
				else
					sendpic( &xmtpicbuff[i], xmtbytes - i);
				if ( (100 * i / xmtbytes) % 2 == 0) {
					int n = snprintf(mfskmsg, sizeof(mfskmsg),
							 "Send picture: %04.1f%% done",
							 (100.0f * i) / xmtbytes);
					print_time_left((xmtbytes - i) * 0.000125 * TXspp, mfskmsg + n,
							sizeof(mfskmsg) - n, ", ", " left");
					put_status(mfskmsg);
				}
				i += blocklen;
			}
			REQ_FLUSH(GET_THREAD_ID());

			txstate = TX_STATE_DATA;
			put_status("Send picture: done");
			FL_LOCK_E();
			btnpicTxSendAbort->hide();
			btnpicTxSPP->show();
			btnpicTxSendColor->show();
			btnpicTxSendGrey->show();
			btnpicTxLoad->show();
			btnpicTxClose->show();
			abortxmt = false;
			rxstate = RX_STATE_DATA;
			memset(picheader, ' ', PICHEADER - 1);
			picheader[PICHEADER -1] = 0;
			FL_UNLOCK_E();
			break;
	}

	return 0;
}

