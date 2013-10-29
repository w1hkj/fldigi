// ----------------------------------------------------------------------------

#ifndef	_MODEM_H
#define	_MODEM_H

#include <string>

#include "threads.h"

#include "sound.h"
#include "digiscope.h"
#include "globals.h"
#include "morse.h"

#define	OUTBUFSIZE	16384
// Constants for signal searching & s/n threshold
#define SIGSEARCH 5

#define TWOPI (2.0 * M_PI)

class modem {
public:
	static double	frequency;
	static double	tx_frequency;
	static bool	freqlock;
protected:
	cMorse	morse;
	trx_mode mode;
	SoundBase	*scard;

	bool	stopflag;
	int		fragmentsize;
	int		samplerate;
	bool	reverse;
	int		sigsearch;

	double	bandwidth;
	double	freqerr;
	double	rx_corr;
	double	tx_corr;
	double  PTTphaseacc;
	double  PTTchannel[OUTBUFSIZE];

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

	bool	historyON;
	Digiscope::scope_mode scopemode;

	int scptr;

	// extended s/n reporting
	double s2n_ncount, s2n_sum, s2n_sum2, s2n_metric;
	bool s2n_valid;

	unsigned cap;

public:
	modem();
	virtual ~modem(){};

// these processes must be declared in the derived class
	virtual void init();
	virtual void tx_init (SoundBase *sc) = 0;
	virtual void rx_init () = 0;
	virtual void restart () = 0;
	virtual void rx_flush() {};
	virtual int  tx_process () = 0;
	virtual int  rx_process (const double *, int len) = 0;
	virtual void shutdown(){};
	virtual void set1(int, int){};
	virtual void set2(int, int){};
	virtual void makeTxViewer(int W, int H){};

	virtual void	searchDown() {};
	virtual void	searchUp() {};

	void		HistoryON(bool val) {historyON = val;}
	bool		HistoryON() const { return historyON;}

	/// Inlined const getters are faster and smaller.
	trx_mode	get_mode() const { return mode; };
	const char	*get_mode_name() const { return mode_info[get_mode()].sname;}
	virtual void	set_freq(double);
	/// Inlining small formulas is still faster and shorter.
	int		get_freq() const { return (int)( frequency + 0.5 ); }
	void		init_freqlock();
	void		set_freqlock(bool);
	void		set_sigsearch(int n) { sigsearch = n; freqerr = 0.0;};
	bool		freqlocked() const { return freqlock;}
	/// Getters are semantically const.
	double		get_txfreq() const;
	double		get_txfreq_woffset() const;
	void		set_metric(double);
	void		display_metric(double);
	double		get_metric() const { return metric;}
	void		set_reverse(bool on);
	bool		get_reverse() const { return reverse;}
	double		get_bandwidth() const { return bandwidth;}
	void		set_bandwidth(double);
	int		get_samplerate() const { return samplerate;}
	void		set_samplerate(int);
	void		init_queues();

	void		ModulateXmtr(double *, int);
	void		ModulateStereo(double *, double *, int);

	void		videoText();
	void		pretone();

	virtual void		send_image(std::string) {}

	void		set_stopflag(bool b) { stopflag = b;};
	bool		get_stopflag() const { return stopflag; };

	unsigned	get_cap(void) const { return cap; }
	enum { CAP_AFC = 1 << 0, CAP_AFC_SR = 1 << 1, CAP_REV = 1 << 2,
	       CAP_IMG = 1 << 3, CAP_BW = 1 << 4, CAP_RX = 1 << 5,
	       CAP_TX = 1 << 6
	};

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
	virtual void		sync_parameters() {};
	virtual void		reset_rx_filter(bool) {};
	virtual void		update_Status() {};

// for waterfall id transmission
private:

	static  double		wfid_w[];
	static  double		wfid_outbuf[];
	int		vidwidth;

	void	wfid_make_tones(int numchars);
	void	wfid_send(int numchars);

	void	wfid_sendchars(std::string s);

	double  PTTnco();

public:
	void	wfid_text(const std::string& s);

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
	void	cwid_sendtext (const std::string& s);
	void	cwid();

// for noise tests
private:
	void	add_noise(double *, int);
	double	sigmaN (double es_ovr_n0);
	double	gauss(double sigma);

protected:
	virtual void s2nreport(void);
};

extern modem *null_modem;

extern modem *cw_modem;

extern modem *mfsk8_modem;
extern modem *mfsk16_modem;
extern modem *mfsk32_modem;
// experimental modes
extern modem *mfsk4_modem;
extern modem *mfsk11_modem;
extern modem *mfsk22_modem;
extern modem *mfsk31_modem;
extern modem *mfsk64_modem;
extern modem *mfsk128_modem;
extern modem *mfsk64l_modem;
extern modem *mfsk128l_modem;

extern modem *wefax576_modem;
extern modem *wefax288_modem;

extern modem *navtex_modem;
extern modem *sitorb_modem;

extern modem *mt63_500S_modem;
extern modem *mt63_1000S_modem;
extern modem *mt63_2000S_modem;
extern modem *mt63_500L_modem;
extern modem *mt63_1000L_modem;
extern modem *mt63_2000L_modem;

extern modem *feld_modem;
extern modem *feld_slowmodem;
extern modem *feld_x5modem;
extern modem *feld_x9modem;
extern modem *feld_FMmodem;
extern modem *feld_FM105modem;
extern modem *feld_80modem;
extern modem *feld_CMTmodem;

extern modem *psk31_modem;
extern modem *psk63_modem;
extern modem *psk63f_modem;
extern modem *psk125_modem;
extern modem *psk250_modem;
extern modem *psk500_modem;
extern modem *psk1000_modem;

extern modem *qpsk31_modem;
extern modem *qpsk63_modem;
extern modem *qpsk125_modem;
extern modem *qpsk250_modem;
extern modem *qpsk500_modem;

extern modem *psk125r_modem;
extern modem *psk250r_modem;
extern modem *psk500r_modem;
extern modem *psk1000r_modem;

extern modem *psk800_c2_modem;
extern modem *psk800r_c2_modem;

extern modem *psk1000_c2_modem;
extern modem *psk1000r_c2_modem;

extern modem *psk63r_c4_modem;
extern modem *psk63r_c5_modem;
extern modem *psk63r_c10_modem;
extern modem *psk63r_c20_modem;
extern modem *psk63r_c32_modem;

extern modem *psk125r_c4_modem;
extern modem *psk125r_c5_modem;
extern modem *psk125r_c10_modem;
extern modem *psk125_c12_modem;
extern modem *psk125r_c12_modem;
extern modem *psk125r_c16_modem;

extern modem *psk250r_c2_modem;
extern modem *psk250r_c3_modem;
extern modem *psk250r_c5_modem;
extern modem *psk250_c6_modem;
extern modem *psk250r_c6_modem;
extern modem *psk250r_c7_modem;

extern modem *psk500_c2_modem;
extern modem *psk500_c4_modem;

extern modem *psk500r_c2_modem;
extern modem *psk500r_c3_modem;
extern modem *psk500r_c4_modem;

extern modem *rtty_modem;
extern modem *pkt_modem;

extern modem *olivia_modem;
extern modem *olivia_4_250_modem;
extern modem *olivia_8_250_modem;
extern modem *olivia_4_500_modem;
extern modem *olivia_8_500_modem;
extern modem *olivia_16_500_modem;
extern modem *olivia_8_1000_modem;
extern modem *olivia_16_1000_modem;
extern modem *olivia_32_1000_modem;
extern modem *olivia_64_2000_modem;

extern modem *contestia_modem;

extern modem *thor4_modem;
extern modem *thor5_modem;
extern modem *thor8_modem;
extern modem *thor11_modem;
extern modem *thor16_modem;
extern modem *thor22_modem;
extern modem *thor25x4_modem;
extern modem *thor50x1_modem;
extern modem *thor50x2_modem;
extern modem *thor100_modem;

extern modem *dominoex4_modem;
extern modem *dominoex5_modem;
extern modem *dominoex8_modem;
extern modem *dominoex11_modem;
extern modem *dominoex16_modem;
extern modem *dominoex22_modem;
extern modem *dominoex44_modem;
extern modem *dominoex88_modem;

extern modem *throb1_modem;
extern modem *throb2_modem;
extern modem *throb4_modem;
extern modem *throbx1_modem;
extern modem *throbx2_modem;
extern modem *throbx4_modem;

extern modem *wwv_modem;
extern modem *anal_modem;
extern modem *ssb_modem;


#endif
