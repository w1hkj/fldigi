// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef	_MODEM_H
#define	_MODEM_H

#include <string>

#include "threads.h"

#include "sound.h"
#include "digiscope.h"
#include "globals.h"
#include "morse.h"
#include "ascii.h"
#include "filters.h"

#include "plot_xy.h"

#define	OUTBUFSIZE	65536  // needed for 5 WPM Farsnworth CW buffers

// Constants for signal searching & s/n threshold
#define SIGSEARCH 5

// How many samples to average the signal-quality value over
#define QUALITYDEPTH 5

#define TWOPI (2.0 * M_PI)

class modem {
public:
	static double	frequency;
	static double	tx_frequency;
	static bool	freqlock;
	static unsigned long tx_sample_count;
	static unsigned int tx_sample_rate;
	static bool XMLRPC_CPS_TEST;
protected:
	cMorse	morse;
	trx_mode mode;
	SoundBase	*scard;

	bool	stopflag;
	int		fragmentsize;
	int		samplerate;
	bool	reverse;
	int		sigsearch;
	bool	sig_start;
	bool	sig_stop;

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

// Audio output
	std::string audio_filename;
	bool play_audio;

// CWID
	bool CW_EOT;
public:
	modem();
	virtual ~modem(){}

// these processes must be declared in the derived class
	virtual void init();
	virtual void tx_init () = 0;
	virtual void rx_init () = 0;
	virtual void restart () = 0;
	virtual void rx_flush() {};
	virtual int  tx_process ();
	virtual int  rx_process (const double *, int len) = 0;
	virtual void Audio_filename(std::string nm) { audio_filename = nm; play_audio = true; }

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
	unsigned int iface_io() const { return mode_info[get_mode()].iface_io;}
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
	int			get_samplerate() const { return samplerate;}
	void		set_samplerate(int);
	void		init_queues();

	void		ModulateXmtr(double *, int);
	void		ModulateStereo(double *, double *, int, bool sample_flag = true);

	void		ModulateVideo(double *, int);
	void		ModulateVideoStereo(double *, double *, int, bool sample_flag = true);

	void		videoText();
	void		pretone();

	virtual void		send_color_image(std::string) {}
	virtual void		send_Grey_image(std::string) {}

	virtual void		ifkp_send_image(std::string s = "", bool grey = false){}
	virtual void		ifkp_send_avatar(){}
	virtual void		m_ifkp_send_avatar(){}

	virtual void		thor_send_image(std::string s = "", bool grey = false){}
	virtual void		thor_send_avatar(){}
	virtual void		m_thor_send_avatar(){}

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
	virtual void		CW_KEYLINE(bool) {};

	virtual	void		incWPM() {};
	virtual void		decWPM() {};
	virtual void		toggleWPM() {};
	virtual void		sync_parameters() {};
	virtual void		reset_rx_filter() {};
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
	std::string macro_video_text;
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
	void	set_CW_EOT() { CW_EOT = true; }
	void	clear_CW_EOT() { CW_EOT = false; }

// for fft scan modem
public:
	virtual void	refresh_scope() {}

// for multi-channel modems
public:
	virtual void clear_viewer() {}
	virtual void clear_ch(int n) {}
	virtual int  viewer_get_freq(int n) {return 0; }

// for noise tests
private:
	void	add_noise(double *, int);
	double	sigmaN (double es_ovr_n0);
	double	gauss(double sigma);

protected:
	virtual void s2nreport(void);

// JD & DF for multiple carriers
public:
	int  numcarriers; //Number of parallel carriers for M CAR PSK and PSKR and QPSKR
	int  symbols; //JD for multiple carriers
	int  acc_symbols;
	int  char_symbols;
	int  xmt_symbols;
	int  ovhd_symbols;
	int  acc_samples;
	int  char_samples;
	int  xmt_samples;
	int  ovhd_samples;

// analysis / fmt modes
	PLOT_XY *unk_pipe;
	PLOT_XY *ref_pipe;
	bool    write_to_csv;

	virtual void reset_unknown() {}
	virtual void reset_reference() {}

	virtual void clear_ref_pipe() {}
	virtual void clear_unk_pipe() {}

	virtual void start_csv() {}
	virtual void stop_csv() {}

// fsq mode
	bool    fsq_tx_image;
	std::string xmt_string;
	virtual double		fsq_xmtdelay() {return 0;};
	virtual void		send_ack(std::string relay) {};
	virtual void		fsq_send_image(std::string s){}
	virtual std::string fsq_mycall() {return "";}
	virtual bool		fsq_squelch_open() {return false;}
	virtual void		fsq_transmit(void *) {}
	
// modem decode-quality data and statistics
public:
	int update_quality(int value, int mode=0); // for displaying signal-quality in the GUI
	int get_quality(int mode=0);	// return an average of the quality values
private:
	int quality[QUALITYDEPTH];		// array containining a 0-100 number representing the signal-quality
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

extern modem *_8psk125_modem;
extern modem *_8psk250_modem;
extern modem *_8psk500_modem;
extern modem *_8psk1000_modem;
extern modem *_8psk1200_modem;

extern modem *_8psk125fl_modem;
extern modem *_8psk125f_modem;
extern modem *_8psk250fl_modem;
extern modem *_8psk250f_modem;
extern modem *_8psk500f_modem;
extern modem *_8psk1000f_modem;
extern modem *_8psk1200f_modem;

extern modem *ofdm_500f_modem;
extern modem *ofdm_750f_modem;
extern modem *ofdm_2000f_modem;
extern modem *ofdm_2000_modem;
extern modem *ofdm_3500_modem;

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
//extern modem *pkt_modem;

extern modem *olivia_modem;
extern modem *olivia_4_125_modem;
extern modem *olivia_4_250_modem;
extern modem *olivia_4_500_modem;
extern modem *olivia_4_1000_modem;
extern modem *olivia_4_2000_modem;

extern modem *olivia_8_125_modem;
extern modem *olivia_8_250_modem;
extern modem *olivia_8_500_modem;
extern modem *olivia_8_1000_modem;
extern modem *olivia_8_2000_modem;

extern modem *olivia_16_500_modem;
extern modem *olivia_16_1000_modem;
extern modem *olivia_16_2000_modem;

extern modem *olivia_32_1000_modem;
extern modem *olivia_32_2000_modem;

extern modem *olivia_64_500_modem;
extern modem *olivia_64_1000_modem;
extern modem *olivia_64_2000_modem;

extern modem *contestia_modem;
extern modem *contestia_4_125_modem;
extern modem *contestia_4_250_modem;
extern modem *contestia_4_500_modem;
extern modem *contestia_4_1000_modem;
extern modem *contestia_4_2000_modem;

extern modem *contestia_8_125_modem;
extern modem *contestia_8_250_modem;
extern modem *contestia_8_500_modem;
extern modem *contestia_8_1000_modem;
extern modem *contestia_8_2000_modem;

extern modem *contestia_16_250_modem;
extern modem *contestia_16_500_modem;
extern modem *contestia_16_1000_modem;
extern modem *contestia_16_2000_modem;

extern modem *contestia_32_1000_modem;
extern modem *contestia_32_2000_modem;

extern modem *contestia_64_500_modem;
extern modem *contestia_64_1000_modem;
extern modem *contestia_64_2000_modem;

extern modem *thormicro_modem;
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

extern modem *dominoexmicro_modem;
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
extern modem *fmt_modem;
extern modem *ssb_modem;

extern modem *fsq_modem;

extern modem *ifkp_modem;

#endif
