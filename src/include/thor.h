//
//    thor.h  --  thor modem
//
//	Copyright (C) 2008
//		David Freese (w1hkj@w1hkj.com)
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

#ifndef _thor_H
#define _thor_H

#include <string>

#include "complex.h"
#include "trx.h"
#include "fft.h"
#include "filters.h"
#include "fftfilt.h"
#include "dominovar.h"
#include "mbuffer.h"

// NASA coefficients for viterbi encode/decode algorithms
#define	THOR_K	7
#define	THOR_POLY1	0x6d
#define	THOR_POLY2	0x4f

//#include "mfskvaricode.h"
#include "interleave.h"
#include "viterbi.h"


using namespace std;

#define THORNUMTONES 18
#define THORMAXFFTS  8
#define THORBASEFREQ 500.0
#define THORFIRSTIF 1000.0

#define THORSCOPESIZE 64

#define THORSLOWPATHS 3
#define THORFASTPATHS 5

struct THORrxpipe {
	complex vector[THORMAXFFTS * THORNUMTONES * 6];
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

	complex currvector;

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

// tx variables
	int txstate;
	int txprevtone;
	unsigned int bitshreg;
	string strSecXmtText;
	unsigned int cptr;
	
	viterbi		*Dec;
	interleave	*Rxinlv;
	encoder		*Enc;
	interleave	*Txinlv;
	int			bitstate;
	unsigned char symbolpair[2];
	
private:
	complex	mixer(int n, const complex& in);

// Rx
	void	recvchar(int c);
	void	decodesymbol();
	int		harddecode();
	void	update_syncscope();
	void	synchronize();
	void	reset_afc();
	void	eval_s2n();
	int		get_secondary_char();
	void	reset_filters();
	void	decodePairs(unsigned char symbol);
//	void	decodeEX(int c);

// Tx
	void	sendtone(int tone, int duration);
	void	sendsymbol(int sym);
	void	sendchar(unsigned char c, int secondary);
	void	sendidle();
	void	sendsecondary();
	void	flushtx();
	void	Clearbits();
			
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
