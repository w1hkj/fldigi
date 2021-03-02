// ----------------------------------------------------------------------------
// fmt.h  --  frequency measurement modem
//
// Copyright (C) 2020
//		Dave Freese, W1HKJ
//		JC Gibbons,  N8OBJ
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

#ifndef _fmt_H
#define _fmt_H

#include <string>
#include <ctime>
#include <stdio.h>

#include "complex.h"
#include "filters.h"
#include "fftfilt.h"
#include "modem.h"

#include "threads.h"

#define DSP_CNT			1		//seconds

#define MAX_MINUTES		120
#define MAX_DATA_PTS	(60 * MAX_MINUTES)

extern pthread_mutex_t scope_mutex;

extern void fmt_write_file();
extern void FMT_thread_close(void);
extern void cb_fmt_record_wav(bool);

extern bool write_recs;
extern bool record_unk;
extern bool record_ref;

class fmt : public modem {

friend void fmt_write_file();

private:

	int      sr; // 1..6; 8000 ... 48000 sps

	double   unk_freq;
	Cmovavg  *unk_ffilt;
	Cmovavg  *unk_afilt;
	double   unk_base_freq;
	double   unk_amp;

	double   ref_freq;
	Cmovavg  *ref_ffilt;
	Cmovavg  *ref_afilt;

	double   ref_base_freq;
	double   ref_amp;

	double   movavg_len;

	double   dspcnt;

	void writeFile();

// T&R discriminator
	double			*fftbuff;
	double			*unkbuff;
	double			*refbuff;

	double			bpf_width;

	double			*BLACKMAN;
	double			Ts;
	int				ref_count;
	int				unk_count;

	double			dft_ref_base;
	double			dft_ref_amp;
	double			dft_unk_base;
	double			dft_unk_amp;

	double			am;
	double			bm;
	double			dm;
	double			delta;
	double			dmK;
	double			srK;
	double			twoPI;

	double			blackman (double omega);
	double			absdft (double *buff, double freq, double incr);
	int				rx_process_dft ();


	C_FIR_filter	*unk_bpfilter;
	C_FIR_filter	*ref_bpfilter;
	void			reset_bpf();

	double			evaluate_dft(double &freq);

public:
	fmt();
	~fmt();
	void init();
	void rx_init();
	void tx_init();
	void restart();

	void clear_ref_pipe();
	void clear_unk_pipe();

	int rx_process(const double *buf, int len);
	int tx_process();

	void reset_unknown();
	void reset_reference();

};

#endif
