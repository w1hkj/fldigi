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

#include <config.h>

#include <sstream>
#include <vector>
#include "waterfall.h"
#include "threads.h"
#include "main.h"
#include "modem.h"
#include "qrunner.h"

#if USE_HAMLIB
	#include "hamlib.h"
#endif
#include "rigMEM.h"
#include "rigio.h"

#include "fldigi-config.h"
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

// RGBI is a structure consisting of the values RED, GREEN, BLUE, INTENSITY
// each value can range from 0 (extinguished) to 255 (full on)
// the INTENSITY value is used for the grayscale waterfall display

RGBI	mag2RGBI[256];
RGB		palette[9];

#define max(a,b) (a)>(b)?(a):(b)
#define min(a,b) (a)<(b)?(a):(b)

short int *tmp_fft_db;

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
//	fft_hist		= new short int[image_area];
	fft_db			= new short int[image_area];
	tmp_fft_db		= new short int[image_area];
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

	oldcarrier = newcarrier = 0;
	tmp_carrier = false;
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
//	delete [] fft_hist;
	delete [] fft_db;
	delete [] tmp_fft_db;
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

	int bw, marker_width = bandwidth;
	if (active_modem) {
		int mode = active_modem->get_mode();
		if (mode >= MODE_BPSK31 && mode <= MODE_QPSK250)
			marker_width += mailserver ? progdefaults.ServerOffset :
				progdefaults.SearchRange;
	}
	marker_width = (int)(marker_width / 2.0 + 1);

	RGBmarker.R = progdefaults.bwTrackRGBI.R;
	RGBmarker.G = progdefaults.bwTrackRGBI.G;
	RGBmarker.B = progdefaults.bwTrackRGBI.B;

	// clamp marker to image width
	bw = marker_width;
	int bw_lower = -bw, bw_upper = +bw;
	if (bw_lower + carrierfreq+0.5 < 0)
		bw_lower -= bw_lower + carrierfreq+0.5;
	if (bw_upper + carrierfreq+0.5 > IMAGE_WIDTH)
		bw_upper -= bw_upper + carrierfreq+0.5 - IMAGE_WIDTH;
	for (int y = 0; y < WFMARKER - 2; y++) {
		for (int i = bw_lower; i <= bw_upper ; i++) {
			clrPos = clrM + i + y * IMAGE_WIDTH;
			if (clrPos > clrMin && clrPos < clrMax)
				*clrPos = RGBmarker;
		}
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
	RGBcursor.R = progdefaults.cursorLineRGBI.R;
	RGBcursor.G = progdefaults.cursorLineRGBI.G;
	RGBcursor.B = progdefaults.cursorLineRGBI.B;

	bw = marker_width;
	for (int y = 0; y < WFMARKER - 2; y++) {
		int incr = y * IMAGE_WIDTH;
		int msize = (WFMARKER - 2 - y)*RGBsize*step/4;
		*(clrM + incr - 1)	= 
		*(clrM + incr)		= 
		*(clrM + incr + 1)	= RGBcursor;

		if (xp - (bw + msize) > 0)
			for (int i = bw - msize; i <= bw + msize; i++)
				*(clrM - i + incr) = RGBcursor;

		if (xp + (bw + msize) < IMAGE_WIDTH)
			for (int i = bw - msize; i <= bw + msize; i++)
				*(clrM + i + incr) = RGBcursor;
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
//	for (int i = 0; i < image_area; i++) fft_hist[i] = -1000;
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
	threshold /= delta;
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
	FL_LOCK_D();
//	for (int i = 0; i < image_area; i++)
//		fft_db[i] = log2disp( fft_hist[i] );
	FL_UNLOCK_D();
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
FL_LOCK_D();		
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
			//fft_hist[i] = 
			ffth = (int)(10.0 * log10(pw + 1e-10) );
			fft_db[i] = log2disp(ffth);
		}
FL_UNLOCK_D();
	}
	if (cursormoved || dispcnt == 0) {
FL_LOCK_D();
		if (dispcnt == 0) {
			memmove(
				(void*)tmp_fft_db,
				(void*)fft_db,
				(IMAGE_WIDTH * image_height) * sizeof(short int));
		}
		redraw();
FL_UNLOCK_D();
//		FL_AWAKE();
		cursormoved = false;
	}
	if (dispcnt == 0)
		if (srate == 8000)
			dispcnt = wfspeed;
		else if (srate == 11025)
			dispcnt = wfspeed * 4 / 3;
		else
			dispcnt = wfspeed * 2;
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
FL_LOCK();
	memset (sig_img, 0, sig_image_area);
	memset (&sig_img[h2*IMAGE_WIDTH], 255, IMAGE_WIDTH);
	for (int c = 0; c < IMAGE_WIDTH; c++) {
		ynext = (int)(h2 * sig[c]);
		for (; sigy < ynext; sigy++) sig_img[sigpixel -= IMAGE_WIDTH] = graylevel;
		for (; sigy > ynext; sigy--) sig_img[sigpixel += IMAGE_WIDTH] = graylevel;
		sig_img[sigpixel++] = graylevel;
	}
	redraw();
FL_UNLOCK();
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
    }
    peakaudio = 0.1 * peak + 0.9 * peakaudio;

	if (mode == SCOPE)
		process_analog(circbuff, FFT_LEN * 2);
	else
		processFFT();
		
	put_WARNstatus(peakaudio);

	static char szFrequency[14];
	
	if (usebands)
		rfc = (long long)(atof(cboBand->value()) * 1000.0);
	if (rfc != 0) {
		if (usb)
			dfreq = rfc + active_modem->get_txfreq();
		else	
			dfreq = rfc - active_modem->get_txfreq();
		sprintf(szFrequency, "%-.3f", dfreq / 1000.0);
	} else {
		dfreq = active_modem->get_txfreq();
		sprintf(szFrequency, "%-.0f", dfreq);
	}
	FL_LOCK_D();
	inpFreq->value(szFrequency);
	inpFreq->redraw();
	FL_UNLOCK_D();

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
	p1 = tmp_fft_db + offset;
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
				*pos1 = *pos2 = progdefaults.bwTrackRGBI;
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
					*pos1 = *pos2 = progdefaults.cursorLineRGBI;
				if (progdefaults.UseCursorCenterLine)
					*pos0 = progdefaults.cursorCenterRGBI;
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
					*pos1 = *pos2 = progdefaults.cursorLineRGBI;
				if (progdefaults.UseCursorCenterLine)
					*pos0 = progdefaults.cursorCenterRGBI;
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
		sig = tmp_fft_db[c];
		if (step == 1)
			sig = tmp_fft_db[c];
		else if (step == 2)
			sig = max(tmp_fft_db[c], tmp_fft_db[c+1]);
		else
			sig = max( max ( max ( tmp_fft_db[c], tmp_fft_db[c+1] ), tmp_fft_db[c+2] ), tmp_fft_db[c+3]);
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

void qsy_cb(Fl_Widget *w, void *v)
{
        static vector<struct qrg_mode_t> qsy_stack;
        struct qrg_mode_t m = { -1, -1, 0 };

        if (Fl::event_button() != FL_RIGHT_MOUSE) {
                // store
                m.rfcarrier = wf->rfcarrier();
                m.carrier = active_modem->get_freq();
                qsy_stack.push_back(m);

                // qsy to the sweet spot frequency that is the center of the PBF in the rig
                switch (active_modem->get_mode()) {
                case MODE_CW:
                        m.carrier = (long long)progdefaults.CWsweetspot;
                        break;
                case MODE_RTTY:
                        m.carrier = (long long)progdefaults.RTTYsweetspot;
                        break;
                default:
                        m.carrier = (long long)progdefaults.PSKsweetspot;
                        break;
                }
		if (wf->USB())
			m.rfcarrier += (wf->carrier() - m.carrier);
		else
			m.rfcarrier -= (wf->carrier() - m.carrier);
        }
        else { // qsy to top of stack
                if (qsy_stack.size()) {
                        m = qsy_stack.back();
                        qsy_stack.pop_back();
                }
        }

        if (m.carrier > 0) {
                rigCAT_set_qsy(m.rfcarrier, m.carrier);
                rigMEM_set_qsy(m.rfcarrier, m.carrier);
#if USE_HAMLIB
                hamlib_set_qsy(m.rfcarrier, m.carrier);
#endif
        }
	restoreFocus();
}

void rate_cb(Fl_Widget *w, void *v) {
	FL_LOCK_D();
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
	FL_UNLOCK_D();
	restoreFocus();
}

void xmtrcv_cb(Fl_Widget *w, void *vi)
{
	FL_LOCK_D();
	Fl_Light_Button *b = (Fl_Light_Button *)w;
	int v = b->value();
	FL_UNLOCK_D();
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
	FL_LOCK_D();
	Fl_Light_Button *b = (Fl_Light_Button *)w;
	int v = b->value();
	FL_UNLOCK_D();
	active_modem->set_freqlock(v ? true : false );
	restoreFocus();
}

void waterfall::set_XmtRcvBtn(bool val)
{
	FL_LOCK();
	xmtrcv->value(val);
	FL_UNLOCK();
}

void mode_cb(Fl_Widget *w, void *v) {
	FL_LOCK_D();
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
	FL_UNLOCK_D();
	restoreFocus();
}

void reflevel_cb(Fl_Widget *w, void *v) {
	FL_LOCK_D();
	waterfall *wf = (waterfall *)w->parent();
	double val = wf->wfRefLevel->value();
	FL_UNLOCK_D();
	wf->wfdisp->Reflevel(val);
	progdefaults.wfRefLevel = val;
	restoreFocus();
}

void ampspan_cb(Fl_Widget *w, void *v) {
	FL_LOCK_D();
	waterfall *wf = (waterfall *)w->parent();
	double val = wf->wfAmpSpan->value();
	FL_UNLOCK_D();
	wf->wfdisp->Ampspan(val);
	progdefaults.wfAmpSpan = val;
	restoreFocus();
}

void btnRev_cb(Fl_Widget *w, void *v) {
	FL_LOCK_D();
	waterfall *wf = (waterfall *)w->parent();
	Fl_Light_Button *b = (Fl_Light_Button *)w;
	wf->Reverse(b->value());
	FL_UNLOCK_D();
	active_modem->set_reverse(wf->Reverse());
	restoreFocus();
}

void btnMem_cb(Fl_Widget *, void *menu_event)
{
	static std::vector<struct qrg_mode_t> qrg_list;
        enum { SELECT, APPEND, REPLACE, REMOVE };
        int op = SELECT, elem = 0;

        if (menu_event) { // event on popup menu
                elem = wf->mbtnMem->value();

                switch (Fl::event_button()) {
                case FL_MIDDLE_MOUSE:
                        op = REPLACE;
                        break;
                case FL_LEFT_MOUSE: case FL_RIGHT_MOUSE: default:
                        if (Fl::event_state() & FL_SHIFT)
                                op = REMOVE;
                        else
                                op = SELECT;
                        break;
                }
        }
        else { // button press
                switch (Fl::event_button()) {
                case FL_RIGHT_MOUSE:
                        return;
                case FL_MIDDLE_MOUSE: // select last
                        if ((elem = qrg_list.size() - 1) < 0);
                                return;
                        op = SELECT;
                        break;
                case FL_LEFT_MOUSE: default:
                        op = APPEND;
                        break;
                }
        }

        struct qrg_mode_t m;
        switch (op) {
        case SELECT:
                m = qrg_list[elem];
                if (active_modem != *mode_info[m.mode].modem) {
                        init_modem_sync(m.mode);
                        QUEUE_FLUSH();
                }
                if (m.rfcarrier && m.rfcarrier != wf->rfcarrier()) {
                        rigCAT_set_qsy(m.rfcarrier, m.carrier);
                        rigMEM_set_qsy(m.rfcarrier, m.carrier);
#if USE_HAMLIB
                        hamlib_set_qsy(m.rfcarrier, m.carrier);
#endif
                }
                else
                        active_modem->set_freq(m.carrier);
                break;
        case REMOVE:
                wf->mbtnMem->remove(elem);
                qrg_list.erase(qrg_list.begin() + elem);
                break;
        case APPEND: case REPLACE:
                m.rfcarrier = wf->rfcarrier();
                m.carrier = active_modem->get_freq();
                m.mode = active_modem->get_mode();
                if (op == APPEND)
                        qrg_list.push_back(m);
                else
                        qrg_list[elem] = m;

        // write the menu item text
        {
                ostringstream o;
                o << mode_info[m.mode].sname << " @@ ";
                if (m.rfcarrier > 0) { // write 1000s separators
                        char s[20], *p = s + sizeof(s) - 1;
                        int i = 0;

                        *p = '\0';
                        do {
                                if (i % 3 == 0 && i)
                                        *--p = '.';
                                *--p = '0' + m.rfcarrier % 10;
                                ++i;
                        } while ((m.rfcarrier /= 10) && p > s);

                        o << p << (wf->USB() ? " + " : " - ");
                }
                o << m.carrier;
                if (op == APPEND)
                        wf->mbtnMem->add(o.str().c_str());
                else
                        wf->mbtnMem->replace(elem, o.str().c_str());

        }
                break;
        }

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
	FL_LOCK_D();	
	wfcarrier->range(val/2-1, IMAGE_WIDTH - val/2-1);
	FL_UNLOCK_D();
}

void waterfall::carrier(int f) {
	wfdisp->carrier(f);
	FL_LOCK_D();
	wfcarrier->value(f);
	wfcarrier->damage(FL_DAMAGE_ALL);
	FL_UNLOCK_D();
}

int waterfall::Speed() { 
	return (int)wfdisp->Speed();
}

void waterfall::Speed(int rate) { 
	WFspeed spd = (WFspeed) rate;
	FL_LOCK_D();
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
	FL_UNLOCK_D();
}

int waterfall::Mag() {
	return wfdisp->Mag();
}

void waterfall::Mag(int m) { 
	FL_LOCK_D();
	wfdisp->Mag(m);
	if (m == MAG_1) x1->label("x1");
	if (m == MAG_2) x1->label("x2");
	if (m == MAG_4) x1->label("x4");
	x1->redraw_label();
	FL_UNLOCK_D();
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
	FL_LOCK_D();
	wfRefLevel->value(progdefaults.wfRefLevel);
	wfdisp->Reflevel(progdefaults.wfRefLevel);
	FL_UNLOCK_D();
}

void waterfall::setAmpSpan() {
	FL_LOCK_D();
	wfAmpSpan->value(progdefaults.wfAmpSpan);
	wfdisp->Ampspan(progdefaults.wfAmpSpan);
	FL_UNLOCK_D();
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
	// wfdisp->tooltip("Click to set tracking point");
	
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
	btnMem = new Fl_Button(xpos, buttonrow, bwMem, BTN_HEIGHT, "M @-9UpArrow");
	btnMem->callback(btnMem_cb, 0);
	btnMem->tooltip("Store mode and frequency\nRight click for menu");
	mbtnMem = new Fl_Menu_Button(btnMem->x(), btnMem->y(), btnMem->w(), btnMem->h(), 0);
	mbtnMem->callback(btnMem->callback(), mbtnMem);
	mbtnMem->type(Fl_Menu_Button::POPUP3);

	xpos = xpos + bwMem + wSpace;
	xmtlock = new Fl_Light_Button(xpos, buttonrow, bwXmtLock, BTN_HEIGHT, "Lk");
	xmtlock->callback(xmtlock_cb, 0);
	xmtlock->value(0);
	xmtlock->selection_color(FL_RED);
	xmtlock->tooltip("Xmt freq locked");

	xpos = xpos + bwXmtLock + wSpace;
	btnRev = new Fl_Light_Button(xpos, buttonrow, bwRev, BTN_HEIGHT, "Rv");
	btnRev->callback(btnRev_cb, 0);
	btnRev->value(0);
	btnRev->selection_color(FL_GREEN);
	btnRev->tooltip("Reverse");
	reverse = false;

	xpos = w() - bwXmtRcv - wSpace;
	xmtrcv = new Fl_Light_Button(xpos, buttonrow, bwXmtRcv, BTN_HEIGHT, "T/R");
	xmtrcv->callback(xmtrcv_cb, 0);
	xmtrcv->selection_color(FL_RED);
	xmtrcv->value(0);
	xmtrcv->tooltip("Transmit/Receive");
}

int waterfall::handle(int event)
{
	if (event != FL_MOUSEWHEEL || Fl::event_inside(wfdisp))
		return Fl_Group::handle(event);

	int d;
	if ( !((d = Fl::event_dy()) || (d = Fl::event_dx())) )
		return 1;

	Fl_Valuator *val;
	if (Fl::event_inside(sldrSquelch) || Fl::event_inside(pgrsSquelch))
		val = sldrSquelch;
	else if (Fl::event_inside(wfcarrier))
		 val = wfcarrier;
	else if (Fl::event_inside(wfRefLevel))
		val = wfRefLevel;
	else if (Fl::event_inside(wfAmpSpan))
		val = wfAmpSpan;
	else
		return 0;

	val->value(val->clamp(val->increment(val->value(), -d)));
	val->do_callback();

	return 1;
}

static void hide_cursor(void *w)
{
	reinterpret_cast<Fl_Widget *>(w)->window()->cursor(FL_CURSOR_NONE);
}

int WFdisp::handle(int event)
{
	if (!(event == FL_LEAVE || Fl::event_inside(this)))
		return 0;

	if (trx_state != STATE_RX)
		return 1;
	int xpos = Fl::event_x() - x();

	switch (event) {
	case FL_MOVE:
		window()->cursor(FL_CURSOR_DEFAULT);
		if (!Fl::has_timeout(hide_cursor, this))
			Fl::add_timeout(1, hide_cursor, this);
		wantcursor = true;
		cursorpos = xpos;
		makeMarker();
		redrawCursor();
		break;
	case FL_DRAG: case FL_PUSH:
		switch (Fl::event_button()) {
		case FL_RIGHT_MOUSE:
			wantcursor = false;
			if (event == FL_PUSH) {
				tmp_carrier = true;
				oldcarrier = carrier();
			}
			// fall through
		case FL_LEFT_MOUSE:
			newcarrier = cursorFreq(xpos);
			active_modem->set_freq(newcarrier);
			active_modem->set_sigsearch(SIGSEARCH);
			redrawCursor();
			restoreFocus();
			break;
		case FL_MIDDLE_MOUSE:
			if (event == FL_DRAG)
				break;
			Fl_Button *b = useCheckButtons ? chk_afconoff : afconoff;
			b->value(!active_modem->get_afcOnOff());
			b->do_callback();
		}
		break;
	case FL_RELEASE:
		switch (Fl::event_button()) {
		case FL_RIGHT_MOUSE:
			tmp_carrier = false;
			active_modem->set_freq(oldcarrier);
			active_modem->set_sigsearch(3);
			redrawCursor();
			restoreFocus();
			// fall through
		case FL_LEFT_MOUSE:
			oldcarrier = newcarrier;
			break;
		}
		break;

	case FL_MOUSEWHEEL:
		handle_mouse_wheel(event);
		return handle(FL_MOVE);
		break;

	case FL_SHORTCUT:
		if (Fl::event_inside(this))
			take_focus();
		break;
	case FL_KEYBOARD:
	{
		int d = (Fl::event_state() & FL_CTRL) ? 10 : 1;
		switch (Fl::event_key()) {
		case FL_Left:
			if (xpos > 0) {
				active_modem->set_freq(oldcarrier = newcarrier = carrier() - d);
				redrawCursor();
			}
			break;
		case FL_Right:
			if (xpos < w()) {
				active_modem->set_freq(oldcarrier = newcarrier = carrier() + d);
				redrawCursor();
			}
			break;
		case FL_Tab:
			restoreFocus();
			break;
		}
		break;
	}
	case FL_KEYUP:
	{
		if (Fl::event_inside(this)) {
			int k = Fl::event_key();
			if (k == FL_Shift_L || k == FL_Shift_R || k == FL_Control_L ||
			    k == FL_Control_R || k == FL_Meta_L || k == FL_Meta_R ||
			    k == FL_Alt_L || k == FL_Alt_R)
				restoreFocus();
		}
		break;
	}

	case FL_LEAVE:
		Fl::remove_timeout(hide_cursor, this);
		window()->cursor(FL_CURSOR_DEFAULT);
		wantcursor = false;
		makeMarker();
		// restoreFocus();
		break;

	}

	return 1;
}

void WFdisp::handle_mouse_wheel(int event)
{
	int d;
	if ( !((d = Fl::event_dy()) || (d = Fl::event_dx())) )
		return;

	int state = Fl::event_state();
	if (state & (FL_META | FL_ALT)) {
		if (d > 0)
			active_modem->searchUp();
		else if (d < 0)
			active_modem->searchDown();
		return;
	}
	if (!(state & (FL_CTRL | FL_SHIFT)))
		return; // suggestions?

	extern Fl_Valuator *cntServerOffset, *cntSearchRange,
			   *sldrHellBW, *sldrCWbandwidth;
	Fl_Valuator *val = 0;
	if (state & FL_CTRL) {
		switch (active_modem->get_mode()) {
		case MODE_BPSK31: case MODE_QPSK31: case MODE_PSK63: case MODE_QPSK63:
		case MODE_PSK125: case MODE_QPSK125: case MODE_PSK250: case MODE_QPSK250:
			val = mailserver ? cntServerOffset : cntSearchRange;
			break;
		case MODE_FELDHELL:
			val = sldrHellBW;
			break;
		case MODE_CW:
			val = sldrCWbandwidth;
			break;
		default:
			return;
		}
	}
	else if (state & FL_SHIFT)
		val = sldrSquelch;

	val->value(val->clamp(val->increment(val->value(), -d)));
	val->do_callback();
	if (val == cntServerOffset || val == cntSearchRange)
		active_modem->set_sigsearch(SIGSEARCH);
	else if (val == sldrSquelch) // sldrSquelch gives focus to TextWidget
		take_focus();

	char msg[60];
	if (val != sldrSquelch)
		snprintf(msg, sizeof(msg), "%s = %2.0f Hz", val->label(), val->value());
	else
		snprintf(msg, sizeof(msg), "Squelch = %2.0f %%", val->value());
	put_status(msg, 2);
}
