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

#include <fcntl.h>
#include <semaphore.h>
#include <cstdlib>
#include <string>

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

#if BENCHMARK_MODE
#  include "benchmark.h"
#endif

LOG_FILE_SOURCE(debug::LOG_MODEM);

using namespace std;

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
SoundBase 	*scard;
static int	_trx_tune;

// Ringbuffer for the audio "history". A pointer into this buffer
// is also passed to the waterfall signal drawing routines.
#define NUMMEMBUFS 1024
static ringbuffer<double> trxrb(ceil2(NUMMEMBUFS * SCBLOCKSIZE));
static float fbuf[SCBLOCKSIZE];
bool    bHistory = false;
bool    bHighSpeed = false;
static  double hsbuff[SCBLOCKSIZE];

static bool trxrunning = false;

#include "tune.cxx"

//=============================================================================

// Draws the xmit data one WFBLOCKSIZE-sized block at a time
static void trx_xmit_wfall_draw(int samplerate)
{
	ENSURE_THREAD(FLMAIN_TID);

	ringbuffer<double>::vector_type rv[2];
	rv[0].buf = 0;
	rv[1].buf = 0;

#define block_read_(vec_)						\
	while (vec_.len >= WFBLOCKSIZE) {				\
		wf->sig_data(vec_.buf, WFBLOCKSIZE, samplerate);	\
		vec_.len -= WFBLOCKSIZE;				\
		vec_.buf += WFBLOCKSIZE;				\
		trxrb.read_advance(WFBLOCKSIZE);				\
	}

	trxrb.get_rv(rv);
	block_read_(rv[0]); // read blocks from the first vector

	if (rv[0].len + rv[1].len < WFBLOCKSIZE)
		return;
	if (rv[0].len == 0)
		block_read_(rv[1]);
#undef block_read_

	// read non-contiguous data into tmp buffer so that we can
	// still draw it one block at a time
	if (unlikely(trxrb.read_space() >= WFBLOCKSIZE)) {
		double buf[WFBLOCKSIZE];
		do {
			trxrb.read(buf, WFBLOCKSIZE);
			wf->sig_data(buf, WFBLOCKSIZE, samplerate);
		} while (trxrb.read_space() >= WFBLOCKSIZE);
	}
}

// Called by trx_trx_transmit_loop() to handle data that may be left in the
// ringbuffer when we stop transmitting. Will pad with zeroes to a multiple of
// WFBLOCKSIZE.
static void trx_xmit_wfall_end(int samplerate)
{
	ENSURE_THREAD(TRX_TID);

	size_t pad = WFBLOCKSIZE - trxrb.read_space() % WFBLOCKSIZE;
	if (pad == WFBLOCKSIZE) // rb empty or multiple of WFBLOCKSIZE
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

	REQ(trx_xmit_wfall_draw, samplerate);
}

// Copy buf to the ringbuffer if it has enough space. Queue a waterfall
// request whenever there are at least WFBLOCKSIZE samples to draw.
void trx_xmit_wfall_queue(int samplerate, const double* buf, size_t len)
{
	ENSURE_THREAD(TRX_TID);
	ringbuffer<double>::vector_type wv[2];
	wv[0].buf = wv[1].buf = 0;

	trxrb.get_wv(wv, len);
	if (unlikely(wv[0].len + wv[1].len < len)) // not enough space
		return;

#define write_(vec_, len_)					\
	for (size_t i = 0; i < len_; i++)			\
		vec_[i] = buf[i] * progdefaults.TxMonitorLevel;

	size_t n = MIN(wv[0].len, len);
	write_(wv[0].buf, n);
	if (len > n) { // write the rest to the second vector
		buf += n;
		n = len - n;
		write_(wv[1].buf, n);
	}
#undef write_

	trxrb.write_advance(len);
	if (trxrb.read_space() >= WFBLOCKSIZE)
		REQ(trx_xmit_wfall_draw, samplerate);
}

//=============================================================================

void trx_trx_receive_loop()
{
	size_t  numread;
	int  current_samplerate;
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

	if (unlikely(!scard)) {
		MilliSleep(10);
		return;
	}

	try {
		current_samplerate = active_modem->get_samplerate();
		if (scard->Open(O_RDONLY, current_samplerate))
			REQ(sound_update, progdefaults.btnAudioIOis);
	}
	catch (const SndException& e) {
		LOG_ERROR("%s", e.what());
		put_status(e.what(), 5);
		scard->Close();
		if (e.error() == EBUSY && progdefaults.btnAudioIOis == SND_IDX_PORT) {
			sound_close();
			sound_init();
		}
		MilliSleep(1000);
		return;
	}
	active_modem->rx_init();

	ringbuffer<double>::vector_type rbvec[2];
	rbvec[0].buf = rbvec[1].buf = 0;

	while (1) {
		try {
			numread = 0;
			while (numread < SCBLOCKSIZE && trx_state == STATE_RX)
				numread += scard->Read(fbuf + numread, SCBLOCKSIZE - numread);
			if (bHighSpeed) {
				for (size_t i = 0; i < numread; i++)
					hsbuff[i] = fbuf[i];
			} else {
				if (trxrb.write_space() == 0) // discard some old data
					trxrb.read_advance(SCBLOCKSIZE);
				trxrb.get_wv(rbvec);
			// convert to double and write to rb
				for (size_t i = 0; i < numread; i++)
					rbvec[0].buf[i] = fbuf[i];
			}
		}
		catch (const SndException& e) {
			scard->Close();
			LOG_ERROR("%s", e.what());
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
			active_modem->HistoryON(true);
			active_modem->rx_process(hsbuff, numread);
			QRUNNER_DROP(false);
			progStatus.afconoff = afc;
			active_modem->HistoryON(false);
		} else {
			trxrb.write_advance(numread);
			REQ(&waterfall::sig_data, wf, rbvec[0].buf, numread, current_samplerate);

			if (!bHistory) {
				active_modem->rx_process(rbvec[0].buf, numread);
				if (progdefaults.rsid)
					ReedSolomon->receive(fbuf, numread);
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
	if (scard->must_close(O_RDONLY))
		scard->Close(O_RDONLY);
}


//=============================================================================
void trx_trx_transmit_loop()
{
	int  current_samplerate;
	if (!scard) {
		MilliSleep(10);
		return;
	}
	if (active_modem) {
		try {
			current_samplerate = active_modem->get_samplerate();
			scard->Open(O_WRONLY, current_samplerate);
		}
		catch (const SndException& e) {
			LOG_ERROR("%s", e.what());
			put_status(e.what(), 1);
			MilliSleep(10);
			return;
		}

		if (active_modem != ssb_modem && active_modem != anal_modem) {
			push2talk->set(true);
			REQ(&waterfall::set_XmtRcvBtn, wf, true);
		}
		active_modem->tx_init(scard);

		if (!progdefaults.DTMFstr.empty()) dtmf->send();

		if ( ReedSolomon->assigned(active_modem->get_mode()) && 
			 (progdefaults.TransmitRSid || progStatus.n_rsids != 0)) {
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
			while (trx_state == STATE_TX) {
				try {
					if (active_modem->tx_process() < 0)
						trx_state = STATE_RX;
				}
				catch (const SndException& e) {
					scard->Close();
					LOG_ERROR("%s", e.what());
					put_status(e.what(), 5);
					MilliSleep(10);
					return;
				}
			}
		} else
			trx_state = STATE_RX;

		if (ReedSolomon->assigned(active_modem->get_mode()) && 
			progdefaults.TransmitRSid &&
			progdefaults.rsid_post &&
			progStatus.n_rsids >= 0) ReedSolomon->send(false);

		progStatus.n_rsids = 0;

		trx_xmit_wfall_end(current_samplerate);

		scard->flush();
		if (scard->must_close(O_WRONLY))
			scard->Close(O_WRONLY);

	} else
		MilliSleep(10);

	push2talk->set(false);
	REQ(&waterfall::set_XmtRcvBtn, wf, false);
	if (progStatus.timer)
		REQ(startMacroTimer);
}

//=============================================================================
void trx_tune_loop()
{
	int  current_samplerate;
	if (!scard) {
		MilliSleep(10);
		return;
	}
	if (active_modem) {
		try {
			current_samplerate = active_modem->get_samplerate();
			scard->Open(O_WRONLY, current_samplerate);
		}
		catch (const SndException& e) {
			LOG_ERROR("%s", e.what());
			put_status(e.what(), 1);
			MilliSleep(10);
			return;
		}

		push2talk->set(true);
		active_modem->tx_init(scard);

		try {
			while (trx_state == STATE_TUNE) {
				if (_trx_tune == 0) {
					REQ(&waterfall::set_XmtRcvBtn, wf, true);
					xmttune::keydown(active_modem->get_txfreq_woffset(), scard);
					_trx_tune = 1;
				} else
					xmttune::tune(active_modem->get_txfreq_woffset(), scard);
			}
			xmttune::keyup(active_modem->get_txfreq_woffset(), scard);
		}
		catch (const SndException& e) {
			scard->Close();
			LOG_ERROR("%s", e.what());
			put_status(e.what(), 5);
			MilliSleep(10);
			return;
		}
		scard->flush();
		if (scard->must_close(O_WRONLY))
			scard->Close(O_WRONLY);

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
			delete scard;
			scard = 0;
			trx_state = STATE_ENDED;
			// fall through
		case STATE_ENDED:
			return 0;
		case STATE_RESTART:
			trx_reset_loop();
			break;
		case STATE_NEW_MODEM:
			trx_start_modem_loop();
			break;
		case STATE_TX:
			trx_trx_transmit_loop();
			break;
		case STATE_TUNE:
			trx_tune_loop();
			break;
		case STATE_RX:
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
		if (new_freq > 0)
			active_modem->set_freq(new_freq);
		active_modem->restart();
		trx_state = STATE_RX;
		return;
	}

	modem* old_modem = active_modem;

	new_modem->init();
	active_modem = new_modem;
	if (new_freq > 0)
		active_modem->set_freq(new_freq);
	trx_state = STATE_RX;
	REQ(&waterfall::opmode, wf);

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
void trx_reset_loop()
{
	if (scard)  {
		delete scard;
		scard = 0;
	}

	switch (progdefaults.btnAudioIOis) {
#if USE_OSS
	case SND_IDX_OSS:
		scard = new SoundOSS(scDevice[0].c_str());
		break;
#endif
#if USE_PORTAUDIO
	case SND_IDX_PORT:
	    scard = new SoundPort(scDevice[0].c_str(), scDevice[1].c_str());
		break;
#endif
#if USE_PULSEAUDIO
	case SND_IDX_PULSE:
		scard = new SoundPulse(scDevice[0].c_str());
		break;
#endif
	case SND_IDX_NULL:
		scard = new SoundNull;
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
	
	if (scard) delete scard;
	if (ReedSolomon) delete ReedSolomon;
	if (dtmf) delete dtmf;


	switch (progdefaults.btnAudioIOis) {
#if USE_OSS
	case SND_IDX_OSS:
		scard = new SoundOSS(scDevice[0].c_str());
		break;
#endif
#if USE_PORTAUDIO
	case SND_IDX_PORT:
		scard = new SoundPort(scDevice[0].c_str(), scDevice[1].c_str());
		break;
#endif
#if USE_PULSEAUDIO
	case SND_IDX_PULSE:
		scard = new SoundPulse(scDevice[0].c_str());
		break;
#endif
	case SND_IDX_NULL:
		scard = new SoundNull;
		break;
	default:
		abort();
	}

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
	trx_state = STATE_ABORT;
	while (trx_state != STATE_ENDED)
		MilliSleep(100);

#if USE_NAMED_SEMAPHORES
	if (sem_close(trx_sem) == -1)
		LOG_PERROR("sem_close");
#else
	if (sem_destroy(trx_sem) == -1)
		LOG_PERROR("sem_destroy");
	delete trx_sem;
#endif

	if (scard) {
		delete scard;
		scard = 0;
	}
}

//=============================================================================

void trx_transmit(void) { trx_state = STATE_TX; }
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
