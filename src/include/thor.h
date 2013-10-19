// ----------------------------------------------------------------------------
//    thor.h  --  thor modem
//
// Copyright (C) 2008-2012
//     David Freese <w1hkj@w1hkj.com>
//     John Douyere <vk2eta@gmail.com>
//     John Phelps  <kl4yfd@gmail.com>
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

#ifndef _thor_H
#define _thor_H

#include <string>

#include "complex.h"
#include "modem.h"
#include "globals.h"
#include "filters.h"
#include "fftfilt.h"
#include "dominovar.h"
#include "mbuffer.h"

// NASA coefficients for viterbi encode/decode algorithms
#define	THOR_K	7
#define	THOR_POLY1	0x6d
#define	THOR_POLY2	0x4f

//VK2ETA high speed modes
// IEEE coefficients for viterbi encode/decode algorithms
#define	THOR_K15	15
#define	K15_POLY1	044735
#define	K15_POLY2	063057

//#include "mfskvaricode.h"
#include "interleave.h"

#include "viterbi.h"


#define THORNUMTONES 18
#define THORMAXFFTS  8
#define THORBASEFREQ 1500.0
#define THORFIRSTIF  2000.0

#define THORSCOPESIZE 64

#define THORSLOWPATHS 3
#define THORFASTPATHS 5

// the following constant changes if a mode with more tones than 25x4 is
// created
#define MAXPATHS (8 * THORFASTPATHS * THORNUMTONES )

struct THORrxpipe {
	cmplx vector[THORMAXFFTS * THORNUMTONES * 6];
};

class thor : public modem {
public:
	enum {
		TX_STATE_PREAMBLE,
		TX_STATE_START,
		TX_STATE_DATA,
		TX_STATE_END,
		TX_STATE_FLUSH
	};
protected:
// common variables
	double	phase[THORMAXFFTS + 1];
	double	txphase;
	int		symlen;
	int		doublespaced;
	double	tonespacing;
	int		counter;
	unsigned int	twosym;
	int		paths;
	int		numbins;
	bool	slowcpu;
	int		basetone;
	int		lotone;
	int		hitone;
	int		extones;
	
// rx variables
	C_FIR_filter	*hilbert;
	sfft			*binsfft[THORMAXFFTS];
	fftfilt			*fft;
	Cmovavg			*vidfilter[THORSCOPESIZE];
	Cmovavg			*syncfilter;
	
	THORrxpipe		*pipe;
	unsigned int	pipeptr;
	unsigned int	datashreg;
	mbuffer<double, 0, 2>	scopedata;
	mbuffer<double, 0, 2>	videodata;

	cmplx currvector;

	int currsymbol;
	int prev1symbol;
	int prev2symbol;
	
	double currmag;
	double prev1mag;
	double prev2mag;

	double met1;
	double met2;
	double sig;
	double noise;
	double s2n;

	int synccounter;

	unsigned char symbolbuf[MAX_VARICODE_LEN];
	int symcounter;

	int symbolbit;
	
	bool filter_reset;
	bool staticburst;
	
	int fec_confidence;

// tx variables
	int txstate;
	int txprevtone;
	unsigned int bitshreg;
	std::string strSecXmtText;
	unsigned int cptr;
	
	viterbi		*Dec;
	interleave	*Rxinlv;
	encoder		*Enc;
	interleave	*Txinlv;
	int			bitstate;
	unsigned char symbolpair[2];
	
	int flushlength;

	
private:
	cmplx	mixer(int n, const cmplx& in);

// Rx
	void	recvchar(int c);
	void	decodesymbol();
	void	softdecodesymbol();
	int		harddecode();
	int		softdecode();
	void	update_syncscope();
	void	synchronize();
	void	reset_afc();
	void	eval_s2n();
	int		get_secondary_char();
	void	reset_filters();
	void	decodePairs(unsigned char symbol);
	bool	preambledetect(int c);
	void	softflushrx();

// Tx
	void	sendtone(int tone, int duration);
	void	sendsymbol(int sym);
	void	sendchar(unsigned char c, int secondary);
	void	sendidle();
	void	sendsecondary();
	void	flushtx();
	void	Clearbits();

protected:
	void	s2nreport(void);

public:
	thor (trx_mode md);
	~thor ();
	void	init();
	void	rx_init();
	void	tx_init(SoundBase *sc);
	void	restart();
	int		rx_process(const double *buf, int len);
	int		tx_process();
};

#endif
