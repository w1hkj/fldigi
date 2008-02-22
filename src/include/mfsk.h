//
//    mfsk.h  --  MFSK modem

#ifndef _MFSK_H
#define _MFSK_H

#include "trx.h"
#include "fft.h"
#include "filters.h"
#include "interleave.h"
#include "viterbi.h"
#include "complex.h"
#include "mfskvaricode.h"
#include "mbuffer.h"

//#include "File_Selector.h"

#include "picture.h"
#include <FL/Fl_Shared_Image.H>

#define	MFSKSampleRate		8000

// 1 ms per pixel
#define	SAMPLES_PER_PIXEL	8
#define MAX_SYMBOLS			32
#define PICHEADER			64

// NASA coefficients for viterbi encode/decode algorithms

#define	K	7
#define	POLY1	0x6d
#define	POLY2	0x4f

struct rxpipe {
	complex vector[MAX_SYMBOLS];	//numtones <= 32
};

class mfsk : public modem {

friend void cb_picRxClose( Fl_Widget *w, void *who);
friend void cb_picTxClose( Fl_Widget *w, void *who);
friend void cb_picTxLoad(Fl_Widget *,void *who);
friend void cb_picTxSendColor( Fl_Widget *w, void *who);
friend void cb_picTxSendGrey( Fl_Widget *w, void *who);
friend void cb_picTxSendAbort( Fl_Widget *w, void *who);
friend void cb_picRxSave( Fl_Widget *w, void *who);

public:
enum {
	TX_STATE_PREAMBLE,
	TX_STATE_START,
	TX_STATE_DATA,
	TX_STATE_END,
	TX_STATE_FLUSH,
	TX_STATE_FINISH,
	TX_STATE_TUNE,
	TX_STATE_PICTURE_START,
	TX_STATE_PICTURE
};

enum {
	RX_STATE_DATA,
	RX_STATE_PICTURE_START_1,
	RX_STATE_PICTURE_START_2,
	RX_STATE_PICTURE
};

protected:
// general
	double phaseacc;
	int symlen;
	int symbits;
	int numtones;
	int basetone;
	double tonespacing;
	int counter;
// receive
	int				rxstate;
	C_FIR_filter	*hbfilt;
	sfft			*binsfft;
	C_FIR_filter	*bpfilt;
	Cmovavg			*afcfilt;

	viterbi		*dec1;
	viterbi		*dec2;
	interleave	*rxinlv;

	rxpipe		*pipe;
	unsigned int pipeptr;

	unsigned int datashreg;

	complex currvector;
	complex prev1vector;
	complex prev2vector;

	int currsymbol;
	int prev1symbol;
	int prev2symbol;

	double met1;
	double met2;
	mbuffer<double, 0, 2> scopedata;
	double s2n;
	double sig;
	double noise;

	int synccounter;

	unsigned char symbolpair[2];
	int symcounter;


	int symbolbit;

// transmit
	int txstate;
	encoder		*enc;
	interleave	*txinlv;
	unsigned int bitshreg;
	int bitstate;

// Picutre data and methods
	int picturesize;
	char picheader[PICHEADER];
	complex prevz;
	double picf;
	int		row;
	int		col;
	int		rgb;
	int		pixelnbr;
	
	int		picW;
	int		picH;
	bool	color;
	
	Fl_Window	*picRxWin;
	Fl_Box		*picRxBox;
	picture		*picRx;
	Fl_Button	*btnpicRxSave;
	Fl_Button	*btnpicRxClose;
	Fl_Window	*picTxWin;
	picture		*picTx;
	Fl_Button	*btnpicTxSendColor;
	Fl_Button	*btnpicTxSendGrey;
	Fl_Button	*btnpicTxSendAbort;
	Fl_Button	*btnpicTxLoad;
	Fl_Button	*btnpicTxClose;
	Fl_Shared_Image	*TxImg;
	unsigned char *xmtimg;
	unsigned char *xmtpicbuff;
	unsigned char picprologue[44];
	int			xmtbytes;
	bool		startpic;
	bool		abortxmt;

	void		updateTxPic(unsigned char data);
	void		TxViewerResize(int W, int H);
	void		updateRxPic(unsigned char data, int pos);
	void		makeRxViewer(int W, int H);
	void		load_file(const char *n);

	void	recvpic(complex z);
	void	recvchar(int c);
	void	recvbit(int bit);

// internal processes
	void	decodesymbol(unsigned char symbol);
	void	softdecode(complex *bins);
	complex	mixer(complex in, double f);
	int		harddecode(complex *in);
	void	update_syncscope();
	void	synchronize();
	void	afc();
	void	reset_afc();
	void	eval_s2n(complex, complex);
	void 	sendsymbol(int sym);
	void	sendbit(int bit);
	void	sendchar(unsigned char c);
	void	sendidle();
	void	flushtx();
	void	clearbits();
	void	sendpic(unsigned char *data, int len);
	bool	check_picture_header(char c);
	int	print_time_left(size_t bytes, char *str, size_t len,
			  const char *prefix = "", const char *suffix = "");
public:
	mfsk (trx_mode md);
	~mfsk ();
	void	init();
	void	rx_init();
	void	tx_init(SoundBase *sc);
	void	restart() {};
	int		rx_process(const double *buf, int len);
	int		tx_process();
	void	shutdown();
	void	makeTxViewer(int W, int H);
};

#endif
