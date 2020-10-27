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

#include <iostream>

#include "complex.h"
#include "modem.h"
#include "globals.h"
#include "filters.h"
#include "fftfilt.h"

#include "morse.h"

#define VCW_MAXCH		30
#define	VCW_DEC_RATIO	16
#define VCW_SAMPLERATE	 8000

// VKWPM conversion factor 
// # samples per dot = VKWPM / wpm
// 30 samples @ 20 wpm
// 15 samples @ 40 wpm
#define VKWPM			600 // ((12 * VCW_SAMPLERATE/10) / VCW_DEC_RATIO)

class view_cw;

struct CW_CHANNEL {

	static cMorse	*morse;

	fftfilt			*VCW_filter; // linear phase finite impulse response bpf
	Cmovavg			bitfilter;
	Cmovavg			trackingfilter;

	int				ch;

	double			phase;
	double			phase_increment;
	double			ch_freq;
	double			agc_peak;
	double			sig_avg;
	double			value;

	double			phi1;
	double			phi2;

	int				smpl_ctr;
	int				dec_ctr;

	cmplx			zout;
	int				timeout;

	std::string		decode_str;

	std::string rx_rep_buf;

	int cw_receive_state;
	int two_dots;

	double		norm_sig;
	double		CWupper;
	double		CWlower;
	double		noise_floor;

	int cw_rr_start_timestamp;
	int cw_rr_end_timestamp;
	int curr_element;
	int last_element;
	int space_sent;

	int clrcount;

	int _ch;
	double metric;

	double lowerwpm;
	double upperwpm;

	CW_CHANNEL();
	~CW_CHANNEL();

	int cw_lower_limit;
	int cw_upper_limit;

	void	init(int ch, double freq);
	void	update_tracking(int dot, int dash);
	inline	int sample_count(unsigned int earlier, unsigned int later);
	void	sync_parameters();
	void	reset();

	int		decode_state(int cw_event);
	void	detect_tone();
	void 	rx_process(const double *value, int len);

	double	avg_signal() { return sig_avg; }
	double	get_metric() { return metric; }
	void	set_noise_floor(double nf) { noise_floor = nf; }
	double	get_chfreq() { return ch_freq; }

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

	int			nchannels;

	unsigned int	smpl_ctr;

public:
	CW_CHANNEL	channel[VCW_MAXCH];

	view_cw();
	~view_cw();
	void init();
	void restart();
	void rx_init(){};
	void tx_init(SoundBase *sc){};
	int rx_process(const double *buf, int len);
	int get_freq(int n);

	void clear();
	void clearch(int n);

};

#endif
