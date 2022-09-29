// ----------------------------------------------------------------------------
// modem.cxx - modem class - base for all modems
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
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

#include <config.h>

#include <string>

#include "misc.h"
#include "filters.h"

#include "confdialog.h"
#include "modem.h"
#include "trx.h"
#include "fl_digi.h"
#include "main.h"
#include "arq_io.h"
#include "configuration.h"
#include "waterfall.h"
#include "qrunner.h"
#include "macros.h"

#include "testsigs.h"
#include "test_signal.h"

#include "status.h"
#include "debug.h"
#include "audio_alert.h"

using namespace std;

modem *null_modem = 0;
modem *cw_modem = 0;

modem *mfsk8_modem = 0;
modem *mfsk16_modem = 0;
modem *mfsk32_modem = 0;
modem *mfsk4_modem = 0;
modem *mfsk11_modem = 0;
modem *mfsk22_modem = 0;
modem *mfsk31_modem = 0;
modem *mfsk64_modem = 0;
modem *mfsk128_modem = 0;
modem *mfsk64l_modem = 0;
modem *mfsk128l_modem = 0;

modem *wefax576_modem = 0;
modem *wefax288_modem = 0;

modem *navtex_modem = 0;
modem *sitorb_modem = 0;

modem *mt63_500S_modem = 0;
modem *mt63_500L_modem = 0;
modem *mt63_1000S_modem = 0;
modem *mt63_1000L_modem = 0;
modem *mt63_2000S_modem = 0;
modem *mt63_2000L_modem = 0;

modem *feld_modem = 0;
modem *feld_slowmodem = 0;
modem *feld_x5modem = 0;
modem *feld_x9modem = 0;
modem *feld_FMmodem = 0;
modem *feld_FM105modem = 0;
modem *feld_80modem = 0;
modem *feld_CMTmodem = 0;

modem *psk31_modem = 0;
modem *psk63_modem = 0;
modem *psk63f_modem = 0;
modem *psk125_modem = 0;
modem *psk250_modem = 0;
modem *psk500_modem = 0;
modem *psk1000_modem = 0;

modem *qpsk31_modem = 0;
modem *qpsk63_modem = 0;
modem *qpsk125_modem = 0;
modem *qpsk250_modem = 0;
modem *qpsk500_modem = 0;

modem *_8psk125_modem = 0;
modem *_8psk250_modem = 0;
modem *_8psk500_modem = 0;
modem *_8psk1000_modem = 0;
modem *_8psk1200_modem = 0;
modem *_8psk1333_modem = 0;

modem *_8psk125fl_modem = 0;
modem *_8psk125f_modem = 0;
modem *_8psk250fl_modem = 0;
modem *_8psk250f_modem = 0;
modem *_8psk500f_modem = 0;
modem *_8psk1000f_modem = 0;
modem *_8psk1200f_modem = 0;
modem *_8psk1333f_modem = 0;

modem *psk125r_modem = 0;
modem *psk250r_modem = 0;
modem *psk500r_modem = 0;
modem *psk1000r_modem = 0;

modem *psk800_c2_modem = 0;
modem *psk800r_c2_modem = 0;
modem *psk1000_c2_modem = 0;
modem *psk1000r_c2_modem = 0;

modem *psk63r_c4_modem = 0;
modem *psk63r_c5_modem = 0;
modem *psk63r_c10_modem = 0;
modem *psk63r_c20_modem = 0;
modem *psk63r_c32_modem = 0;

modem *psk125r_c4_modem = 0;
modem *psk125r_c5_modem = 0;
modem *psk125r_c10_modem = 0;
modem *psk125_c12_modem = 0;
modem *psk125r_c12_modem = 0;
modem *psk125r_c16_modem = 0;

modem *psk250r_c2_modem = 0;
modem *psk250r_c3_modem = 0;
modem *psk250r_c5_modem = 0;
modem *psk250_c6_modem = 0;
modem *psk250r_c6_modem = 0;
modem *psk250r_c7_modem = 0;

modem *psk500_c2_modem = 0;
modem *psk500_c4_modem = 0;

modem *psk500r_c2_modem = 0;
modem *psk500r_c3_modem = 0;
modem *psk500r_c4_modem = 0;

modem *olivia_modem = 0;

modem *olivia_4_125_modem = 0;
modem *olivia_4_250_modem = 0;
modem *olivia_4_500_modem = 0;
modem *olivia_4_1000_modem = 0;
modem *olivia_4_2000_modem = 0;

modem *olivia_8_125_modem = 0;
modem *olivia_8_250_modem = 0;
modem *olivia_8_500_modem = 0;
modem *olivia_8_1000_modem = 0;
modem *olivia_8_2000_modem = 0;

modem *olivia_16_500_modem = 0;
modem *olivia_16_1000_modem = 0;
modem *olivia_16_2000_modem = 0;

modem *olivia_32_1000_modem = 0;
modem *olivia_32_2000_modem = 0;

modem *olivia_64_500_modem = 0;
modem *olivia_64_1000_modem = 0;
modem *olivia_64_2000_modem = 0;

modem *contestia_modem = 0;

modem *contestia_4_125_modem = 0;
modem *contestia_4_250_modem = 0;
modem *contestia_4_500_modem = 0;
modem *contestia_4_1000_modem = 0;
modem *contestia_4_2000_modem = 0;

modem *contestia_8_125_modem = 0;
modem *contestia_8_250_modem = 0;
modem *contestia_8_500_modem = 0;
modem *contestia_8_1000_modem = 0;
modem *contestia_8_2000_modem = 0;

modem *contestia_16_500_modem = 0;
modem *contestia_16_1000_modem = 0;
modem *contestia_16_2000_modem = 0;

modem *contestia_32_1000_modem = 0;
modem *contestia_32_2000_modem = 0;

modem *contestia_64_500_modem = 0;
modem *contestia_64_1000_modem = 0;
modem *contestia_64_2000_modem = 0;

modem *rtty_modem = 0;
modem *pkt_modem = 0;

modem *thormicro_modem = 0;
modem *thor4_modem = 0;
modem *thor5_modem = 0;
modem *thor8_modem = 0;
modem *thor11_modem = 0;
modem *thor16_modem = 0;
modem *thor22_modem = 0;
modem *thor25x4_modem = 0;
modem *thor50x1_modem = 0;
modem *thor50x2_modem = 0;
modem *thor100_modem = 0;

modem *fsq_modem = 0;

modem *ifkp_modem = 0;

modem *dominoexmicro_modem = 0;
modem *dominoex4_modem = 0;
modem *dominoex5_modem = 0;
modem *dominoex8_modem = 0;
modem *dominoex11_modem = 0;
modem *dominoex16_modem = 0;
modem *dominoex22_modem = 0;
modem *dominoex44_modem = 0;
modem *dominoex88_modem = 0;

modem *throb1_modem = 0;
modem *throb2_modem = 0;
modem *throb4_modem = 0;
modem *throbx1_modem = 0;
modem *throbx2_modem = 0;
modem *throbx4_modem = 0;

modem *wwv_modem = 0;
modem *anal_modem = 0;
modem *fmt_modem = 0;
modem *ssb_modem = 0;

double modem::frequency = 1000;
double modem::tx_frequency = 1000;
bool   modem::freqlock = false;

// For xml socket command
unsigned long modem::tx_sample_count = 0;
unsigned int  modem::tx_sample_rate  = 0;
bool modem::XMLRPC_CPS_TEST = false;

modem::modem()
{
	scptr = 0;

	if (wf)
		frequency = tx_frequency = wf->Carrier();
	else
		frequency = tx_frequency = 1000;

	sigsearch = 0;
	if (wf) {
		bool wfrev = wf->Reverse();
		bool wfsb = wf->USB();
		reverse = wfrev ^ !wfsb;
	} else
		reverse = false;
	historyON = false;
	cap = CAP_RX | CAP_TX;
	PTTphaseacc = 0.0;
	s2n_ncount = s2n_sum = s2n_sum2 = s2n_metric = 0.0;
	s2n_valid = false;

	bandwidth = 0.0;

	CW_EOT = false;
	sig_start = sig_stop = false;
}

// modem types CW and RTTY do not use the base init()
void modem::init()
{
	stopflag = false;
	if (!wf) return;

	bool wfrev = wf->Reverse();
	bool wfsb = wf->USB();
	reverse = wfrev ^ !wfsb;
}

void modem::set_freq(double freq)
{
	frequency = CLAMP(
		freq,
		progdefaults.LowFreqCutoff + bandwidth / 2,
		progdefaults.HighFreqCutoff - bandwidth / 2);

	if (freqlock == false)
		tx_frequency = frequency;

	if (progdefaults.RxFilt_track_wf)
		center_rxfilt_at_track();

	REQ(put_freq, frequency);
}

void modem::set_freqlock(bool on)
{
	freqlock = on;
	set_freq(frequency);
}


double modem::get_txfreq(void) const
{
	if (unlikely(!(cap & CAP_TX)))
		return 0;
	else if (mailserver && progdefaults.PSKmailSweetSpot)
		return progdefaults.PSKsweetspot;
	if (get_mode() == MODE_FSQ) return 1500;
	return tx_frequency;
}

double modem::get_txfreq_woffset(void) const
{
	if (mailserver && progdefaults.PSKmailSweetSpot)
		return (progdefaults.PSKsweetspot - progdefaults.TxOffset);
	if (get_mode() == MODE_FSQ) return (1500 - progdefaults.TxOffset);

	if (test_signal_window && test_signal_window->visible() && btnOffsetOn->value())
		return tx_frequency + ctrl_freq_offset->value();

	return (tx_frequency - progdefaults.TxOffset);
}

void modem::set_bandwidth(double bw)
{
	bandwidth = bw;
	put_Bandwidth((int)bandwidth);
}

void modem::set_reverse(bool on)
{
	if (likely(wf))
		reverse = on ^ (!wf->USB());
	else
		reverse = false;
}

void modem::set_metric(double m)
{
	metric = m;
}

extern void callback_set_metric(double metric);

void modem::display_metric(double m)
{
	set_metric(m);
	if (!progStatus.kpsql_enabled) 
		REQ(callback_set_metric, m);
}

bool modem::get_cwTrack()
{
	return cwTrack;
}

void modem::set_cwTrack(bool b)
{
	cwTrack = b;
}

bool modem::get_cwLock()
{
	return cwLock;
}

void modem::set_cwLock(bool b)
{
	cwLock = b;
}

double modem::get_cwRcvWPM()
{
	return cwRcvWPM;
}

double modem::get_cwXmtWPM()
{
	return cwXmtWPM;
}

void modem::set_cwXmtWPM(double wpm)
{
	cwXmtWPM = wpm;
}

void modem::set_samplerate(int smprate)
{
	samplerate = smprate;
}

double modem::PTTnco()
{
	double amp;
// sine wave PTT signal
	amp = 0.9 * sin(PTTphaseacc);

// square wave PTT signal
//	if (PTTphaseacc > M_PI)
//		amp = - 0.5;
//	else
//		amp = 0.5;

	PTTphaseacc += TWOPI * progdefaults.QSKfrequency / samplerate;
	if (PTTphaseacc > TWOPI) PTTphaseacc -= TWOPI;

	return amp;
}

double modem::sigmaN (double es_ovr_n0)
{
	double sn_ratio, sigma;
	double mode_factor = 0.707;
	switch (mode) {
	case MODE_CW:
		mode_factor /= 0.44;
		break;
	case MODE_FELDHELL: case MODE_SLOWHELL:
	case MODE_HELLX5: case MODE_HELLX9:
		mode_factor /= 0.22;
		break;
	case MODE_MT63_500S: case MODE_MT63_1000S: case MODE_MT63_2000S :
	case MODE_MT63_500L: case MODE_MT63_1000L: case MODE_MT63_2000L :
		mode_factor *= 3.0;
		break;
	case MODE_PSK31: case MODE_PSK63: case MODE_PSK63F:
	case MODE_PSK125: case MODE_PSK250: case MODE_PSK500:
	case MODE_QPSK31: case MODE_QPSK63: case MODE_QPSK125: case MODE_QPSK250:
	case MODE_PSK125R: case MODE_PSK250R: case MODE_PSK500R:
		mode_factor = 400;
		break;
	case MODE_THROB1: case MODE_THROB2: case MODE_THROB4:
	case MODE_THROBX1: case MODE_THROBX2: case MODE_THROBX4:
		mode_factor *= 6.0;
		break;
//	case MODE_RTTY:
//	case MODE_OLIVIA:
//	case MODE_DOMINOEX4: case MODE_DOMINOEX5: case MODE_DOMINOEX8:
//	case MODE_DOMINOEX11: case MODE_DOMINOEX16: case MODE_DOMINOEX22:
//	case MODE_MFSK4: case MODE_MFSK11: case MODE_MFSK22: case MODE_MFSK31:
//	case MODE_MFSK64: case MODE_MFSK8: case MODE_MFSK16: case MODE_MFSK32:
//	case MODE_THOR4: case MODE_THOR5: case MODE_THOR8:
//	case MODE_THOR11:case MODE_THOR16: case MODE_THOR22:
//	case MODE_FSKH245: case MODE_FSKH105: case MODE_HELL80:
	default: break;
	}
	if (trx_state == STATE_TUNE) mode_factor = 0.707;

	sn_ratio = pow(10, ( es_ovr_n0 / 10) );
	sigma =  sqrt ( mode_factor / sn_ratio );
	return sigma;
}

// A Rayleigh-distributed random variable R, with the probability
// distribution
//	F(R) = 0 where R < 0 and
//	F(R) = 1 - exp(-R^2/2*sigma^2) where R >= 0,
// is related to a pair of Gaussian variables C and D
// through the transformation
//	C = R * cos(theta) and
//	D = R * sin(theta),
// where theta is a uniformly distributed variable in the interval
// 0 to 2 * Pi.

double modem::gauss(double sigma) {
	double u, r;
	u = 1.0 * rand() / RAND_MAX;
	r = sigma * sqrt( 2.0 * log( 1.0 / (1.0 - u) ) );
	u = 1.0 * rand() / RAND_MAX;
	return r * cos(2 * M_PI * u);
}

// given the desired Es/No, calculate the standard deviation of the
// additive white gaussian noise (AWGN). The standard deviation of
// the AWGN will be used to generate Gaussian random variables
// simulating the noise that is added to the signal.
// return signal + noise, limiting value to +/- 1.0

void modem::add_noise(double *buffer, int len)
{
	double sigma = sigmaN(noiseDB->value());
	double sn = 0;

	for (int n = 0; n < len; n++) {
		if (btnNoiseOn->value()) {
			sn = (buffer[n] + gauss(sigma)) / (1.0 + 3.0 * sigma);
			buffer[n] = clamp(sn, -1.0, 1.0);
		}
	}
}

void modem::s2nreport(void)
{
	double s2n_avg = s2n_sum / s2n_ncount;
	double s2n_stddev = sqrt((s2n_sum2 / s2n_ncount) - (s2n_avg * s2n_avg));

	pskmail_notify_s2n(s2n_ncount, s2n_avg, s2n_stddev);
}

bool disable_modem = false;
#define SIGLIMIT 0.95

void modem::ModulateXmtr(double *buffer, int len)
{
	if (unlikely(!TXscard)) return;
	if (disable_modem) return;

	tx_sample_count += len;

	if (sig_start) {
		int num = len / 2;
		for (int i = 0; i < num; i++)
			buffer[i] *= (0.5 * (1.0 - cos (M_PI * i / num)));
		sig_start = false;
	}
	if (sig_stop) {
		int num = len / 2;
		for (int i = 0; i < num; i++)
			buffer[len - i - 1] *= (0.5 * (1.0 - cos (M_PI * i / num)));
		sig_stop = false;
	}

	if (progdefaults.PTTrightchannel) {
		for (int i = 0; i < len; i++)
			PTTchannel[i] = PTTnco();
		ModulateStereo( buffer, PTTchannel, len, false);
		return;
	}

	if (test_signal_window && test_signal_window->visible()) add_noise(buffer, len);

	if (progdefaults.viewXmtSignal &&
		!(PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST))
		trx_xmit_wfall_queue(samplerate, buffer, (size_t)len);

	double mult = pow(10, progdefaults.txlevel / 20.0);
	if (mult > SIGLIMIT) mult = SIGLIMIT;
	for (int i = 0; i < len; i++) {
		buffer[i] *= mult;
		if (buffer[i] < -SIGLIMIT) buffer[i] = -SIGLIMIT;
		if (buffer[i] >  SIGLIMIT) buffer[i] = SIGLIMIT;
	}

	try {
		unsigned n = 4;
		while (TXscard->Write(buffer, len) == 0 && --n);
		if (n == 0)
			throw SndException(-99, "Sound write failed");
	}
	catch (const SndException& e) {
		if(e.error() < 0) {
 			LOG_ERROR("%s", e.what());
 			throw;
		}
		return;
	}
}

#include <iostream>
using namespace std;

void modem::ModulateStereo(double *left, double *right, int len, bool sample_flag)
{
	if (unlikely(!TXscard)) return;
	if (disable_modem) return;

	if(sample_flag)
		tx_sample_count += len;

	if (test_signal_window && test_signal_window->visible() && progdefaults.noise) add_noise(left, len);

	if (progdefaults.viewXmtSignal &&
		!(PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST))
		trx_xmit_wfall_queue(samplerate, left, (size_t)len);

	double mult = pow(10, progdefaults.txlevel / 20.0);
	if (mult > SIGLIMIT) mult = SIGLIMIT;

	for (int i = 0; i < len; i++) {
		if (right[i] < -SIGLIMIT) right[i] = -SIGLIMIT;
		if (right[i] >  SIGLIMIT) right[i] = SIGLIMIT;
		left[i] *= mult;
		if (left[i] < -SIGLIMIT) left[i] = -SIGLIMIT;
		if (left[i] >  SIGLIMIT) left[i] = SIGLIMIT;
	}
	try {
		unsigned n = 4;
		while (TXscard->Write_stereo(left, right, len) == 0 && --n);
		if (n == 0)
			throw SndException(-99, "Sound write failed");
	}
	catch (const SndException& e) {
		if(e.error() < 0) {
 			LOG_ERROR("%s", e.what());
 			throw;
		}
		return;
	}

}

//------------------------------------------------------------------------------
// tx process
//------------------------------------------------------------------------------
int  modem::tx_process ()
{
	if (!macro_video_text.empty()) {
		wfid_text(macro_video_text);
		macro_video_text.clear();
	}
	if (play_audio) {
		disable_modem = true;
		TXscard->Audio(audio_filename);
		play_audio = false;
		disable_modem = false;
	}
	return 0;
}

//------------------------------------------------------------------------------
// modulate video signal
//------------------------------------------------------------------------------
void modem::ModulateVideoStereo(double *left, double *right, int len, bool sample_flag)
{
	if (unlikely(!TXscard)) return;

	if(sample_flag)
		tx_sample_count += len;

	if (test_signal_window && test_signal_window->visible() && progdefaults.noise) add_noise(left, len);

	if (progdefaults.viewXmtSignal &&
		!(PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST))
		trx_xmit_wfall_queue(samplerate, left, (size_t)len);

	double mult = SIGLIMIT * pow(10, progdefaults.txlevel / 20.0);

	for (int i = 0; i < len; i++) {
		if (right[i] < -SIGLIMIT) right[i] = -SIGLIMIT;
		if (right[i] >  SIGLIMIT) right[i] = SIGLIMIT;
		left[i] *= mult;
		if (left[i] < -SIGLIMIT) left[i] = -SIGLIMIT;
		if (left[i] >  SIGLIMIT) left[i] = SIGLIMIT;
	}
	try {
		unsigned n = 4;
		while (TXscard->Write_stereo(left, right, len) == 0 && --n);
		if (n == 0)
			throw SndException(-99, "Sound write failed");
	}
	catch (const SndException& e) {
		if(e.error() < 0) {
 			LOG_ERROR("%s", e.what());
 			throw;
		}
		return;
	}
}

void modem::ModulateVideo(double *buffer, int len)
{
	if (unlikely(!TXscard)) return;

	tx_sample_count += len;

	if (progdefaults.PTTrightchannel) {
		for (int i = 0; i < len; i++)
			PTTchannel[i] = PTTnco();
		ModulateVideoStereo( buffer, PTTchannel, len, false);
		return;
	}

	if (test_signal_window && test_signal_window->visible() && progdefaults.noise) add_noise(buffer, len);

	if (progdefaults.viewXmtSignal &&
		!(PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST))
		trx_xmit_wfall_queue(samplerate, buffer, (size_t)len);

	double mult = SIGLIMIT * pow(10, progdefaults.txlevel / 20.0);
	for (int i = 0; i < len; i++) {
		buffer[i] *= mult;
		if (buffer[i] < -SIGLIMIT) buffer[i] = -SIGLIMIT;
		if (buffer[i] >  SIGLIMIT) buffer[i] = SIGLIMIT;
	}

	try {
		unsigned n = 4;
		while (TXscard->Write(buffer, len) == 0 && --n);
		if (n == 0)
			throw SndException(-99, "Sound write failed");
	}
	catch (const SndException& e) {
		if(e.error() < 0) {
 			LOG_ERROR("%s", e.what());
 			throw;
		}
		return;
	}
}

//------------------------------------------------------------------------------
void modem::videoText()
{
	if (trx_state == STATE_TUNE)
		return;

	if (progdefaults.pretone > 0.2)
		pretone();

	if (progdefaults.sendtextid == true) {
		wfid_text(progdefaults.strTextid);
	} else if (progdefaults.macrotextid == true) {
		wfid_text(progdefaults.strTextid);
		progdefaults.macrotextid = false;
	}

	if (progdefaults.videoid_modes.test(mode) &&
		(progdefaults.sendid || progdefaults.macroid)) {
		#define TLEN 20
		char idtxt[TLEN] = "";
		switch(mode_info[mode].mode) {
		case MODE_CONTESTIA:
			snprintf(idtxt, TLEN, "%s-%d/%d", mode_info[mode].vid_name,
				2*(1<<progdefaults.contestiatones),
				125*(1<<progdefaults.contestiabw));
			break;
		case MODE_OLIVIA:
			snprintf(idtxt, TLEN, "%s-%d/%d", mode_info[mode].vid_name,
				2*(1<<progdefaults.oliviatones),
				125*(1<<progdefaults.oliviabw));
			break;
		case MODE_RTTY:
			snprintf(idtxt, TLEN, "%s-%d/%d", mode_info[mode].vid_name,
				static_cast<int>(rtty::BAUD[progdefaults.rtty_baud]),
				rtty::BITS[progdefaults.rtty_bits]);
			break;
		case MODE_DOMINOEX4: case MODE_DOMINOEX5: case MODE_DOMINOEX8:
		case MODE_DOMINOEX11: case MODE_DOMINOEX16: case MODE_DOMINOEX22:
			if (progdefaults.DOMINOEX_FEC)
				snprintf(idtxt, TLEN, "%s-FEC", mode_info[mode].vid_name);
			else
				strcpy(idtxt, mode_info[mode].vid_name);
			break;
		default:
			strcpy(idtxt, mode_info[mode].vid_name);
			break;
		}
		wfid_text(idtxt);
		progdefaults.macroid = false;
	}
}

// CW ID transmit routines

//===========================================================================
// cw transmit routines to send a post amble message
// Define the amplitude envelop for key down events
// this is 1/2 cycle of a raised cosine
//===========================================================================

void modem::cwid_makeshape()
{
	for (int i = 0; i < 128; i++) cwid_keyshape[i] = 1.0;
	for (int i = 0; i < RT; i++)
		cwid_keyshape[i] = 0.5 * (1.0 - cos (M_PI * i / RT));
}

double modem::cwid_nco(double freq)
{
	cwid_phaseacc += 2.0 * M_PI * freq / samplerate;

	if (cwid_phaseacc > TWOPI) cwid_phaseacc -= TWOPI;

	return sin(cwid_phaseacc);
}

//=====================================================================
// cwid_send_symbol()
// Sends a part of a morse character (one dot duration) of either
// sound at the correct freq or silence. Rise and fall time is controlled
// with a raised cosine shape.
//
// Left channel contains the shaped A2 CW waveform
//=======================================================================

void modem::cwid_send_symbol(int bits)
{
	double freq;
	int i,
		keydown,
		keyup,
		sample = 0,
		currsym = bits & 1;

	freq = get_txfreq_woffset() - progdefaults.TxOffset;

	if ((currsym == 1) && (cwid_lastsym == 0))
		cwid_phaseacc = 0.0;

	keydown = cwid_symbollen - RT;
	keyup = cwid_symbollen - RT;

	if (currsym == 1) {
		for (i = 0; i < RT; i++, sample++) {
			if (cwid_lastsym == 0)
				outbuf[sample] = cwid_nco(freq) * cwid_keyshape[i];
			else
				outbuf[sample] = cwid_nco(freq);
		}
		for (i = 0; i < keydown; i++, sample++) {
			outbuf[sample] = cwid_nco(freq);
		}
	}
	else {
		for (i = RT - 1; i >= 0; i--, sample++) {
			if (cwid_lastsym == 1) {
				outbuf[sample] = cwid_nco(freq) * cwid_keyshape[i];
			} else {
				outbuf[sample] = 0.0;
			}
		}
		for (i = 0; i < keyup; i++, sample++) {
			outbuf[sample] = 0.0;
		}
	}

	ModulateXmtr(outbuf, cwid_symbollen);

	cwid_lastsym = currsym;
}

//=====================================================================
// send_ch()
// sends a morse character and the space afterwards
//=======================================================================

void modem::cwid_send_ch(int ch)
{
	std::string code;

// handle word space separately (7 dots spacing)
// last char already had 2 elements of inter-character spacing

	if ((ch == ' ') || (ch == '\n')) {
		cwid_send_symbol(0);
		cwid_send_symbol(0);
		cwid_send_symbol(0);
		cwid_send_symbol(0);
		cwid_send_symbol(0);
		put_echo_char(ch);
		return;
	}

// convert character code to a morse representation
	code = morse.tx_lookup(ch); //cw_tx_lookup(ch);
	if (!code.length())
		return;
// loop sending out binary bits of cw character
	for (size_t n = 0; n < code.length(); n++) {
		cwid_send_symbol(0);
		cwid_send_symbol(1);
		if (code[n] == '-') {
			cwid_send_symbol(1);
			cwid_send_symbol(1);
		}
	}
	cwid_send_symbol(0);
	cwid_send_symbol(0);

}

void modem::cwid_sendtext (const string& s)
{
	cwid_symbollen = (int)(1.2 * samplerate / progdefaults.CWIDwpm);
	RT = (int) (samplerate * 6 / 1000.0); // 6 msec risetime for CW pulse
	cwid_makeshape();
	cwid_lastsym = 0;
	for (unsigned int i = 0; i < s.length(); i++) {
		cwid_send_ch(s[i]);
	}
}

void modem::cwid()
{
	if ((CW_EOT && progdefaults.cwid_modes.test(mode) &&
		progdefaults.CWid == true) || progdefaults.macroCWid == true) {
		string tosend = " DE ";
		tosend += progdefaults.myCall;
		cwid_sendtext(tosend);
		progdefaults.macroCWid = false;
		CW_EOT = false;
	}
}

//=====================================================================
// transmit processing of waterfall video id
//=====================================================================

static int NUMROWS;
static int NUMCOLS;
static int TONESPACING;
static int IDSYMLEN;
static int CHARSPACE;

static bool useIDSMALL = true;

#define MAXROWS 14
#define MAXIDSYMLEN 16384
#define MAXTONES 128
#define MAXCHARS 10

struct mfntchr  { char c; int byte[MAXROWS]; };
extern mfntchr  idch1[]; // original id font definition
extern mfntchr  idch2[]; // extended id font definition

static int id_symbols[MAXCHARS];

static C_FIR_filter vidfilt;

void modem::wfid_make_tones(int numchars)
{
	double f, flo, fhi;
	int vwidth = (numchars*NUMCOLS + (numchars-1)*CHARSPACE - 1);
	f = get_txfreq_woffset() + TONESPACING * vwidth/2.0;
	fhi = f + TONESPACING;
	flo = fhi - (vwidth + 2) * TONESPACING;
	for (int i = 1; i <= NUMCOLS * numchars; i++) {
		wfid_w[i-1] = f * 2.0 * M_PI / samplerate;
		f -= TONESPACING;
		if ( (i > 0) && (i % NUMCOLS == 0) )
			f -= TONESPACING * CHARSPACE;
	}
	vidfilt.init_bandpass( 1024, 1, flo/samplerate, fhi/samplerate) ;
}

void modem::wfid_send(int numchars)
{
	int i, j, k;
	int sym;
	double val;

	for (i = 0; i < IDSYMLEN; i++) {
		val = 0.0;
		for (k = 0; k < numchars; k++) {
			sym = id_symbols[numchars - k - 1];
			for (j = 0; j < NUMCOLS; j++) {
				if (sym & 1)
					val += sin(wfid_w[j + k * NUMCOLS] * i);
				sym = sym >> 1;
			}
		}
// soft limit the signal - heuristic formulation
		val = (1.0 - exp(-fabs(val)/3.0)) * (val >= 0.0 ? 1 : -1);
// band pass filter the soft limited signal
		vidfilt.Irun( val, val );
		wfid_outbuf[i] = val;
	}
	ModulateVideo(wfid_outbuf, IDSYMLEN);
}

void modem::wfid_sendchars(string s)
{
	int len = s.length();
	int  n[len];
	int  c;
	wfid_make_tones(s.length());
	for (int i = 0; i < len; i++) {
		if (useIDSMALL) {
			c = toupper(s[i]);
			if (c > 'Z' || c < ' ') c = ' ';
		} else {
			c = s[i];
			if (c > '~' || c < ' ') c = ' ';
		}
		n[i] = c - ' ';
	}
// send rows from bottom to top so they appear to scroll down the waterfall correctly
	for (int row = 0; row < NUMROWS; row++) {
		for (int i = 0; i < len; i++) {
			if (useIDSMALL)
				id_symbols[i] = idch1[n[i]].byte[NUMROWS - 1 - row];
			else
				id_symbols[i] = idch2[n[i]].byte[NUMROWS - 1 - row];
		}
		wfid_send(len);
		if (stopflag)
			return;
	}
}

void modem::pretone()
{
	int sr = get_samplerate();
	int symlen = sr / 10;
	double phaseincr = 2.0 * M_PI * get_txfreq_woffset() / sr;
	double phase = 0.0;
	double outbuf[symlen];

	for (int j = 0; j < symlen; j++) {
		outbuf[j] = (0.5 * (1.0 - cos (M_PI * j / symlen)))*sin(phase);
		phase += phaseincr;
		if (phase > TWOPI) phase -= TWOPI;
	}
	ModulateXmtr(outbuf, symlen);

	for (int i = 0; i < progdefaults.pretone * 10 - 2; i++) {
		for (int j = 0; j < symlen; j++) {
			outbuf[j] = sin(phase);
			phase += phaseincr;
			if (phase > TWOPI) phase -= TWOPI;
		}
		ModulateXmtr(outbuf, symlen);
	}

	for (int j = 0; j < symlen; j++) {
		outbuf[j] = (0.5 * (1.0 - cos (M_PI * (symlen - j) / symlen)))*sin(phase);
		phase += phaseincr;
		if (phase > TWOPI) phase -= TWOPI;
	}
	ModulateXmtr(outbuf, symlen);

	memset(outbuf, 0, symlen * sizeof(*outbuf));
	ModulateXmtr(outbuf, symlen);

}

void modem::wfid_text(const string& s)
{
	int len = s.length();
	string video = "Video text: ";
	video += s;

	if (progdefaults.ID_SMALL) {
		NUMROWS = 7;
		NUMCOLS = 5;
		CHARSPACE = 2;
		vidwidth = progdefaults.videowidth;
		TONESPACING = 6;
		IDSYMLEN = 3072;
		useIDSMALL = true;
	} else {
		NUMROWS = 14;
		NUMCOLS = 8;
		CHARSPACE = 2;
		vidwidth = progdefaults.videowidth;
		TONESPACING = 8;
		IDSYMLEN = 2560;
		useIDSMALL = false;
	}
		if (progdefaults.vidlimit) {
			if ((vidwidth * TONESPACING * (NUMCOLS + CHARSPACE)) > 500)
				vidwidth = 500 / (TONESPACING * (NUMCOLS + CHARSPACE));
		}
		if (progdefaults.vidmodelimit) {
			if ((vidwidth * TONESPACING * (NUMCOLS + CHARSPACE)) > get_bandwidth())
				vidwidth = (int)ceil(get_bandwidth() / (TONESPACING * (NUMCOLS + CHARSPACE)));
		}

	put_status(video.c_str());

	disable_modem = true;

	int numlines = 0;
	string tosend;
	while (numlines < len) numlines += vidwidth;
	numlines -= vidwidth;
	while (numlines >= 0) {
		tosend = s.substr(numlines, vidwidth);
		wfid_sendchars(tosend);
		numlines -= vidwidth;
		if (stopflag)
			break;
	}
// blank lines
	for (int i = 0; i < vidwidth; i++) id_symbols[i] = 0;
	wfid_send(vidwidth);
	wfid_send(vidwidth);

	put_status("");
	disable_modem = false;
}

double	modem::wfid_outbuf[MAXIDSYMLEN];
double  modem::wfid_w[MAXTONES];

mfntchr idch1[] = {
{ ' ', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }, },
{ '!', { 0x00, 0x08, 0x08, 0x08, 0x00, 0x08, 0x00, }, },
{ '"', { 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, }, },
{ '#', { 0x00, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x00, }, },
{ '$', { 0x00, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x00, }, },
{ '%', { 0x00, 0x19, 0x02, 0x04, 0x08, 0x13, 0x00, }, },
{ '&', { 0x00, 0x08, 0x1C, 0x0D, 0x12, 0x0F, 0x00, }, },
{ '\'', { 0x00, 0x18, 0x08, 0x10, 0x00, 0x00, 0x00, }, },
{ '(', { 0x00, 0x0C, 0x10, 0x10, 0x10, 0x0C, 0x00, }, },
{ ')', { 0x00, 0x18, 0x04, 0x04, 0x04, 0x18, 0x00, }, },
{ '*', { 0x00, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x00, }, },
{ '+', { 0x00, 0x00, 0x04, 0x1F, 0x04, 0x00, 0x00, }, },
{ ',', { 0x00, 0x00, 0x00, 0x00, 0x18, 0x08, 0x00, }, },
{ '-', { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, }, },
{ '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, }, },
{ '/', { 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00, }, },
{ '0', { 0x00, 0x0E, 0x13, 0x15, 0x19, 0x0E, 0x00, }, },
{ '1', { 0x00, 0x0C, 0x14, 0x04, 0x04, 0x04, 0x00, }, },
{ '2', { 0x00, 0x1C, 0x02, 0x04, 0x08, 0x1F, 0x00, }, },
{ '3', { 0x00, 0x1E, 0x01, 0x06, 0x01, 0x1E, 0x00, }, },
{ '4', { 0x00, 0x12, 0x12, 0x1F, 0x02, 0x02, 0x00, }, },
{ '5', { 0x00, 0x1F, 0x10, 0x1E, 0x01, 0x1E, 0x00, }, },
{ '6', { 0x00, 0x0E, 0x10, 0x1E, 0x11, 0x0E, 0x00, }, },
{ '7', { 0x00, 0x1F, 0x01, 0x02, 0x04, 0x08, 0x00, }, },
{ '8', { 0x00, 0x0E, 0x11, 0x0E, 0x11, 0x0E, 0x00, }, },
{ '9', { 0x00, 0x0E, 0x11, 0x0F, 0x01, 0x0E, 0x00, }, },
{ ':', { 0x00, 0x00, 0x18, 0x00, 0x18, 0x00, 0x00, }, },
{ ';', { 0x00, 0x18, 0x00, 0x18, 0x08, 0x10, 0x00, }, },
{ '<', { 0x00, 0x01, 0x06, 0x18, 0x06, 0x01, 0x00, }, },
{ '=', { 0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00, }, },
{ '>', { 0x00, 0x10, 0x0C, 0x03, 0x0C, 0x10, 0x00, }, },
{ '?', { 0x00, 0x1C, 0x02, 0x04, 0x00, 0x04, 0x00, }, },
{ '@', { 0x00, 0x0E, 0x11, 0x16, 0x10, 0x0F, 0x00, }, },
{ 'A', { 0x00, 0x0E, 0x11, 0x1F, 0x11, 0x11, 0x00, }, },
{ 'B', { 0x00, 0x1E, 0x09, 0x0E, 0x09, 0x1E, 0x00, }, },
{ 'C', { 0x00, 0x0F, 0x10, 0x10, 0x10, 0x0F, 0x00, }, },
{ 'D', { 0x00, 0x1E, 0x11, 0x11, 0x11, 0x1E, 0x00, }, },
{ 'E', { 0x00, 0x1F, 0x10, 0x1C, 0x10, 0x1F, 0x00, }, },
{ 'F', { 0x00, 0x1F, 0x10, 0x1C, 0x10, 0x10, 0x00, }, },
{ 'G', { 0x00, 0x0F, 0x10, 0x13, 0x11, 0x0F, 0x00, }, },
{ 'H', { 0x00, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x00, }, },
{ 'I', { 0x00, 0x1C, 0x08, 0x08, 0x08, 0x1C, 0x00, }, },
{ 'J', { 0x00, 0x01, 0x01, 0x01, 0x11, 0x0E, 0x00, }, },
{ 'K', { 0x00, 0x11, 0x12, 0x1C, 0x12, 0x11, 0x00, }, },
{ 'L', { 0x00, 0x10, 0x10, 0x10, 0x10, 0x1F, 0x00, }, },
{ 'M', { 0x00, 0x11, 0x1B, 0x15, 0x11, 0x11, 0x00, }, },
{ 'N', { 0x00, 0x11, 0x19, 0x15, 0x13, 0x11, 0x00, }, },
{ 'O', { 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00, }, },
{ 'P', { 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10, 0x00, }, },
{ 'Q', { 0x00, 0x0E, 0x11, 0x11, 0x15, 0x0E, 0x01, }, },
{ 'R', { 0x00, 0x1E, 0x11, 0x1E, 0x12, 0x11, 0x00, }, },
{ 'S', { 0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E, 0x00, }, },
{ 'T', { 0x00, 0x1F, 0x04, 0x04, 0x04, 0x04, 0x00, }, },
{ 'U', { 0x00, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00, }, },
{ 'V', { 0x00, 0x11, 0x12, 0x14, 0x18, 0x10, 0x00, }, },
{ 'W', { 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A, 0x00, }, },
{ 'X', { 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00, }, },
{ 'Y', { 0x00, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x00, }, },
{ 'Z', { 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F, 0x00, }, },
};

mfntchr idch2[] = {
{' ', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, },
{'!', { 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00 }, },
{'"', { 0x00, 0x36, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, },
{'#', { 0x00, 0x50, 0x50, 0xF8, 0xF8, 0x50, 0x50, 0xF8, 0xF8, 0x50, 0x50, 0x00, 0x00, 0x00 }, },
{'$', { 0x00, 0x20, 0x20, 0x78, 0xF8, 0xA0, 0xF0, 0x78, 0x28, 0xF8, 0xF0, 0x20, 0x20, 0x00 }, },
{'%', { 0x00, 0x40, 0xE4, 0xE4, 0x4C, 0x18, 0x30, 0x60, 0xC8, 0x9C, 0x9C, 0x88, 0x00, 0x00 }, },
{'&', { 0x00, 0x30, 0x78, 0x48, 0x48, 0x70, 0xF4, 0x8C, 0x88, 0xFC, 0x74, 0x00, 0x00, 0x00 }, },
{'\'', { 0x00, 0x40, 0x40, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, },
{'(', { 0x00, 0x00, 0x20, 0x60, 0xC0, 0x80, 0x80, 0x80, 0x80, 0xC0, 0x60, 0x20, 0x00, 0x00 }, },
{')', { 0x00, 0x00, 0x80, 0xC0, 0x60, 0x20, 0x20, 0x20, 0x20, 0x60, 0xC0, 0x80, 0x00, 0x00 }, },
{'*', { 0x00, 0x00, 0x00, 0x10, 0x10, 0xFE, 0x7C, 0x38, 0x6C, 0x44, 0x00, 0x00, 0x00, 0x00 }, },
{'+', { 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0xF8, 0xF8, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00 }, },
{',', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0xC0, 0x40, 0xC0, 0x80 }, },
{'-', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, },
{'.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00 }, },
{'/', { 0x00, 0x08, 0x08, 0x18, 0x10, 0x30, 0x20, 0x60, 0x40, 0xC0, 0x80, 0x80, 0x00, 0x00 }, },
{'0', { 0x00, 0x00, 0x78, 0xFC, 0x8C, 0x9C, 0xB4, 0xE4, 0xC4, 0x84, 0xFC, 0x78, 0x00, 0x00 }, },
{'1', { 0x00, 0x00, 0x10, 0x30, 0x70, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00 }, },
{'2', { 0x00, 0x00, 0x78, 0xFC, 0x84, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0xFC, 0xFC, 0x00, 0x00 }, },
{'3', { 0x00, 0x00, 0xFC, 0xFC, 0x04, 0x0C, 0x18, 0x1C, 0x04, 0x84, 0xFC, 0x78, 0x00, 0x00 }, },
{'4', { 0x00, 0x00, 0x38, 0x78, 0x48, 0xC8, 0x88, 0xFC, 0xFC, 0x08, 0x08, 0x08, 0x00, 0x00 }, },
{'5', { 0x00, 0x00, 0xFC, 0xFC, 0x80, 0x80, 0xF8, 0xFC, 0x04, 0x04, 0xFC, 0xF8, 0x00, 0x00 }, },
{'6', { 0x00, 0x00, 0x78, 0xF8, 0x80, 0x80, 0xF8, 0xFC, 0x84, 0x84, 0xFC, 0x78, 0x00, 0x00 }, },
{'7', { 0x00, 0x00, 0xFC, 0xFC, 0x04, 0x04, 0x0C, 0x18, 0x30, 0x20, 0x20, 0x20, 0x00, 0x00 }, },
{'8', { 0x00, 0x00, 0x78, 0xFC, 0x84, 0x84, 0x78, 0xFC, 0x84, 0x84, 0xFC, 0x78, 0x00, 0x00 }, },
{'9', { 0x00, 0x00, 0x78, 0xFC, 0x84, 0x84, 0xFC, 0x7C, 0x04, 0x04, 0x7C, 0x78, 0x00, 0x00 }, },
{':', { 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0x00 }, },
{';', { 0x00, 0x60, 0x60, 0x60, 0x00, 0x00, 0x60, 0x60, 0x20, 0x20, 0xE0, 0xC0, 0x00, 0x00 }, },
{'<', { 0x00, 0x00, 0x08, 0x18, 0x30, 0x60, 0xC0, 0xC0, 0x60, 0x30, 0x18, 0x08, 0x00, 0x00 }, },
{'=', { 0x00, 0x00, 0x00, 0x00, 0xF8, 0xF8, 0x00, 0x00, 0xF8, 0xF8, 0x00, 0x00, 0x00, 0x00 }, },
{'>', { 0x00, 0x00, 0x80, 0xC0, 0x60, 0x30, 0x18, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00, 0x00 }, },
{'?', { 0x00, 0x00, 0x70, 0xF8, 0x88, 0x08, 0x18, 0x30, 0x20, 0x00, 0x20, 0x20, 0x00, 0x00 }, },
{'@', { 0x00, 0x00, 0x7C, 0xFE, 0x82, 0x82, 0xB2, 0xBE, 0xBC, 0x80, 0xFC, 0x7C, 0x00, 0x00 }, },
{'A', { 0x00, 0x00, 0x30, 0x78, 0xCC, 0x84, 0x84, 0xFC, 0xFC, 0x84, 0x84, 0x84, 0x00, 0x00 }, },
{'B', { 0x00, 0x00, 0xF8, 0xFC, 0x84, 0x84, 0xF8, 0xF8, 0x84, 0x84, 0xFC, 0xF8, 0x00, 0x00 }, },
{'C', { 0x00, 0x00, 0x38, 0x7C, 0xC4, 0x80, 0x80, 0x80, 0x80, 0xC4, 0x7C, 0x38, 0x00, 0x00 }, },
{'D', { 0x00, 0x00, 0xF0, 0xF8, 0x8C, 0x84, 0x84, 0x84, 0x84, 0x8C, 0xF8, 0xF0, 0x00, 0x00 }, },
{'E', { 0x00, 0x00, 0xFC, 0xFC, 0x80, 0x80, 0xF0, 0xF0, 0x80, 0x80, 0xFC, 0xFC, 0x00, 0x00 }, },
{'F', { 0x00, 0x00, 0xFC, 0xFC, 0x80, 0x80, 0xF0, 0xF0, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00 }, },
{'G', { 0x00, 0x00, 0x3C, 0x7C, 0xC0, 0x80, 0x8C, 0x8C, 0x84, 0xC4, 0x7C, 0x38, 0x00, 0x00 }, },
{'H', { 0x00, 0x00, 0x84, 0x84, 0x84, 0x84, 0xFC, 0xFC, 0x84, 0x84, 0x84, 0x84, 0x00, 0x00 }, },
{'I', { 0x00, 0x00, 0xF8, 0xF8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xF8, 0xF8, 0x00, 0x00 }, },
{'J', { 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x84, 0x84, 0xFC, 0x78, 0x00, 0x00 }, },
{'K', { 0x00, 0x00, 0x84, 0x84, 0x8C, 0x98, 0xF0, 0xF0, 0x98, 0x8C, 0x84, 0x84, 0x00, 0x00 }, },
{'L', { 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFC, 0xFC, 0x00, 0x00 }, },
{'M', { 0x00, 0x00, 0x82, 0xC6, 0xEE, 0xBA, 0x92, 0x82, 0x82, 0x82, 0x82, 0x82, 0x00, 0x00 }, },
{'N', { 0x00, 0x00, 0x84, 0xC4, 0xE4, 0xB4, 0x9C, 0x8C, 0x84, 0x84, 0x84, 0x84, 0x00, 0x00 }, },
{'O', { 0x00, 0x00, 0x30, 0x78, 0xCC, 0x84, 0x84, 0x84, 0x84, 0xCC, 0x78, 0x30, 0x00, 0x00 }, },
{'P', { 0x00, 0x00, 0xF8, 0xFC, 0x84, 0x84, 0xFC, 0xF8, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00 }, },
{'Q', { 0x00, 0x00, 0x78, 0xFC, 0x84, 0x84, 0x84, 0x84, 0x94, 0x94, 0xFC, 0x78, 0x08, 0x08 }, },
{'R', { 0x00, 0x00, 0xF8, 0xFC, 0x84, 0x84, 0xFC, 0xF8, 0x88, 0x8C, 0x84, 0x84, 0x00, 0x00 }, },
{'S', { 0x00, 0x00, 0x78, 0xFC, 0x84, 0x80, 0xF8, 0x7C, 0x04, 0x84, 0xFC, 0x78, 0x00, 0x00 }, },
{'T', { 0x00, 0x00, 0xF8, 0xF8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00 }, },
{'U', { 0x00, 0x00, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0xFC, 0x78, 0x00, 0x00 }, },
{'V', { 0x00, 0x00, 0x82, 0x82, 0x82, 0xC6, 0x44, 0x6C, 0x28, 0x38, 0x10, 0x10, 0x00, 0x00 }, },
{'W', { 0x00, 0x00, 0x82, 0x82, 0x82, 0x82, 0x82, 0x92, 0x92, 0x92, 0xFE, 0x6C, 0x00, 0x00 }, },
{'X', { 0x00, 0x00, 0x82, 0x82, 0xC6, 0x6C, 0x38, 0x38, 0x6C, 0xC6, 0x82, 0x82, 0x00, 0x00 }, },
{'Y', { 0x00, 0x00, 0x82, 0x82, 0xC6, 0x6C, 0x38, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00 }, },
{'Z', { 0x00, 0x00, 0xFC, 0xFC, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0xFC, 0xFC, 0x00, 0x00 }, },
{'[', { 0x00, 0x00, 0xE0, 0xE0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xE0, 0xE0, 0x00, 0x00 }, },
{'\\', { 0x00, 0x80, 0x80, 0xC0, 0x40, 0x60, 0x20, 0x30, 0x10, 0x18, 0x08, 0x08, 0x00, 0x00 }, },
{']', { 0x00, 0x00, 0xE0, 0xE0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xE0, 0xE0, 0x00, 0x00 }, },
{'^', { 0x00, 0x20, 0x20, 0x70, 0x50, 0xD8, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, },
{'_', { 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00 }, },
{'`', { 0x00, 0xC0, 0xC0, 0xC0, 0xC0, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, },
{'a', { 0x00, 0x00, 0x00, 0x00, 0x78, 0x7C, 0x04, 0x7C, 0xFC, 0x84, 0xFC, 0x7C, 0x00, 0x00 }, },
{'b', { 0x00, 0x00, 0x80, 0x80, 0xB8, 0xFC, 0xC4, 0x84, 0x84, 0x84, 0xFC, 0xF8, 0x00, 0x00 }, },
{'c', { 0x00, 0x00, 0x00, 0x00, 0x78, 0xF8, 0x80, 0x80, 0x80, 0x80, 0xF8, 0x78, 0x00, 0x00 }, },
{'d', { 0x00, 0x00, 0x04, 0x04, 0x74, 0xFC, 0x8C, 0x84, 0x84, 0x84, 0xFC, 0x7C, 0x00, 0x00 }, },
{'e', { 0x00, 0x00, 0x00, 0x00, 0x78, 0xFC, 0x84, 0xFC, 0xFC, 0x80, 0xF8, 0x78, 0x00, 0x00 }, },
{'f', { 0x00, 0x00, 0x3C, 0x7C, 0x40, 0x40, 0xF8, 0xF8, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00 }, },
{'g', { 0x00, 0x00, 0x00, 0x7C, 0xFC, 0x84, 0x84, 0x8C, 0xFC, 0x74, 0x04, 0x7C, 0x78, 0x00 }, },
{'h', { 0x00, 0x00, 0x80, 0x80, 0xB8, 0xFC, 0xC4, 0x84, 0x84, 0x84, 0x84, 0x84, 0x00, 0x00 }, },
{'i', { 0x00, 0x20, 0x20, 0x00, 0xE0, 0xE0, 0x20, 0x20, 0x20, 0x20, 0xF8, 0xF8, 0x00, 0x00 }, },
{'j', { 0x00, 0x08, 0x08, 0x00, 0x38, 0x38, 0x08, 0x08, 0x08, 0x08, 0x08, 0x88, 0xF8, 0x70 }, },
{'k', { 0x00, 0x00, 0x80, 0x88, 0x98, 0xB0, 0xE0, 0xE0, 0xB0, 0x98, 0x88, 0x88, 0x00, 0x00 }, },
{'l', { 0x00, 0x00, 0xE0, 0xE0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xF8, 0xF8, 0x00, 0x00 }, },
{'m', { 0x00, 0x00, 0x00, 0x00, 0xEC, 0xFE, 0x92, 0x92, 0x82, 0x82, 0x82, 0x82, 0x00, 0x00 }, },
{'n', { 0x00, 0x00, 0x00, 0x00, 0xB8, 0xFC, 0xC4, 0x84, 0x84, 0x84, 0x84, 0x84, 0x00, 0x00 }, },
{'o', { 0x00, 0x00, 0x00, 0x00, 0x78, 0xFC, 0x84, 0x84, 0x84, 0x84, 0xFC, 0x78, 0x00, 0x00 }, },
{'p', { 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFC, 0x84, 0x84, 0xC4, 0xFC, 0xB8, 0x80, 0x80, 0x80 }, },
{'q', { 0x00, 0x00, 0x00, 0x00, 0x7C, 0xFC, 0x84, 0x84, 0x8C, 0xFC, 0x74, 0x04, 0x04, 0x04 }, },
{'r', { 0x00, 0x00, 0x00, 0x00, 0xB8, 0xFC, 0xC4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00 }, },
{'s', { 0x00, 0x00, 0x00, 0x00, 0x7C, 0xFC, 0x80, 0xF8, 0x7C, 0x04, 0xFC, 0xF8, 0x00, 0x00 }, },
{'t', { 0x00, 0x00, 0x40, 0x40, 0xF0, 0xF0, 0x40, 0x40, 0x40, 0x40, 0x78, 0x38, 0x00, 0x00 }, },
{'u', { 0x00, 0x00, 0x00, 0x00, 0x84, 0x84, 0x84, 0x84, 0x84, 0x8C, 0xFC, 0x74, 0x00, 0x00 }, },
{'v', { 0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0x82, 0x82, 0xC6, 0x6C, 0x38, 0x10, 0x00, 0x00 }, },
{'w', { 0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0x82, 0x92, 0x92, 0x92, 0xFE, 0x6C, 0x00, 0x00 }, },
{'x', { 0x00, 0x00, 0x00, 0x00, 0x82, 0xC6, 0x6C, 0x38, 0x38, 0x6C, 0xC6, 0x82, 0x00, 0x00 }, },
{'y', { 0x00, 0x00, 0x00, 0x00, 0x84, 0x84, 0x84, 0x84, 0x8C, 0xFC, 0x74, 0x04, 0x7C, 0x78 }, },
{'z', { 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFC, 0x18, 0x30, 0x60, 0xC0, 0xFC, 0xFC, 0x00, 0x00 }, },
{'{', { 0x00, 0x20, 0x60, 0x40, 0x40, 0x40, 0xC0, 0xC0, 0x40, 0x40, 0x40, 0x60, 0x20, 0x00 }, },
{'|', { 0x00, 0x80, 0x80, 0xC0, 0x40, 0x60, 0x20, 0x30, 0x10, 0x18, 0x08, 0x08, 0x00, 0x00 }, },
{'}', { 0x00, 0x80, 0xC0, 0x40, 0x40, 0x40, 0x60, 0x60, 0x40, 0x40, 0x40, 0xC0, 0x80, 0x00 }, },
{'~', { 0x00, 0x98, 0xFC, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }
};
