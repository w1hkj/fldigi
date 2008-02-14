// ----------------------------------------------------------------------------

#ifndef	_MODEM_H
#define	_MODEM_H

#include <string>

#include "threads.h"

#include "misc.h"
#include "sound.h"
#include "waterfall.h"
#include "fl_digi.h"
#include "globals.h"
#include "fl_digi.h"

#include "fldigi-config.h"
#include "morse.h"

#define	OUTBUFSIZE	16384
// Constants for signal searching & s/n threshold
#define SIGSEARCH 5


class modem : public morse {
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
	virtual int  rx_process (const double *, int len) = 0;
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
	void		set_afcOnOff(int val) {afcon = val;}
	bool		get_afcOnOff() { return afcon;}

//	void 		set_mode(trx_mode);
	trx_mode	get_mode();
	const char	*get_mode_name() { return mode_info[get_mode()].sname;}
	void		set_state(state_t);
	void		set_state_wait(state_t);
	state_t		get_state();
	void		set_freq(double);
	int			get_freq();
	void		init_freqlock();
	void		set_freqlock(bool);
	void		set_sigsearch(int n) { sigsearch = n; freqerr = 0.0;};
	bool		freqlocked();
	double		get_txfreq();
	double		get_txfreq_woffset();
	void		set_metric(double);
	double		get_metric();
	void		set_reverse(bool on);
	double		get_bandwidth();
	void		set_bandwidth(double);
	int			get_samplerate();
	void		set_samplerate(int);
	void		init_queues();
	int			get_echo_char();
	
	void		ModulateXmtr(double *, int);
	void		ModulateStereo(double *, double *, int);
	
	void		videoText();
	
	void		set_stopflag(bool b) { stopflag = b;};

// for CW modem use only
	bool		get_cwTrack();
	void		set_cwTrack(bool);
	bool		get_cwLock();
	void		set_cwLock(bool);
	double		get_cwXmtWPM();
	void		set_cwXmtWPM(double);
	double		get_cwRcvWPM();
	virtual	void		incWPM() {};
	virtual void		decWPM() {};
	virtual void		toggleWPM() {};
	
// for waterfall id transmission
public:

private:
	
	static	int			wfid_mask[];
	static  double		wfid_w[];
	static  double		wfid_txpulse[];
	static  double		wfid_outbuf[];
	
	void	wfid_make_pulse();
	void	wfid_make_tones();
	void	wfid_send(long int);
	double	peakval(int symbol, int mask);
	int 	findmask(int symbol);

	void	wfid_sendchar(char c);
	void	wfid_sendchars(string s);

public:
	void	wfid_text(string s);

// for CW ID transmission
private:
	double	cwid_keyshape[128];
	double	cwid_phaseacc;
	int		RT;
	int		cwid_symbollen;
	int		cwid_lastsym;
public:
	void	cwid_makeshape();
	double	cwid_nco(double freq);
	void	cwid_send_symbol(int bits);
	void	cwid_send_ch(int ch);
	void	cwid_sendtext (string s);
	void	cwid();

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
extern modem *psk250_modem;
extern modem *qpsk31_modem;
extern modem *qpsk63_modem;
extern modem *qpsk125_modem;
extern modem *qpsk250_modem;
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
