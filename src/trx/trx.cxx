// ----------------------------------------------------------------------------
// trx.cxx  --  Main transmit/receive control loop / thread
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted in part from code contained in gmfsk 
// source code distribution.
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

#include "soundconf.h"
#include "ringbuffer.h"
#include "qrunner.h"
#include "debug.h"

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
SoundBase 	*scard;
static int	_trx_tune;

// Ringbuffer for the audio "history". A pointer into this buffer
// is also passed to the waterfall signal drawing routines.
#define NUMMEMBUFS 1024
ringbuffer<double> trxrb(ceil2(NUMMEMBUFS * SCBLOCKSIZE));
// Vector used for direct access to the ringbuffer
ringbuffer<double>::vector_type rbvec[2];
bool    bHistory = false;

static bool trxrunning = false;

static bool rsid_detecting = false;

#include "tune.cxx"

//=============================================================================
void trx_trx_receive_loop()
{
	int  numread;
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

	while (1) {
		if (progdefaults.rsid == true && rsid_detecting == false) {
			rsid_detecting = true;
			try {
				current_samplerate = ReedSolomon->samplerate();
				scard->Open(O_RDONLY, current_samplerate);
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
		}
		if (progdefaults.rsid == false && rsid_detecting == true) {
			rsid_detecting = false;
			active_modem->rx_init();
			try {
				current_samplerate = active_modem->get_samplerate();
				scard->Open(O_RDONLY, current_samplerate);
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
		}
		// If we change to an 8000Hz modem while RSID is on we'll never detect anything.
		// Toggle rsid_detecting so that the audio device is reopened with the ReedSolomon
		// samplerate in the next loop iteration.
		if (progdefaults.rsid && rsid_detecting && current_samplerate != ReedSolomon->samplerate())
			rsid_detecting = false;

		try {
			if (trxrb.write_space() == 0) // discard some old data
				trxrb.read_advance(SCBLOCKSIZE);
			trxrb.get_wv(rbvec);
			numread = 0;
			while (numread < SCBLOCKSIZE && trx_state == STATE_RX)
				numread += scard->Read(rbvec[0].buf + numread, SCBLOCKSIZE - numread);
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

		trxrb.write_advance(numread);
		REQ(&waterfall::sig_data, wf, rbvec[0].buf, numread, current_samplerate);

		if (!bHistory) {
			if (rsid_detecting == true)
				ReedSolomon->search(rbvec[0].buf, numread);
			else
				active_modem->rx_process(rbvec[0].buf, numread);
		}
		else {
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

		push2talk->set(true);
		active_modem->tx_init(scard);

		if (progdefaults.TransmitRSid == true)
			ReedSolomon->send(true);
		
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

		if (progdefaults.TransmitRSid == true)
			ReedSolomon->send(false);
		
        scard->flush();
		if (scard->must_close())
			scard->Close();
	} else
		MilliSleep(10);

	push2talk->set(false);
	REQ(&waterfall::set_XmtRcvBtn, wf, false);
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
		if (scard->must_close())
			scard->Close();
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
			trx_signal_state();
		}
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
			if (progStatus.timer)
				REQ(startMacroTimer);
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
modem* new_modem;

void trx_start_modem_loop()
{
	if (new_modem == active_modem) {
		active_modem->restart();
		trx_state = STATE_RX;
		return;
	}

	modem* old_modem = active_modem;

	new_modem->init();
	active_modem = new_modem;
	trx_state = STATE_RX;
	REQ(&waterfall::opmode, wf);

	if (old_modem) {
		*mode_info[old_modem->get_mode()].modem = 0;
		delete old_modem;
	}
}

//=============================================================================
void trx_start_modem(modem *m)
{
	new_modem = m;
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
