// ----------------------------------------------------------------------------
// waterfall.cxx - Waterfall Spectrum Analyzer Widget
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
// Copyright (C) 2007-2010
//		Stelios Bounanos, M0GLD
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

//#define USE_BLACKMAN
//#define USE_HAMMING
//#define USE_HANNING

#include <config.h>

#include <sstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <map>

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Repeat_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Counter.H>
#include <FL/Enumerations.H>

#include "fl_digi.h"
#include "trx.h"
#include "misc.h"
#include "waterfall.h"
#include "main.h"
#include "modem.h"
#include "qrunner.h"
#include "threads.h"

#if USE_HAMLIB
	#include "hamlib.h"
#endif
#include "rigio.h"

#include "fldigi-config.h"
#include "configuration.h"
#include "status.h"
#include "squelch_status.h"
#include "Viewer.h"
#include "macros.h"
#include "arq_io.h"
#include "confdialog.h"
#include "flmisc.h"
#include "gettext.h"
#include "rtty.h"
#include "flslider2.h"
#include "debug.h"
#include "rigsupport.h"
#include "xmlrpc.h"
#include "psm/psm.h"
#include "kiss_io.h"
#include "fmt_dialog.h"

#include "spectrum_viewer.h"

//pthread_mutex_t draw_mutex = PTHREAD_MUTEX_INITIALIZER;

#define bwFFT		30
#define cwRef		50
#define bwX1		25
#define bwMov		18
#define bwRate		45
#define cwCnt		92
#define bwQsy		32
#define bwXmtLock	32
#define bwRev		32
#define bwMem		40
#define bwXmtRcv	40
#define wSpace		1

#define bwdths	(wSpace + bwFFT + wSpace + cwRef + wSpace + cwRef + wSpace + bwX1 + \
				wSpace + 3*bwMov + wSpace + bwRate + wSpace + \
				cwCnt + wSpace + bwQsy + wSpace + bwMem + wSpace + \
				bwXmtLock + wSpace + bwRev + wSpace + bwXmtRcv + wSpace)

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

short int *tmp_fft_db;

static pthread_mutex_t waterfall_mutex = PTHREAD_MUTEX_INITIALIZER;

WFdisp::WFdisp (int x0, int y0, int w0, int h0, char *lbl) :
			  Fl_Widget(x0,y0,w0,h0,"") {
	disp_width = w();
	if (disp_width > progdefaults.HighFreqCutoff/4)
		disp_width = progdefaults.HighFreqCutoff/4;
	scale_width = IMAGE_WIDTH * 2;
	image_height = h() - WFTEXT - WFSCALE - WFMARKER;
	image_area      = IMAGE_WIDTH * image_height;
	sig_image_area  = IMAGE_WIDTH * h();
	RGBsize			= sizeof(RGB);
	RGBwidth		= RGBsize * scale_width;
	fft_img			= new RGBI[image_area];
	markerimage		= new RGB[scale_width * WFMARKER];
	scaleimage		= new uchar[scale_width * WFSCALE];
	scline			= new uchar[scale_width];
	fft_sig_img 	= new uchar[image_area];
	sig_img			= new uchar[sig_image_area];
	pwr				= new wf_fft_type[IMAGE_WIDTH];
	fft_db			= new short int[image_area];
	tmp_fft_db		= new short int[image_area];
	circbuff		= new wf_fft_type[WF_FFTLEN];
	wfbuf			= new wf_cpx_type[WF_FFTLEN];
	wfft			= new g_fft<wf_fft_type>(WF_FFTLEN);
	fftwindow		= new wf_fft_type[WF_FFTLEN];
	setPrefilter(progdefaults.wfPreFilter);

	memset(circbuff, 0, WF_FFTLEN * sizeof(double));

	mag = 1;
	step = 4;
	offset = 0;
	sigoffset = 0;
	ampspan = 75;
//	reflevel = -10;
	initmaps();
	bandwidth = 32;
	RGBmarker = RGBred;
	RGBcursor = RGByellow;
	RGBInotch.I = progdefaults.notchRGBI.I;
	RGBInotch.R = progdefaults.notchRGBI.R;
	RGBInotch.G = progdefaults.notchRGBI.G;
	RGBInotch.B = progdefaults.notchRGBI.B;
	mode = WATERFALL;
	centercarrier = false;
	overload = false;
	rfc = 0L;
	usb = true;
	wfspeed = NORMAL;
	wfspdcnt = 0;
	dispcnt = 1.0 * WF_BLOCKSIZE / WF_SAMPLERATE;
	dispdec = 1.0 * WF_BLOCKSIZE / WF_SAMPLERATE;
	wantcursor = false;
	cursormoved = false;
	for (int i = 0; i < IMAGE_WIDTH; i++)
		pwr[i] = 0.0;

	carrier(1500);

	oldcarrier = newcarrier = 0;
	tmp_carrier = false;
	ptrCB = 0;
	ptrFFTbuff = 0;

	for (int i = 0; i < 256; i++)
		mag2RGBI[i].I = mag2RGBI[i].R = mag2RGBI[i].G = mag2RGBI[i].B = 0;

	int error = 0;

// use fastest sync converter
//	SRC_SINC_BEST_QUALITY		= 0,
//	SRC_SINC_MEDIUM_QUALITY		= 1,
//	SRC_SINC_FASTEST			= 2,
//	SRC_ZERO_ORDER_HOLD			= 3,
//	SRC_LINEAR					= 4,

	src_state = src_new(2, 1, &error);
	if (error) {
		LOG_ERROR("src_new error %d: %s", error, src_strerror(error));
		abort();
	}

	error = src_reset(src_state);
	if (error)
		LOG_ERROR("src_reset error %d: %s", error, src_strerror(error));

	src_data.end_of_input = 0;
	src_data.src_ratio = 0.0;
	genptr = 0;
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
	delete [] fft_db;
	delete [] tmp_fft_db;
}

void WFdisp::initMarkers() {
	memset(markerimage, 224, RGBwidth * WFMARKER); 
}

// draw a marker of specified width and colour centred at freq and clrM
inline void WFdisp::makeMarker_(int width, const RGB* color, int freq, const RGB* clrMin, RGB* clrM, const RGB* clrMax)
{
	if (!active_modem) return;
	trx_mode marker_mode = active_modem->get_mode();

	if (marker_mode == MODE_RTTY) {
	// rtty has two bandwidth indicators on the waterfall
	// upper and lower frequency
		int shift = static_cast<int>(
			(progdefaults.rtty_shift < rtty::numshifts ?
				rtty::SHIFT[progdefaults.rtty_shift] :
				progdefaults.rtty_custom_shift));
		int bw_limit_hi = (int)(shift / 2 + progdefaults.RTTY_BW / 2.0);
		int bw_limit_lo = (int)(shift / 2 - progdefaults.RTTY_BW / 2.0);
		int bw_freq = static_cast<int>(freq + 0.5);
		int bw_lower1 = -bw_limit_hi;
		int bw_upper1 = -bw_limit_lo;
		int bw_lower2 = bw_limit_lo;
		int bw_upper2 = bw_limit_hi;
		if (bw_lower1 + bw_freq < 0)
			bw_lower1 -= bw_lower1 + bw_freq;
		if (bw_upper1 + bw_freq < 0)
			bw_lower2 -= bw_lower2 + bw_freq;
		if (bw_upper2 + bw_freq > scale_width)
			bw_upper2 -= bw_upper2 + bw_freq - scale_width;
		if (bw_lower2 + bw_freq > scale_width)
			bw_lower2 -= bw_lower2 + bw_freq - scale_width;
	// draw it
		RGB* clrPos;
		for (int y = 0; y < WFMARKER - 2; y++) {
			for (int x = bw_lower1; x < bw_upper1; x++) {
				clrPos = clrM + x + y * scale_width;
				if (clrPos > clrMin && clrPos < clrMax)
					*clrPos = *color;
			}
			for (int x = bw_lower2; x < bw_upper2; x++) {
				clrPos = clrM + x + y * scale_width;
				if (clrPos > clrMin && clrPos < clrMax)
					*clrPos = *color;
			}
		}
		return;
	}

	int bw_lower = -width, bw_upper = width;

	if (marker_mode >= MODE_MT63_500S && marker_mode <= MODE_MT63_2000L)
			bw_upper = (int)(width * 31 / 32);

	if (marker_mode == MODE_FSQ || marker_mode == MODE_IFKP)
			bw_upper = (int)(width * 32 / 33);

	if (bw_lower + static_cast<int>(freq+0.5) < 0)
		bw_lower -= bw_lower + static_cast<int>(freq+0.5);

	if (bw_upper + static_cast<int>(freq+0.5) > scale_width)
		bw_upper -= bw_upper + static_cast<int>(freq+0.5) - scale_width;

	// draw it
	RGB* clrPos;
	for (int y = 0; y < WFMARKER - 2; y++) {
		for (int x = bw_lower; x < bw_upper; x++) {
			clrPos = clrM + x + y * scale_width;
			if (clrPos > clrMin && clrPos < clrMax)
				*clrPos = *color;
		}
	}
}

void WFdisp::make_fmt_marker ()
{
	RGB unk_trk_color, ref_trk_color;

	memset(markerimage + scale_width, 0, RGBwidth * (WFMARKER - 2));

	Fl::get_color(progdefaults.FMT_unk_color, unk_trk_color.R, unk_trk_color.G, unk_trk_color.B);

	Fl::get_color(progdefaults.FMT_ref_color, ref_trk_color.R, ref_trk_color.G, ref_trk_color.B);

	int fmt_bw = progdefaults.FMT_filter;

	RGB *mrkr_U = markerimage + scale_width + int(round(cnt_unk_freq->value()));
	RGB *mrkr_R = markerimage + scale_width + int(round(cnt_ref_freq->value()));

	// draw marker

	for (int y = 0; y < WFMARKER - 2; y++) {
		for (int x = -fmt_bw; x <= fmt_bw; x++) {
			*(mrkr_U + x + y * scale_width) = unk_trk_color;
			*(mrkr_R + x + y * scale_width) = ref_trk_color;
		}
	}

}

void WFdisp::makeMarker()
{
	if (unlikely(!active_modem))
		return;

	int mode = active_modem->get_mode();
	RGB *clrMin, *clrMax, *clrM;
	int marker_width = bandwidth;

	if (mode == MODE_FMT) {
		make_fmt_marker ();
		marker_width = progdefaults.FMT_filter;
	} else {
		clrMin = markerimage + scale_width;
		clrMax = clrMin + (WFMARKER - 2) * scale_width;
		memset(clrMin, 0, RGBwidth * (WFMARKER - 2));
		clrM = clrMin + (int)((double)carrierfreq + 0.5);

		if (mode >= MODE_PSK_FIRST && mode <= MODE_PSK_LAST)
			marker_width += mailserver ? progdefaults.ServerOffset :
				progdefaults.SearchRange;
		else if (mode >= MODE_FELDHELL && mode <= MODE_HELL80)
			marker_width = (int)progdefaults.HELL_BW;
		else if (mode == MODE_RTTY)
			marker_width = static_cast<int>((progdefaults.rtty_shift < rtty::numshifts ?
					rtty::SHIFT[progdefaults.rtty_shift] :
					progdefaults.rtty_custom_shift));
		marker_width = (int)(marker_width / 2.0 + 1);

		RGBmarker.R = progdefaults.bwTrackRGBI.R;
		RGBmarker.G = progdefaults.bwTrackRGBI.G;
		RGBmarker.B = progdefaults.bwTrackRGBI.B;

		makeMarker_(marker_width, &RGBmarker, carrierfreq, clrMin, clrM, clrMax);

		if (unlikely(active_modem->freqlocked() || mode == MODE_FSQ)) {
			int txfreq = static_cast<int>(active_modem->get_txfreq());
			adjust_color_inv(RGBmarker.R, RGBmarker.G, RGBmarker.B, FL_BLACK, FL_RED);
			makeMarker_( static_cast<int>(bandwidth / 2.0 + 1),
						&RGBmarker, txfreq,
						clrMin, clrMin + (int)((double)txfreq + 0.5), clrMax);
		}
	}

	if (!wantcursor) return;

	if (cursorpos > progdefaults.HighFreqCutoff - bandwidth / 2 / step)
		cursorpos = progdefaults.HighFreqCutoff - bandwidth / 2 / step;
	if (cursorpos >= (progdefaults.HighFreqCutoff - offset - bandwidth/2)/step)
		cursorpos = (progdefaults.HighFreqCutoff - offset - bandwidth/2)/step;
	if (cursorpos < (progdefaults.LowFreqCutoff + bandwidth / 2) / step)
		cursorpos = (progdefaults.LowFreqCutoff + bandwidth / 2) / step;

// Create the cursor marker
	double xp = offset + step * cursorpos;
	if (xp < bandwidth / 2.0 || xp > (progdefaults.HighFreqCutoff - bandwidth / 2.0))
		return;
	clrM = markerimage + scale_width + (int)(xp + 0.5);
	RGBcursor.R = progdefaults.cursorLineRGBI.R;
	RGBcursor.G = progdefaults.cursorLineRGBI.G;
	RGBcursor.B = progdefaults.cursorLineRGBI.B;

	int bw_lo = marker_width;
	int bw_hi = marker_width;
	if (mode >= MODE_MT63_500S && mode <= MODE_MT63_2000L)
		bw_hi = bw_hi * 31 / 32;
	if (mode == MODE_FSQ || mode == MODE_IFKP) bw_hi = bw_hi * 32 / 33;

	for (int y = 0; y < WFMARKER - 2; y++) {
		int incr = y * scale_width;
		int msize = (WFMARKER - 2 - y)*RGBsize*step/4;
		for (int m = -step; m < step; m++)
			*(clrM + incr + m) = RGBcursor;

		if (xp - (bw_lo + msize) > 0)
			for (int i = bw_lo - msize; i <= bw_lo + msize; i++)
				*(clrM - i + incr) = RGBcursor;

		if (xp + (bw_hi + msize) < scale_width)
			for (int i = bw_hi - msize; i <= bw_hi + msize; i++)
				*(clrM + i + incr) = RGBcursor;
	}
}

void WFdisp::makeScale() {
	uchar *gmap = scaleimage;
	memset(scline, 0, scale_width);

	for (int tic = 500; tic < scale_width; tic += 500) {
		scline[tic] = 255;
		for (int ticn = 1; ticn < step; ticn++)
			if (tic + ticn < scale_width) scline[tic + ticn] = 255;
	}

	for (int i = 0; i < WFSCALE - 5; i++) {
		memcpy(gmap, scline, scale_width);
		gmap += (scale_width);
	}

	for (int tic = 100; tic < scale_width ; tic += 100) {
		scline[tic] = 255;
		for (int ticn = 1; ticn < step; ticn++)
			if (tic + ticn < scale_width) scline[tic + ticn] = 255;
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
	for (int i = 0; i < image_area; i++) fft_db[i] = tmp_fft_db[i] = log2disp(-1000);

	memset (fft_img, 0, image_area * sizeof(RGBI) );
	memset (scaleimage, 0, scale_width * WFSCALE);
	memset (markerimage, 0, RGBwidth * WFMARKER);
	memset (fft_sig_img, 0, image_area);
	memset (sig_img, 0, sig_image_area);

	memset (mag2RGBI, 0, sizeof(mag2RGBI));
	initMarkers();
	makeScale();
	setcolors();
}

int WFdisp::peakFreq(int f0, int delta)
{
	guard_lock waterfall_lock(&waterfall_mutex);

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
	{
		guard_lock waterfall_lock(&waterfall_mutex);
		for (int i = flower; i <= fupper; i++)
			pwrdensity += pwr[i];
	}
	return pwrdensity/(bw+1);
}

/// Frequency of the maximum power for a given bandwidth. Used for AFC.
double WFdisp::powerDensityMaximum(int bw_nb, const int (*bw)[2]) const
{
	if (bw_nb < 1) return carrierfreq;

	int fmax[bw_nb];
	double pnbw[bw_nb];
	int f_lowest = carrierfreq;
	int f_highest = carrierfreq;
	double max_pwr = 0;
	for (int i = 0; i < bw_nb; i++) {
		fmax[i] = carrierfreq;
		pnbw[i] = 0;
	}

	{
		guard_lock waterfall_lock(&waterfall_mutex);
		for (int i = 0; i < bw_nb; i++) {
			f_lowest = carrierfreq + bw[i][0];
			if (f_lowest <= 0) f_lowest = 0;
			f_highest = carrierfreq + bw[i][1];
			if (f_highest > IMAGE_WIDTH) f_highest = IMAGE_WIDTH;
			max_pwr = 0;
			pnbw[i] = 0;
			for (int n = f_lowest; n < f_highest; n++) {
				if (pwr[n] > max_pwr) {
					max_pwr = pwr[n];
					fmax[i] = n;
				}
				pnbw[i] += pwr[n];
			}
			if (pnbw[i] == 0) return carrierfreq;
		}
	}
	int fmid = 0;
	double total_pwr = 0;
	for (int i = 0; i < bw_nb; i++) total_pwr += pnbw[i];

	for (int i = 0; i < bw_nb; i++) fmid += fmax[i] * pnbw[i] / total_pwr;

	return fmid;
}

void WFdisp::setPrefilter(int v)
{
	switch (v) {
	case WF_FFT_RECTANGULAR: RectWindow(fftwindow, WF_FFTLEN); break;
	case WF_FFT_BLACKMAN: BlackmanWindow(fftwindow, WF_FFTLEN); break;
	case WF_FFT_HAMMING: HammingWindow(fftwindow, WF_FFTLEN); break;
	case WF_FFT_HANNING: HanningWindow(fftwindow, WF_FFTLEN); break;
	case WF_FFT_TRIANGULAR: TriangularWindow(fftwindow, WF_FFTLEN); break;
	}
//	BlackmanWindow(fftwindow, WF_FFTLEN);
	prefilter = v;
}

int WFdisp::log2disp(int v)
{
	double val = -255.0 * v / ampspan;
	if (val < 0) return 255;
	if (val > 255 ) return 0;
	return (int)(255 - val);
}

void WFdisp::processFFT() {
	if (prefilter != progdefaults.wfPreFilter)
		setPrefilter(progdefaults.wfPreFilter);

	wf_fft_type scale = WF_FFTLEN / 8000.0;

	if ((wfspeed != PAUSE) && ((dispcnt -= dispdec) <= 0)) {
		static const int log2disp100 = log2disp(-100);
		double vscale = 2.0 / WF_FFTLEN;

		for (int i = 0; i < WF_FFTLEN; i++) wfbuf[i] = 0;

		void *pv = static_cast<void*>(wfbuf);
		wf_fft_type *pbuf = static_cast<wf_fft_type*>(pv);

		int latency = progdefaults.wf_latency;
		if (latency < 1) latency = 1;
		if (latency > 16) latency = 16;
		int nsamples = WF_FFTLEN * latency / 16;
		vscale *= sqrt(16.0 / latency);
		for (int i = 0; i < nsamples; i++)
			pbuf[i] = fftwindow[i * 16 / latency] * circbuff[i] * vscale;

		for (int i = 0; i < WF_FFTLEN; i++)
			pbuf[i] = fftwindow[i] * circbuff[i] * vscale;

		wfft->RealFFT(wfbuf);

		memset(pwr, 0, progdefaults.LowFreqCutoff * sizeof(wf_fft_type));
		memset(&fft_db[ptrFFTbuff * IMAGE_WIDTH],
				log2disp100,
				progdefaults.LowFreqCutoff * sizeof(*fft_db));

		int n = 0;
		for (int i = progdefaults.LowFreqCutoff + 1; i < IMAGE_WIDTH; i++) {
			n = round(scale * i);
			pwr[i] = norm(wfbuf[n]);
			int ffth = round(10.0 * log10(pwr[i] + 1e-10) );
			fft_db[ptrFFTbuff * IMAGE_WIDTH + i] = log2disp(ffth);
		}

		ptrFFTbuff--;
		if (ptrFFTbuff < 0) ptrFFTbuff += image_height;

		for (int i = 0; i < image_height; i++) {
			int j = (i + 1 + ptrFFTbuff) % image_height;
			memmove( (void *)(tmp_fft_db + i * IMAGE_WIDTH),
					 (void *)(fft_db + j * IMAGE_WIDTH),
					 IMAGE_WIDTH * sizeof(short int));
		}

		dispdec = 1.0 * WF_BLOCKSIZE / WF_SAMPLERATE;
		dispcnt = 1.0 * WF_BLOCKSIZE / WF_SAMPLERATE; // FAST
		if (wfspeed == NORMAL) dispcnt *= NORMAL;
		if (wfspeed == SLOW) dispcnt *= progdefaults.drop_speed;
	}
	redraw();
}

void WFdisp::process_analog (wf_fft_type *sig, int len) {
	int h1, h2, h3;
	int sigy, sigpixel, ynext, graylevel;
	h1 = h()/8 - 1;
	h2 = h()/2 - 1;
	h3 = h()*7/8 + 1;
	graylevel = 220;
// clear the signal display area
	sigy = 0;
	sigpixel = IMAGE_WIDTH*h2;
	memset (sig_img, 0, sig_image_area);
	memset (&sig_img[h1*IMAGE_WIDTH], 160, IMAGE_WIDTH);
	memset (&sig_img[h2*IMAGE_WIDTH], 255, IMAGE_WIDTH);
	memset (&sig_img[h3*IMAGE_WIDTH], 160, IMAGE_WIDTH);
	int cbc = ptrCB;
	for (int c = 0; c < IMAGE_WIDTH; c++) {
		ynext = (int)(h2 * sig[cbc]);
		if (ynext < -h2) ynext = -h2;
		if (ynext > h2) ynext = h2;
		cbc = (cbc + 1) % (WF_FFTLEN);
		for (; sigy < ynext; sigy++) sig_img[sigpixel -= IMAGE_WIDTH] = graylevel;
		for (; sigy > ynext; sigy--) sig_img[sigpixel += IMAGE_WIDTH] = graylevel;
		sig_img[sigpixel++] = graylevel;
	}
	redraw();
}

//----------------------------------------------------------------------
// queue audio_blocks used to separate audio stream process timing
// from GUI processing and timing
// mutex guards the audio_blocks data
//----------------------------------------------------------------------

extern state_t trx_state;
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
struct AUDIO_BLOCK {
	wf_fft_type sig[WF_BLOCKSIZE];
};
bool clear_audio_blocks;

std::queue<AUDIO_BLOCK> audio_blocks;

void WFdisp::sig_data( double *sig, int len )
{
	double src_ratio = 1.0 * WF_SAMPLERATE / active_modem->get_samplerate();

	if (audio_blocks.size() > 32) { //8) {
		clear_audio_blocks = true;
//		LOG_ERROR("%s", "audio_blocks overflow");
		return;
	}
	if ((len * src_ratio) > WF_BLOCKSIZE * 2) {
		LOG_ERROR("%s", "len * src_ratio > WFBLOCKSIZE * 2");
		return;
	}

	AUDIO_BLOCK audio_block;

	buf = insamples;
	srclen = len;
	int error;

	if (src_data.src_ratio != src_ratio) {
		src_data.src_ratio = src_ratio;
		src_set_ratio(src_state, src_data.src_ratio);
		LOG_INFO("Waterfall sample rate ratio %f", src_ratio);
	}
	for (int n = 0; n < len; n++) insamples[n] = sig[n];

	while (srclen > 0) {
		src_data.data_in = insamples;
		src_data.input_frames = srclen;
		src_data.data_out = &outsamples[genptr];
		src_data.output_frames = WF_BLOCKSIZE * 2 - genptr;
		src_data.input_frames_used = 0;

		if (unlikely(error = src_process(src_state, &src_data))) {
			LOG_ERROR("src_process error %d: %s", error, src_strerror(error));
			return;
		}
		size_t gend = src_data.output_frames_gen;
		size_t used = src_data.input_frames_used;

		genptr	+= gend;
		buf		+= used;
		srclen	-= used;

		while (genptr >= WF_BLOCKSIZE) {
			for (int n = 0; n < WF_BLOCKSIZE; n++) audio_block.sig[n] = outsamples[n];
			{
				guard_lock data_lock (&data_mutex);
				audio_blocks.push(audio_block);
			}
			for (int n = 0; n < WF_BLOCKSIZE; n++) outsamples[n] = outsamples[n+WF_BLOCKSIZE];
			genptr -= WF_BLOCKSIZE;
		}
	}
	return;
}

// this method must be called from main thread

void WFdisp::handle_sig_data()
{
	ENSURE_THREAD(FLMAIN_TID);

	double gain = pow(10, progdefaults.wfRefLevel / -20.0);
	AUDIO_BLOCK current;
	while (1) {//!audio_blocks.empty()) {
		if (clear_audio_blocks) {
			guard_lock data_lock(&data_mutex);
			while ( !audio_blocks.empty() ) audio_blocks.pop();
			clear_audio_blocks = false;
		}
		if (audio_blocks.empty())
			return;

		for (int n = 0; n < WF_FFTLEN - WF_BLOCKSIZE; n++)
			circbuff[n] = circbuff[n + WF_BLOCKSIZE];
		{
// this block guarded by data_mutex
			guard_lock data_lock(&data_mutex);
			current = audio_blocks.front();
			audio_blocks.pop();
		}
		for (int n = 0; n < WF_BLOCKSIZE; n++)
			circbuff[n + WF_FFTLEN - WF_BLOCKSIZE - 1] = current.sig[n];

		overload = false;
		double overval = 0, peak = 0.0;
		for (int i = WF_FFTLEN - WF_BLOCKSIZE; i < WF_FFTLEN; i++) {
			overval = fabs(circbuff[i]);
			if (overval > peak) peak = overval;
			circbuff[i] *= gain;
		}

		if (mode == SCOPE)
			process_analog(circbuff, WF_FFTLEN);
		else
			processFFT();

		put_WARNstatus(peak);

		static char szFrequency[14];
		if (active_modem && rfc != 0) {
			int offset = 0;
			double afreq = active_modem->get_txfreq();
			trx_mode mode = active_modem->get_mode();
			std::string testmode = qso_opMODE->value();

			bool xcvr_useFSK = ((testmode.find("RTTY") != std::string::npos) ||
								(testmode.find("FSK") != std::string::npos) ||
								((testmode.find("DATA") != std::string::npos) &&
								 (use_nanoIO || progdefaults.PseudoFSK)) );
			usb = !ModeIsLSB(testmode);
			if ((testmode.find("DATA") != std::string::npos) && xcvr_useFSK)
				usb = !usb;

			if (mode == MODE_RTTY && progdefaults.useMARKfreq && !xcvr_useFSK) {
				offset = (progdefaults.rtty_shift < rtty::numshifts ?
					rtty::SHIFT[progdefaults.rtty_shift] :
					progdefaults.rtty_custom_shift);
				offset /= 2;
				if (active_modem->get_reverse()) offset *= -1;
			}
			if (testmode.find("CW") != std::string::npos)
				afreq = 0;
			if (xcvr_useFSK)
				afreq = 0;
			if (mode == MODE_ANALYSIS) {
				dfreq = 0;
			} else {
				if (usb)
					dfreq = rfc + afreq + offset;
				else
					dfreq = rfc - afreq - offset;
			}
			snprintf(szFrequency, sizeof(szFrequency), "%-.3f", dfreq / 1000.0);
		} else {
			dfreq = active_modem->get_txfreq();
			snprintf(szFrequency, sizeof(szFrequency), "%-.0f", dfreq);
		}
		inpFreq->value(szFrequency);

	}
}

// Check the display offset & limit to 0 to max IMAGE_WIDTH displayed
void WFdisp::checkoffset() {
	if (mode == SCOPE) {
		if (sigoffset < 0)
			sigoffset = 0;
		if (sigoffset > (IMAGE_WIDTH - disp_width))
			sigoffset = IMAGE_WIDTH - disp_width;
	} else {
		if (offset > (int)(progdefaults.HighFreqCutoff - step * disp_width))
			offset = (int)(progdefaults.HighFreqCutoff - step * disp_width);
		if (offset < 0)
			offset = 0;
	}
}

void WFdisp::setOffset(int v) {
	offset = v;
	checkoffset();
}

void WFdisp::slew(int dir) {
	if (mode == SCOPE)
		sigoffset += dir;
	else
		offset += dir;
	checkoffset();
}

void WFdisp::movetocenter() {
	if (mode == SCOPE) {
		sigoffset = IMAGE_WIDTH / 2;
	} else if (active_modem->get_mode() == MODE_FMT) {
		if (progdefaults.fmt_center_on_unknown)
			offset = cnt_unk_freq->value() - (disp_width * step / 2);
		else if (progdefaults.fmt_center_on_reference)
			offset = cnt_ref_freq->value() - (disp_width * step / 2);
		else if (progdefaults.fmt_center_on_median)
			offset = (cnt_unk_freq->value() + cnt_ref_freq->value())/2 - 
					 (disp_width * step / 2);
		else
			offset = progdefaults.PSKsweetspot - (disp_width * step / 2);
	} else
		offset = carrierfreq - (disp_width * step / 2);
	checkoffset();
}

void WFdisp::carrier(int cf) {
	if (cf >= bandwidth / 2 && cf < (IMAGE_WIDTH - bandwidth / 2)) {
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
	if (mag == MAG_1 && disp_width > progdefaults.HighFreqCutoff/4)
		disp_width = progdefaults.HighFreqCutoff/4;
	if (mag == MAG_2) step = 2;
	if (mag == MAG_2 && disp_width > progdefaults.HighFreqCutoff/2)
		disp_width = progdefaults.HighFreqCutoff/2;
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
	int fw = 60, xoff;
	static char szFreq[20];
	double fr;
	uchar *pixmap;

	if (progdefaults.wf_audioscale) {
		pixmap = (scaleimage + (int)offset);
		fl_draw_image_mono(
			pixmap,
			x(), y() + WFTEXT,
			w(), WFSCALE,
			step, scale_width);

		fl_color(0xFFFFFF00);
		fl_font(progdefaults.WaterfallFontnbr, progdefaults.WaterfallFontsize);

		for (int i = 1; ; i++) {
			fr = 500.0 * i;
			snprintf(szFreq, sizeof(szFreq), "%7.0f", fr);
			fw = (int)fl_width(szFreq);
			xoff = (int) (( (1000.0/step) * i - fw) / 2.0 - offset /step );
			if (xoff > 0 && xoff < w() - fw)
				fl_draw(szFreq, x() + xoff, y() + 10 );
			if (xoff > w() - fw) break;
		}
		return;
	}

	int mdoffset = 0;
	std::string testmode = qso_opMODE->value();

	bool xcvr_useFSK = ((testmode.find("RTTY") != std::string::npos) ||
						(testmode.find("FSK") != std::string::npos) ||
						((testmode.find("DATA") != std::string::npos) &&
						 (use_nanoIO ||progdefaults.PseudoFSK)) );

	usb = !ModeIsLSB(testmode);
	if ((testmode.find("DATA") != std::string::npos) && xcvr_useFSK)
		usb = !usb;

	if (testmode.find("CW") != std::string::npos)
		mdoffset = progdefaults.CWsweetspot;

	if (xcvr_useFSK) {
		if (usb) mdoffset = progdefaults.xcvr_FSK_MARK + rtty::SHIFT[progdefaults.rtty_baud] * 2;
		else mdoffset = progdefaults.xcvr_FSK_MARK;
	}

	if (usb)
		pixmap = (scaleimage +  (int)(((rfc - mdoffset) % 1000 + offset)) );
	else
		pixmap = (scaleimage + (int)((1000 - (rfc + mdoffset) % 1000 + offset)));

	fl_draw_image_mono(
		pixmap,
		x(), y() + WFTEXT,
		w(), WFSCALE,
		step, scale_width);

	fl_color(0xFFFFFF00);
	fl_font(progdefaults.WaterfallFontnbr, progdefaults.WaterfallFontsize);

	for (int i = 1; ; i++) {
		if (usb)
			fr = (rfc - mdoffset - (rfc - mdoffset) % 500 + 500 * i)/1000.0;
		else
			fr = (rfc + mdoffset - (rfc + mdoffset) % 500 - 500 * i + 500)/1000.0;

		snprintf(szFreq, sizeof(szFreq), "%7.1f", fr);
		fw = (int)fl_width(szFreq);
		if (usb)
			xoff = (int) ( ( (1000.0/step) * i - fw) / 2.0 -
							(offset + (rfc - mdoffset) % 500) / step );
		else
			xoff = (int) ( ( (1000.0/step) * i - fw) / 2.0 -
							(offset + 500 - (rfc + mdoffset) % 500) / step );
		if (xoff > 0 && xoff < w() - fw)
			fl_draw(szFreq, x() + xoff, y() + 10 );
		if (xoff > w() - fw) break;
	}
}

void WFdisp::drawMarker() {
	if (mode == SCOPE) return;
	int msize = RGBsize * scale_width;
	int psize = scale_width * WFMARKER;
	uchar *pixmap = (uchar *)(markerimage + (int)(offset));
	uchar map[msize];
	memset(map, 0, sizeof(map));
	int y1 = y() + WFSCALE + WFTEXT;
	for (int yp = 0; yp < WFMARKER; yp++) {
		for (int xp = 0; xp < scale_width; xp++) {
			if ((RGBsize * xp + 2 < msize) && 
				(RGBsize * (yp * scale_width + xp * step + 2) < psize)) {
				map[RGBsize * xp] = pixmap[RGBsize * (yp * scale_width + xp * step)];
				map[RGBsize * xp + 1] = pixmap[RGBsize * (yp * scale_width + xp * step) + 1];
				map[RGBsize * xp + 2] = pixmap[RGBsize * (yp * scale_width + xp * step) + 2];
			}
		}
		fl_draw_image((const uchar *)map, x(), y1 + yp, w(), 1, RGBsize, 0);
	}
	return;
}

void WFdisp::update_waterfall() {
// transfer the fft history data into the WF image
	short int * __restrict__ p1, * __restrict__ p2;
	RGBI * __restrict__ p3, * __restrict__ p4;
	p1 = tmp_fft_db + offset + step/2;
	p2 = p1;
	p3 = fft_img;
	p4 = p3;

	short*  __restrict__ limit = tmp_fft_db + image_area - step + 1;

#define UPD_LOOP( Step, Operation ) \
case Step: for (int row = 0; row < image_height; row++) { \
		p2 = p1; \
		p4 = p3; \
		for ( const short *  __restrict__ last_p2 = std::min( p2 + Step * disp_width, limit +1 ); p2 < last_p2; p2 += Step ) { \
			*(p4++) = mag2RGBI[ Operation ]; \
		} \
		p1 += IMAGE_WIDTH; \
		p3 += disp_width; \
	}; break

	if (progdefaults.WFaveraging) {
		switch(step) {
			UPD_LOOP( 4, (*p2 + *(p2+1) + *(p2+2) + *(p2-1) + *(p2-1))/5 );
			UPD_LOOP( 2, (*p2 + *(p2+1) + *(p2-1))/3 );
			UPD_LOOP( 1, *p2 );
			default:;
		}
	} else {
		switch(step) {
			UPD_LOOP( 4, MAX( MAX( MAX ( MAX ( *p2, *(p2+1) ), *(p2+2) ), *(p2-2) ), *(p2-1) ) );
			UPD_LOOP( 2, MAX( MAX( *p2, *(p2+1) ), *(p2-1) ) );
			UPD_LOOP( 1, *p2 );
			default:;
		}
	}
#undef UPD_LOOP

	if (active_modem && progdefaults.UseBWTracks) {
		trx_mode mode = active_modem->get_mode();
		if (mode == MODE_FMT) {
			int bw =progdefaults.FMT_filter;
			int trk = int(round(cnt_unk_freq->value()));
			RGBI  *pos1 = fft_img + (trk - offset - bw) / step;
			RGBI  *pos2 = fft_img + (trk - offset + bw) / step;
			RGBI unk_trk_color, ref_trk_color;

			Fl::get_color(progdefaults.FMT_unk_color, unk_trk_color.R, unk_trk_color.G, unk_trk_color.B);
			Fl::get_color(progdefaults.FMT_ref_color, ref_trk_color.R, ref_trk_color.G, ref_trk_color.B);

			if (likely(pos1 >= fft_img && pos2 < fft_img + disp_width)) {
				if (progdefaults.UseWideTracks) {
					for (int y = 0; y < image_height; y ++) {
						*(pos1 + 1) = *pos1 = unk_trk_color;
						*(pos2 - 1) = *pos2 = unk_trk_color;
						pos1 += disp_width;
						pos2 += disp_width;
					}
				} else {
					for (int y = 0; y < image_height; y ++) {
						*pos1 = unk_trk_color;
						*pos2 = unk_trk_color;
						pos1 += disp_width;
						pos2 += disp_width;
					}
				}
			}

			trk = int(round(cnt_ref_freq->value()));
			pos1 = fft_img + (trk - offset - bw) / step;
			pos2 = fft_img + (trk - offset + bw) / step;

			if (likely(pos1 >= fft_img && pos2 < fft_img + disp_width)) {
				if (progdefaults.UseWideTracks) {
					for (int y = 0; y < image_height; y ++) {
						*(pos1 + 1) = *pos1 = ref_trk_color;
						*(pos2 - 1) = *pos2 = ref_trk_color;
						pos1 += disp_width;
						pos2 += disp_width;
					}
				} else {
					for (int y = 0; y < image_height; y ++) {
						*pos1 = ref_trk_color;
						*pos2 = ref_trk_color;
						pos1 += disp_width;
						pos2 += disp_width;
					}
				}
			}

		} else {

			int bw_lo = bandwidth / 2;
			int bw_hi = bandwidth / 2;
			trx_mode mode = active_modem->get_mode();
			if (mode >= MODE_MT63_500S && mode <= MODE_MT63_2000L)
				bw_hi = bw_hi * 31 / 32;
			if (mode == MODE_FSQ || mode == MODE_IFKP) {
				bw_hi = bw_lo = 69 * bandwidth / 100;
			}
			RGBI  *pos1 = fft_img + (carrierfreq - offset - bw_lo) / step;
			RGBI  *pos2 = fft_img + (carrierfreq - offset + bw_hi) / step;
			if (unlikely(pos2 == fft_img + disp_width))
				pos2--;
			if (likely(pos1 >= fft_img && pos2 < fft_img + disp_width)) {
				RGBI rgbi1, rgbi2 ;

				if (mode == MODE_RTTY && progdefaults.useMARKfreq) {
					if (active_modem->get_reverse()) {
						rgbi1 = progdefaults.rttymarkRGBI;
						rgbi2 = progdefaults.bwTrackRGBI;
					} else {
						rgbi1 = progdefaults.bwTrackRGBI;
						rgbi2 = progdefaults.rttymarkRGBI;
					}
				} else {
					rgbi1 = progdefaults.bwTrackRGBI;
					rgbi2 = progdefaults.bwTrackRGBI;
				}
				if (progdefaults.UseWideTracks) {
					for (int y = 0; y < image_height; y ++) {
						*(pos1 + 1) = *pos1 = rgbi1;
						*(pos2 - 1) = *pos2 = rgbi2;
						pos1 += disp_width;
						pos2 += disp_width;
					}
				} else {
					for (int y = 0; y < image_height; y ++) {
						*pos1 = rgbi1;
						*pos2 = rgbi2;
						pos1 += disp_width;
						pos2 += disp_width;
					}
				}
			}
		}
	}

// draw notch
	if ((notch_frequency > 1) && (notch_frequency < progdefaults.HighFreqCutoff - 1)) {
		RGBInotch.I = progdefaults.notchRGBI.I;
		RGBInotch.R = progdefaults.notchRGBI.R;
		RGBInotch.G = progdefaults.notchRGBI.G;
		RGBInotch.B = progdefaults.notchRGBI.B;
		RGBI  *notch = fft_img + (notch_frequency - offset) / step;
		int dash = 0;
		for (int y = 0; y < image_height; y++) {
			dash = (dash + 1) % 6;
			if (dash == 0 || dash == 1 || dash == 2)
				*(notch-1) = *notch = *(notch+1) = RGBInotch;
			notch += disp_width;
		}
	}

	if (progdefaults.show_psm_btn &&
		progStatus.kpsql_enabled && 
		(trx_state == STATE_RX))
		signal_psm();
}

void WFdisp::drawcolorWF() {
	uchar *pixmap = (uchar *)fft_img;

	update_waterfall();

	if (active_modem && wantcursor &&
		(progdefaults.UseCursorLines || progdefaults.UseCursorCenterLine) ) {
		trx_mode mode = active_modem->get_mode();

		int bw_lo = bandwidth / 2;
		int bw_hi = bandwidth / 2;
		if (mode >= MODE_MT63_500S && mode <= MODE_MT63_2000L)
			bw_hi = bw_hi * 31 / 32;
		if (mode == MODE_FSQ || mode == MODE_IFKP) bw_hi = bw_hi * 32 / 33;
		if (mode == MODE_FMT) {
			bw_lo = bw_hi = progdefaults.FMT_filter;
		}
		RGBI  *pos0 = (fft_img + cursorpos);
		RGBI  *pos1 = (fft_img + cursorpos - bw_lo/step);
		RGBI  *pos2 = (fft_img + cursorpos + bw_hi/step);
		if (pos1 >= fft_img && pos2 < fft_img + disp_width) {
			for (int y = 0; y < image_height; y ++) {
				if (progdefaults.UseCursorLines) {
					*pos1 = *pos2 = progdefaults.cursorLineRGBI;
					if (progdefaults.UseWideCursor)
						*(pos1 + 1) = *(pos2 - 1) = *pos1;
				}
				if (progdefaults.UseCursorCenterLine) {
					*pos0 = progdefaults.cursorCenterRGBI;
					if (progdefaults.UseWideCenter)
						*(pos0 - 1) = *(pos0 + 1) = *pos0;
				}
				pos0 += disp_width;
				pos1 += disp_width;
				pos2 += disp_width;
			}
		}

	}

	fl_color(FL_BLACK);
	fl_rectf(x(), y(), w(), WFSCALE + WFMARKER + WFTEXT);
	fl_color(fl_rgb_color(palette[0].R, palette[0].G, palette[0].B));
	fl_rectf(x(), y() + WFSCALE + WFMARKER + WFTEXT, w(), image_height);
	fl_draw_image(
		pixmap, x(), y() + WFSCALE + WFMARKER + WFTEXT,
		disp_width, image_height,
		sizeof(RGBI), disp_width * sizeof(RGBI) );
	drawScale();
}

void WFdisp::drawspectrum() {
	int sig;
	long offset_idx = 0;
	long ynext,
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
			sig = MAX(tmp_fft_db[c], tmp_fft_db[c+1]);
		else
			sig = MAX( MAX ( MAX ( tmp_fft_db[c], tmp_fft_db[c+1] ), tmp_fft_db[c+2] ), tmp_fft_db[c+3]);
		ynext = h1 * sig / 256;
		offset_idx = (IMAGE_WIDTH/step);
		while ((ffty < ynext)) {
			fft_sig_img[fftpixel -= offset_idx] = graylevel; 
			ffty++;
			if (fftpixel < offset_idx) {
				std::cout << "corrupt index 1\n";
				break;
			}
		}
		while ((ffty > ynext)) {
			fft_sig_img[fftpixel += offset_idx] = graylevel; 
			ffty--;
			if (fftpixel >= (image_area - 1)) {
				std::cout << "corrupt index 2\n";
				break;
			}
		}
		if (fftpixel >= 0 && fftpixel <= image_area)
			fft_sig_img[fftpixel++] = graylevel;
		else
			std::cout << "fft_sig_image index out of bounds: " << fftpixel << std::endl;
	}

	if (progdefaults.UseBWTracks) {
		if (active_modem == fmt_modem) {
			uchar  *pos1;
			uchar  *pos2;
			int trk1 = int(round(cnt_unk_freq->value()));
			int trk2 = int(round(cnt_ref_freq->value()));
			int bw = int(round(progdefaults.FMT_filter));
			pos1 = pixmap + (trk1 - offset - bw) / step;
			pos2 = pixmap + (trk1 - offset + bw) / step;
			if (pos1 >= pixmap &&
				pos2 < pixmap + disp_width)
				for (int y = 0; y < image_height; y ++) {
					*pos1 = *pos2 = 255;
					if (progdefaults.UseWideTracks) {
						*(pos1 + 1) = *(pos2 - 1) = 255;
					}
					pos1 += IMAGE_WIDTH/step;
					pos2 += IMAGE_WIDTH/step;
				}
			pos1 = pixmap + (trk2 - offset - bw) / step;
			pos2 = pixmap + (trk2 - offset + bw) / step;
			if (pos1 >= pixmap &&
				pos2 < pixmap + disp_width) {
				for (int y = 0; y < image_height; y ++) {
					*pos1 = *pos2 = 255;
					if (progdefaults.UseWideTracks) {
						*(pos1 + 1) = *(pos2 - 1) = 255;
					}
				pos1 += IMAGE_WIDTH/step;
				pos2 += IMAGE_WIDTH/step;
				}
			}
		} else {
			uchar  *pos1 = pixmap + (carrierfreq - offset - bandwidth/2) / step;
			uchar  *pos2 = pixmap + (carrierfreq - offset + bandwidth/2) / step;
			if (pos1 >= pixmap &&
				pos2 < pixmap + disp_width) {
				for (int y = 0; y < image_height; y ++) {
					*pos1 = *pos2 = 255;
					if (progdefaults.UseWideTracks) {
						*(pos1 + 1) = 255;
						*(pos2 - 1) = 255;
					}
					pos1 += IMAGE_WIDTH/step;
					pos2 += IMAGE_WIDTH/step;
				}
			}
		}
	}

	if (active_modem && wantcursor &&
		(progdefaults.UseCursorLines || progdefaults.UseCursorCenterLine)) {
		trx_mode mode = active_modem->get_mode();
		int bw_lo = bandwidth / 2;
		int bw_hi = bandwidth / 2;
		if (mode >= MODE_MT63_500S && mode <= MODE_MT63_2000L)
			bw_hi = bw_hi * 31 / 32;
		if (mode == MODE_FSQ || mode == MODE_IFKP) bw_hi = bw_hi * 32 / 33;
		uchar  *pos0 = pixmap + cursorpos;
		uchar  *pos1 = (pixmap + cursorpos - bw_lo/step);
		uchar  *pos2 = (pixmap + cursorpos + bw_hi/step);
		for (int y = 0; y < h1; y ++) {
			if (progdefaults.UseCursorLines) {
				*pos1 = *pos2 = 255;
				if (progdefaults.UseWideCursor)
					*(pos1 + 1) = *(pos2 - 1) = *pos1;
			}
			if (progdefaults.UseCursorCenterLine) {
				*pos0 = 255;
				if (progdefaults.UseWideCenter) *(pos0-1) = *(pos0+1) = *(pos0);
			}
			pos0 += IMAGE_WIDTH/step;
			pos1 += IMAGE_WIDTH/step;
			pos2 += IMAGE_WIDTH/step;
		}
	}

// draw notch
	if ((notch_frequency > 1) && (notch_frequency < progdefaults.HighFreqCutoff - 1)) {
		uchar  *notch = pixmap + (notch_frequency - offset) / step;
		int dash = 0;
		for (int y = 0; y < image_height; y++) {
			dash = (dash + 1) % 6;
			if (dash == 0 || dash == 1 || dash == 2)
				*(notch-1) = *notch = *(notch+1) = 255;
			notch += IMAGE_WIDTH/step;
		}
	}

	fl_color(FL_BLACK);
	fl_rectf(x(), y(), w(), WFSCALE + WFMARKER + WFTEXT + image_height);

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

	if (progdefaults.show_psm_btn && progStatus.kpsql_enabled) {
		drawcolorWF();
		drawMarker();
		return;
	}

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
		drawcolorWF();
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

void killMacroTimer()
{
	stopMacroTimer();
}

void carrier_cb(Fl_Widget *w, void *v) {
	Fl_Counter *cntr = (Fl_Counter *)w;
	waterfall *wf = (waterfall *)w->parent();
	int selfreq = (int) cntr->value();
	if (selfreq > progdefaults.HighFreqCutoff) selfreq = progdefaults.HighFreqCutoff - wf->wfdisp->Bandwidth() / 2;
	killMacroTimer();
	if (active_modem)
		active_modem->set_freq(selfreq);
	wf->wfdisp->carrier(selfreq);
	restoreFocus();
}

void do_qsy(bool dir)
{
	if (!active_modem) return;
	static std::vector<qrg_mode_t> qsy_stack;
	qrg_mode_t m;

	wf->xmtlock->value(0);
	wf->xmtlock->do_callback();

	if (dir) {
// store
		m.rfcarrier = wf->rfcarrier();
		int wfc = m.carrier = active_modem->get_freq();
		qsy_stack.push_back(m);
		m.rmode = qso_opMODE->value();
		trx_mode md = active_modem->get_mode();

		std::string testmode = qso_opMODE->value();
		bool xcvr_useFSK = ((testmode.find("RTTY") != std::string::npos) ||
							(testmode.find("FSK") != std::string::npos) ||
							((testmode.find("DATA") != std::string::npos) &&
							 (use_nanoIO)) );

// qsy to the sweet spot frequency that is the center of the PBF in the rig
		switch (md) {
			case MODE_CW:
				m.carrier = progdefaults.CWsweetspot;
				break;
			case MODE_RTTY:
				if (xcvr_useFSK) {
					// qsy operates on change in audio center track
					m.carrier = progdefaults.xcvr_FSK_MARK + rtty::SHIFT[progdefaults.rtty_shift]/2;
				} else
					m.carrier = progdefaults.RTTYsweetspot;
				break;
			case MODE_FMT:
				if (progdefaults.fmt_center_on_unknown)
					m.carrier = cnt_unk_freq->value();
				else if (progdefaults.fmt_center_on_reference)
					m.carrier = cnt_ref_freq->value();
				else if (progdefaults.fmt_center_on_median)
					m.carrier = (cnt_unk_freq->value() + cnt_ref_freq->value())/2;
				else
					m.carrier = progdefaults.PSKsweetspot;
				break;
			default:
				m.carrier = progdefaults.PSKsweetspot;
				break;
		}
		if (m.rmode.find("CW") != std::string::npos) {
			if (wf->USB())
				m.rfcarrier += (wfc - m.carrier);
			else
				m.rfcarrier -= (wfc - m.carrier);
		} else if ( (md == MODE_RTTY) && xcvr_useFSK ) {
			if (wf->USB()) {
				m.rfcarrier += (wfc - m.carrier);
			} else {
				m.rfcarrier -= (wfc - m.carrier);
			}
		} else {
			if (wf->USB())
				m.rfcarrier += (wf->carrier() - m.carrier);
			else
				m.rfcarrier -= (wf->carrier() - m.carrier);
		}
	}
	else { // qsy to top of stack
		if (qsy_stack.size()) {
			m = qsy_stack.back();
			qsy_stack.pop_back();
		}
	}

	if (m.carrier > 0)
		qsy(m.rfcarrier, m.carrier);
}

void qsy_cb(Fl_Widget *w, void *v)
{
	if (Fl::event_button() != FL_RIGHT_MOUSE)
		do_qsy(true);
	else
		do_qsy(false);
	restoreFocus();
}

void rate_cb(Fl_Widget *w, void *v) {
	waterfall* wf = static_cast<waterfall*>(w->parent());
	WFspeed new_speed;

	switch (wf->wfdisp->Speed()) {
	case SLOW:
		new_speed = NORMAL;
		break;
	case NORMAL: default:
		new_speed = FAST;
		break;
	case FAST:
		new_speed = PAUSE;
		break;
	case PAUSE:
		new_speed = SLOW;
		break;
	}

	wf->Speed(new_speed);
	restoreFocus();
}

//extern void reset_xmlchars();

void xmtrcv_cb(Fl_Widget *w, void *vi)
{
	if (!active_modem) return;
	Fl_Light_Button *b = (Fl_Light_Button *)w;
	int v = b->value();
	if (!(active_modem->get_cap() & modem::CAP_TX)) {
		b->value(0);
		restoreFocus();		return;
	}
	if (v == 1) {
		killMacroTimer();
		active_modem->set_stopflag(false);

		if (progdefaults.show_psm_btn && progStatus.kpsql_enabled)
			set_xmtrcv_selection_color_pending();
		trx_transmit();
	} else {
		if (progdefaults.show_psm_btn && progStatus.kpsql_enabled) {
			psm_transmit_ended(PSM_ABORT);
			xmtrcv_selection_color(progdefaults.XmtColor);
		}

		if (btnTune->value()) {
			btnTune->value(0);
			btnTune->do_callback();
		}
		else {
			TransmitText->clear();
			if (active_modem->get_mode() == MODE_FSQ && fsq_tx_text)
				fsq_tx_text->clear();
			else if (active_modem->get_mode() == MODE_IFKP && ifkp_tx_text)
				ifkp_tx_text->clear();

			if (arq_text_available)
				AbortARQ();

			if(xmltest_char_available)
				reset_xmlchars();

			if(kiss_text_available)
				flush_kiss_tx_buffer();

			if (progStatus.timer) {
				progStatus.timer = 0;
			}

			queue_reset();
			active_modem->set_stopflag(true);
		}
	}
	restoreFocus();
}

void xmtlock_cb(Fl_Widget *w, void *vi)
{
	if (!active_modem) return;
	Fl_Light_Button *b = (Fl_Light_Button *)w;
	int v = b->value();
	active_modem->set_freqlock(v ? true : false );
	restoreFocus();
}

void waterfall::set_XmtRcvBtn(bool val)
{
	xmtrcv->value(val);
	if (!val && btnTune->value()) {
		btnTune->value(0);
		btnTune->labelcolor(FL_FOREGROUND_COLOR);
	}
}

void set_wf_mode(void)
{
   static const char* names[NUM_WF_MODES] = { "WF", "FFT", "SIG" };
   int m = 0;

   if (progdefaults.show_psm_btn && progStatus.kpsql_enabled) {
	  if(wf->wfdisp->Mode() == WATERFALL) {
		 return;
	  }
	  m = WATERFALL;
   } else {
	  m = wf->wfdisp->Mode() + (Fl::event_button() == FL_LEFT_MOUSE ? 1 : -1);
   }

	m = WCLAMP(m, WATERFALL, NUM_WF_MODES-1);

	if (m == SCOPE)
		wf->x1->deactivate();
	else
		wf->x1->activate();

	wf->wfdisp->Mode(static_cast<WFmode>(m));
	wf->mode->label(names[m]);
	restoreFocus();

}

void mode_cb(Fl_Widget* w, void*)
{
	set_wf_mode();
}

void reflevel_cb(Fl_Widget *w, void *v) {
	waterfall *wf = (waterfall *)w->parent();
	double val = wf->wfRefLevel->value();
	progdefaults.wfRefLevel = val;
	restoreFocus();
}

void ampspan_cb(Fl_Widget *w, void *v) {
	waterfall *wf = (waterfall *)w->parent();
	double val = wf->wfAmpSpan->value();
	wf->wfdisp->Ampspan(val);
	progdefaults.wfAmpSpan = val;
	restoreFocus();
}

void btnRev_cb(Fl_Widget *w, void *v)
{
	if (!active_modem) return;
	waterfall *wf = (waterfall *)w->parent();
	Fl_Light_Button *b = (Fl_Light_Button *)w;
	wf->Reverse(b->value());
	active_modem->set_reverse(wf->Reverse());
	progdefaults.rtty_reverse = b->value();
	set_mode_reverse(active_modem->get_mode(), progdefaults.rtty_reverse);
	progdefaults.changed = true;
	restoreFocus();
}

void btnMem_cb(Fl_Widget *, void *menu_event)
{
	if (!active_modem) return;
	static std::vector<qrg_mode_t> qrg_list;
	enum { SELECT, APPEND, REPLACE, REMOVE, CLEAR };
	int op = SELECT, elem = 0;

	if (menu_event) { // event on popup menu
		elem = wf->mbtnMem->value();

		switch (Fl::event_button()) {
			case FL_MIDDLE_MOUSE:
				op = REPLACE;
				break;
			case FL_LEFT_MOUSE: case FL_RIGHT_MOUSE: default:
				op = (Fl::event_state() & FL_SHIFT) ? REMOVE : SELECT;
				break;
		}
	}
	else { // button press
		switch (Fl::event_button()) {
			case FL_RIGHT_MOUSE:
				return;
			case FL_MIDDLE_MOUSE: // select last
				if ((elem = qrg_list.size() - 1) < 0)
					return;
				op = SELECT;
				break;
			case FL_LEFT_MOUSE: default:
				op = (Fl::event_state() & FL_SHIFT) ? CLEAR : APPEND;
				break;
			}
	}

	qrg_mode_t m;
	switch (op) {
		case SELECT:
			m = qrg_list[elem];
			if (active_modem != *mode_info[m.mode].modem) {
				init_modem_sync(m.mode);
			}
			if (m.rfcarrier && m.rfcarrier != wf->rfcarrier())
				qsy(m.rfcarrier, m.carrier);
			else
				active_modem->set_freq(m.carrier);
			break;
		case REMOVE:
			wf->mbtnMem->remove(elem);
			qrg_list.erase(qrg_list.begin() + elem);
			break;
		case CLEAR:
			wf->mbtnMem->clear();
			qrg_list.clear();
			break;
		case APPEND: case REPLACE:
			m.rfcarrier = wf->rfcarrier();
			m.carrier = active_modem->get_freq();
			m.mode = active_modem->get_mode();
			if (op == APPEND) {
				if (find(qrg_list.begin(), qrg_list.end(), m) == qrg_list.end())
					qrg_list.push_back(m);
			else
				break;
			}
			else
				qrg_list[elem] = m;
// write the menu item text
			{
				std::ostringstream o;
				o << mode_info[m.mode].name << " @@ ";
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
				if (op == APPEND) {
					wf->mbtnMem->add(o.str().c_str());
				} else {
					wf->mbtnMem->replace(elem, o.str().c_str());
				}
			}
			break;
	}

	restoreFocus();
}

void waterfall::opmode() {
	if (!active_modem) return;
	int val = (int)active_modem->get_bandwidth();

	wfdisp->carrier((int)CLAMP(
		wfdisp->carrier(),
		progdefaults.LowFreqCutoff + val / 2,
		progdefaults.HighFreqCutoff - val / 2));

	wfdisp->Bandwidth( val );
	wfcarrier->range(progdefaults.LowFreqCutoff + val/2, progdefaults.HighFreqCutoff - val/2);
}

void waterfall::carrier(int f) {
	wfdisp->carrier(f);
	wfcarrier->value(f);
	wfcarrier->damage(FL_DAMAGE_ALL);
}

int waterfall::Speed() {
	return (int)wfdisp->Speed();
}

void waterfall::Speed(int rate)
{
	WFspeed speed = static_cast<WFspeed>(rate);
	wfdisp->Speed(speed);

	const char* label;
	switch (speed) {
	case SLOW:
		label = "SLOW";
		break;
	case NORMAL: default:
		label = "NORM";
		break;
	case FAST:
		label = "FAST";
		break;
	case PAUSE:
		label = "PAUSE";
		break;
	}

	wfrate->label(label);
	wfrate->redraw_label();
}

int waterfall::Mag() {
	return wfdisp->Mag();
}

void waterfall::Mag(int m) {
	wfdisp->Mag(m);
	if (m == MAG_1) x1->label("x1");
	if (m == MAG_2) x1->label("x2");
	if (m == MAG_4) x1->label("x4");
	x1->redraw_label();
}

int waterfall::Offset() {
	return wfdisp->Offset();
}

void waterfall::Offset(int v) {
	wfdisp->Offset(v);
}

int waterfall::Carrier()
{
	return wfdisp->carrier();
}

void waterfall::Carrier(int f)
{
	if (active_modem) active_modem->set_freq(f);
}

void waterfall::rfcarrier(long long cf) {
	wfdisp->rfcarrier(cf);
}

long long waterfall::rfcarrier() {
	return wfdisp->rfcarrier();
}

void waterfall::setRefLevel() {
	wfRefLevel->value(progdefaults.wfRefLevel);
}

void waterfall::setAmpSpan() {
	wfAmpSpan->value(progdefaults.wfAmpSpan);
	wfdisp->Ampspan(progdefaults.wfAmpSpan);
}

void waterfall::USB(bool b) {
	if (wfdisp->USB() == b)
		return;
	wfdisp->USB(b);
	if (active_modem) active_modem->set_reverse(reverse);
	REQ(&viewer_redraw);
}

bool waterfall::USB() {
	return wfdisp->USB();
}

void waterfall::show_scope(bool on)
{
	if (on) {
		wfscope->show();
		wfscope->position(wf->x() + wf->w() - wf_dim - BEZEL, wf->y());
		wfdisp->size( wf->w() - 2 * BEZEL - wf_dim, wf_dim - 2 * BEZEL);
		rs1->init_sizes();
	} else {
		wfscope->hide();
		wfscope->position(wf->x() + wf->w(), wf->y());
		wfdisp->size( wf->w() - 2 * BEZEL, wf_dim - 2 * BEZEL);
		rs1->init_sizes();
	}
	wfscope->redraw();
}

waterfall::waterfall(int x0, int y0, int w0, int h0, char *lbl) :
	Fl_Group(x0,y0,w0,h0,lbl) {
	int xpos;
	float ratio;
	ratio = w0 * 1.0 / bwdths;

	wf_dim = h() - BTN_HEIGHT - 4;

	buttonrow = h() + y() - BTN_HEIGHT - 1;

	rs1 = new Fl_Group(x(), y(), w(), wf_dim);
		rs1->box(FL_DOWN_BOX);
		wfdisp = new WFdisp(
			x() + BEZEL,
			y() + BEZEL,
			w() - 2 * BEZEL,
			wf_dim - 2 * BEZEL);
		wfscope = new Digiscope (x() + w(), y(), wf_dim, wf_dim);
		rs1->resizable(wfdisp);
	rs1->end();
	wfscope->hide();

	xpos = x() + wSpace;

	mode = new Fl_Button(xpos, buttonrow, (int)(bwFFT*ratio), BTN_HEIGHT, "WF");
	mode->callback(mode_cb, 0);
	mode->tooltip(_("Waterfall / FFT / Scope"));

	xpos = xpos + (int)(bwFFT*ratio) + wSpace;
	wfRefLevel = new Fl_Counter2(xpos, buttonrow, (int)(cwRef*ratio), BTN_HEIGHT );
	wfRefLevel->callback(reflevel_cb, 0);
	wfRefLevel->step(1.0);
	wfRefLevel->precision(0);
	wfRefLevel->range(-80.0, 0.0);//(-40.0, 0.0);
	wfRefLevel->value(0.0);//(-20.0);
	wfRefLevel->tooltip(_("Upper signal level (dB)"));
	wfRefLevel->type(FL_SIMPLE_COUNTER);

	xpos = xpos + (int)(cwRef*ratio) + wSpace;
	wfAmpSpan = new Fl_Counter2(xpos, buttonrow, (int)(cwRef*ratio), BTN_HEIGHT );
	wfAmpSpan->callback(ampspan_cb, 0);
	wfAmpSpan->step(1.0);
	wfAmpSpan->precision(0);
	wfAmpSpan->range(6.0, 90.0);
	wfAmpSpan->value(70.0);
	wfdisp->Ampspan(70.0);
	wfAmpSpan->tooltip(_("Signal range (dB)"));
	wfAmpSpan->type(FL_SIMPLE_COUNTER);

	xpos = xpos + (int)(cwRef*ratio) + wSpace;
	x1 = new Fl_Button(xpos, buttonrow, (int)(bwX1*ratio), BTN_HEIGHT, "x1");
	x1->callback(x1_cb, 0);
	x1->tooltip(_("Change waterfall scale"));

	xpos = xpos + (int)(bwX1*ratio) + wSpace;
	left = new Fl_Repeat_Button(xpos, buttonrow, (int)(bwMov*ratio), BTN_HEIGHT, "@<");
	left->callback(slew_left, 0);
	left->tooltip(_("Slew display lower in frequency"));

	xpos = xpos + (int)(bwMov*ratio);
	center = new Fl_Button(xpos, buttonrow, (int)(bwMov*ratio), BTN_HEIGHT, "@||");
	center->callback(center_cb, 0);
	center->tooltip(_("Center display on signal"));

	xpos = xpos + (int)(bwMov*ratio);
	right = new Fl_Repeat_Button(xpos, buttonrow, (int)(bwMov*ratio), BTN_HEIGHT, "@>");
	right->callback(slew_right, 0);
	right->tooltip(_("Slew display higher in frequency"));

	xpos = xpos + (int)(bwMov*ratio) + wSpace;
	wfrate = new Fl_Button(xpos, buttonrow, (int)(bwRate*ratio), BTN_HEIGHT, "Norm");
	wfrate->callback(rate_cb, 0);
	wfrate->tooltip(_("Waterfall drop speed"));

	xpos = xpos + (int)(bwRate*ratio) + wSpace;
	wfcarrier = new Fl_Counter2(xpos, buttonrow, (int)(cwCnt*ratio), BTN_HEIGHT );
	wfcarrier->callback(carrier_cb, 0);
	wfcarrier->step(1.0);
	wfcarrier->lstep(10.0);
	wfcarrier->precision(0);
	wfcarrier->range(16.0, progdefaults.HighFreqCutoff - 16.0);
	wfcarrier->value(wfdisp->carrier());
	wfcarrier->tooltip(_("Adjust cursor frequency"));

	xpos = xpos + (int)(cwCnt*ratio) + wSpace;
	qsy = new Fl_Button(xpos, buttonrow, (int)(bwQsy*ratio), BTN_HEIGHT, "QSY");
	qsy->callback(qsy_cb, 0);
	qsy->tooltip(_("Center in passband\nRight click to undo"));
	qsy->deactivate();

	xpos = xpos + (int)(bwQsy*ratio) + wSpace;
	btnMem = new Fl_Button(xpos, buttonrow, (int)(bwMem*ratio), BTN_HEIGHT, "Store");
	btnMem->callback(btnMem_cb, 0);
	btnMem->tooltip(_("Store mode and frequency\nRight click for list"));
	mbtnMem = new Fl_Menu_Button(btnMem->x(), btnMem->y(), btnMem->w(), btnMem->h(), 0);
	mbtnMem->callback(btnMem->callback(), mbtnMem);
	mbtnMem->type(Fl_Menu_Button::POPUP3);

	xpos = xpos + (int)(bwMem*ratio) + wSpace;
	xmtlock = new Fl_Light_Button(xpos, buttonrow, (int)(bwXmtLock*ratio), BTN_HEIGHT, "Lk");
	xmtlock->callback(xmtlock_cb, 0);
	xmtlock->value(0);
	xmtlock->selection_color(progdefaults.LkColor);
	xmtlock->tooltip(_("Lock transmit frequency"));

	/// We save this flag which is used by rtty decoding.
	xpos = xpos + (int)(bwXmtLock*ratio) + wSpace;
	btnRev = new Fl_Light_Button(xpos, buttonrow, (int)(bwRev*ratio), BTN_HEIGHT, "Rv");
	btnRev->callback(btnRev_cb, 0);
	reverse = progdefaults.rtty_reverse;
	btnRev->value(reverse);
	btnRev->selection_color(progdefaults.RevColor);
	btnRev->tooltip(_("Reverse"));

	xpos = w() - (int)(bwXmtRcv*ratio) - wSpace;
	xmtrcv = new Fl_Light_Button(xpos, buttonrow, (int)(bwXmtRcv*ratio) - BEZEL, BTN_HEIGHT, "T/R");
	xmtrcv->callback(xmtrcv_cb, 0);
	xmtrcv->selection_color(progdefaults.XmtColor);
	xmtrcv->value(0);
	xmtrcv->tooltip(_("Transmit/Receive"));
	end();
}

void waterfall::UI_select(bool on) {
	if (on) {
		if (!progdefaults.WF_UIrev)
			btnRev->hide();
		else 
			btnRev->show();
		if (!progdefaults.WF_UIwfcarrier)
			wfcarrier->hide(); else wfcarrier->show();
		if (!progdefaults.WF_UIwfreflevel)
			wfRefLevel->hide(); else wfRefLevel->show();
		if (!progdefaults.WF_UIwfampspan)
			wfAmpSpan->hide(); else wfAmpSpan->show();
		if (!progdefaults.WF_UIxmtlock)
			xmtlock->hide(); else xmtlock->show();
		if (!progdefaults.WF_UIqsy)
			qsy->hide(); else qsy->show();
		if (!progdefaults.WF_UIwfmode)
			mode->hide(); else mode->show();
		if (!progdefaults.WF_UIx1)
			x1->hide(); else x1->show();
		if (!progdefaults.WF_UIwfshift) {
			left->hide();
			center->hide();
			right->hide();
		} else {
			left->show();
			center->show();
			right->show();
		}
		if (!progdefaults.WF_UIwfdrop)
			wfrate->hide(); else wfrate->show();
		if (!progdefaults.WF_UIwfstore) {
			btnMem->hide();
			mbtnMem->hide();
		} else {
			btnMem->show();
			mbtnMem->show();
		}
//if (noUI) xmtrcv->hide();
	} else {
//		btnRev->show();
		if (!progdefaults.WF_UIrev)
			btnRev->hide();
		else btnRev->show();
		wfcarrier->show();
		wfRefLevel->show();
		wfAmpSpan->show();
		xmtlock->show();
		qsy->show();
		mode->show();
		x1->show();
		left->show();
		center->show();
		right->show();
		wfrate->show();
		btnMem->show();
		mbtnMem->show();
	}
	btnRev->redraw();
	wfcarrier->redraw();
	wfRefLevel->redraw();
	wfAmpSpan->redraw();
	xmtlock->redraw();
	qsy->redraw();
	mode->redraw();
	x1->redraw();
	left->redraw();
	center->redraw();
	right->redraw();
	wfrate->redraw();
	btnMem->redraw();
	mbtnMem->redraw();
}

int waterfall::handle(int event)
{
	if (event != FL_MOUSEWHEEL || Fl::event_inside(wfdisp))
		return Fl_Group::handle(event);

	int d;
	if ( !((d = Fl::event_dy()) || (d = Fl::event_dx())) )
		return 1;

	// this does not belong here, but we don't have access to this widget's
	// handle method (or its parent's)
	if (active_modem && Fl::event_inside(MODEstatus)) {
		trx_mode mode = active_modem->get_mode();
		for (;;) {
			mode = WCLAMP(mode + d, 0, NUM_MODES - 1);
			if ((mode >= NUM_RXTX_MODES && mode < NUM_MODES) ||
				progdefaults.visible_modes.test(mode))
				break;
		}
		init_modem(mode);
		return 1;
	}
	// as above; handle wheel events for the macro bar
	extern void altmacro_cb(Fl_Widget *w, void *v);
	if (progdefaults.macro_wheel) {
		if (progdefaults.mbar_scheme > MACRO_SINGLE_BAR_MAX) {
			if (Fl::event_inside(macroFrame2)) {
				altmacro_cb(btnAltMacros2, reinterpret_cast<void *>(d));
				return 1;
			}
		} else {
			if (Fl::event_inside(macroFrame1)) {
				altmacro_cb(btnAltMacros1, reinterpret_cast<void *>(d));
				return 1;
			}
		}
	}

	return Fl_Group::handle(event);
}

static Fl_Cursor cursor = FL_CURSOR_DEFAULT;

static void hide_cursor(void *w)
{
	if (cursor != FL_CURSOR_NONE)
		reinterpret_cast<Fl_Widget *>(w)->window()->cursor(cursor = FL_CURSOR_NONE);
}

void waterfall::insert_text(bool check)
{
	if (active_modem && check) {
		qrg_mode_t m;
		m.rfcarrier = wf->rfcarrier();
		m.carrier = active_modem->get_freq();
		m.mode = active_modem->get_mode();
		extern qrg_mode_t last_marked_qrg;
		if (last_marked_qrg.mode == m.mode && last_marked_qrg.rfcarrier == m.rfcarrier &&
			abs(last_marked_qrg.carrier - m.carrier) <= 16)
			return;
		last_marked_qrg = m;
	}

	std::string::size_type i;
	if ((i = progdefaults.WaterfallClickText.find("<FREQ>")) != std::string::npos) {
		std::string s = progdefaults.WaterfallClickText;
		s[i] = '\0';
		ReceiveText->addstr(s);
		note_qrg(false);
//		ReceiveText->addstr(s);
//		ReceiveText->addstr(s.c_str() + i + strlen("<FREQ>"));
	}
	else
		ReceiveText->addstr(progdefaults.WaterfallClickText, FTextView::SKIP);
}

static void find_signal_text(void)
{
	if (!active_modem) return;
	int freq = active_modem->get_freq();
	trx_mode mode = active_modem->get_mode();

	extern std::map<std::string, qrg_mode_t> qrg_marks;
	std::map<std::string, qrg_mode_t>::const_iterator i;
	for (i = qrg_marks.begin(); i != qrg_marks.end(); ++i)
		if (i->second.mode == mode && abs(i->second.carrier - freq) <= 20)
			break;
	if (i != qrg_marks.end()) {
		// Search backward from the current text cursor position, then
		// try the other direction
		int pos = ReceiveText->insert_position();
		if (ReceiveText->buffer()->search_backward(pos, i->first.c_str(), &pos, 1) ||
			ReceiveText->buffer()->search_forward(pos, i->first.c_str(), &pos, 1)) {
			ReceiveText->insert_position(pos);
			ReceiveText->show_insert_position();
		}
	}
}

int WFdisp::handle(int event)
{
	static int pxpos, push;
	if (!(event == FL_LEAVE || Fl::event_inside(this))) {
		if (event == FL_RELEASE)
			push = 0;
		return 0;
	}

	if (trx_state != STATE_RX)
		return 1;

	int xpos = Fl::event_x() - x();
	int ypos = Fl::event_y() - y();
	int eb;

	if (active_modem == fmt_modem) {
		int nuf = cursorFreq(xpos);
		if ((Fl::event_state() & (FL_SHIFT)) == FL_SHIFT) {
			set_unk_freq_value(nuf);
		} else if ((Fl::event_state() & (FL_CTRL)) == FL_CTRL) {
			set_ref_freq_value(nuf);
		}
		return 1;
	}

	switch (event) {
	case FL_MOVE:
		if (progdefaults.WaterfallQSY && ypos < WFTEXT + WFSCALE) {
			Fl::remove_timeout(hide_cursor, this);
			if (cursor != FL_CURSOR_WE)
				window()->cursor(cursor = FL_CURSOR_WE);
			if (wantcursor) {
				wantcursor = false;
				makeMarker();
			}
			break;
		}
		if (cursor != FL_CURSOR_DEFAULT)
			window()->cursor(cursor = FL_CURSOR_DEFAULT);
		if (!Fl::has_timeout(hide_cursor, this))
			Fl::add_timeout(1, hide_cursor, this);
		wantcursor = true;
		cursorpos = xpos;
		makeMarker();
		break;
	case FL_DRAG: case FL_PUSH:
		killMacroTimer();

		switch (eb = Fl::event_button()) {
		case FL_RIGHT_MOUSE:
			wantcursor = false;
			if (event == FL_PUSH) {
				tmp_carrier = true;
				oldcarrier = carrier();
				if (progdefaults.WaterfallHistoryDefault)
					bHistory = true;
			}
			goto lrclick;
		case FL_LEFT_MOUSE:
			if ((Fl::event_state() & (FL_ALT | FL_CTRL)) == (FL_ALT | FL_CTRL)) {
				if (notch_frequency)
					notch_off();
				else
					notch_on(cursorFreq(xpos));
				return 1;
			}
			if (event == FL_PUSH) {
				push = ypos;
				pxpos = xpos;
				if (Fl::event_clicks())
					return 1;
			}
			if (progdefaults.WaterfallQSY && push < WFTEXT + WFSCALE) {
				long long newrfc = (pxpos - xpos) * step;
				if (!USB())
					newrfc = -newrfc;
				newrfc += rfcarrier();
				qsy(newrfc, active_modem ? active_modem->get_freq() : 1500);
				pxpos = xpos;
				return 1;
			}
		lrclick:
			if (Fl::event_state() & FL_CTRL) {
				if (event == FL_DRAG)
					break;
				if (!progdefaults.WaterfallHistoryDefault)
					bHistory = true;
				if (eb == FL_LEFT_MOUSE) {
					   restoreFocus();
					   break;
				}
			}
			if (progdefaults.WaterfallHistoryDefault)
				bHistory = true;
			newcarrier = cursorFreq(xpos);
			if (active_modem) {
				newcarrier = (int)CLAMP(
					newcarrier,
					progdefaults.LowFreqCutoff + active_modem->get_bandwidth() / 2,
					progdefaults.HighFreqCutoff - active_modem->get_bandwidth() / 2);
				active_modem->set_freq(newcarrier);
				viewer_paste_freq(newcarrier);
				if (!(Fl::event_state() & FL_SHIFT))
					active_modem->set_sigsearch(SIGSEARCH);
			}
			restoreFocus();
			break;
		case FL_MIDDLE_MOUSE:
			if (event == FL_DRAG)
				break;
			btnAFC->value(!btnAFC->value());
			btnAFC->do_callback();
		}
		break;
	case FL_RELEASE:
		switch (eb = Fl::event_button()) {
		case FL_RIGHT_MOUSE:
			tmp_carrier = false;
			if (active_modem) active_modem->set_freq(oldcarrier);
			restoreFocus();
			// fall through
		case FL_LEFT_MOUSE:
			push = 0;
			oldcarrier = newcarrier;
			if (eb != FL_LEFT_MOUSE || !ReceiveText->visible())
				break;
			if (eb == FL_LEFT_MOUSE)
				recenter_spectrum_viewer();
			if (!(Fl::event_state() & (FL_CTRL | FL_META | FL_ALT | FL_SHIFT))) {
				if (Fl::event_clicks() == 1)
					note_qrg(true, "\n", "\n");
				else
					if (progdefaults.WaterfallClickInsert)
						wf->insert_text(true);
			}
			else if (Fl::event_state() & (FL_META | FL_ALT))
				find_signal_text();
			break;
		}
		break;

	case FL_MOUSEWHEEL:
	{
		killMacroTimer();

		int d;
		if ( !((d = Fl::event_dy()) || (d = Fl::event_dx())) )
			break;
		int state = Fl::event_state();
		if (state & FL_CTRL)
			wf->handle_mouse_wheel(waterfall::WF_AFC_BW, d);
		else if (state & (FL_META | FL_ALT))
			wf->handle_mouse_wheel(waterfall::WF_SIGNAL_SEARCH, d);
		else if (state & FL_SHIFT)
			wf->handle_mouse_wheel(waterfall::WF_SQUELCH, d);
		else {
			if (progdefaults.WaterfallQSY && Fl::event_inside(x(), y(), w(), WFTEXT+WFSCALE+WFMARKER))
				qsy(wf->rfcarrier() - 500*d);
			else
				wf->handle_mouse_wheel(progdefaults.WaterfallWheelAction, d);
		}
		return handle(FL_MOVE);
	}
	case FL_SHORTCUT:
		if (Fl::event_inside(this))
			take_focus();
		break;
	case FL_KEYBOARD:
	{
		killMacroTimer();

		int d = (Fl::event_state() & FL_CTRL) ? 10 : 1;
		int k = Fl::event_key();
		switch (k) {
		case FL_Left: case FL_Right:
			if (k == FL_Left)
				d = -d;
			if (active_modem) {
				oldcarrier = newcarrier = (int)CLAMP(
					carrier() + d,
					progdefaults.LowFreqCutoff + active_modem->get_bandwidth() / 2,
					progdefaults.HighFreqCutoff - active_modem->get_bandwidth() / 2);
				active_modem->set_freq(newcarrier);
			}
			break;
		case FL_Tab:
			restoreFocus();
			break;
		default:
			restoreFocus();
			return TransmitText->handle(event);
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
		if (cursor != FL_CURSOR_DEFAULT)
			window()->cursor(cursor = FL_CURSOR_DEFAULT);
		wantcursor = false;
		makeMarker();
		break;

	}

	return 1;
}

void waterfall::handle_mouse_wheel(int what, int d)
{
	if (d == 0)
		return;

	Fl_Valuator *val = 0;
	const char* msg_fmt = 0, *msg_label = 0;

	switch (what) {
	case WF_NOP:
		return;
	case WF_AFC_BW:
	{
		if (active_modem) {
			trx_mode m = active_modem->get_mode();
			if (m >= MODE_PSK_FIRST && m <= MODE_PSK_LAST) {
				val = mailserver ? cntServerOffset : cntSearchRange;
				msg_label = "Srch Rng";
			}
			else if (m >= MODE_HELL_FIRST && m <= MODE_HELL_LAST) {
				val = sldrHellBW;
				msg_label = "BW";
			}
			else if (m == MODE_CW) {
				val = sldrCWbandwidth;
				msg_label = "BW";
			}
			else
				return;
			msg_fmt = "%s: %2.0f Hz";
		}
		break;
	}
	case WF_SIGNAL_SEARCH:
		if (d > 0) {
			if (active_modem) active_modem->searchDown();
		} else {
			if (active_modem) active_modem->searchUp();
		}
		return;
	case WF_SQUELCH:
		val = sldrSquelch;
		d = -d;
		msg_fmt = "%s = %2.0f %%";
		msg_label = "Squelch";
		break;
	case WF_CARRIER:
		val = wfcarrier;
		break;
	case WF_MODEM:
		init_modem(d > 0 ? MODE_NEXT : MODE_PREV);
		return;
	case WF_SCROLL:
		(d > 0 ? right : left)->do_callback();
		return;
	}

	val->value(val->clamp(val->increment(val->value(), -d)));
	bool changed_save = progdefaults.changed;
	val->do_callback();
	progdefaults.changed = changed_save;
	if (val == cntServerOffset || val == cntSearchRange) {
		if (active_modem) active_modem->set_sigsearch(SIGSEARCH);
	} else if (val == sldrSquelch) { // sldrSquelch gives focus to TransmitText
		take_focus();
	}
	if (msg_fmt) {
		char msg[60];
		snprintf(msg, sizeof(msg), msg_fmt, msg_label, val->value());
		put_status(msg, 2.0);
	}
}

const char* waterfall::wf_wheel_action[] = {
	_("None"), _("AFC range or BW"),
	_("Signal search"), _("Squelch level"),
	_("Modem carrier"), _("Modem"), _("Scroll")
};
