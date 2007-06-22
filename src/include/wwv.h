// ----------------------------------------------------------------------------
// wwv.h  --  wwv receive only modem
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
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

#ifndef _wwv_H
#define _wwv_H

//#include "complex.h"
#include "modem.h"
#include "filters.h"
#include "fftfilt.h"


#define	wwvSampleRate	8000
#define	MaxSymLen	512

// lp filter #1
//#define	DEC_1		40
#define DEC_1		8
#define FIRLEN_1	512
#define BW_1		20
// lp filter #2
#define DEC_2		5
#define	FIRLEN_2	256
#define BW_2		100

// wwv function return status codes. 
#define	wwv_SUCCESS		0
#define	wwv_ERROR		-1

class wwv : public modem {
protected:
	double			phaseacc;
	double			phaseincr;
	int				smpl_ctr;		// sample counter for timing wwv rx 
	double			agc;			// threshold for tick detection 

	C_FIR_filter	*hilbert;
	C_FIR_filter	*lpfilter;
	Cmovavg			*vidfilter;

	double			*buffer;	// storage for 1000 samples/sec video
	unsigned int	buffptr;
	int				sync;
	int				sync0;
	int				ticks;
	int				x1;
	int				y1;
	int				x2;
	int				y2;
	bool			calc;
	bool			zoom;

public:
	wwv();
	~wwv();
	void	init();
	void	rx_init();
	void	tx_init(cSound *sc);
	void 	restart() {};
	int		rx_process(double *buf, int len);
	int		tx_process() {return -1;}
	void	update_syncscope();
	void	set1(int x, int y);
	void	set2(int x, int y);

};

#endif
