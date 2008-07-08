//
//    dominoex.h  --  DominoEX modem
//
//	Copyright (C) 2001, 2002, 2003
//	Tomi Manninen (oh2bns@sral.fi)
//	Copyright (C) 2006
//	Hamish Moffatt (hamish@debian.org)
//	Copyright (C) 2006
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

#ifndef _DOMINOEX_H
#define _DOMINOEX_H

#include <string>

#include "complex.h"
#include "trx.h"
#include "fft.h"
#include "filters.h"
#include "fftfilt.h"
#include "dominovar.h"
#include "mbuffer.h"

// NASA coefficients for viterbi encode/decode algorithms
#define	K	7
#define	POLY1	0x6d
#define	POLY2	0x4f

//#include "mfskvaricode.h"
#include "interleave.h"
#include "viterbi.h"


using namespace std;

#define NUMTONES 18
//#define MAXFFTS  4
#define MAXFFTS  8
//#define BASEFREQ 1000.0
#define BASEFREQ 500.0
#define FIRSTIF 1000.0

#define SCOPESIZE 64

struct domrxpipe {
	complex vector[MAXFFTS * NUMTONES * 6];
};

class dominoex : public modem {
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
	double	phase[MAXFFTS + 1];
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
	sfft			*binsfft[MAXFFTS];
	fftfilt			*fft;
	Cmovavg			*vidfilter[SCOPESIZE];
	Cmovavg			*syncfilter;
	
	domrxpipe		*pipe;
	unsigned int	pipeptr;
	mbuffer<double, 0, 2>	scopedata;
	mbuffer<double, 0, 2>	videodata;

	complex currvector;

	int currsymbol;
	int prev1symbol;
	int prev2symbol;

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
	bool outofrange;

// tx variables
	int txstate;
	int txprevtone;
	unsigned int bitshreg;
	string strSecXmtText;
	
// FEC variables
	viterbi		*MuPskDec;
	interleave	*MuPskRxinlv;
	encoder		*MuPskEnc;
	interleave	*MuPskTxinlv;
	int			Mu_bitstate;
	unsigned char Mu_symbolpair[2];
	unsigned int	Mu_datashreg;
	int			Mu_symcounter;
	
private:
	complex	mixer(int n, complex in);
	void	recvchar(int c);
	void	decodesymbol();
	void	decodeDomino(int c);
	int		harddecode();
	void	update_syncscope();
	void	synchronize();
	void	afc();
	void	reset_afc();
	void	eval_s2n();
	void	sendtone(int tone, int duration);
	void	sendsymbol(int sym);
	void	sendchar(unsigned char c, int secondary);
	void	sendidle();
	void	sendsecondary();
	void	flushtx();
	int		get_secondary_char();
	void	reset_filters();
// MultiPsk FEC scheme
// Rx
	unsigned int	MuPskPriSecChar(unsigned int c);
	void	decodeMuPskSymbol(unsigned char symbol);
	void	decodeMuPskEX(int c);
// Tx
	unsigned char MuPskSec2Pri(int c);
	void	sendMuPskEX(unsigned char c, int secondary);
	void	MuPskClearbits();
	void	MuPskFlushTx();
	void	MuPsk_sec2pri_init(void);
public:
	dominoex (trx_mode md);
	~dominoex ();
	void	init();
	void	rx_init();
	void	tx_init(SoundBase *sc);
	void	restart();
	int		rx_process(const double *buf, int len);
	int		tx_process();
};

#endif
