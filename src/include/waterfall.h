// ----------------------------------------------------------------------------
// Waterfall Spectrum Analyzer Widget
// Copyright (C) 2006-2010 Dave Freese, W1HKJ
// Copyright (C) 2008 Stelios Bounanos, M0GLD
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

#ifndef _WF_H
#define _WF_H

#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Box.H>

#include "gfft.h"
#include "fldigi-config.h"
#include "digiscope.h"
#include "flslider2.h"

enum {
	WF_FFT_RECTANGULAR, WF_FFT_BLACKMAN, WF_FFT_HAMMING,
	WF_FFT_HANNING, WF_FFT_TRIANGULAR
};

#define FFT_LEN		8192
#define SC_SMPLRATE	8000
#define WFBLOCKSIZE 512

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

// you can change the basic fft processing type by a simple change in the
// following typedef.  change to float if you need to skimp on cpu cycles.

typedef double wf_fft_type;
typedef std::complex<wf_fft_type> wf_cpx_type;

extern	RGBI	mag2RGBI[256];
extern	RGB		palette[9];

enum WFmode {
	WATERFALL,
	SPECTRUM,
	SCOPE,
	NUM_WF_MODES
};

#define MAG_1 1
#define MAG_2 2
#define MAG_4 3

//enum WFspeed {FAST = 1, NORMAL = 2, SLOW = 8};
enum WFspeed { PAUSE = 0, FAST = 1, NORMAL = 2, SLOW = 4 };

extern void do_qsy(bool);

class WFdisp : public Fl_Widget {
public:

	WFdisp (int x, int y, int w, int h, char *lbl = 0);
	~WFdisp ();
	int wfmag();
	int setMag(int m);
	void setOffset(int v);

	void Mode(WFmode M) {
		mode = M;
	}
	WFmode Mode() {
		return mode;
	}
	int cursorFreq(int xpos) {
		return (offset + step * xpos);
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
	int Offset() { return offset;}
	void Offset(int v) { setOffset(v);}

	void initmaps();
	void draw();
	int handle(int event);
	void update_sigmap();
	void update_waterfall();
	void checkoffset();
	void slew(int);
	void movetocenter();
	void carrier(int cf);
	int  carrier();
	inline void  makeNotch_(int notch_frequency);
	inline void makeMarker_(int width, const RGB* color, int freq, const RGB* clrMin, RGB* clrM, const RGB* clrMax);
	void makeMarker();
	void process_analog(wf_fft_type *sig, int len);
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

	void updateMarker() {
		drawMarker();};
	int peakFreq(int f0, int delta);
	double powerDensity(double f0, double bw);
	double powerDensityMaximum(int bw_nb, const int (*bw)[2]) const ;

	void setPrefilter(int v);
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
	bool	cursormoved;
	WFspeed	wfspeed;
	int		srate;
	RGBI	*fft_img;
	RGB		*markerimage;
	RGB		RGBmarker;
	RGB		RGBcursor;
	RGBI		RGBInotch;
    double  *fftwindow;
	uchar	*scaleimage;
	uchar	*fft_sig_img;
	uchar	*sig_img;
	uchar	*scline;

	wf_cpx_type *wfbuf;

	short int	*fft_db;
	int			ptrFFTbuff;
	double		*circbuff;
	int			ptrCB;
	wf_fft_type	*pwr;
	g_fft<wf_fft_type> *wfft;
	int     prefilter;


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
	void show_scope(bool on);
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

	void xmtrcv_selection_color(Fl_Color clr) {xmtrcv->selection_color(clr);}
	void reverse_selection_color(Fl_Color clr) {btnRev->selection_color(clr);}
	void xmtlock_selection_color(Fl_Color clr) {xmtlock->selection_color(clr);}

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

	double powerDensityMaximum(int bw_nb, const int (*bw)[2]) const
	{
		return (wfdisp->powerDensityMaximum(bw_nb,bw));
	}

	int Speed();
	void Speed(int rate);
	int Mag();
	void Mag(int m);
	int Offset();
	void Offset(int v);
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
	}
	double Pwr(int i) { return wfdisp->Pwr(i); }

	int handle(int event);

	void insert_text(bool check = false);

	enum { WF_NOP, WF_AFC_BW, WF_SIGNAL_SEARCH, WF_SQUELCH,
	       WF_CARRIER, WF_MODEM, WF_SCROLL };
	static const char* wf_wheel_action[7];
	void handle_mouse_wheel(int what, int d);

	Fl_Button	*btnRev;
	Fl_Counter2	*wfcarrier;
	Fl_Counter2	*wfRefLevel;
	Fl_Counter2	*wfAmpSpan;
	Fl_Light_Button	*xmtrcv;
	Fl_Light_Button *xmtlock;
	Fl_Button	*qsy;

	void UI_select(bool);

	Digiscope	*wfscope;

private:
	bool		ishidden;
	int			wf_dim;
	Fl_Box		*bezel;
	WFdisp		*wfdisp;
	Fl_Group	*rs1, *rs2, *hidden;
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
