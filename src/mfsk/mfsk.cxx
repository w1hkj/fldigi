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

#include <stdlib.h>
#include <iostream>

#include "mfsk.h"
#include "modem.h"
#include "configuration.h"
#include "trx.h"

#include "ascii.h"

#include "File_Selector.h"

#undef  MAX
#define MAX(a,b)		(((a)>(b))?(a):(b))
#undef  CLAMP
#define CLAMP(x,low,high)       (((x)>(high))?(high):(((x)<(low))?(low):(x)))

#define AFC_COUNT	32


char mfskmsg[80];

void  mfsk::tx_init(cSound *sc)
{
	scard = sc;
	txstate = TX_STATE_PREAMBLE;
	bitstate = 0;
	counter = 0;
	if (trx_state != STATE_TUNE && progdefaults.sendid == true)
		wfid->transmit(mode);
	else if (trx_state != STATE_TUNE && progdefaults.macroid == true) {
		wfid->transmit(mode);
		progdefaults.macroid = false;
	}
}

void  mfsk::rx_init()
{
	rxstate = RX_STATE_DATA;
	synccounter = 0;
	symcounter = 0;
	met1 = 0.0;
	met2 = 0.0;
	counter = 0;
	for (int i = 0; i < 2 * symlen; i++)
		for (int j = 0; j < 32; j++)
			(pipe[i].vector[j]).re = (pipe[i].vector[j]).im = 0.0;
	reset_afc();
	s2n = 0.0;
	put_MODEstatus(mode);
}

void mfsk::init()
{
	modem::init();
	rx_init();
	digiscope->mode(Digiscope::SCOPE);
}

mfsk::~mfsk()
{
	if (scopedata) delete [] scopedata;
	if (bpfilt) delete bpfilt;
	if (rxinlv) delete rxinlv;
	if (txinlv) delete txinlv;
	if (dec2) delete dec2;
	if (dec1) delete dec1;
	if (enc) delete enc;
	if (pipe) delete [] pipe;
	if (hbfilt) delete hbfilt;
	if (binsfft) delete binsfft;
	if (wfid) delete wfid;
}

mfsk::mfsk(trx_mode mfsk_mode) : modem()
{
	double bw, cf, flo, fhi;
	mode = mfsk_mode;

	switch (mode) {

	case MODE_MFSK8:
		symlen =  1024;
		symbits =    5;
		basetone = 128;		/* 1000 Hz */
		break;
	case MODE_MFSK16:
	default:
		symlen =  512;
		symbits =   4;
		basetone = 64;		/* 1000 Hz */
        break;
	}

	numtones = 1 << symbits;
	tonespacing = (double) MFSKSampleRate / symlen;

	binsfft		= new sfft (symlen, basetone, basetone + numtones + 3);
	hbfilt		= new C_FIR_filter();
	hbfilt->init_hilbert(37, 1);
	afcfilt		= new Cmovavg(AFC_COUNT);

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

	flo = (cf - bw) / MFSKSampleRate;
	fhi = (cf + bw) / MFSKSampleRate;

	bpfilt = new C_FIR_filter();
	bpfilt->init_bandpass (127, 1, flo, fhi);

	scopedata = new double [symlen * 2];

	samplerate = MFSKSampleRate;
	fragmentsize = symlen;
	bandwidth = (numtones - 1) * tonespacing;
	wfid = new id(this);
	
	picRxWin = 0;
	picRxBox = 0;
	picRx = 0;
	picTxWin = 0;
	picTx = 0;
	startpic = false;
	abortxmt = false;
	bitshreg = 0;
	bitstate = 0;
	phaseacc = 0;
	pipeptr = 0;
	squelch = 0;
	metric = 0;

	init();
}

void mfsk::shutdown()
{
	stopflag = true;
	if (picTxWin)
		picTxWin->hide();
}


//=====================================================================
// receive processing
//=====================================================================


bool mfsk::check_picture_header()
{
	char *p;

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
			updateRxPic( byte, pixelnbr);
			if (++col == picW) {
				col = 0;
				if (++rgb == 3) {
					rgb = 0;
					row += 3 * picW;
				}
			}
		} else {
			updateRxPic( byte, pixelnbr++ );
			updateRxPic( byte, pixelnbr++ );
			updateRxPic( byte, pixelnbr++ );
		}			
		picf = 0.0;
	}
}

void mfsk::recvchar(int c)
{
	if (c == -1)
		return;

	memmove(picheader, picheader + 1, sizeof(picheader) - 1);

	picheader[sizeof(picheader) - 2] = (c == 0) ? ' ' : c;
	picheader[sizeof(picheader) - 1] = 0;

	if (check_picture_header() == true) {
		if (symbolbit == 4) {
			rxstate = RX_STATE_PICTURE_START_1;
		}
		else {
			rxstate = RX_STATE_PICTURE_START_2;
		}
		
		picturesize = SAMPLES_PER_PIXEL * picW * picH * (color ? 3 : 1);
		counter = 0;
		
		makeRxViewer(picW, picH);
		pixelnbr = 0;
		col = 0;
		row = 0;
		rgb = 0;

		memset(picheader, ' ', sizeof(picheader));
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

	/* MFSK16 doesn't need a vote */
	if (mode == MODE_MFSK16 && symcounter)
		return;

	if (symcounter) {
		if ((c = dec1->decode(symbolpair, &met)) == -1)
			return;

		met1 = decayavg(met1, met, 32.0);

		if (met1 < met2)
			return;

		metric = met1 / 2.5;
	} else {
		if ((c = dec2->decode(symbolpair, &met)) == -1)
			return;

		met2 = decayavg(met2, met, 32.0);

		if (met2 < met1)
			return;

		metric = met2 / 2.5;
	}
	display_metric(metric);
	
	if (squelchon && metric < squelch)
		return;

	recvbit(c);

}

void mfsk::softdecode(complex *bins)
{
	double tone, sum, *b;
	unsigned char *symbols;
	int i, j, k;

	b		= new double [symbits];
	symbols = new unsigned char[symbits];

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

// shift to range 0...260 
	for (i = 0; i < symbits; i++)
		symbols[i] = (unsigned char)clamp(128.0 + (b[i] / sum * 128.0), 0, 260);

	rxinlv->symbols(symbols);

	for (i = 0; i < symbits; i++) {
		symbolbit = i + 1;
		decodesymbol(symbols[i]);
	}
	delete [] b;
	delete [] symbols;
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
	double x, max = 0.0;
	int i, symbol = 0;

	for (i = 0; i < numtones; i++) {
		x = in[i].mag();
		if ( x > max) {
			max = x;
			symbol = i;
		}
	}
	return symbol;
}

void mfsk::update_syncscope()
{
	int j;
	memset(scopedata, 0, 2 * symlen * sizeof(double));
	if (!squelchon || metric >= squelch)
		for (int i = 0; i < 2 * symlen; i++) {
			j = (i + pipeptr) % (2 * symlen);
			scopedata[i] = (pipe[j].vector[prev1symbol]).mag();
		}
	set_scope(scopedata, 2 * symlen);
	sprintf(mfskmsg, "s/n %3.0f dB", 20.0 * log10(s2n) );
	put_Status1(mfskmsg);
}

void mfsk::synchronize()
{
	int i, j, syn = -1;
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

	synccounter += (int) floor((syn - symlen) / numtones + 0.5); //16.0 + 0.5);
}

void mfsk::reset_afc() {
	freqerr = 0.0;
	for (int i = 0; i < AFC_COUNT; i++) afcfilt->run(0.0);
	return;
}

void mfsk::afc()
{
	complex z;
	complex prevvector;
	double f;

	if (sigsearch) {
		reset_afc();
		sigsearch = 0;
	}
	
	if (pipeptr == 0)
		prevvector = pipe[2*symlen - 1].vector[currsymbol];
	else
		prevvector = pipe[pipeptr - 1].vector[currsymbol];
	
	z = prevvector % currvector;

	f = z.arg() * samplerate / twopi;
	f -= (1000 + tonespacing * currsymbol);

	if (afcon && (metric > squelch || squelchon == false)) {
		if (fabs(f) <= tonespacing / 2.0)
			freqerr = afcfilt->run(f / numtones);
		set_freq(frequency + freqerr);
	}
}

void mfsk::eval_s2n(complex c, complex n)
{
	sig = c.mag(); // signal + noise energy
	noise = n.mag() + 1e-10; // noise energy

	s2n = decayavg( s2n, fabs((sig - noise) / noise), 8);
}

int mfsk::rx_process(double *buf, int len)
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
				rxstate = RX_STATE_DATA;
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
			eval_s2n(currvector, bins[numtones + 2]);
// decode symbol 
			softdecode(bins);
// symbol sync 
			synchronize();
// update the scope
			update_syncscope();

			prev2symbol = prev1symbol;
			prev2vector = prev1vector;
			prev1symbol = currsymbol;
			prev1vector = currvector;
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

	f = tx_frequency - bandwidth / 2;
	
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
	char *code = varienc(c);
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
			updateTxPic(data[i]);
		if (reverse)
			f = tx_frequency - bandwidth * (data[i] - 128) / 256.0;
		else
			f = tx_frequency + bandwidth * (data[i] - 128) / 256.0;
			
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
			} else if (xmtbyte == -1)
				sendidle();
			else if ( xmtbyte == 0x03 || stopflag) {
				txstate = TX_STATE_FLUSH;
			}
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
				sprintf(mfskmsg,"Send picture: %.1f %%", (100.0 * i) / xmtbytes);
				put_status(mfskmsg);
			}
			txstate = TX_STATE_DATA;
			put_status("Send picture: done");
			Fl::lock();
			btnpicTxSendAbort->hide();
			btnpicTxSendColor->show();
			btnpicTxSendGrey->show();
			btnpicTxLoad->show();
			btnpicTxClose->show();
			abortxmt = false;
			rxstate = RX_STATE_DATA;
			counter = 0;
			memset(picheader, ' ', sizeof(picheader));
			Fl::unlock();
			break;
	}

	return 0;
}



void mfsk::updateRxPic(unsigned char data, int pos)
{
	picRx->pixel(data, pos);
}

void cb_picRxClose( Fl_Widget *w, void *who)
{
	mfsk *me = (mfsk *)who;
//	Fl::lock();
	me->picRxWin->hide();
	me->rxstate = mfsk::RX_STATE_DATA;
//	Fl::unlock();
}

void cb_picRxSave( Fl_Widget *w, void *who)
{
	mfsk *me = (mfsk *)who;
	char *fn = 
		File_Select("Save Image file?","*.{gif,jpg,png}", "", 0);
	if (!fn) return;
	me->picRx->save_jpeg(fn);
}

void mfsk::makeRxViewer(int W, int H)
{
	int winW, winH;
	int picX, picY;
	winW = W < 136 ? 140 : W + 4;
	winH = H + 34;
	picX = (winW - W) / 2;
	picY = 2;
	Fl::lock();
	if (!picRxWin) {
		picRxWin = new Fl_Window(winW, winH);
		picRx = new picture(picX, picY, W, H);
		btnpicRxSave = new Fl_Button(winW/2 - 65, H + 6, 60, 24,"Save");
		btnpicRxSave->callback(cb_picRxSave,this);
		btnpicRxClose = new Fl_Button(winW/2 + 5, H + 6, 60, 24, "Close");
		btnpicRxClose->callback(cb_picRxClose,this);
	} else {
		picRxWin->size(winW, winH);
		picRx->resize(picX, picY, W, H);
		btnpicRxSave->resize(winW/2 - 65, H + 6, 60, 24);
		btnpicRxClose->resize(winW/2 + 5, H + 6, 60, 24);
		picRx->clear();
	}
	picRxWin->show();
	Fl::unlock();
}

void mfsk::load_file(const char *n) {
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
	Fl::lock();
	picTx->video(xmtimg, W * H * 3);
	btnpicTxSendColor->activate();
	btnpicTxSendGrey->activate();
	Fl::unlock();
}

void mfsk::updateTxPic(unsigned char data)
{
	if (color) {
		pixelnbr = rgb + row + 3*col;
		picTx->pixel(data, pixelnbr);
		if (++col == TxImg->w()) {
			col = 0;
			if (++rgb == 3) {
				rgb = 0;
				row += 3 * TxImg->w();
			}
		}
	} else {
		picTx->pixel( data, pixelnbr++ );
		picTx->pixel( data, pixelnbr++ );
		picTx->pixel( data, pixelnbr++ );
	}
}

void cb_picTxLoad(Fl_Widget *,void *who) {
	mfsk *TxWho = (mfsk *)who;
	char *fn = 
		File_Select("Image file?","*.{gif,jpg,png}", "", 0);
	if (!fn) return;
	TxWho->load_file(fn);
}

void cb_picTxClose( Fl_Widget *w, void *who)
{
	mfsk *me = (mfsk *)who;
	Fl::lock();
	me->picTxWin->hide();
	Fl::unlock();
}

void cb_picTxSendColor( Fl_Widget *w, void *who)
{
	mfsk *my = (mfsk *)who;
	int W, H, rowstart;
	W = my->TxImg->w();
	H = my->TxImg->h();
	if (my->xmtpicbuff) delete [] my->xmtpicbuff;
	my->xmtpicbuff = new unsigned char [W*H*3];
	unsigned char *outbuf = my->xmtpicbuff;
	unsigned char *inbuf = my->xmtimg;
	int iy, ix, rgb;
	for (iy = 0; iy < H; iy++) {
		rowstart = iy * W * 3;
		for (rgb = 0; rgb < 3; rgb++)
			for (ix = 0; ix < W; ix++)
				outbuf[rowstart + rgb*W + ix] = inbuf[rowstart + rgb + ix*3];
	}
	sprintf(my->picheader, "\nSending Pic:%dx%dC;", W, H);
	my->xmtbytes = W * H * 3;
	my->color = true;
	my->rgb = 0;
	my->col = 0;
	my->row = 0;
	my->pixelnbr = 0;
	Fl::lock();
	my->btnpicTxSendColor->hide();
	my->btnpicTxSendGrey->hide();
	my->btnpicTxLoad->hide();
	my->btnpicTxClose->hide();
	my->btnpicTxSendAbort->show();
	my->picTx->clear();
	Fl::unlock();
// start the transmission
	fl_lock(&trx_mutex);
	if (trx_state != STATE_TX)
		trx_state = STATE_TX;
	fl_unlock(&trx_mutex);
	wf->set_XmtRcvBtn(true);
	my->startpic = true;
}

void cb_picTxSendGrey( Fl_Widget *w, void *who)
{
	mfsk *my = (mfsk *)who;
	int W, H;
	W = my->TxImg->w();
	H = my->TxImg->h();
	if (my->xmtpicbuff) delete [] my->xmtpicbuff;
	my->xmtpicbuff = new unsigned char [W*H];
	unsigned char *outbuf = my->xmtpicbuff;
	unsigned char *inbuf = my->xmtimg;
	for (int i = 0; i < W*H; i++)
		outbuf[i] = ( 31 * inbuf[i*3] + 61 * inbuf[i*3 + 1] + 8 * inbuf[i*3 + 2])/100;
	sprintf(my->picheader, "\nSending Pic:%dx%d;", W, H);
	my->xmtbytes = W * H;
	my->color = false;
	my->col = 0;
	my->row = 0;
	my->pixelnbr = 0;
	my->picTx->clear();
	Fl::lock();
	my->btnpicTxSendColor->hide();
	my->btnpicTxSendGrey->hide();
	my->btnpicTxLoad->hide();
	my->btnpicTxClose->hide();
	my->btnpicTxSendAbort->show();
	Fl::unlock();
// start the transmission
	fl_lock(&trx_mutex);
	if (trx_state != STATE_TX)
		trx_state = STATE_TX;
	fl_unlock(&trx_mutex);
	wf->set_XmtRcvBtn(true);
	my->startpic = true;
}


void cb_picTxSendAbort( Fl_Widget *w, void *who)
{
	mfsk *my = (mfsk *)who;
	my->abortxmt = true;
// reload the picture widget with the rgb image
	Fl::lock();
	my->picTx->video(my->xmtimg, my->TxImg->w() * my->TxImg->h() * 3);
	Fl::unlock();
}

void mfsk::TxViewerResize(int W, int H)
{
	int winW, winH;
	int picX, picY;
	winW = W < 246 ? 250 : W + 4;
	winH = H < 180 ? 180 : H + 30;
	picX = (winW - W) / 2;
	picY =  (winH - 30 - H)/2;
	Fl::lock();
	picTxWin->size(winW, winH);
	picTx->resize(picX, picY, W, H);
	picTx->clear();
	btnpicTxSendColor->resize(winW/2 - 123, winH - 28, 60, 24);
	btnpicTxSendGrey->resize(winW/2 - 61, winH - 28, 60, 24);
	btnpicTxSendAbort->resize(winW/2 - 123, winH - 28, 122, 24);
	btnpicTxLoad->resize(winW/2 + 1, winH - 28, 60, 24);
	btnpicTxClose->resize(winW/2 + 63, winH - 28, 60, 24);
	Fl::unlock();
}

void mfsk::makeTxViewer(int W, int H)
{
	int winW, winH;
	int picX, picY;
	winW = W < 246 ? 250 : W + 4;
	winH = H < 180 ? 180 : H + 30;
	picX = (winW - W) / 2;
	picY =  2;
	Fl::lock();
	if (!picTxWin) {
		picTxWin = new Fl_Window(winW, winH);
		picTx = new picture (picX, picY, W, H);
		btnpicTxSendColor = new Fl_Button(winW/2 - 123, winH - 28, 60, 24, "XmtClr");
		btnpicTxSendColor->callback(cb_picTxSendColor, this);
		btnpicTxSendGrey = new Fl_Button(winW/2 - 61, winH - 28, 60, 24, "XmtGry");
		btnpicTxSendGrey->callback( cb_picTxSendGrey, this);
		btnpicTxSendAbort = new Fl_Button(winW/2 - 123, winH - 28, 122, 24, "Abort Xmt");
		btnpicTxSendAbort->callback(cb_picTxSendAbort, this);
		btnpicTxLoad = new Fl_Button(winW/2 + 1, winH - 28, 60, 24, "Load");
		btnpicTxLoad->callback(cb_picTxLoad, this);
		btnpicTxClose = new Fl_Button(winW/2 + 63, winH - 28, 60, 24, "Close");
		btnpicTxClose->callback(cb_picTxClose,this);
		btnpicTxSendAbort->hide();
		btnpicTxSendColor->deactivate();
		btnpicTxSendGrey->deactivate();
	} else {
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
	}
	picTxWin->show();
	Fl::unlock();
}

