//
//  Waterfall Spectrum Analyzer Widget
//
// Copyright W1HKJ, Dave Freese 2006
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "w1hkj@w1hkj.com".
//


//#define USE_BLACKMAN
//#define USE_HAMMING
//#define USE_HANNING

#include "waterfall.h"
#include "threads.h"
#include "main.h"
#include "modem.h"

#ifndef NOHAMLIB
	#include "hamlib.h"
#endif
#include "rigMEM.h"
#include "rigio.h"

#include "config.h"
#include "configuration.h"

Fl_Mutex	wf_mutex = PTHREAD_MUTEX_INITIALIZER;

extern modem *active_modem;

static	RGB RGByellow	= {254,254,0};
//static	RGB RGBgreen	= {0,254,0};
//static	RGB RGBdkgreen	= {0,128,0};
//static	RGB RGBblue		= {0,0,255};
static	RGB RGBred		= {254,0,0};
//static	RGB RGBwhite	= {254,254,254};
//static	RGB RGBblack	= {0,0,0};
//static RGB RGBmagenta = {196,0,196};
//static RGB RGBblack   = {0,0,0};

RGBI	mag2RGBI[256];
static RGBI RGBIwhite	= {255,255,255,255};
static RGBI RGBIred		= {255, 0, 0, 255};
static RGBI RGBIyellow	= {255,255,0,255};

RGB	palette[9];

#define max(a,b) (a)>(b)?(a):(b)
#define min(a,b) (a)<(b)?(a):(b)


WFdisp::WFdisp (int x0, int y0, int w0, int h0, char *lbl) :
			  Fl_Widget(x0,y0,w0,h0,"") {
	disp_width = w();
	if (disp_width > IMAGE_WIDTH/4)
		disp_width = IMAGE_WIDTH/4;
	scale_width = IMAGE_WIDTH + 1000;
	image_height = h() - WFTEXT - WFSCALE - WFMARKER;
    image_area      = IMAGE_WIDTH * image_height;
    sig_image_area  = IMAGE_WIDTH * h();
	RGBsize			= sizeof(RGB);
	RGBwidth		= RGBsize * IMAGE_WIDTH;
	fft_img			= new RGBI[image_area];
	markerimage		= new RGB[IMAGE_WIDTH * WFMARKER];
	scaleimage		= new uchar[scale_width * WFSCALE];
	scline			= new uchar[scale_width];
	fft_sig_img 	= new uchar[image_area];
	sig_img			= new uchar[sig_image_area];
	pwr				= new double[IMAGE_WIDTH];
	fft_hist		= new short int[image_area];
	fft_db			= new short int[image_area];
	circbuff		= new double[FFT_LEN * 2];
	fftout			= new double[FFT_LEN * 2];
	wfft			= new Cfft(FFT_LEN);
    fftwindow       = new double[FFT_LEN * 2];
	setPrefilter(progdefaults.wfPreFilter);
	
	for (int i = 0; i < FFT_LEN*2; i++)
		circbuff[i] = fftout[i] = 0.0;

	mag = 1;
	step = 4;
	dispcolor = true;
	offset = 0;	
	sigoffset = 0;
	ampspan = 75;
	reflevel = -10;
	initmaps();
	bandwidth = 32;
	RGBmarker = RGBred;
	RGBcursor = RGByellow;
	mode = WATERFALL;
	centercarrier = false;
	overload = false;
    peakaudio = 0.0;
	rfc = 0L;
	usb = true;
	wfspeed = NORMAL;
	srate = 8000;
	wfspdcnt = 0;
	dispcnt = 4;
	wantcursor = false;
	cursormoved = false;
	usebands = false;
	for (int i = 0; i < IMAGE_WIDTH; i++)
		pwr[i] = 0.0;

	carrier(1000);
}

WFdisp::~WFdisp() {
	delete wfft;
	delete [] fft_img;
	delete [] scaleimage;
	delete [] markerimage;
	delete [] fft_sig_img;
	delete [] sig_img;
	delete [] pwr;
	delete [] scline;
	delete [] fft_hist;
	delete [] fft_db;
}

void WFdisp::initMarkers() {
	uchar *c1 = (uchar *)markerimage,
		  *c2 = c1 + RGBwidth * (WFMARKER - 1);
	memset(c1, 196, RGBwidth);
	memset(c2, 196, RGBwidth);
}

void WFdisp::makeMarker() {
	RGB *clrMin, *clrMax, *clrM, *clrPos;
	clrMin = markerimage + IMAGE_WIDTH;
	clrMax = clrMin + (WFMARKER - 2) * IMAGE_WIDTH;
	memset(clrMin, 0, RGBwidth * (WFMARKER - 2));
	
	clrM = clrMin + (int)((double)carrierfreq + 0.5);
	int bw = (int)((double) bandwidth / 2.0) + 1;
	for (int y = 0; y < WFMARKER - 2; y++) 
		for (int i = -bw; i <= bw ; i++) {
			clrPos = clrM + i + y * IMAGE_WIDTH;
			if (clrPos > clrMin && clrPos < clrMax)
				*clrPos = RGBmarker;
		}
	if (!wantcursor) return;
	
	if (cursorpos > disp_width - bandwidth / 2 / step)
		cursorpos = disp_width - bandwidth / 2 / step;
	if (cursorpos >= (IMAGE_WIDTH - offset - bandwidth/2)/step)
		cursorpos = (IMAGE_WIDTH - offset - bandwidth/2)/step - 1;
	if (cursorpos < bandwidth / 2 / step)
		cursorpos = bandwidth / 2 / step + 1;
	
// Create the cursor marker
	double xp = offset + step * cursorpos;
	if (xp < bandwidth / 2.0 || xp > (IMAGE_WIDTH - bandwidth / 2.0))
		return;
	clrM = markerimage + IMAGE_WIDTH + (int)(xp + 0.5);
	for (int y = 0; y < WFMARKER - 2; y++) {
		int incr = y * IMAGE_WIDTH;
		int msize = (WFMARKER - 2 - y)*RGBsize*step/4;
		*(clrM + incr - 1)	= 
		*(clrM + incr) 		= 
		*(clrM + incr + 1) 	= RGBcursor;
		for (int i = bw - msize; i <= bw + msize; i++) {
			*(clrM + i + incr) = 
			*(clrM - i + incr) = RGBcursor;
		}	
	}
}


void WFdisp::makeScale() {
	uchar *gmap = scaleimage;
	int hwidth = step / 2;
	memset(scline, 0, scale_width);

	for (int tic = 500; tic < scale_width; tic += 500) {
		if (hwidth)
			for (int ticn = -hwidth; ticn < hwidth; ticn++)
				scline[tic + ticn] = 255;
		else
			scline[tic] = 255;
	}
	for (int i = 0; i < WFSCALE - 5; i++) {
		memcpy(gmap, scline, scale_width);
		gmap += (scale_width);
	}

	for (int tic = 100; tic < scale_width ; tic += 100) {
		if (hwidth)
			for (int ticn = -hwidth; ticn < hwidth; ticn++)
				scline[tic + ticn] = 255;
		else
			scline[tic] = 255;
	}
	for (int i = 0; i < 5; i++) {
		memcpy(gmap, scline, scale_width);
		gmap += (scale_width);
	}
}

void WFdisp::setcolors() {
	double di;
	int r, g, b;
	for (int i = 0; i < 256; i++) {
		di = sqrt((double)i / 256.0);
		mag2RGBI[i].I = (uchar)(200*di);
	}
	for (int n = 0; n < 8; n++) {
		for (int i = 0; i < 32; i++) {
			r = palette[n].R + (int)(1.0 * i * (palette[n+1].R - palette[n].R) / 32.0);
			g = palette[n].G + (int)(1.0 * i * (palette[n+1].G - palette[n].G) / 32.0);
			b = palette[n].B + (int)(1.0 * i * (palette[n+1].B - palette[n].B) / 32.0);
			mag2RGBI[i + 32*n].R = r; 
			mag2RGBI[i + 32*n].G = g; 
			mag2RGBI[i + 32*n].B = b;
		}
	}
}


void WFdisp::initmaps() {
	for (int i = 0; i < image_area; i++) fft_hist[i] = -1000;
	for (int i = 0; i < image_area; i++) fft_db[i] = log2disp(-1000);

	memset (fft_img, 0, image_area * sizeof(RGBI) );
	memset (scaleimage, 0, scale_width * WFSCALE);
	memset (markerimage, 0, IMAGE_WIDTH * WFMARKER);
	memset (fft_sig_img, 0, image_area);
	memset (sig_img, 0, sig_image_area);
	
	memset (mag2RGBI, 0, sizeof(mag2RGBI));
	initMarkers();
	makeScale();
	setcolors();
}

int WFdisp::peakFreq(int f0, int delta)
{
	double threshold = 0.0;
	int f1, fmin =	(int)((f0 - delta)), 
		f2, fmax =	(int)((f0 + delta));
	f1 = fmin; f2 = fmax;
	if (fmin < 0 || fmax > IMAGE_WIDTH) return f0;
	for (int f = fmin; f <= fmax; f++)
		threshold += pwr[f];
	threshold /= (2*delta);
	for (int f = fmin; f <= fmax; f++)
		if (pwr[f] > threshold) {
			f2 = f;
		}
	for (int f = fmax; f >= fmin; f--)
		if (pwr[f] > threshold) {
			f1 = f;
		}
	return (f1 + f2) / 2;
}

double WFdisp::powerDensity(double f0, double bw)
{
	double pwrdensity = 0.0;
	int flower = (int)((f0 - bw/2)),
		fupper = (int)((f0 + bw/2));
	if (flower < 0 || fupper > IMAGE_WIDTH) 
		return 0.0;
	for (int i = flower; i <= fupper; i++)
		pwrdensity += pwr[i];
	return pwrdensity/(bw+1);
}

int WFdisp::log2disp(int v)
{
	double val = 255.0 * (reflevel- v) / ampspan;
	if (val < 0) val = 0;
	if (val > 255 ) val = 255;
	return (int)(255 - val);
}

void WFdisp::update_fft_db()
{
	Fl::lock();
	for (int i = 0; i < image_area; i++)
		fft_db[i] = log2disp( fft_hist[i] );
	Fl::unlock();
}

void WFdisp::processFFT() {
	int		n;
	double scale;

	scale = (double)SC_SMPLRATE / active_modem->get_samplerate();
    scale *= FFT_LEN / 2000.0;

	if (dispcnt == 0) {
        for (int i = 0; i < FFT_LEN*2; i++)
            fftout[i] = fftwindow[i] * circbuff[i];
		wfft->rdft(fftout);
Fl::lock();		
		memmove(
			(void*)(fft_hist + IMAGE_WIDTH), 
			(void*)fft_hist, 
			(IMAGE_WIDTH * (image_height - 1)) * sizeof(short int));
			
		memmove(
			(void*)(fft_db + IMAGE_WIDTH),
			(void*)fft_db,
			(IMAGE_WIDTH * (image_height - 1)) * sizeof(short int));

		double pw;
		int ffth;
		for (int i = 0; i < IMAGE_WIDTH; i++) {
			n = 2*(int)((scale * i / 2 + 0.5));
			if (i <= progdefaults.LowFreqCutoff)
				pw = 0.0;
			else
				pw = fftout[n]*fftout[n];
			pwr[i] = pw;
			ffth = fft_hist[i] = (int)(10.0 * log10(pw + 1e-10) );
			fft_db[i] = log2disp(ffth);
		}
Fl::unlock();
	}
	if (cursormoved || dispcnt == 0) {
Fl::lock();
		redraw();
Fl::unlock();
		cursormoved = false;
	}
	if (dispcnt == 0)
		dispcnt = wfspeed * (srate > 8000 ? 2 : 1);
	--dispcnt;
}

void WFdisp::process_analog (double *sig, int len) {
	int h2, sigw, sigy, sigpixel, ynext, graylevel;
	h2 = h()/2 - 1;
	sigw = IMAGE_WIDTH;
	graylevel = 220;
// clear the signal display area
	sigy = 0;
	sigpixel = IMAGE_WIDTH*h2;
Fl::lock();
	memset (sig_img, 0, sig_image_area);
	memset (&sig_img[h2*IMAGE_WIDTH], 255, IMAGE_WIDTH);
	for (int c = 0; c < IMAGE_WIDTH; c++) {
		ynext = (int)(h2 * sig[c]);
		for (; sigy < ynext; sigy++) sig_img[sigpixel -= IMAGE_WIDTH] = graylevel;
		for (; sigy > ynext; sigy--) sig_img[sigpixel += IMAGE_WIDTH] = graylevel;
		sig_img[sigpixel++] = graylevel;
	}
//	inpFreq->redraw();
	redraw();
Fl::unlock();
}

void WFdisp::redrawCursor()
{
	cursormoved = true;
}

void WFdisp::sig_data( double *sig, int len ) {
	int   movedbls = (FFT_LEN * 2) - len; //SCBLOCKSIZE;
	int	  movesize = movedbls * sizeof(double);
	double *pcircbuff1 = &circbuff[len];
	
//if sound card sampling rate changed reset the waterfall buffer
	if (srate != active_modem->get_samplerate()) {
		srate = active_modem->get_samplerate();
		memset (circbuff, 0, FFT_LEN * 2 * sizeof(double));
	}
	else
		memmove(circbuff, pcircbuff1, movesize);
	overload = false;
	int i, j;
    double overval, peak = 0.0;
	for (i = movedbls, j = 0; j < len; i++, j++) {
        overval = fabs(circbuff[i] = sig[j]);
        if (overval > peak) peak = overval;
        peakaudio = 0.05 * peak + 0.95 * peakaudio;
    }

	static char szFrequency[14];
	
	if (mode == SCOPE)
		process_analog(circbuff, FFT_LEN * 2);

	processFFT();

	Fl::lock();
	if (usebands)
		rfc = (long long)(atof(cboBand->value()) * 1000.0);
	if (rfc) {
		if (usb)
			dfreq = rfc + active_modem->get_txfreq();
		else	
			dfreq = rfc - active_modem->get_txfreq();
		sprintf(szFrequency, "%-.3f", dfreq / 1000.0);
	} else {
		dfreq = active_modem->get_txfreq();
		sprintf(szFrequency, "%-.0f", dfreq);
	}
	
	inpFreq->value(szFrequency);
	Fl::unlock();
	Fl::awake();

// signal level indicator
	put_WARNstatus(peakaudio);

	return;
}



// Check the display offset & limit to 0 to max IMAGE_WIDTH displayed
void WFdisp::checkoffset() {
	if (mode == SCOPE) {
		if (sigoffset < 0)
			sigoffset = 0;
		if (sigoffset > (IMAGE_WIDTH - disp_width))
			sigoffset = IMAGE_WIDTH - disp_width;
	} else {
		if (offset < 0)
			offset = 0;
		if (offset > (int)(IMAGE_WIDTH - step * disp_width))
			offset = (int)(IMAGE_WIDTH - step * disp_width);
	}
}

void WFdisp::slew(int dir) {
	if (mode == SCOPE)
		sigoffset += dir;
	else
		offset += dir;
	checkoffset();
}

void WFdisp::movetocenter() {
	if (mode == SCOPE)
		sigoffset = IMAGE_WIDTH / 2;
	else
		offset = carrierfreq - (disp_width * step / 2);
	checkoffset();
}

void WFdisp::carrier(int cf) {
	if (cf > bandwidth / 2 && cf < (IMAGE_WIDTH - bandwidth / 2)) {
		carrierfreq = cf;
		makeMarker();
	}
}

int WFdisp::carrier() {
	return carrierfreq;
}

void WFdisp::checkWidth()
{
	disp_width = w();
	if (mag == MAG_1) step = 4;
	if (mag == MAG_1 && disp_width > IMAGE_WIDTH/4)
		disp_width = IMAGE_WIDTH/4;
	if (mag == MAG_2) step = 2;
	if (mag == MAG_2 && disp_width > IMAGE_WIDTH/2)
		disp_width = IMAGE_WIDTH/2;
	if (mag == MAG_4) step = 1;
}

int WFdisp::checkMag()
{
	checkWidth();
	makeScale();
	return mag;
}

int WFdisp::setMag(int m)
{
	int mid = offset + (disp_width * step / 2);
	mag = m;
	checkMag();
	if (centercarrier || Fl::event_shift()) {
		offset = mid - (disp_width * step / 2);
	}
	else {
		movetocenter();
	}
	return mag;
}

int WFdisp::wfmag() {
	int mid = offset + (disp_width * step / 2);
	if (mag == MAG_1) mag = MAG_2;
	else if (mag == MAG_2) mag = MAG_4;
	else mag = MAG_1;
	checkMag();
	if (centercarrier || Fl::event_shift()) {
		offset = mid - (disp_width * step / 2);
	}
	else {
		movetocenter();
	}
	return mag;
}


void WFdisp::drawScale() {
	int fw = 60, xchar;
	static char szFreq[20];
	double fr;
	uchar *pixmap;
	
	if (usb || !rfc)
		pixmap = (scaleimage +  (int)((rfc % 1000 + offset)) );
	else
		pixmap = (scaleimage + (int)((1000 - rfc % 1000 + offset)));
	
	fl_draw_image_mono(
		pixmap, 
		x(), y() + WFTEXT, 
		disp_width, WFSCALE, 
		step, scale_width);
		
	fl_color(FL_BLACK);
	fl_rectf(x(), y(), disp_width, WFTEXT);
	
	fl_color(fl_rgb_color(228));
	fl_font(FL_SCREEN, 12);
	for (int i = 1; i < 10; i++) {
		if (rfc == 0)
			fr = 500.0 * i;
		else {
			if (usb)
				fr = (rfc - (rfc%500))/1000.0 + 0.5*i;
			else
				fr = (rfc - (rfc %500))/1000.0 + 0.5 - 0.5*i;
		}
		sprintf(szFreq,"%7.1f", fr);
		fw = (int)fl_width(szFreq);
		if (usb)
			xchar = (int) ( ( (1000.0/step) * i - fw) / 2.0 - 
							(offset + rfc % 500) /step );
		else
			xchar = (int) ( ( (1000.0/step) * i - fw) / 2.0 - 
							(offset + 500 - rfc % 500) /step );
		if (xchar > 0 && (xchar + fw) < disp_width)
			fl_draw(szFreq, x() + xchar, y() + 10 );
	}
}

void WFdisp::drawMarker() {
	if (mode == SCOPE) return;
	uchar *pixmap = (uchar *)(markerimage + (int)(offset));
	fl_draw_image(
		pixmap, 
		x(), y() + WFSCALE + WFTEXT, 
		disp_width, WFMARKER, 
		step * RGBsize, RGBwidth);
}

void WFdisp::update_waterfall() {
// transfer the fft history data into the WF image
	int sig;
	short int *p1, *p2;
	RGBI *p3, *p4;
	p1 = fft_db + offset;
	p3 = fft_img;
	for (int row = 0; row < image_height; row++) {
		p2 = p1;
		p4 = p3;
		for (int col = 0; col < disp_width; col++) {
			if (step == 4)
				sig = max( max ( max ( *p2, *(p2+1) ), *(p2+2) ), *(p2+3) );
			else if (step == 2)
				sig = max( *p2, *(p2 + 1) );
			else 
				sig = *p2;
			*p4 = mag2RGBI[ sig ];
			p2 += step;
			p4++;
		}
		
		p1 += IMAGE_WIDTH;
		p3 += disp_width;
	}
	if (progdefaults.UseBWTracks) {
		RGBI  *pos1 = fft_img + (carrierfreq - offset - bandwidth/2) / step;
		RGBI  *pos2 = fft_img + (carrierfreq - offset + bandwidth/2) / step;
		if (pos1 >= fft_img && pos2 < fft_img + disp_width)
			for (int y = 0; y < image_height; y ++) {
				*pos1 = *pos2 = RGBIred;
				pos1 += disp_width;
				pos2 += disp_width;
			}
	}
}

void WFdisp::drawcolorWF() {
	uchar *pixmap = (uchar *)fft_img;
	fl_color(FL_BLACK);
	fl_rectf(x() + disp_width, y(), w() - disp_width, h());
	update_waterfall();

	if (wantcursor && (progdefaults.UseCursorLines || progdefaults.UseCursorCenterLine) ) {
		RGBI  *pos0 = (fft_img + cursorpos);
		RGBI  *pos1 = (fft_img + cursorpos - bandwidth/2/step);
		RGBI  *pos2 = (fft_img + cursorpos + bandwidth/2/step);
		if (pos1 >= fft_img && pos2 < fft_img + disp_width)
			for (int y = 0; y < image_height; y ++) {
				if (progdefaults.UseCursorLines)
					*pos1 = *pos2 = RGBIwhite;
				if (progdefaults.UseCursorCenterLine)
					*pos0 = RGBIyellow;
				pos0 += disp_width;
				pos1 += disp_width;
				pos2 += disp_width;
			}
	}

	fl_draw_image(
		pixmap, x(), y() + WFSCALE + WFMARKER + WFTEXT, 
		disp_width, image_height, 
		sizeof(RGBI), disp_width * sizeof(RGBI) );
	drawScale();
}

void WFdisp::drawgrayWF() {
	uchar *pixmap = (uchar*)fft_img;
	fl_color(FL_BLACK);
	fl_rectf(x() + disp_width, y(), w() - disp_width, h());
	update_waterfall();

	if (wantcursor && (progdefaults.UseCursorLines || progdefaults.UseCursorCenterLine) ) {
		RGBI  *pos0 = (fft_img + cursorpos);
		RGBI  *pos1 = (fft_img + cursorpos - bandwidth/2/step);
		RGBI  *pos2 = (fft_img + cursorpos + bandwidth/2/step);
		if (pos1 >= fft_img && pos2 < fft_img + disp_width)
			for (int y = 0; y < image_height; y ++) {
				if (progdefaults.UseCursorLines)
					*pos1 = *pos2 = RGBIwhite;
				if (progdefaults.UseCursorCenterLine)
					*pos0 = RGBIyellow;
				pos0 += disp_width;
				pos1 += disp_width;
				pos2 += disp_width;
			}
	}

	fl_draw_image_mono(
		pixmap + 3, 
		x(), y() + WFSCALE + WFMARKER + WFTEXT, 
		disp_width, image_height, 
		sizeof(RGBI), disp_width * sizeof(RGBI));
	drawScale();
}

void WFdisp::drawspectrum() {
	int sig;
	int ynext,
		h1 = image_height - 1,
		ffty = 0,
		fftpixel = IMAGE_WIDTH * h1,
		graylevel = 220;
	uchar *pixmap = (uchar *)fft_sig_img + offset / step;

	memset (fft_sig_img, 0, image_area);

	fftpixel /= step;
	for (int c = 0; c < IMAGE_WIDTH; c += step) {
		sig = fft_db[c];
		if (step == 1)
			sig = fft_db[c];
		else if (step == 2)
			sig = max(fft_db[c], fft_db[c+1]);
		else
			sig = max( max ( max ( fft_db[c], fft_db[c+1] ), fft_db[c+2] ), fft_db[c+3]);
		ynext = h1 * sig / 256;
		while (ffty < ynext) { fft_sig_img[fftpixel -= IMAGE_WIDTH/step] = graylevel; ffty++;}
		while (ffty > ynext) { fft_sig_img[fftpixel += IMAGE_WIDTH/step] = graylevel; ffty--;}
		fft_sig_img[fftpixel++] = graylevel;
	}

	if (progdefaults.UseBWTracks) {
		uchar  *pos1 = pixmap + (carrierfreq - offset - bandwidth/2) / step;
		uchar  *pos2 = pixmap + (carrierfreq - offset + bandwidth/2) / step;
		if (pos1 >= pixmap && pos2 < pixmap + disp_width)
			for (int y = 0; y < image_height; y ++) {
				*pos1 = *pos2 = 255;
				pos1 += IMAGE_WIDTH/step;
				pos2 += IMAGE_WIDTH/step;
			}
	}
	if (wantcursor && (progdefaults.UseCursorLines || progdefaults.UseCursorCenterLine)) {
		uchar  *pos0 = pixmap + cursorpos;
		uchar  *pos1 = (pixmap + cursorpos - bandwidth/2/step);
		uchar  *pos2 = (pixmap + cursorpos + bandwidth/2/step);
		for (int y = 0; y < h1; y ++) {
			if (progdefaults.UseCursorLines)
				*pos1 = *pos2 = 255;
			if (progdefaults.UseCursorCenterLine)
				*pos0 = 255;
			pos0 += IMAGE_WIDTH/step;
			pos1 += IMAGE_WIDTH/step;
			pos2 += IMAGE_WIDTH/step;
		}
	}
	
	fl_color(FL_BLACK);
	fl_rectf(x() + disp_width, y(), w() - disp_width, h());
	
	fl_draw_image_mono(
		pixmap, 
		x(), y() + WFSCALE + WFMARKER + WFTEXT, 
		disp_width, image_height, 
		1, IMAGE_WIDTH / step);
	drawScale();
}

void WFdisp::drawsignal() {
	uchar *pixmap = (uchar *)(sig_img + sigoffset);

	fl_color(FL_BLACK);
	fl_rectf(x() + disp_width, y(), w() - disp_width, h());
	fl_draw_image_mono(pixmap, x(), y(), disp_width, h(), 1, IMAGE_WIDTH);
}

void WFdisp::draw() {
	checkoffset();
	checkWidth();
	switch (mode) {
	case SPECTRUM :
			drawspectrum();
		drawMarker();
		break;
	case SCOPE :
		drawsignal();
		break;
	case WATERFALL :
	default:
			if (dispcolor)
				drawcolorWF();
			else
				drawgrayWF();
		drawMarker();
	}
}

//=======================================================================
// waterfall
//=======================================================================

void x1_cb(Fl_Widget *w, void* v) {
	waterfall *wf = (waterfall *)w->parent();
	int m = wf->wfdisp->wfmag();
	if (m == MAG_1) w->label("x1");
	if (m == MAG_2) w->label("x2");
	if (m == MAG_4) w->label("x4");
	restoreFocus();
}

void bwclr_cb(Fl_Widget *w, void * v) {
	waterfall *wf = (waterfall *)w->parent();
	wf->wfdisp->DispColor(!wf->wfdisp->DispColor());
	if (wf->wfdisp->DispColor() == 0)
		w->label("gry");
	else
		w->label("clr");
	restoreFocus();
}

void slew_left(Fl_Widget *w, void * v) {
	waterfall *wf = (waterfall *)w->parent();
	wf->wfdisp->slew(-100);
	restoreFocus();
} 

void slew_right(Fl_Widget *w, void * v) {
	waterfall *wf = (waterfall *)w->parent();
	wf->wfdisp->slew(100);
	restoreFocus();
} 


void center_cb(Fl_Widget *w, void *v) {
	waterfall *wf = (waterfall *)w->parent();
	wf->wfdisp->movetocenter();
	restoreFocus();
}

void carrier_cb(Fl_Widget *w, void *v) {
	Fl_Counter *cntr = (Fl_Counter *)w;
	waterfall *wf = (waterfall *)w->parent();
	int selfreq = (int) cntr->value();
	active_modem->set_freq(selfreq);
	wf->wfdisp->carrier(selfreq);
	restoreFocus();
}

void qsy_cb(Fl_Widget *w, void *v) {
	static const int qrgs_size = 16 + 1;
	static struct qrg {
		long long f;
		long long fmid;
	} qrgs[qrgs_size], *top = qrgs, *p;

	if (Fl::event_button() != FL_RIGHT_MOUSE) { // store
		if (top < qrgs + qrgs_size - 1)
			top++;
		else
			memmove(qrgs + 1, qrgs + 2, qrgs_size - 2);
		top->f = wf->rfcarrier();
		top->fmid = active_modem->get_freq();

		p = qrgs;
		// this is the sweet spot frequency that is the center of the PBF in the rig
		if (active_modem->get_mode() == MODE_CW)
			p->fmid = (long long)progdefaults.CWsweetspot;
		else if (active_modem->get_mode() == MODE_RTTY)
			p->fmid = (long long)progdefaults.RTTYsweetspot;
		else
			p->fmid = (long long)progdefaults.PSKsweetspot;
		p->f = wf->rfcarrier();
		if (wf->USB())
			p->f += (wf->carrier() - p->fmid);
		else
			p->f -= (wf->carrier() - p->fmid);
	}
	else { // qsy to top of stack
		if (top == qrgs)
			return;
		p = top--;
	}

	rigCAT_set_qsy(p->f, p->fmid);
	rigMEM_set_qsy(p->f, p->fmid);
#ifndef NOHAMLIB
	hamlib_set_qsy(p->f, p->fmid);
#endif	
	restoreFocus();
}

void rate_cb(Fl_Widget *w, void *v) {
	Fl::lock();
	waterfall *wf = (waterfall *)w->parent();
	WFspeed spd = wf->wfdisp->Speed();
	if (spd == SLOW) {
		wf->wfrate->label("NORM");
		wf->wfdisp->Speed(NORMAL);
	}
	else if (spd == NORMAL) {
		wf->wfrate->label("FAST");
		wf->wfdisp->Speed(FAST);
	} else {
		wf->wfrate->label("SLOW");
		wf->wfdisp->Speed(SLOW);
	}
	Fl::unlock();
	restoreFocus();
}

void xmtrcv_cb(Fl_Widget *w, void *vi)
{
	Fl::lock();
	Fl_Light_Button *b = (Fl_Light_Button *)w;
	int v = b->value();
	Fl::unlock();
	if (v == 1) {
		active_modem->set_stopflag(false);
		fl_lock(&trx_mutex);
		trx_state = STATE_TX;
		fl_unlock(&trx_mutex);
	} else {
		active_modem->set_stopflag(true);
		TransmitText->clear();
		if (progdefaults.useTimer)
			progdefaults.useTimer = false;
	}
	restoreFocus();
}

void xmtlock_cb(Fl_Widget *w, void *vi)
{
	Fl::lock();
	Fl_Light_Button *b = (Fl_Light_Button *)w;
	int v = b->value();
	Fl::unlock();
	active_modem->set_freqlock(v ? true : false );
	restoreFocus();
}

void waterfall::set_XmtRcvBtn(bool val)
{
	Fl::lock();
	xmtrcv->value(val);
	Fl::unlock();
}

void mode_cb(Fl_Widget *w, void *v) {
	Fl::lock();
	waterfall *wf = (waterfall *)w->parent();
	if (Fl::event_shift()) {
		wf->wfdisp->Mode(SCOPE);
		w->label("sig");
		wf->x1->deactivate();
		wf->bwclr->deactivate();
		wf->wfcarrier->deactivate();
		wf->wfRefLevel->deactivate();
		wf->wfAmpSpan->deactivate();
	} else if (wf->wfdisp->Mode() == WATERFALL) {
		wf->wfdisp->Mode(SPECTRUM);
		w->label("fft");
	} else if (wf->wfdisp->Mode() == SPECTRUM) {
		wf->wfdisp->Mode(WATERFALL);
		w->label("Wtr");
	} else {
		wf->wfdisp->Mode(WATERFALL);
		w->label("Wtr");
		wf->x1->activate();
		wf->bwclr->activate();
		wf->wfcarrier->activate();
		wf->wfRefLevel->activate();
		wf->wfAmpSpan->activate();
	}
	Fl::unlock();
	restoreFocus();
}

void reflevel_cb(Fl_Widget *w, void *v) {
	Fl::lock();
	waterfall *wf = (waterfall *)w->parent();
	double val = wf->wfRefLevel->value();
	Fl::unlock();
	wf->wfdisp->Reflevel(val);
	progdefaults.wfRefLevel = val;
	restoreFocus();
}

void ampspan_cb(Fl_Widget *w, void *v) {
	Fl::lock();
	waterfall *wf = (waterfall *)w->parent();
	double val = wf->wfAmpSpan->value();
	Fl::unlock();
	wf->wfdisp->Ampspan(val);
	progdefaults.wfAmpSpan = val;
	restoreFocus();
}

void btnRev_cb(Fl_Widget *w, void *v) {
	Fl::lock();
	waterfall *wf = (waterfall *)w->parent();
	Fl_Light_Button *b = (Fl_Light_Button *)w;
	wf->Reverse(b->value());
	Fl::unlock();
	active_modem->set_reverse(wf->Reverse());
	restoreFocus();
}

void waterfall::opmode() {
	int val;
	fl_lock(&trx_mutex);
		val = 	(int)active_modem->get_bandwidth();
	fl_unlock(&trx_mutex);
	if (wfdisp->carrier() < val/2)
		wfdisp->carrier( val/2 );
	if (wfdisp->carrier() > IMAGE_WIDTH - val/2)
		wfdisp->carrier( IMAGE_WIDTH - val/2);
	wfdisp->Bandwidth( val );
	Fl::lock();	
	wfcarrier->range(val/2-1, IMAGE_WIDTH - val/2-1);
	Fl::unlock();
}

void waterfall::carrier(int f) {
	wfdisp->carrier(f);
	Fl::lock();
	wfcarrier->value(f);
	wfcarrier->damage(FL_DAMAGE_ALL);
	Fl::unlock();
}

int waterfall::Speed() { 
	return (int)wfdisp->Speed();
}

void waterfall::Speed(int rate) { 
	WFspeed spd = (WFspeed) rate;
	Fl::lock();
	if (spd == SLOW) {
		wfdisp->Speed(spd);
		wfrate->label("SLOW");
	} else if (rate == FAST) {
		wfdisp->Speed(spd);
		wfrate->label("FAST");
	} else {
		wfdisp->Speed(NORMAL);
		wfrate->label("NORM");
	}
	wfrate->redraw_label();
	Fl::unlock();
}

int waterfall::Mag() {
	return wfdisp->Mag();
}

void waterfall::Mag(int m) { 
	Fl::lock();
	wfdisp->Mag(m);
	if (m == MAG_1) x1->label("x1");
	if (m == MAG_2) x1->label("x2");
	if (m == MAG_4) x1->label("x4");
	x1->redraw_label();
	Fl::unlock();
}

int waterfall::Carrier()
{
	return wfdisp->carrier();
}

void waterfall::Carrier(int f)
{
	active_modem->set_freq(f);
}

void waterfall::rfcarrier(long long cf) {
	wfdisp->rfcarrier(cf);
}
	
long long waterfall::rfcarrier() {
	return wfdisp->rfcarrier();
}

void waterfall::setRefLevel() {
	Fl::lock();
	wfRefLevel->value(progdefaults.wfRefLevel);
	wfdisp->Reflevel(progdefaults.wfRefLevel);
	Fl::unlock();
}

void waterfall::setAmpSpan() {
	Fl::lock();
	wfAmpSpan->value(progdefaults.wfAmpSpan);
	wfdisp->Ampspan(progdefaults.wfAmpSpan);
	Fl::unlock();
}

void waterfall::USB(bool b) {
	if (wfdisp->USB() == b)
		return;
	wfdisp->USB(b);
	active_modem->set_reverse(reverse);
}
	
bool waterfall::USB() {
	return wfdisp->USB();
}

waterfall::waterfall(int x0, int y0, int w0, int h0, char *lbl) :
	Fl_Group(x0,y0,w0,h0,lbl) {
	int xpos;

	buttonrow = h() + y() - BTN_HEIGHT - BEZEL;
	bezel = new Fl_Box(
				FL_DOWN_BOX, 
				x(), 
				y(), 
				w(), 
				h() - BTN_HEIGHT - 2 * BEZEL, 0);
	wfdisp = new WFdisp(x() + BEZEL, 
			y() + BEZEL, 
			w() - 2 * BEZEL,
			h() - BTN_HEIGHT - 4 * BEZEL);
	wfdisp->tooltip("Click to set tracking point");
	
	xpos = x() + wSpace;
	bwclr = new Fl_Button(xpos, buttonrow, bwColor, BTN_HEIGHT, "clr");
	bwclr->callback(bwclr_cb, 0);
	bwclr->tooltip("Color / BW waterfall");

	xpos = xpos + bwColor + wSpace;
	mode = new Fl_Button(xpos, buttonrow, bwFFT, BTN_HEIGHT,"Wtr");
	mode->callback(mode_cb, 0);
	mode->tooltip("Waterfall/FFT - Shift click for signal scope");

	xpos = xpos + bwFFT + wSpace;
	x1 = new Fl_Button(xpos, buttonrow, bwX1, BTN_HEIGHT, "x1");
	x1->callback(x1_cb, 0);
	x1->tooltip("Change scale");

	xpos = xpos + bwX1 + wSpace;
	wfrate = new Fl_Button(xpos, buttonrow, bwRate, BTN_HEIGHT, "Norm");
	wfrate->callback(rate_cb, 0);
	wfrate->tooltip("Waterfall drop speed");

	xpos = xpos + bwRate + wSpace;
	left = new Fl_Repeat_Button(xpos, buttonrow, bwMov, BTN_HEIGHT, "@<");
	left->callback(slew_left, 0);
	left->tooltip("Slew display lower in freq");

	xpos += bwMov;
	center = new Fl_Button(xpos, buttonrow, bwMov, BTN_HEIGHT, "@||");
	center->callback(center_cb, 0);
	center->tooltip("Center display on signal");

	xpos += bwMov;
	right = new Fl_Repeat_Button(xpos, buttonrow, bwMov, BTN_HEIGHT, "@>");
	right->callback(slew_right, 0);
	right->tooltip("Slew display higher in freq");

	xpos = xpos + bwMov + wSpace;
	wfcarrier = new Fl_Counter(xpos, buttonrow, cwCnt, BTN_HEIGHT );
	wfcarrier->callback(carrier_cb, 0);
	wfcarrier->step(1.0);
	wfcarrier->lstep(10.0);
	wfcarrier->precision(0);
	wfcarrier->range(16.0, IMAGE_WIDTH - 16.0);
	wfcarrier->value(wfdisp->carrier());
	wfcarrier->tooltip("Adjust selected tracking freq");

	xpos = xpos + cwCnt + wSpace;
	wfRefLevel = new Fl_Counter(xpos, buttonrow, cwRef, BTN_HEIGHT );
	wfRefLevel->callback(reflevel_cb, 0);
	wfRefLevel->step(1.0);
	wfRefLevel->precision(0);
	wfRefLevel->range(-40.0, 0.0);
	wfRefLevel->value(0.0);
	wfdisp->Reflevel(0.0);
	wfRefLevel->tooltip("Upper signal limit in dB");
	wfRefLevel->type(FL_SIMPLE_COUNTER);

	xpos = xpos + cwRef + wSpace;
	wfAmpSpan = new Fl_Counter(xpos, buttonrow, cwRef, BTN_HEIGHT );
	wfAmpSpan->callback(ampspan_cb, 0);
	wfAmpSpan->step(1.0);
	wfAmpSpan->precision(0);
	wfAmpSpan->range(6.0, 90.0);
	wfAmpSpan->value(80.0);
	wfdisp->Ampspan(80.0);
	wfAmpSpan->tooltip("Signal range in dB");
	wfAmpSpan->type(FL_SIMPLE_COUNTER);

	xpos = xpos + cwRef + wSpace;
	qsy = new Fl_Button(xpos, buttonrow, bwQsy, BTN_HEIGHT, "QSY");
	qsy->callback(qsy_cb, 0);
	qsy->tooltip("Cntr in Xcvr PB\nRight click to undo");
	qsy->deactivate();

	xpos = xpos + bwQsy + wSpace;
	xmtlock = new Fl_Light_Button(xpos, buttonrow, bwXmtLock, BTN_HEIGHT, "Lk");
	xmtlock->callback(xmtlock_cb, 0);
	xmtlock->value(0);
	xmtlock->tooltip("Xmt freq locked");

	xpos = xpos + bwXmtLock + wSpace;
	btnRev = new Fl_Light_Button(xpos, buttonrow, bwRev, BTN_HEIGHT, "Rv");
	btnRev->callback(btnRev_cb, 0);
	btnRev->value(0);
	btnRev->tooltip("Reverse");
	reverse = false;
	
	xpos = w() - bwXmtRcv - wSpace;
	xmtrcv = new Fl_Light_Button(xpos, buttonrow, bwXmtRcv, BTN_HEIGHT, "T/R");
	xmtrcv->callback(xmtrcv_cb, 0);
	xmtrcv->selection_color(FL_RED);
	xmtrcv->value(0);
	xmtrcv->tooltip("Transmit/Receive");

}

int waterfall::handle(int event) {
	if (Fl::event() == FL_LEAVE) {
		wfdisp->wantcursor = false;
		wfdisp->makeMarker();
		return 1;
	}
	
	if (Fl::event_inside( wfdisp )) {
		if (trx_state != STATE_RX)
			return 1;
		int xpos = Fl::event_x() - wfdisp->x();

		static int nucarrier, oldcarrier;
		switch (event) {
		case FL_MOVE:
			wfdisp->wantcursor = true;
			wfdisp->cursorpos = xpos;
			wfdisp->makeMarker();
			wfdisp->redrawCursor();
			break;
		case FL_DRAG: case FL_PUSH:
			switch (Fl::event_button()) {
			case FL_RIGHT_MOUSE:
				wfdisp->wantcursor = false;
				if (event == FL_PUSH)
					oldcarrier = carrier();
				// fall through
			case FL_LEFT_MOUSE:
				nucarrier = wfdisp->cursorFreq(xpos);
				active_modem->set_freq(nucarrier);
				active_modem->set_sigsearch(3);
				wfdisp->redrawCursor();
				restoreFocus();
				break;
			case FL_MIDDLE_MOUSE:
				if (event == FL_DRAG)
					break;
				bool toggle = !active_modem->get_afcOnOff();
				active_modem->set_afcOnOff(toggle);
				extern Fl_Light_Button *afconoff;
				afconoff->value(toggle);
				break;
			}
			break;
		case FL_RELEASE:
			switch (Fl::event_button()) {
			case FL_RIGHT_MOUSE:
				active_modem->set_freq(oldcarrier);
				active_modem->set_sigsearch(3);
				wfdisp->redrawCursor();
				restoreFocus();
				break;
			case FL_LEFT_MOUSE:
				oldcarrier = nucarrier;
				break;
			}
			break;
		case FL_MOUSEWHEEL:
			if (Fl::event_dy() > 0 || Fl::event_dx() > 0)
				active_modem->searchUp();
			else if (Fl::event_dy() < 0 || Fl::event_dx() < 0)
				active_modem->searchDown();
			break;
		}
		return 1;
	} else if (wfdisp->wantcursor == true) {
		wfdisp->wantcursor = false;
		wfdisp->makeMarker();
	}
	return Fl_Group::handle(event);
}
