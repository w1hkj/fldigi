// ----------------------------------------------------------------------------

#ifndef	_MODEM_H
#define	_MODEM_H

#include "threads.h"

#include "misc.h"
//#include "configuration.h"
#include "sound.h"
#include "waterfall.h"
#include "ScopeDialog.h"
#include "fl_digi.h"
#include "CWdialog.h"
#include "globals.h"
#include "fl_digi.h"

#include "config.h"

#define	OUTBUFSIZE	16384

class modem {
protected:
	trx_mode mode;
	cSound	*scard;
	
	int		afcon;
	int		squelchon;
	bool	stopflag;
	int		fragmentsize;
	int		samplerate;
	bool	reverse;
	int		sigsearch;

	bool	freqlock;
	double	bandwidth;
	double	frequency;
	double	freqerr;
	double	rx_corr;
	double	tx_corr;
	double	tx_frequency;
	double	twopi;
	
// for CW modem use only
	bool	cwTrack;
	bool	cwLock;
	double	cwRcvWPM;
	double	cwXmtWPM;

	double 	squelch;
	double	metric;
	double	syncpos;

	int	backspaces;
	unsigned char *txstr;
	unsigned char *txptr;

	double outbuf[OUTBUFSIZE];
// these processes must be declared in the derived class
	Digiscope::scope_mode scopemode;
	
	double scdata[512];
	int scptr;
	
public:
	modem();
	virtual ~modem(){};

	virtual void init();
	virtual void tx_init (cSound *sc) = 0;
	virtual void rx_init () = 0;
	virtual void restart () = 0;
	virtual int  tx_process () = 0;
	virtual int  rx_process (double *, int len) = 0;
	virtual void shutdown(){};
	virtual void set1(int, int){};
	virtual void set2(int, int){};
	virtual void makeTxViewer(int W, int H){};
	
	virtual void	searchDown() {};
	virtual void	searchUp() {};

//	void update_syncscope();

	void 		set_squelch(double val) { squelch = val; }
	double		get_squelch() { return squelch; }
	void		set_sqlchOnOff(bool val) {squelchon = val;}
	bool		get_sqlchOnOff() { return squelchon;}
	void		set_afcOnOff(bool val) {afcon = val;}
	bool		get_afcOnOff() { return afcon;}

//	void 		set_mode(trx_mode);
	trx_mode	get_mode();
	char 		*get_mode_name() { return mode_names[get_mode()];}
	void		set_state(state_t);
	void		set_state_wait(state_t);
	state_t		get_state();
	void		set_freq(int);
	void		set_freq(double);
	int			get_freq();
	void		init_freqlock();
	void		set_freqlock(bool);
	void		set_sigsearch(int n) { sigsearch = n; freqerr = 0.0;};
	bool		freqlocked();
	double		get_txfreq();
	void		set_metric(double);
	double		get_metric();
	void		set_txoffset(double);
	double		get_txoffset();
	void		set_reverse(bool on);
	double		get_bandwidth();
	void		set_bandwidth(double);
	int			get_samplerate();
	void		set_samplerate(int);
	void		init_queues();
	int			get_echo_char();
	
	void		ModulateXmtr(double *, int);
	
	void		set_stopflag(bool b) { stopflag = b;};

// for CW modem use only
	bool		get_cwTrack();
	void		set_cwTrack(bool);
	bool		get_cwLock();
	void		set_cwLock(bool);
	double		get_cwXmtWPM();
	void		set_cwXmtWPM(double);
	double		get_cwRcvWPM();
//	void		set_cwBandwidth(double);
//	double		get_cwBandwidth();
};

extern modem *cw_modem;
extern modem *mfsk8_modem;
extern modem *mfsk16_modem;
extern modem *feld_modem;
extern modem *feld_FMmodem;
extern modem *feld_FM105modem;
extern modem *feld_CMTmodem;
extern modem *psk31_modem;
extern modem *psk63_modem;
extern modem *psk125_modem;
extern modem *qpsk31_modem;
extern modem *qpsk63_modem;
extern modem *qpsk125_modem;
extern modem *rtty_modem;
extern modem *olivia_modem;
extern modem *dominoex4_modem;
extern modem *dominoex5_modem;
extern modem *dominoex8_modem;
extern modem *dominoex11_modem;
extern modem *dominoex16_modem;
extern modem *dominoex22_modem;
extern modem *throb1_modem;
extern modem *throb2_modem;
extern modem *throb4_modem;
extern modem *throbx1_modem;
extern modem *throbx2_modem;
extern modem *throbx4_modem;
extern modem *wwv_modem;
extern modem *anal_modem;


#endif
