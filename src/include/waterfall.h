/* 
 * Waterfall Spectrum Analyzer Widget
 * Copyright (C) 2006 Dave Freese, W1HKJ
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 * Please report all bugs and problems to "w1hkj@w1hkj.com".
 */

#ifndef _WF_H
#define _WF_H

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "complex.h"
#include "fft.h"
#include "sound.h"
#include "globals.h"
#include "fldigi-config.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Repeat_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Choice.H>
#include <FL/Enumerations.H>

/*
#ifdef HAVE_DFFTW_H
#  include <dfftw.h>
#endif
#ifdef HAVE_FFTW_H
#  include <fftw.h>
#endif
*/

// recommended minimum size for the control is width = 504, height = 104;
// the actual waterfall will be width -4 (bezel size) and
//                              height - 4 - 24 (bezel, text, scale & marker)

#define FFT_LEN		4096

#define SC_SMPLRATE	8000
 
#define fftabs(a,b) sqrt((a)*(a) + (b)*(b))

struct RGB {
	uchar R;
	uchar G;
	uchar B;
};

struct RGBI {
	uchar R;
	uchar G;
	uchar B;
	uchar I;
};

extern 	RGBI	mag2RGBI[256];
extern	RGB		palette[9];

enum WFmode {
	WATERFALL,
	SPECTRUM,
	SCOPE
};

#define MAG_1 1
#define MAG_2 2
#define MAG_4 3

//enum WFspeed {FAST = 1, NORMAL = 2, SLOW = 8};
enum WFspeed {FAST = 1, NORMAL = 2, SLOW = 4, PAUSE = INT_MAX};

class WFdisp : public Fl_Widget {
public:

	WFdisp (int x, int y, int w, int h, char *lbl = 0);
	~WFdisp ();
	int wfmag();
	int setMag(int m);

	void Mode(WFmode M) {
		mode = M;
	}
	WFmode Mode() {
		return mode;
	}
	int cursorFreq(int xpos) {
		return (offset + step * xpos);
	}
	void DispColor(bool Y) {
		dispcolor = Y;
	}
	bool DispColor() {
		return dispcolor;
	}
	void Ampspan(double AmpSpn) {
		ampspan = (int)AmpSpn;
	}
	double Ampspan() {
		return ampspan;
	}
	void Reflevel(double RefLev) {
		reflevel = (int)RefLev;
	}
	double Reflevel() {
		return reflevel;
	}
	void Bandwidth (int bw) {
		bandwidth = bw;
		makeMarker();
	}
	int  Bandwidth () {
		return bandwidth;
	}
	void Overload(int ovr) {
		if (overload == ovr) return;
		overload = ovr;
	}

    double AudioPeak() { return peakaudio; }

	WFspeed Speed() { return wfspeed;}
	void Speed(WFspeed rate) { wfspeed = rate;}
	
	int Mag() { return mag;}
	void Mag(int m) { setMag(m);}
	
	void initmaps();
	void draw();
//	void resize (int, int, int, int);
	int handle(int event);
	void update_sigmap();
	void update_waterfall();
	void checkoffset();
	void slew(int);
	void movetocenter();
	void carrier(int cf);
	int  carrier();
	void makeMarker();
	void process_analog(double *sig, int len);
	void processFFT();
	void sig_data( double *sig, int len, int sr );
	void rfcarrier(long long f) { 
		rfc = f;
	}
	void USB(bool b) { 
		usb = b;
	}
	bool USB() {return usb;};
	long long rfcarrier() { return rfc;};
	
//	void useBands(bool b) { usebands = b;};
	
	void updateMarker() { 
		drawMarker();};
	int peakFreq(int f0, int delta);
	double powerDensity(double f0, double bw);
	void setPrefilter(int v) {
		switch (v) {
			case 0: RectWindow(fftwindow, FFT_LEN*2); break;
			case 1: BlackmanWindow(fftwindow, FFT_LEN*2); break;
			case 2: HammingWindow(fftwindow, FFT_LEN*2); break;
			case 3: HanningWindow(fftwindow, FFT_LEN*2); break;
			case 4: TriangularWindow(fftwindow, FFT_LEN*2); break;
		}
//		switch (v) {
//			case 0: wfft->setWindow(FFT_NONE); break;
//			case 1: wfft->setWindow(FFT_BLACKMAN); break;
//			case 2: wfft->setWindow(FFT_HAMMING); break;
//			case 3: wfft->setWindow(FFT_HANNING); break;
//			case 4: wfft->setWindow(FFT_TRIANGULAR); break;
//		}
	}
	void setcolors();
	double dFreq() {return dfreq;}
	void redrawCursor();
	void defaultColors();
	
private:
	int disp_width;
	int image_width;
	int scale_width;
	int RGBwidth;
	int RGBsize;
	int image_height;
	int image_area;
	int sig_image_area;
	int	mag;
	int magset;
	WFmode	mode;
	bool	overload;
	bool	usb;
	long long	rfc;
//	bool	usebands;
	int		offset;
	int		sigoffset;
	int		step;
	int		carrierfreq;
	int		bandwidth;
	int		wfspdcnt;
	int		dispcnt;
	int 	ampspan;
    double  peakaudio;
	int 	reflevel;
	double	dfreq;
	bool	centercarrier;
	bool	dispcolor;
	bool	cursormoved;
	WFspeed	wfspeed;
	int		srate;
	RGBI	*fft_img;
//	RGBI	mag2RGBI[256];
	RGB		*markerimage;
	RGB		RGBmarker;
	RGB		RGBcursor;
	double	*fftout;
    double  *fftwindow;
	uchar	*scaleimage;
	uchar	*fft_sig_img;
	uchar	*sig_img;
	uchar	*scline;
	
	short int	*fft_db;
	int			ptrFFTbuff;
	double	 	*circbuff;
	int			ptrCB;
	double		*pwr;
	Cfft		*wfft;


	int checkMag();
	void checkWidth();
	void initMarkers();
	void makeScale();
	void drawScale();
	void drawMarker();

	int	 log2disp(int v);
	void drawcolorWF();
	void drawgrayWF();
	void drawspectrum();
	void drawsignal();
	

protected:
public:
	bool	wantcursor;
	int	cursorpos;

	int	newcarrier;
	int	oldcarrier;
	bool	tmp_carrier;
	double Pwr(int i) {
		if ( i > 0 && i < IMAGE_WIDTH) return pwr[i];
		return 0.0;
	}
};

class waterfall: public Fl_Group {
	friend void x1_cb(Fl_Widget *w, void* v);
	friend void bw_rsid_cb(Fl_Widget *w, void * v);
	friend void bw_rsid_toggle(waterfall *);
//	friend void slew_cb(Fl_Widget *w, void * v);
	friend void slew_left(Fl_Widget *w, void * v);
	friend void slew_right(Fl_Widget *w, void * v);
	friend void center_cb(Fl_Widget *w, void *v);
	friend void carrier_cb(Fl_Widget *w, void *v);
	friend void mode_cb(Fl_Widget *w, void *v);
	friend void reflevel_cb(Fl_Widget *w, void *v);
	friend void ampspan_cb(Fl_Widget *w, void *v);
	friend void qsy_cb(Fl_Widget *w, void *v);
	friend void rate_cb(Fl_Widget *w, void *v);
	friend void btnMem_cb(Fl_Widget *w, void *v);
public:
	waterfall(int x, int y, int w, int h, char *lbl= 0);
	~waterfall(){};
	void opmode();
	void sig_data(double *sig, int len, int sr){
		wfdisp->sig_data(sig, len, sr);
	}
	void Overload(bool ovr) { 
		wfdisp->Overload(ovr);
	}
	int carrier() {
		return wfdisp->carrier();
	}
	void carrier(int f);
	void rfcarrier(long long cf);
	long long rfcarrier();
	bool tmp_carrier(void) { return wfdisp->tmp_carrier; }
	void set_XmtRcvBtn(bool val);
	void USB(bool b);
	bool USB();
	void Reverse( bool v) { reverse = v;}
	bool Reverse() { return reverse;}
	void Bandwidth(int bw)
	{
		wfdisp->Bandwidth(bw);
	}
	int peakFreq(int f0, int delta)
	{
		return (wfdisp->peakFreq(f0, delta));
	}
	double powerDensity(double f0, double bw)
	{
		return (wfdisp->powerDensity(f0,bw));
	}

	int Speed();
	void Speed(int rate);
	int Mag();
	void Mag(int m);
	int Carrier();
	void Carrier(int f);

	void movetocenter() { wfdisp->movetocenter();}
	void redraw_marker() { wfdisp->makeMarker(); }
	void setPrefilter(int v) {wfdisp->setPrefilter(v);}
	
	void setcolors() { wfdisp->setcolors(); }
	void setRefLevel();
	void setAmpSpan();
	double dFreq() { return wfdisp->dFreq();}
	
	void setQSY(bool on) {
		if (on)
			qsy->activate();
		else
			qsy->deactivate();
//		wfdisp->useBands(!on);
	}
	void setXMLRPC(bool on) {
//		wfdisp->useBands(!on);
	}
	double Pwr(int i) { return wfdisp->Pwr(i); }	
	
	int handle(int event);

	enum { WF_NOP, WF_AFC_BW, WF_SIGNAL_SEARCH, WF_SQUELCH,
	       WF_CARRIER, WF_MODEM, WF_SCROLL };
	static const char wf_wheel_action[];
	void handle_mouse_wheel(int what, int d);

	Fl_Button	*btnRev;
	Fl_Counter	*wfcarrier;
	Fl_Counter	*wfRefLevel;
	Fl_Counter	*wfAmpSpan;
	Fl_Light_Button	*xmtrcv;
	Fl_Light_Button *xmtlock;
	Fl_Button	*qsy;

private:
	Fl_Box		*bezel;
	WFdisp		*wfdisp;
	Fl_Button	*bw_rsid;
	Fl_Button	*mode;
	Fl_Button	*x1;
	Fl_Button	*left; 
	Fl_Button	*center;
	Fl_Button	*right;
	Fl_Button	*wfrate;
	Fl_Button	*btnMem;
	Fl_Menu_Button	*mbtnMem;
	int			buttonrow;
	bool	reverse;
};

#endif
