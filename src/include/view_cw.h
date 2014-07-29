// ----------------------------------------------------------------------------
// view_cw.h  --  cw modem
//
// Copyright (C) 2008
//		Dave Freese, W1HKJ
// Modified for CW decoder 
// Copyright (C) 2014 
//		Mauri Niininen, AG1LE
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

#ifndef _VIEWCW_H
#define _VIEWCW_H

#include "complex.h"
#include "modem.h"
#include "globals.h"
#include "filters.h"
#include "fftfilt.h"
#include "cw.h"


//=====================================================================
#define MAXCHANNELS 30
#define NULLFREQ 1e6
//=====================================================================

struct CW_CHANNEL {
	double			phaseacc;
	double			frequency;

	fftfilt			*cw_FFT_filter; // sinc / matched filter
	C_FIR_filter	*cw_FIR_filter; // linear phase finite impulse response filter
	Cmovavg			*bitfilter;

	morse 			*mp;
	double			agc_peak;
	unsigned int 	smpl_ctr;
	float  			rn,  px,  spdhat, pmax, zout;
	int				timeout;
	bool			reset;
};


#define PEAKS_SIZE 4000

struct PEAKS {
	double 	mx[PEAKS_SIZE];
	double 	mn[PEAKS_SIZE];
	int 	mxpos[PEAKS_SIZE];
	int 	mnpos[PEAKS_SIZE];
	int 	mxcount;
	int		mncount;
};

class view_cw {
private:
	trx_mode	viewmode;


	double		bandwidth;

/*	int			fa;
	int			fb;
	int			ftest;
	double		test_peak_amp;
	time_t		now;
*/	bool		reset_all;

	CW_CHANNEL		channel[MAXCHANNELS];
	int			nchannels;
	int			lowfreq;
	unsigned int	smpl_ctr;		// sample counter for timing cw rx
	Cmovavg		*bitfilter;


	void 		decode_stream(int ch, double value);

	inline void		timeout_check();


public:
	view_cw(trx_mode mode);
	~view_cw();
	void init();
	void restart(trx_mode mode);
	void rx_init(){};
	void tx_init(SoundBase *sc){};
	void restart() {};
	int rx_process(const double *buf, int len);
	int get_freq(int n);
	void set_freq(int n, double f) { channel[n].frequency = f; }
	void findsignals(struct PEAKS *p);
	void clearch(int n);
	void clear();

};

extern view_cw *cwviewer;

#endif
