// ----------------------------------------------------------------------------
// trx.cxx  --  Main transmit/receive control loop / thread
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
// Copyright (C) 2007-2010
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.  Adapted in part from code contained in gmfsk
// source code distribution.
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

#include <sys/time.h>
#include <fcntl.h>
#include <semaphore.h>
#include <cstdlib>
#include <string>
#include <string.h>

#include "trx.h"
#include "main.h"
#include "fl_digi.h"
#include "ascii.h"
#include "misc.h"
#include "configuration.h"
#include "status.h"
#include "dtmf.h"

#include "soundconf.h"
#include "ringbuffer.h"
#include "qrunner.h"
#include "debug.h"
#include "nullmodem.h"
#include "macros.h"
#include "rigsupport.h"
#include "psm/psm.h"
#include "icons.h"
#include "fft-monitor.h"
#include "audio_alert.h"

extern fftmon *fft_modem;

#include "spectrum_viewer.h"

#if BENCHMARK_MODE
#  include "benchmark.h"
#endif

LOG_FILE_SOURCE(debug::LOG_MODEM);

void	trx_reset_loop();
void	trx_start_modem_loop();
void	trx_receive_loop();
void	trx_transmit_loop();
void	trx_tune_loop();
static void trx_signal_state(void);

//#define DEBUG

/* ---------------------------------------------------------------------- */

static sem_t*	trx_sem;
static pthread_t trx_thread;
state_t 	trx_state;

modem		*active_modem = 0;
cRsId		*ReedSolomon = 0;
cDTMF		*dtmf = 0;
SoundBase 	*RXscard = 0;
bool		RXsc_is_open = false;
bool		TXsc_is_open = false;
SoundBase	*TXscard = 0;
static int	current_RXsamplerate = 0;
static int	current_TXsamplerate = 0;

static int	_trx_tune;

// Ringbuffer for the audio "history". A pointer into this buffer
// is also passed to the waterfall signal drawing routines.
#define NUMMEMBUFS 1024
static ringbuffer<double> trxrb(ceil2(NUMMEMBUFS * SCBLOCKSIZE));
static float fbuf[SCBLOCKSIZE];
bool	bHistory = false;
bool	bHighSpeed = false;
static  double hsbuff[SCBLOCKSIZE];

static bool trxrunning = false;

extern bool	trx_inhibit;
bool	rx_only = false;

#include "tune.cxx"

//=============================================================================

// Draws the xmit data one WF_BLOCKSIZE-sized block at a time
static void trx_xmit_wfall_draw(int samplerate)
{
	ENSURE_THREAD(TRX_TID);

	ringbuffer<double>::vector_type rv[2];
	rv[0].buf = 0;
	rv[1].buf = 0;

#define block_read_(vec_)						\
	while (vec_.len >= WF_BLOCKSIZE) {			\
		wf->sig_data(vec_.buf, WF_BLOCKSIZE);	\
		REQ(&waterfall::handle_sig_data, wf);	\
		vec_.len -= WF_BLOCKSIZE;				\
		vec_.buf += WF_BLOCKSIZE;				\
		trxrb.read_advance(WF_BLOCKSIZE);		\
	}

	trxrb.get_rv(rv);
	block_read_(rv[0]); // read blocks from the first vector

	if (rv[0].len + rv[1].len < WF_BLOCKSIZE)
		return;
	if (rv[0].len == 0)
		block_read_(rv[1]);
#undef block_read_

	// read non-contiguous data into tmp buffer so that we can
	// still draw it one block at a time
	if (unlikely(trxrb.read_space() >= WF_BLOCKSIZE)) {
		double buf[WF_BLOCKSIZE];
		do {
			trxrb.read(buf, WF_BLOCKSIZE);
			wf->sig_data(buf, WF_BLOCKSIZE);
			REQ(&waterfall::handle_sig_data, wf);
		} while (trxrb.read_space() >= WF_BLOCKSIZE);
	}
}

// Called by trx_trx_transmit_loop() to handle data that may be left in the
// ringbuffer when we stop transmitting. Will pad with zeroes to a multiple of
// WF_BLOCKSIZE.
static void trx_xmit_wfall_end(int samplerate)
{
	ENSURE_THREAD(TRX_TID);

	size_t pad = WF_BLOCKSIZE - trxrb.read_space() % WF_BLOCKSIZE;
	if (pad == WF_BLOCKSIZE) // rb empty or multiple of WF_BLOCKSIZE
		return;

	ringbuffer<double>::vector_type wv[2];
	wv[0].buf = wv[1].buf = 0;

	trxrb.get_wv(wv, pad);
	assert(wv[0].len + wv[1].len == pad);

	if (likely(wv[0].len)) { // fill first vector, write rest to second vector
		memset(wv[0].buf, 0, wv[0].len * sizeof(*wv[0].buf));
		if (pad > wv[0].len)
			memset(wv[1].buf, 0, (pad - wv[0].len) * sizeof(*wv[1].buf));
	}
	else // all write space is in the second write vector
		memset(wv[1].buf, 0, pad * sizeof(*wv[1].buf));

	trxrb.write_advance(pad);

	trx_xmit_wfall_draw(samplerate);
}

// Copy buf to the ringbuffer if it has enough space. Queue a waterfall
// request whenever there are at least WF_BLOCKSIZE samples to draw.
void trx_xmit_wfall_queue(int samplerate, const double* buf, size_t len)
{
	ENSURE_THREAD(TRX_TID);
	ringbuffer<double>::vector_type wv[2];
	wv[0].buf = wv[1].buf = 0;

	trxrb.get_wv(wv, len);
	if (unlikely(wv[0].len + wv[1].len < len)) // not enough space
		return;

	size_t n = MIN(wv[0].len, len);

	for (size_t i = 0; i < n; i++)
		wv[0].buf[i] = buf[i] * progdefaults.TxMonitorLevel;

	if (len > n) { // write the remainder to the second vector
		buf += n;
		n = len - n;
		for (size_t i = 0; i < n; i++)
			wv[1].buf[i] = buf[i] * progdefaults.TxMonitorLevel;

	}

	trxrb.write_advance(len);
	if (trxrb.read_space() >= WF_BLOCKSIZE)
		trx_xmit_wfall_draw(samplerate);
}

//=============================================================================

void audio_select_failure(std::string errmsg)
{
	progdefaults.btnAudioIOis = SND_IDX_NULL; // file i/o
	sound_update(progdefaults.btnAudioIOis);
	btnAudioIO[0]->value(0);
	btnAudioIO[1]->value(0);
	btnAudioIO[2]->value(0);
	btnAudioIO[3]->value(1);
	delete RXscard;
	RXscard = 0;
	fl_alert2("Could not open audio device: %s\nCheck for h/w connection, and restart fldigi", errmsg.c_str());
}

void trx_trx_receive_loop()
{
	size_t  numread;
	assert(powerof2(SCBLOCKSIZE));

	if (unlikely(!active_modem)) {
	MilliSleep(10);
	return;
	}

#if BENCHMARK_MODE
	do_benchmark();
	trx_state = STATE_ENDED;
	return;
#endif

	if (!RXscard) {
		MilliSleep(10);
		return;
	}

	try {
		if (!progdefaults.is_full_duplex || !RXsc_is_open ||
			current_RXsamplerate != active_modem->get_samplerate() ) {
			current_RXsamplerate = active_modem->get_samplerate();
			if (RXscard) {
				RXscard->Close(O_RDONLY);
				RXscard->Open(O_RDONLY, current_RXsamplerate);
				REQ(sound_update, progdefaults.btnAudioIOis);
				RXsc_is_open = true;
			}
		}
	}
	catch (const SndException& e) {
		LOG_ERROR("%s. line: %i", e.what(), __LINE__);
		put_status(e.what(), 5);
		if (RXscard) RXscard->Close();
		RXsc_is_open = false;
		current_RXsamplerate = 0;
		if (progdefaults.btnAudioIOis == SND_IDX_PORT) {
			sound_close();
			sound_init();
		}
		REQ(audio_select_failure, e.what());
		MilliSleep(100);
		return;
	}
	active_modem->rx_init();

	ringbuffer<double>::vector_type rbvec[2];
	rbvec[0].buf = rbvec[1].buf = 0;

	if (RXscard) RXscard->flush(O_RDONLY);

	while (1) {
		try {
			numread = 0;
			if (current_RXsamplerate != active_modem->get_samplerate() ) {
				current_RXsamplerate = active_modem->get_samplerate();
				if (RXscard) {
					RXscard->Close(O_RDONLY);
					RXscard->Open(O_RDONLY, current_RXsamplerate);
					REQ(sound_update, progdefaults.btnAudioIOis);
					RXsc_is_open = true;
				}
			}
			if (RXscard) {
				while (numread < SCBLOCKSIZE && trx_state == STATE_RX)
					numread += RXscard->Read(fbuf + numread, SCBLOCKSIZE - numread);
			}
			if (numread > SCBLOCKSIZE) {
				LOG_ERROR("numread error %lu", (unsigned long) numread);
				numread = SCBLOCKSIZE;
			}
			if (bHighSpeed) {
				for (size_t i = 0; i < numread; i++)
					hsbuff[i] = fbuf[i];
			} else {
				if (trxrb.write_space() == 0) // diRXscard some old data
					trxrb.read_advance(SCBLOCKSIZE);

				size_t room = trxrb.get_wv(rbvec, numread);

				if (room < numread) {
					LOG_ERROR("trxrb.get_wv(rbvec) = %d, numread = %d", (int)room, (int)numread);
				} else {
			// convert to double and write to rb
				for (size_t i = 0; i < numread; i++)
					rbvec[0].buf[i] = fbuf[i];
				}
			}
		}
		catch (const SndException& e) {
			if (RXscard) RXscard->Close();
			RXsc_is_open = false;
			LOG_ERROR("%s. line: %i", e.what(), __LINE__);
			put_status(e.what(), 5);
			MilliSleep(10);
			return;
		}
		if (trx_state != STATE_RX)
			break;

		if (bHighSpeed) {
			bool afc = progStatus.afconoff;
			progStatus.afconoff = false;
			QRUNNER_DROP(true);
			if (progdefaults.rsid)
				ReedSolomon->receive(fbuf, numread);
			else if (active_modem->get_mode() == MODE_OFDM_500F || active_modem->get_mode() == MODE_OFDM_750F || active_modem->get_mode() == MODE_OFDM_2000F)
				ReedSolomon->receive(fbuf, numread); // OFDM modes use RSID as AFC mechanism. Force RxRSID.
			active_modem->HistoryON(true);
			active_modem->rx_process(hsbuff, numread);
			QRUNNER_DROP(false);
			progStatus.afconoff = afc;
			active_modem->HistoryON(false);
		} else {
			trxrb.write_advance(numread);

			wf->sig_data(rbvec[0].buf, numread);

			if (!trx_inhibit)
				REQ(&waterfall::handle_sig_data, wf);
			if (!bHistory) {
				if (fft_modem && spectrum_viewer->visible())
					fft_modem->rx_process(rbvec[0].buf, numread);
				active_modem->rx_process(rbvec[0].buf, numread);

				if (audio_alert)
					audio_alert->monitor(rbvec[0].buf, numread, current_RXsamplerate);

				if (progdefaults.rsid)
					ReedSolomon->receive(fbuf, numread);
				else if (active_modem->get_mode() == MODE_OFDM_500F || active_modem->get_mode() == MODE_OFDM_750F || active_modem->get_mode() == MODE_OFDM_2000F)
					ReedSolomon->receive(fbuf, numread);  // OFDM modes use RSID as AFC mechanism. Force RxRSID.
				dtmf->receive(fbuf, numread);
			} else {
				bool afc = progStatus.afconoff;
				progStatus.afconoff = false;
				QRUNNER_DROP(true);
				active_modem->HistoryON(true);
				trxrb.get_rv(rbvec);
				if (rbvec[0].len)
					active_modem->rx_process(rbvec[0].buf, rbvec[0].len);
				if (rbvec[1].len)
					active_modem->rx_process(rbvec[1].buf, rbvec[1].len);
				QRUNNER_DROP(false);
				progStatus.afconoff = afc;
				bHistory = false;
				active_modem->HistoryON(false);
			}
		}
	}
	if (trx_state == STATE_RESTART)
		return;

	if (!progdefaults.is_full_duplex ) {
		if (RXscard) RXscard->Close(O_RDONLY);
		RXsc_is_open = false;
	}
}


//=============================================================================
void trx_trx_transmit_loop()
{
	if (rx_only) return;

	if (!TXscard) {
		MilliSleep(10);
		return;
	}
	if (active_modem) {

		try {
			if (current_TXsamplerate != active_modem->get_samplerate() || !TXsc_is_open) {
				current_TXsamplerate = active_modem->get_samplerate();
				if (TXscard) {
					TXscard->Close(O_WRONLY);
					TXscard->Open(O_WRONLY, current_TXsamplerate);
					TXsc_is_open = true;
				}
			}
		} catch (const SndException& e) {
			LOG_ERROR("%s. line: %i", e.what(), __LINE__);
			put_status(e.what(), 1);
			current_TXsamplerate = 0;
			MilliSleep(10);
			return;
		}

		if ((active_modem != ssb_modem) &&
			(active_modem != anal_modem) &&
			!active_modem->XMLRPC_CPS_TEST &&
			!PERFORM_CPS_TEST ) {
			push2talk->set(true);
			REQ(&waterfall::set_XmtRcvBtn, wf, true);
		}
		active_modem->tx_init();

		bool _txrsid = false;
		if ( ReedSolomon->assigned(active_modem->get_mode()) && (progdefaults.TransmitRSid || progStatus.n_rsids != 0))
			_txrsid = true;
		else if (ReedSolomon->assigned(active_modem->get_mode()) && (active_modem->get_mode() == MODE_OFDM_500F || active_modem->get_mode() == MODE_OFDM_750F || active_modem->get_mode() == MODE_OFDM_2000F) )
			_txrsid = true; // RSID is used as header/preamble for OFDM modes. Make mandatory.
			
		if (_txrsid) {
			if (progStatus.n_rsids < 0) {
				for (int i = 0; i > progStatus.n_rsids; i--) {
					ReedSolomon->send(true);
					MilliSleep(200);
				}
			} else if ( progStatus.n_rsids > 0 ) {
				for (int i = 0; i < progStatus.n_rsids; i++) {
					ReedSolomon->send(true);
					MilliSleep(200);
				}
				MilliSleep(200);
				if (progStatus.n_rsids == 1) progStatus.n_rsids = 0;
			} else
				ReedSolomon->send(true);
		}

		if (progStatus.n_rsids >= 0) {

			active_modem->tx_sample_count = 0;
			active_modem->tx_sample_rate = active_modem->get_samplerate();

			while (trx_state == STATE_TX) {
				try {
					if (!progdefaults.DTMFstr.empty())
						dtmf->send();
					if (active_modem->tx_process() < 0) {
						active_modem->cwid();
						if (trx_state != STATE_ABORT)
							trx_state = STATE_RX;
					}
				}
				catch (const SndException& e) {
					if (TXscard) TXscard->Close();
					TXsc_is_open = false;
					LOG_ERROR("%s", e.what());
					put_status(e.what(), 5);
					current_TXsamplerate = 0;
					MilliSleep(10);
					return;
				}
			}
		} else
			if (trx_state != STATE_ABORT && trx_state != STATE_RESTART)
				trx_state = STATE_RX;

		if (ReedSolomon->assigned(active_modem->get_mode()) &&
			progdefaults.TransmitRSid &&
			progdefaults.rsid_post &&
			progStatus.n_rsids >= 0) ReedSolomon->send(false);

		progStatus.n_rsids = 0;

		trx_xmit_wfall_end(current_TXsamplerate);

		if (TXscard) TXscard->flush();

		if (trx_state == STATE_RX) {
			if (!progdefaults.is_full_duplex) {
				if (TXscard) TXscard->Close(O_WRONLY);
				TXsc_is_open = false;
			}
		}

	} else
		MilliSleep(10);

	push2talk->set(false);
	REQ(&waterfall::set_XmtRcvBtn, wf, false);
	psm_transmit_ended(PSM_STOP);
	if (progStatus.timer)
		REQ(startMacroTimer);
	WriteARQ(0x06);
}

//=============================================================================
void trx_tune_loop()
{
	if (rx_only) return;

	if (!TXscard) {
		MilliSleep(10);
		return;
	}
	if (active_modem) {
		try {
			if (!progdefaults.is_full_duplex || !TXsc_is_open ||
				current_TXsamplerate != active_modem->get_samplerate() ) {
				current_TXsamplerate = active_modem->get_samplerate();
				if (TXscard) TXscard->Close(O_WRONLY);
				if (TXscard) {
					TXscard->Open(O_WRONLY, current_TXsamplerate);
					TXsc_is_open = true;
				}
			}
		} catch (const SndException& e) {
			LOG_ERROR("%s. line: %i", e.what(), __LINE__);
			put_status(e.what(), 1);
			MilliSleep(10);
			current_TXsamplerate = 0;
			return;
		}

		push2talk->set(true);
		active_modem->tx_init();

		try {
			if (active_modem->get_mode() == MODE_CW && 
				(use_nanoIO ||
				 progStatus.WK_online || 
				 progdefaults.CW_KEYLINE_on_cat_port ||
				 CW_KEYLINE_isopen)) {
					if (CW_KEYLINE_isopen || progdefaults.CW_KEYLINE_on_cat_port)
						active_modem->CW_KEYLINE(1);
					else if (use_nanoIO)
						nanoCW_tune(1);
					else WK_tune(1);
					cwio_ptt(1);
					cwio_key(1);

					REQ(&waterfall::set_XmtRcvBtn, wf, true);
					while (trx_state == STATE_TUNE) MilliSleep(10);

					if (CW_KEYLINE_isopen) active_modem->CW_KEYLINE(0);
					else if (use_nanoIO) nanoCW_tune(0);
					else WK_tune(0);
					cwio_key(0);
					cwio_ptt(0);
			} else {
				while (trx_state == STATE_TUNE) {
					if (_trx_tune == 0) {
						REQ(&waterfall::set_XmtRcvBtn, wf, true);
						xmttune::keydown(active_modem->get_txfreq_woffset(), TXscard);
						_trx_tune = 1;
					} else
						xmttune::tune(active_modem->get_txfreq_woffset(), TXscard);
				}
				xmttune::keyup(active_modem->get_txfreq_woffset(), TXscard);
			}
		}
		catch (const SndException& e) {
			if (TXscard) TXscard->Close();
			TXsc_is_open = false;
			LOG_ERROR("%s. line: %i", e.what(), __LINE__);
			put_status(e.what(), 5);
			MilliSleep(10);
			current_TXsamplerate = 0;
			return;
		}
		if (TXscard) TXscard->flush();

		if (trx_state == STATE_RX) {
			if (!progdefaults.is_full_duplex) {
				if (TXscard) TXscard->Close(O_WRONLY);
				TXsc_is_open = false;
			}
		}

		_trx_tune = 0;
	} else
		MilliSleep(10);

	push2talk->set(false);
	REQ(&waterfall::set_XmtRcvBtn, wf, false);
}

//=============================================================================
void *trx_loop(void *args)
{
	SET_THREAD_ID(TRX_TID);

	state_t old_state = STATE_NOOP;

	for (;;) {
		if (unlikely(old_state != trx_state)) {
			old_state = trx_state;
			if (trx_state == STATE_TX || trx_state == STATE_TUNE)
				trxrb.reset();
			trx_signal_state();
		}

		LOG_DEBUG("trx state %s",
			trx_state == STATE_ABORT ? "abort" :
			trx_state == STATE_ENDED ? "ended" :
			trx_state == STATE_RESTART ? "restart" :
			trx_state == STATE_NEW_MODEM ? "new modem" :
			trx_state == STATE_TX ? "tx" :
			trx_state == STATE_TUNE ? "tune" :
			trx_state == STATE_RX ? "rx" :
			"unknown");

		switch (trx_state) {
		case STATE_ABORT:
			delete RXscard;
			RXscard = 0;
			delete TXscard;
			TXscard = 0;
			trx_state = STATE_ENDED;
			// fall through
		case STATE_ENDED:
			REQ(set_flrig_ptt, 0);
			stop_deadman();
			return 0;
		case STATE_RESTART:
			REQ(set_flrig_ptt, 0);
			stop_deadman();
			trx_reset_loop();
			break;
		case STATE_NEW_MODEM:
			REQ(set_flrig_ptt, 0);
			trx_start_modem_loop();
			break;
		case STATE_TX:
			REQ(set_flrig_ptt, 1);
			start_deadman();
			trx_trx_transmit_loop();
			break;
		case STATE_TUNE:
			REQ(set_flrig_ptt, 1);
			start_deadman();
			trx_tune_loop();
			break;
		case STATE_RX:
			REQ(set_flrig_ptt, 0);
			stop_deadman();
			trx_trx_receive_loop();
			break;
		default:
			LOG(debug::ERROR_LEVEL, debug::LOG_MODEM, "trx in bad state %d\n", trx_state);
			MilliSleep(100);
		}
	}
}

//=============================================================================
static modem* new_modem;
static int new_freq;

void trx_start_modem_loop()
{
	if (new_modem == active_modem) {
		if (new_freq > 0 && !progdefaults.retain_freq_lock)
			active_modem->set_freq(new_freq);
		else if (new_freq > 0 && (active_modem->get_mode() == MODE_OFDM_500F || active_modem->get_mode() == MODE_OFDM_750F || active_modem->get_mode() == MODE_OFDM_2000F || active_modem->get_mode() == MODE_OFDM_2000))
			active_modem->set_freq(new_freq); // OFDM modes use RSID as AFC mechanism. Always allow QSY of Rx Frequency.
		active_modem->restart();
		trx_state = STATE_RX;
		if (progdefaults.show_psm_btn &&
			progStatus.kpsql_enabled &&
			progStatus.psm_use_histogram)
			psm_reset_histogram();
		return;
	}

	modem* old_modem = active_modem;

	new_modem->init();
	active_modem = new_modem;
	if (new_freq > 0 && !progdefaults.retain_freq_lock)
		active_modem->set_freq(new_freq);
	else if (new_freq > 0 && (active_modem->get_mode() == MODE_OFDM_500F || active_modem->get_mode() == MODE_OFDM_750F || active_modem->get_mode() == MODE_OFDM_2000F || active_modem->get_mode() == MODE_OFDM_2000))
		active_modem->set_freq(new_freq); // OFDM modes use RSID as AFC mechanism. Always allow QSY of Rx Frequency.
	trx_state = STATE_RX;
	REQ(&waterfall::opmode, wf);
	REQ(set599);

	if (old_modem) {
		*mode_info[old_modem->get_mode()].modem = 0;
		delete old_modem;
	}
}

//=============================================================================
void trx_start_modem(modem* m, int f)
{
	new_modem = m;
	new_freq = f;
	trx_state = STATE_NEW_MODEM;
}

//=============================================================================
static std::string reset_loop_msg;
void show_reset_loop_alert()
{
//	if (btnAudioIO[0]) {
	btnAudioIO[0]->value(0);
	btnAudioIO[1]->value(0);
	btnAudioIO[2]->value(0);
	btnAudioIO[3]->value(1);
	fl_alert2("%s", reset_loop_msg.c_str());
//	}
}

void trx_reset_loop()
{
	if (RXscard)  {
		RXscard->Close();
		RXsc_is_open = false;
		delete RXscard;
		RXscard = 0;
	}
	if (TXscard)  {
		TXscard->Close();
		TXsc_is_open = false;
		delete TXscard;
		TXscard = 0;
	}

	switch (progdefaults.btnAudioIOis) {
#if USE_OSS
	case SND_IDX_OSS:
		try {
			RXscard = new SoundOSS(scDevice[0].c_str());
			if (!RXscard) break;

			RXscard->Open(O_RDONLY, current_RXsamplerate = 8000);
			RXsc_is_open = true;

			TXscard = new SoundOSS(scDevice[0].c_str());
			if (!TXscard) break;

			TXscard->Open(O_WRONLY, current_TXsamplerate = 8000);
			TXsc_is_open = true;
		} catch (...) {
			reset_loop_msg = "OSS open failure";
			progdefaults.btnAudioIOis = SND_IDX_NULL; // file i/o
			sound_update(progdefaults.btnAudioIOis);
			REQ(show_reset_loop_alert);
		}
		break;
#endif
#if USE_PORTAUDIO
/// All of this very convoluted logic is needed to allow a Linux user
/// to switch from PulseAudio to PortAudio.  PulseAudio does not immediately
/// release the sound card resources after closing the pulse audio object.
	case SND_IDX_PORT:
	{
		RXscard = new SoundPort(scDevice[0].c_str(), scDevice[1].c_str());
		TXscard = new SoundPort(scDevice[0].c_str(), scDevice[1].c_str());
		unsigned long tm1 = zmsec();
		int RXret = 0, TXret = 0;
		int i;
		RXsc_is_open = false;
		TXsc_is_open = false;
		for (i = 0; i < 10; i++) { // try 10 times
			try {
				if (!RXret)
					RXret = RXscard->Open(O_RDONLY, current_RXsamplerate = 8000);
				if (progdefaults.is_full_duplex) {
					if (!TXret)
						TXret = TXscard->Open(O_WRONLY, current_TXsamplerate = 8000);
				}
				if (RXret) RXsc_is_open = true;
				if (TXret) TXsc_is_open = true;
				break;
			} catch (const SndException& e) {
				MilliSleep(50);
				Fl::awake();
			}
		}
		unsigned long tm = zmsec() - tm1;
		if (tm < 0) tm = 0;
		if (i == 10) {
			if (RXscard) delete RXscard;
			if (TXscard) delete TXscard;
			RXscard = 0;
			TXscard = 0;
			LOG_PERROR("Port Audio device not available");
			reset_loop_msg = "Port Audio device not available";
			progdefaults.btnAudioIOis = SND_IDX_NULL; // file i/o
			sound_update(progdefaults.btnAudioIOis);
			REQ(show_reset_loop_alert);
		} else {
			LOG_INFO ("Port Audio device available after %0.1f seconds", tm / 1000.0 );
		}
		break;
	}
#endif
#if USE_PULSEAUDIO
	case SND_IDX_PULSE:
		try {
			RXscard = new SoundPulse(scDevice[0].c_str());
			if (!RXscard) break;

			RXscard->Open(O_RDONLY, current_RXsamplerate = 8000);
			RXsc_is_open = true;

			TXscard = new SoundPulse(scDevice[0].c_str());
			if (!TXscard) break;
		
// needed to open playback device in PaVolumeControl
			TXscard->Open(O_WRONLY, current_TXsamplerate = 8000);
			double buffer[1024];
			for (int i = 0; i < 1024; buffer[i++] = 0);
			TXscard->Write_stereo(buffer, buffer, 1024);

			if (progdefaults.is_full_duplex)
				TXsc_is_open = true;
			else {
				TXscard->Close();
				TXsc_is_open = false;
			}
		} catch (const SndException& e) {
			LOG_ERROR("%s", e.what());
			if (RXscard) delete RXscard;
			if (TXscard) delete TXscard;
			RXscard = 0;
			TXscard = 0;
			reset_loop_msg = "Pulse Audio error:\n";
			reset_loop_msg.append(e.what());
			reset_loop_msg.append("\n\nIs the server running?\nClose fldigi and execute 'pulseaudio --start'");
			progdefaults.btnAudioIOis = SND_IDX_NULL; // file i/o
			sound_update(progdefaults.btnAudioIOis);
			REQ(show_reset_loop_alert);
		}
		break;
#endif
	case SND_IDX_NULL:
		RXscard = new SoundNull;
		TXscard = new SoundNull;
		current_RXsamplerate = current_TXsamplerate = 0;
		break;
	default:
		abort();
	}

	trx_state = STATE_RX;
}

//=============================================================================

void trx_reset(void)
{
	trx_state = STATE_RESTART;
}

//=============================================================================

void trx_start(void)
{
#if !BENCHMARK_MODE
	if (trxrunning) {
		LOG(debug::ERROR_LEVEL, debug::LOG_MODEM, "trx already running!");
		return;
	}

	if (RXscard) {
		delete RXscard;
		RXscard = 0;
	}
	if (TXscard) {
		delete TXscard;
		TXscard = 0;
	}
	if (ReedSolomon) delete ReedSolomon;
	if (dtmf) delete dtmf;


	switch (progdefaults.btnAudioIOis) {
#if USE_OSS
	case SND_IDX_OSS:
		RXscard = new SoundOSS(scDevice[0].c_str());
		TXscard = new SoundOSS(scDevice[0].c_str());
		break;
#endif
#if USE_PORTAUDIO
	case SND_IDX_PORT:
		RXscard = new SoundPort(scDevice[0].c_str(), scDevice[1].c_str());
		TXscard = new SoundPort(scDevice[0].c_str(), scDevice[1].c_str());
		break;
#endif
#if USE_PULSEAUDIO
	case SND_IDX_PULSE:
		try {
			RXscard = new SoundPulse(scDevice[0].c_str());
			if (!RXscard) break;

			TXscard = new SoundPulse(scDevice[0].c_str());
			if (!TXscard) break;

// needed to open playback device in PaVolumeControl
			TXscard->Open(O_WRONLY, current_TXsamplerate = 8000);
			double buffer[1024];
			for (int i = 0; i < 1024; buffer[i++] = 0);
			TXscard->Write_stereo(buffer, buffer, 1024);

			if (progdefaults.is_full_duplex)
				TXsc_is_open = true;
			else {
				TXscard->Close();
				TXsc_is_open = false;
			}
		} catch (const SndException& e) {
			LOG_ERROR("%s", e.what());
			if (RXscard) delete RXscard;
			if (TXscard) delete TXscard;
			RXscard = 0;
			TXscard = 0;
			reset_loop_msg = "Pulse Audio error:\n";
			reset_loop_msg.append(e.what());
			reset_loop_msg.append("\n\nIs the server running?");
			progdefaults.btnAudioIOis = SND_IDX_NULL; // file i/o
			sound_update(progdefaults.btnAudioIOis);
			REQ(show_reset_loop_alert);
		}
		break;
#endif
	case SND_IDX_NULL:
		RXscard = new SoundNull;
		TXscard = new SoundNull;
		break;
	default:
		abort();
	}
	current_RXsamplerate = current_TXsamplerate = 0;

	ReedSolomon = new cRsId;
	dtmf = new cDTMF;

#endif // !BENCHMARK_MODE

#if USE_NAMED_SEMAPHORES
	char sname[32];
	snprintf(sname, sizeof(sname), "trx-%u-%s", getpid(), PACKAGE_TARNAME);
	if ((trx_sem = sem_open(sname, O_CREAT | O_EXCL, 0600, 0)) == (sem_t*)SEM_FAILED) {
		LOG_PERROR("sem_open");
		abort();
	}
#  if HAVE_SEM_UNLINK
	if (sem_unlink(sname) == -1) {
		LOG_PERROR("sem_unlink");
		abort();
	}
#  endif
#else
	trx_sem = new sem_t;
	if (sem_init(trx_sem, 0, 0) == -1) {
		LOG_PERROR("sem_init");
		abort();
	}
#endif

	trx_state = STATE_RX;
	_trx_tune = 0;
	active_modem = 0;
	if (pthread_create(&trx_thread, NULL, trx_loop, NULL) < 0) {
		LOG(debug::ERROR_LEVEL, debug::LOG_MODEM, "pthread_create failed");
		trxrunning = false;
		exit(1);
	}
	trxrunning = true;
}

//=============================================================================
void trx_close()
{
	LOG_INFO("%s", "closing trx thread");
	int count = 1000;
	active_modem->set_stopflag(true);
	while (trx_state != STATE_RX && count--)
		MilliSleep(10);
	if (trx_state != STATE_RX) {
		LOG_INFO("%s", "trx_state != STATE_RX");
		exit(1);
	}
	count = 1000;
	trx_state = STATE_ABORT;
	while (trx_state != STATE_ENDED && count--)
		MilliSleep(10);
	if (trx_state != STATE_ENDED) {
		LOG_INFO("%s", "trx_state != STATE_ENDED");
		exit(2);
	}
#if USE_NAMED_SEMAPHORES
	if (sem_close(trx_sem) == -1)
		LOG_PERROR("sem_close");
#else
	if (sem_destroy(trx_sem) == -1)
		LOG_PERROR("sem_destroy");
	delete trx_sem;
#endif

	if (RXscard) {
		delete RXscard;
		RXscard = 0;
	}
	LOG_INFO("%s", "trx thread closed");
}

//=============================================================================
void trx_transmit_psm(void) { trx_state = STATE_TX; };

void trx_transmit(void) {
	if (progdefaults.show_psm_btn &&
		progStatus.kpsql_enabled &&
		(!PERFORM_CPS_TEST || !active_modem->XMLRPC_CPS_TEST))
		psm_transmit();
	else
		trx_state = STATE_TX;
}

void trx_tune(void) { trx_state = STATE_TUNE; }
void trx_receive(void) { trx_state = STATE_RX; }

//=============================================================================

void trx_wait_state(void)
{
	ENSURE_NOT_THREAD(TRX_TID);
	sem_wait(trx_sem);
}

static void trx_signal_state(void)
{
	ENSURE_THREAD(TRX_TID);
	sem_post(trx_sem);
}
