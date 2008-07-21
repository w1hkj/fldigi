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

//#include "fileselect.h"

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

class	mfsk;

extern 	int		print_time_left(float secs, char *str, size_t len,
			  		const char *prefix = "", const char *suffix = "");
extern	void	updateTxPic(unsigned char data);
extern	void	updateRxPic(unsigned char data, int pos);
extern	void	TxViewerResize(int W, int H);
extern	void	showTxViewer(int W, int H);
extern	void	createTxViewer();
extern	void	createRxViewer();
extern	void	showRxViewer(int W, int H);
extern	void	deleteRxViewer();
extern	void	deleteTxViewer();

extern void cb_picRxClose( Fl_Widget *w, void *);
extern void cb_picRxAbort( Fl_Widget *w, void *);
extern void cb_picTxSendColor( Fl_Widget *w, void *);
extern void cb_picTxSendGrey( Fl_Widget *w, void *);
extern void cb_picTxSendAbort( Fl_Widget *w, void *);

extern	void	load_file(const char *n);

extern	Fl_Double_Window	*picRxWin;
extern	picture		*picRx;
extern	Fl_Button	*btnpicRxSave;
extern	Fl_Button	*btnpicRxAbort;
extern	Fl_Button	*btnpicRxClose;

extern	Fl_Double_Window	*picTxWin;
extern	picture		*picTx;
extern  Fl_Button	*btnpicTxSPP;
extern	Fl_Button	*btnpicTxSendColor;
extern	Fl_Button	*btnpicTxSendGrey;
extern	Fl_Button	*btnpicTxSendAbort;
extern	Fl_Button	*btnpicTxLoad;
extern	Fl_Button	*btnpicTxClose;

extern	Fl_Shared_Image	*TxImg;
extern	unsigned char *xmtimg;
extern	unsigned char *xmtpicbuff;

struct rxpipe {
	complex vector[MAX_SYMBOLS];	//numtones <= 32
};

struct history {
	complex val;
	int symnbr;
};
	
class mfsk : public modem {

#define SCOPESIZE 64

friend void updateTxPic(unsigned char data);
friend void cb_picRxClose( Fl_Widget *w, void *);
friend void cb_picRxAbort( Fl_Widget *w, void *);
friend void cb_picTxSendColor( Fl_Widget *w, void *);
friend void cb_picTxSendGrey( Fl_Widget *w, void *);
friend void cb_picTxSendAbort( Fl_Widget *w, void *);
friend void cb_picTxSPP( Fl_Widget *w, void *);
friend void load_image(const char *n);

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
	RX_STATE_PICTURE_START,
//	RX_STATE_PICTURE_START_1,
//	RX_STATE_PICTURE_START_2,
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
	double basefreq;
	int counter;
// receive
	int				rxstate;
	C_FIR_filter	*hbfilt;
	sfft			*binsfft;
	C_FIR_filter	*bpfilt;
	Cmovavg			*met1filt;
	Cmovavg			*met2filt;
	Cmovavg			*vidfilter[SCOPESIZE];
	Cmovavg			*syncfilter;

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
	double maxval;
	double prevmaxval;

	double met1;
	double met2;
	mbuffer<double, 0, 2> scopedata;
	double s2n;
	double sig;
	double noise;
	double afcmetric;
	bool	staticburst;
	
	double currfreq;

	int synccounter;
	int AFC_COUNT;

	unsigned char symbolpair[2];
	int symcounter;
	
	int RXspp; // samples per pixel
	int TXspp;

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
	
	unsigned char picprologue[176];
	int			xmtbytes;
	bool		startpic;
	bool		abortxmt;

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
	void	eval_s2n();
	void 	sendsymbol(int sym);
	void	sendbit(int bit);
	void	sendchar(unsigned char c);
	void	sendidle();
	void	flushtx();
	void	clearbits();
	void	sendpic(unsigned char *data, int len);
	bool	check_picture_header(char c);
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
};

#endif
