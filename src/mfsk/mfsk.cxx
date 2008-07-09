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

#include <stdlib.h>
#include <iostream>

#include "mfsk.h"
#include "modem.h"
#include "afcind.h"

#include "configuration.h"
#include "status.h"
#include "trx.h"

#include "ascii.h"

#include "fileselect.h"

#include "qrunner.h"

//#define AFC_COUNT	16
//32

using namespace std;

char mfskmsg[80];
char txclr_tooltip[24];
char txgry_tooltip[24];

void  mfsk::tx_init(SoundBase *sc)
{
	scard = sc;
	txstate = TX_STATE_PREAMBLE;
	bitstate = 0;
	counter = 0;
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
	set_AFCrange(tonespacing / 10.0);
	syncfilter->reset();
	staticburst = false;
}

void mfsk::init()
{
	modem::init();
	rx_init();
	set_scope_mode(Digiscope::SCOPE);
}

mfsk::~mfsk()
{
	if (bpfilt) delete bpfilt;
	if (rxinlv) delete rxinlv;
	if (txinlv) delete txinlv;
	if (dec2) delete dec2;
	if (dec1) delete dec1;
	if (enc) delete enc;
	if (pipe) delete [] pipe;
	if (hbfilt) delete hbfilt;
	if (binsfft) delete binsfft;
	if (met1filt) delete met1filt;
	if (met2filt) delete met2filt;
	if (xmtimg) delete [] xmtimg;
	for (int i = 0; i < SCOPESIZE; i++) {
		if (vidfilter[i]) delete vidfilter[i];
	}
	deleteRxViewer();
	deleteTxViewer();
	if (syncfilter) delete syncfilter;
}

mfsk::mfsk(trx_mode mfsk_mode) : modem()
{
	double bw, cf, flo, fhi;
	mode = mfsk_mode;

	switch (mode) {

	case MODE_MFSK8:
		samplerate = 8000;
		symlen =  1024;
		symbits =    5;
		basetone = 128;
		break;
	case MODE_MFSK16:
		samplerate = 8000;
		symlen =  512;
		symbits =   4;
		basetone = 64;
        break;
	case MODE_MFSK32:
		samplerate = 8000;
		symlen =  256;
		symbits =    4;
		basetone = 32;
		break;
#ifdef EXPERIMENTAL
	case MODE_MFSK11:
		samplerate = 11025;
		symlen =  1024;
		symbits =   4;
		basetone = 93;
        break;
	case MODE_MFSK22:
		samplerate = 11025;
		symlen =  512;
		symbits =    4;
		basetone = 46;
		break;
#endif
	default:
		samplerate = 8000;
		symlen =  512;
		symbits =   4;
		basetone = 64;
        break;
	}

	numtones = 1 << symbits;
	tonespacing = (double) samplerate / symlen;
//	basetone = (int)floor(1000.0 * symlen / samplerate + 0.5);

	binsfft		= new sfft (symlen, basetone, basetone + numtones );//+ 3); // ?
	hbfilt		= new C_FIR_filter();
	hbfilt->init_hilbert(37, 1);
	afcfilt		= new Cmovavg(AFC_COUNT);
	met1filt	= new Cmovavg(32);
	met2filt	= new Cmovavg(32);
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
	cf = 1000.0 + bw / 2.0;

	flo = (cf - bw/2 - 2 * tonespacing) / samplerate;
	fhi = (cf + bw/2 + 2 * tonespacing) / samplerate;

	bpfilt = new C_FIR_filter();
	bpfilt->init_bandpass (127, 1, flo, fhi);

	scopedata.alloc(symlen * 2);

	fragmentsize = symlen;
	bandwidth = (numtones - 1) * tonespacing;
	
	picRxWin = (Fl_Double_Window *)0;
	picRx = (picture *)0;
	btnpicRxSave = (Fl_Button *)0;
	btnpicRxClose = (Fl_Button *)0;

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
	
	init();
}

void mfsk::shutdown()
{
	stopflag = true;
	if (picTxWin) {
		picTxWin->hide();
		activate_mfsk_image_item(false);
	}
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

	if (*p != ';')
		return false;

	if (picW == 0 || picH == 0 || picW > 4095 || picH > 4095)
		return false;

	return true;
}

void mfsk::recvpic(complex z)
{
	int byte;
	picf += (prevz % z).arg() * samplerate / twopi;
	prevz = z;

	if ((counter % SAMPLES_PER_PIXEL) == 0) {
		picf = 256 * (picf / SAMPLES_PER_PIXEL - 1000) / bandwidth;
		byte = (int)CLAMP(picf, 0.0, 255.0);
		if (reverse)
			byte = 255 - byte;
		
		if (color) {
			pixelnbr = rgb + row + 3*col;
//!			updateRxPic(byte, pixelnbr);
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
//!			updateRxPic(byte, pixelnbr++);
				REQ(updateRxPic, byte, pixelnbr++);
		}
		picf = 0.0;

		int n = picW * picH * 3;
		int s = snprintf(mfskmsg, sizeof(mfskmsg),
				 "Recv picture: %04.1f%% done",
				 (100.0f * pixelnbr) / n);
		print_time_left(n - pixelnbr, mfskmsg + s,
				sizeof(mfskmsg) - s, ", ", " left");
		put_status(mfskmsg);
	}
}

void mfsk::recvchar(int c)
{
	if (c == -1 || c == 0)
		return;

	if (check_picture_header(c) == true) {
		if (symbolbit == 4) {
			rxstate = RX_STATE_PICTURE_START_1;
		}
		else {
			rxstate = RX_STATE_PICTURE_START_2;
		}
		
		picturesize = SAMPLES_PER_PIXEL * picW * picH * (color ? 3 : 1);
		counter = 0;

		showRxViewer(picW, picH, this);

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

// only MFSK8 needs a vote
	if (mode == MODE_MFSK8) {
		if (symcounter) {
			if ((c = dec1->decode(symbolpair, &met)) == -1)
				return;
			met1 = met1filt->run(met);
			if (met1 < met2)
				return;
			metric = met1 / 2.0;
		} else {
			if ((c = dec2->decode(symbolpair, &met)) == -1)
				return;
			met2 = met2filt->run(met);
			if (met2 < met1)
				return;
			metric = met2 / 2.0;
		}
	} else {
		if (symcounter) return;
		if ((c = dec2->decode(symbolpair, &met)) == -1)
			return;
		metric = met2filt->run(met / 2.0);
	}
	
	display_metric(metric);
	
	if (progStatus.sqlonoff && metric < progStatus.sldrSquelchValue)
		return;

	recvbit(c);

}

void mfsk::softdecode(complex *bins)
{
	double tone, sum, b[symbits];
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

		tone = bins[k].mag();

		for (k = 0; k < symbits; k++)
			b[k] += (j & (1 << (symbits - k - 1))) ? tone : -tone;

		sum += tone;
	}

// shift to range 0...260 ??? symbols are unsigned char
	for (i = 0; i < symbits; i++)
		symbols[i] = (unsigned char)clamp(128.0 + (b[i] / sum * 128.0), 0, 255);// 260);

	rxinlv->symbols(symbols);

	for (i = 0; i < symbits; i++) {
		symbolbit = i + 1;
		decodesymbol(symbols[i]);
	}
}

complex mfsk::mixer(complex in, double f)
{
	complex z;

// Basetone is always 1000 Hz 
	f -= (1000.0 + bandwidth / 2);
	z = in * complex( cos(phaseacc), sin(phaseacc) );

	phaseacc -= twopi * f / samplerate;
	if (phaseacc > twopi) phaseacc -= twopi;
	if (phaseacc < -twopi) phaseacc += twopi;
	
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

	staticburst = (burstcount > numtones / 2);
	
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
//	double max = prevmaxval;
//	if (max == 0.0) max = 1e10;
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
	afcfilt->reset();
	return;
}

void mfsk::afc()
{
	complex z;
	complex prevvector;
	double f, f1, f2;
	double ts = tonespacing / 8;

	if (sigsearch) {
		reset_afc();
		sigsearch = 0;
	}
	
	if (staticburst || !progStatus.afconoff)
		return;
	if (metric < progStatus.sldrSquelchValue)
		return;
	if (afcmetric < 6.0)
		return;
	
	if (pipeptr == 0)
		prevvector = pipe[2*symlen - 1].vector[currsymbol];
	else
		prevvector = pipe[pipeptr - 1].vector[currsymbol];
	z = prevvector % currvector;

	f = z.arg() * samplerate / twopi;
	
	f1 = tonespacing * (basetone + currsymbol);	
	f1 -= f;

	f1 /= numtones;
	
	f2 = CLAMP ( f1, freqerr - ts, freqerr + ts);

	freqerr = decayavg ( freqerr, f2, 128 );

	set_freq(frequency - freqerr);
	set_AFCind( freqerr );
}

void mfsk::eval_s2n()
{
	sig = pipe[pipeptr].vector[currsymbol].mag();
	noise = 0.0;
	for (int i = 0; i < numtones; i++) {
		if (i != currsymbol)
			noise += pipe[pipeptr].vector[i].mag();
	}	
//	noise /= (numtones - 1);
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
// shift in frequency to the base freq of 1000 hz
		z = mixer(z, frequency);
// bandpass filter around the shifted center frequency
// with required bandwidth 
		bpfilt->run ( z, z );
		
		if (rxstate == RX_STATE_PICTURE_START_2) {
			if (counter++ == 352) {
				counter = 0;
				rxstate = RX_STATE_PICTURE;
			}
			continue;
		}
		if (rxstate == RX_STATE_PICTURE_START_1) {
			if (counter++ == 352 + symlen) {
				counter = 0;
				rxstate = RX_STATE_PICTURE;
			}
			continue;
		}

		if (rxstate == RX_STATE_PICTURE) {
			if (counter++ == picturesize) {
				counter = 0;
				if (btnpicRxAbort) {
					FL_LOCK_E();
					btnpicRxAbort->hide();
					btnpicRxSave->show();
					FL_UNLOCK_E();
				}
				rxstate = RX_STATE_DATA;
				// REQ_FLUSH();
				put_status("");
#if USE_LIBPNG
				string autosave_dir = HomeDir + "mfsk_pics/";
				picRx->save_png(autosave_dir.c_str());
#else
#  if USE_LIBJPEG
				string autosave_dir = HomeDir + "mfsk_pics/";
				picRx->save_jpeg(autosave_dir.c_str());
#  endif
#endif
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
// frequency tracking 
			afc();
			
			eval_s2n();
// decode symbol 
			softdecode(bins);
// symbol sync 
			synchronize();

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
//printf("%5d", sym);
	if (reverse)
		sym = (numtones - 1) - sym;

	phaseincr = twopi * (f + sym*tonespacing) / samplerate;
	
	for (int i = 0; i < symlen; i++) {
		outbuf[i] = cos(phaseacc);
		phaseacc -= phaseincr;
		if (phaseacc > M_PI)
			phaseacc -= twopi;
		else if (phaseacc < M_PI)
			phaseacc += twopi;
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
//!			updateTxPic(data[i], this);
		    REQ(updateTxPic, data[i], this);
		if (reverse)
			f = get_txfreq_woffset() - bandwidth * (data[i] - 128) / 256.0;
		else
			f = get_txfreq_woffset() + bandwidth * (data[i] - 128) / 256.0;
			
		for (j = 0; j < SAMPLES_PER_PIXEL; j++) {
			*ptr++ = cos(phaseacc);

			phaseacc += twopi * f / samplerate;

			if (phaseacc > M_PI)
				phaseacc -= 2.0 * M_PI;
		}
	}

	ModulateXmtr(outbuf, SAMPLES_PER_PIXEL * len);
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
			memset(picprologue, 0, 44);
			sendpic(picprologue, 44);
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
				i += blocklen;
				int n = snprintf(mfskmsg, sizeof(mfskmsg),
						 "Send picture: %04.1f%% done",
						 (100.0f * i) / xmtbytes);
				print_time_left(xmtbytes - i, mfskmsg + n,
						sizeof(mfskmsg) - n, ", ", " left");
				put_status(mfskmsg);
			}
			REQ_FLUSH();

			txstate = TX_STATE_DATA;
			put_status("Send picture: done");
			FL_LOCK_E();
			btnpicTxSendAbort->hide();
			btnpicTxSendColor->show();
			btnpicTxSendGrey->show();
			btnpicTxLoad->show();
			btnpicTxClose->show();
			abortxmt = false;
			rxstate = RX_STATE_DATA;
			counter = 0;
			memset(picheader, ' ', PICHEADER - 1);
			picheader[PICHEADER -1] = 0;
			FL_UNLOCK_E();
			break;
	}

	return 0;
}

//=============================================================================
// picture viewers for mfsk-pic mode
//=============================================================================
Fl_Double_Window	*picRxWin = (Fl_Double_Window *)0;
picture		*picRx = (picture *)0;
Fl_Button	*btnpicRxSave = (Fl_Button *)0;
Fl_Button	*btnpicRxAbort = (Fl_Button *)0;
Fl_Button	*btnpicRxClose = (Fl_Button *)0;

Fl_Double_Window	*picTxWin = (Fl_Double_Window *)0;
picture		*picTx = (picture *)0;
Fl_Button	*btnpicTxSendColor = (Fl_Button *)0;
Fl_Button	*btnpicTxSendGrey = (Fl_Button *)0;
Fl_Button	*btnpicTxSendAbort = (Fl_Button *)0;
Fl_Button	*btnpicTxLoad = (Fl_Button *)0;
Fl_Button	*btnpicTxClose = (Fl_Button *)0;

Fl_Shared_Image	*TxImg = (Fl_Shared_Image *)0;
unsigned char *xmtimg = (unsigned char *)0;
unsigned char *xmtpicbuff = (unsigned char *)0;

void updateRxPic(unsigned char data, int pos)
{
	picRx->pixel(data, pos);
}

void cb_picRxClose( Fl_Widget *w, void *who)
{
//	FL_LOCK();
	picRxWin->hide();
//	FL_UNLOCK();
}

void cb_picRxAbort( Fl_Widget *w, void *who)
{
	mfsk *me = (mfsk *)who;
	me->rxstate = me->RX_STATE_DATA;
	put_status("");
	picRx->clear();
}

void cb_picRxSave( Fl_Widget *w, void *who)
{
//	mfsk *me = (mfsk *)who;
	const char ffilter[] = ""
#if USE_LIBPNG
		"Portable Network Graphics\t*.png\n"
#endif
#if USE_LIBJPEG
		"Independent JPEG Group\t*.{jpg,jpeg}"
#endif
		;
	const char dfname[] = "image."
#if USE_LIBPNG
		"png"
#else
		"jpg"
#endif
		;

	int fsel;
	const char *fn = FSEL::saveas("Save image as:", ffilter, dfname, &fsel);
	if (!fn) return;
        // selected filter determines format
	switch (fsel) {
	case 0:
#if USE_LIBPNG
		picRx->save_png(fn);
		break;
#endif
		// fall through if no libpng
	case 1:
#if USE_LIBJPEG
		picRx->save_jpeg(fn);
#endif
		break;
	}
}

void createRxViewer(mfsk *who)
{
	FL_LOCK_D();
	picRxWin = new Fl_Double_Window(200, 140);
	picRxWin->xclass(PACKAGE_NAME);
	picRx = new picture(2, 2, 136, 104);
	btnpicRxSave = new Fl_Button(5, 140 - 30, 60, 24,"Save...");
	btnpicRxSave->callback(cb_picRxSave, who);
	btnpicRxSave->hide();
#if !(USE_LIBPNG || USE_LIBJPEG)
	btnpicRxSave->deactivate();
#endif
	btnpicRxAbort = new Fl_Button(70, 140 - 30, 60, 24, "Abort");
	btnpicRxAbort->callback(cb_picRxAbort, who);
	btnpicRxClose = new Fl_Button(135, 140 - 30, 60, 24, "Hide");
	btnpicRxClose->callback(cb_picRxClose, who);
	activate_mfsk_image_item(true);
	FL_UNLOCK_D();
}

void showRxViewer(int W, int H, mfsk *who)
{
	FL_LOCK_E();
	if (!picRxWin) createRxViewer(who);
	int winW, winH;
	int picX, picY;
	winW = W < 136 ? 140 : W + 4;
	winH = H + 34;
	picX = (winW - W) / 2;
	picY = 2;
	picRxWin->size(winW, winH);
	picRx->resize(picX, picY, W, H);
	btnpicRxSave->resize(winW/2 - 65, H + 6, 60, 24);
	btnpicRxSave->hide();
	btnpicRxAbort->resize(winW/2 - 65, H + 6, 60, 24);
	btnpicRxAbort->show();
	btnpicRxClose->resize(winW/2 + 5, H + 6, 60, 24);
	picRx->clear();
#ifndef __CYGWIN__
	picRxWin->show();
#endif
	FL_UNLOCK_E();
}

void load_file(const char *n) {
	int W, H, D;
	unsigned char *img_data;
	
	if (TxImg) {
		TxImg->release();
		TxImg = 0L;
	}
	TxImg = Fl_Shared_Image::get(n);
	if (!TxImg)
		return;
	img_data = (unsigned char *)TxImg->data()[0];
	W = TxImg->w();
	H = TxImg->h();
	D = TxImg->d();
	if (xmtimg) delete [] xmtimg;
	xmtimg = new unsigned char [W * H * 3];
	if (D == 3)
		memcpy(xmtimg, img_data, W*H*3);
	else if (D == 4) {
		int i, j, k;
		for (i = 0; i < W*H; i++) {
			j = i*3; k = i*4;
			xmtimg[j] = img_data[k];
			xmtimg[j+1] = img_data[k+1];
			xmtimg[j+2] = img_data[k+2];
		}
	} else if (D == 1) {
		int i, j;
		for (i = 0; i < W*H; i++) {
			j = i * 3;
			xmtimg[j] = xmtimg[j+1] = xmtimg[j+2] = img_data[i];
		}
	} else
		return;

	TxViewerResize(W, H);
	
// load the picture widget with the rgb image
	FL_LOCK_D();
	picTx->clear();
	picTxWin->redraw();
	picTx->video(xmtimg, W * H * 3);
	if (print_time_left(W * H * 3, txclr_tooltip, sizeof(txclr_tooltip), "Time needed: ") > 0)
		btnpicTxSendColor->tooltip(txclr_tooltip);
	btnpicTxSendColor->activate();
	if (print_time_left(W * H, txgry_tooltip, sizeof(txgry_tooltip), "Time needed: ") > 0)
		btnpicTxSendGrey->tooltip(txgry_tooltip);
	btnpicTxSendGrey->activate();
	FL_UNLOCK_D();
}

void updateTxPic(unsigned char data, mfsk *me)
{
	if (me->color) {
		me->pixelnbr = me->rgb + me->row + 3*me->col;
		picTx->pixel(data, me->pixelnbr);
		if (++me->col == TxImg->w()) {
			me->col = 0;
			if (++me->rgb == 3) {
				me->rgb = 0;
				me->row += 3 * TxImg->w();
			}
		}
	} else {
		picTx->pixel( data, me->pixelnbr++ );
		picTx->pixel( data, me->pixelnbr++ );
		picTx->pixel( data, me->pixelnbr++ );
	}
}

void cb_picTxLoad(Fl_Widget *,void *who) {
//	mfsk *TxWho = (mfsk *)who;
	const char *fn = 
		FSEL::select("Load image file", "Portable Network Graphics\t*.png\n"
			    "Independent JPEG Group\t*.{jpg,jif,jpeg,jpe}\n"
			    "Graphics Interchange Format\t*.gif");
	if (!fn) return;
	load_file(fn);
}

void cb_picTxClose( Fl_Widget *w, void *who)
{
//	mfsk *me = (mfsk *)who;
	FL_LOCK_D();
	picTxWin->hide();
	FL_UNLOCK_D();
}

void cb_picTxSendColor( Fl_Widget *w, void *who)
{
	mfsk *me = (mfsk *)who;
	int W, H, rowstart;
	W = TxImg->w();
	H = TxImg->h();
	if (xmtpicbuff) delete [] xmtpicbuff;
	xmtpicbuff = new unsigned char [W*H*3];
	unsigned char *outbuf = xmtpicbuff;
	unsigned char *inbuf = xmtimg;
	int iy, ix, rgb;
	for (iy = 0; iy < H; iy++) {
		rowstart = iy * W * 3;
		for (rgb = 0; rgb < 3; rgb++)
			for (ix = 0; ix < W; ix++)
				outbuf[rowstart + rgb*W + ix] = inbuf[rowstart + rgb + ix*3];
	}
	snprintf(me->picheader, PICHEADER, "\nSending Pic:%dx%dC;", W, H);
	me->xmtbytes = W * H * 3;
	me->color = true;
	me->rgb = 0;
	me->col = 0;
	me->row = 0;
	me->pixelnbr = 0;
	FL_LOCK_D();
	btnpicTxSendColor->hide();
	btnpicTxSendGrey->hide();
	btnpicTxLoad->hide();
	btnpicTxClose->hide();
	btnpicTxSendAbort->show();
	picTx->clear();
	FL_UNLOCK_D();
// start the transmission
	start_tx();
	me->startpic = true;
}

void cb_picTxSendGrey( Fl_Widget *w, void *who)
{
	mfsk *me = (mfsk *)who;
	int W, H;
	W = TxImg->w();
	H = TxImg->h();
	if (xmtpicbuff) delete [] xmtpicbuff;
	xmtpicbuff = new unsigned char [W*H];
	unsigned char *outbuf = xmtpicbuff;
	unsigned char *inbuf = xmtimg;
	for (int i = 0; i < W*H; i++)
		outbuf[i] = ( 31 * inbuf[i*3] + 61 * inbuf[i*3 + 1] + 8 * inbuf[i*3 + 2])/100;
	snprintf(me->picheader, PICHEADER, "\nSending Pic:%dx%d;", W, H);
	me->xmtbytes = W * H;
	me->color = false;
	me->col = 0;
	me->row = 0;
	me->pixelnbr = 0;
	FL_LOCK_D();
	btnpicTxSendColor->hide();
	btnpicTxSendGrey->hide();
	btnpicTxLoad->hide();
	btnpicTxClose->hide();
	btnpicTxSendAbort->show();
	picTx->clear();
	FL_UNLOCK_D();
// start the transmission
	start_tx();
	me->startpic = true;
}


void cb_picTxSendAbort( Fl_Widget *w, void *who)
{
	mfsk *me = (mfsk *)who;
	me->abortxmt = true;
// reload the picture widget with the rgb image
	FL_LOCK_D();
	picTx->video(xmtimg, TxImg->w() * TxImg->h() * 3);
	FL_UNLOCK_D();
}

void createTxViewer(mfsk *who)
{
	FL_LOCK_D();
	picTxWin = new Fl_Double_Window(250, 180);
	picTxWin->xclass(PACKAGE_NAME);
	picTx = new picture (2, 2, 246, 150);
	btnpicTxSendColor = new Fl_Button(250/2 - 123, 180 - 30, 60, 24, "XmtClr");
	btnpicTxSendColor->callback(cb_picTxSendColor, who);
	btnpicTxSendGrey = new Fl_Button(250/2 - 61, 180 - 30, 60, 24, "XmtGry");
	btnpicTxSendGrey->callback( cb_picTxSendGrey, who);
	btnpicTxSendAbort = new Fl_Button(250/2 - 123, 180 - 30, 122, 24, "Abort Xmt");
	btnpicTxSendAbort->callback(cb_picTxSendAbort, who);
	btnpicTxLoad = new Fl_Button(250/2 + 1, 180 - 30, 60, 24, "Load");
	btnpicTxLoad->callback(cb_picTxLoad, who);
	btnpicTxClose = new Fl_Button(250/2 + 63, 180 - 30, 60, 24, "Close");
	btnpicTxClose->callback(cb_picTxClose, who);
	btnpicTxSendAbort->hide();
	btnpicTxSendColor->deactivate();
	btnpicTxSendGrey->deactivate();
	FL_UNLOCK_D();
}

void TxViewerResize(int W, int H)
{
	int winW, winH;
	int picX, picY;
	winW = W < 246 ? 250 : W + 4;
	winH = H < 180 ? 180 : H + 30;
	picX = (winW - W) / 2;
	picY =  (winH - 30 - H)/2;
	FL_LOCK_D();
	picTxWin->size(winW, winH);
	picTx->resize(picX, picY, W, H);
	picTx->clear();
	btnpicTxSendColor->resize(winW/2 - 123, winH - 28, 60, 24);
	btnpicTxSendGrey->resize(winW/2 - 61, winH - 28, 60, 24);
	btnpicTxSendAbort->resize(winW/2 - 123, winH - 28, 122, 24);
	btnpicTxLoad->resize(winW/2 + 1, winH - 28, 60, 24);
	btnpicTxClose->resize(winW/2 + 63, winH - 28, 60, 24);
	FL_UNLOCK_D();
}

void showTxViewer(int W, int H, mfsk *who)
{
	if (picTxWin == 0) 
		createTxViewer(who);
	int winW, winH;
	int picX, picY;
	winW = W < 246 ? 250 : W + 4;
	winH = H < 180 ? 180 : H + 30;
	picX = (winW - W) / 2;
	picY =  2;
	FL_LOCK_D();
	picTxWin->size(winW, winH);
	picTx->resize(picX, picY, W, H);
	btnpicTxSendColor->resize(winW/2 - 123, winH - 28, 60, 24);
	btnpicTxSendGrey->resize(winW/2 - 61, winH - 28, 60, 24);
	btnpicTxSendAbort->resize(winW/2 - 123, winH - 28, 122, 24);
	btnpicTxLoad->resize(winW/2 + 1, winH - 28, 60, 24);
	btnpicTxClose->resize(winW/2 + 63, winH - 28, 60, 24);
	btnpicTxSendColor->show();
	btnpicTxSendGrey->show();
	btnpicTxLoad->show();
	btnpicTxClose->show();
	btnpicTxSendAbort->hide();
	picTxWin->show();
	FL_UNLOCK_D();
}

void deleteTxViewer()
{
	if (picTxWin == 0) return
	delete picTxWin;
	picTxWin = 0;	
}

void deleteRxViewer()
{
	if (picRxWin == 0) return
	delete picRxWin;
	picRxWin = 0;
}

int print_time_left(size_t bytes, char *str, size_t len,
			  const char *prefix, const char *suffix)
{
	float time_sec = bytes * 0.001;
	int time_min = (int)(time_sec / 60);
	time_sec -= time_min * 60;

	if (time_min)
		return snprintf(str, len, "%s%02dm%2.1fs%s",
				prefix, time_min, time_sec, suffix);
	else
		return snprintf(str, len, "%s%2.1fs%s", prefix, time_sec, suffix);
}
