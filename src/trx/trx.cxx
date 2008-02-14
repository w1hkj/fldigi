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

#include <string>

#include "trx.h"
#include "main.h"
#include "fl_digi.h"
#include "ascii.h"
//#include "rigio.h"
//#include "rigMEM.h"
#include "misc.h"
//#include "modeIO.h"
#include "configuration.h"
#include "macros.h"

#include <FL/Fl.H>

#include "ringbuffer.h"
#include "qrunner.h"

using namespace std;

void	trx_reset_loop();
void	trx_start_modem_loop();
void	trx_receive_loop();
void	trx_transmit_loop();
void	trx_tune_loop();

//#define DEBUG

/* ---------------------------------------------------------------------- */


Fl_Mutex	trx_mutex = PTHREAD_MUTEX_INITIALIZER;
Fl_Mutex	trx_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
Fl_Cond		trx_cond = PTHREAD_COND_INITIALIZER;
Fl_Thread	trx_thread;
state_t 	trx_state;
bool		restartOK = false;
bool		trx_wait = false;

modem		*active_modem = 0;
SoundBase 		*scard;

int			_trx_tune;

// Ringbuffer for the audio "history". A pointer into this buffer
// is also passed to the waterfall signal drawing routines.
ringbuffer<double> trxrb(ceil2(1024 * SCBLOCKSIZE));
// Vector used for direct access to the ringbuffer
ringbuffer<double>::vector_type rbvec[2];
bool    bHistory = false;

static int dummy = 0;
static bool trxrunning = false;
#include "tune.cxx"

/* ---------------------------------------------------------------------- */

void trx_trx_receive_loop()
{
	int  numread;
	assert(powerof2(SCBLOCKSIZE));

	if (!scard) {
		MilliSleep(10);
		return;
	}
	if (active_modem) {
		try {
			scard->Open(O_RDONLY, active_modem->get_samplerate());
		}
		catch (const SndException& e) {
			put_status(e.what(), 5);
#if USE_PORTAUDIO
			if (e.error() == EBUSY && progdefaults.btnAudioIOis == 1) {
				SoundPort::terminate();
				SoundPort::initialize();
			}
#endif
			MilliSleep(1000);
			return;
		}
		active_modem->rx_init();

		while (1) {
			try {
				if (trxrb.write_space() == 0) // discard some old data
					trxrb.read_advance(SCBLOCKSIZE);
				trxrb.get_wv(rbvec);
				numread = scard->Read(rbvec[0].buf, SCBLOCKSIZE);
			}
			catch (const SndException& e) {
				scard->Close();
				put_status(e.what(), 5);
				MilliSleep(10);
				return;
			}
			if (numread == -1 || (trx_state != STATE_RX))
				break;
			else if (numread == 0) // overflow
				continue;

			trxrb.write_advance(SCBLOCKSIZE);
			REQ(&waterfall::sig_data, wf, rbvec[0].buf, numread);

			if (!bHistory)
				active_modem->rx_process(rbvec[0].buf, numread);
			else {
				bool afc = active_modem->get_afcOnOff();
				active_modem->set_afcOnOff(0);
				trxrb.get_rv(rbvec);
				if (rbvec[0].len)
					active_modem->rx_process(rbvec[0].buf, rbvec[0].len);
				if (rbvec[1].len)
					active_modem->rx_process(rbvec[1].buf, rbvec[1].len);
				active_modem->set_afcOnOff(afc);
				bHistory = false;
			}
		}
		if (!scard->full_duplex())
			scard->Close();
	} else
		MilliSleep(10);
}

void trx_trx_transmit_loop()
{
	if (!scard) {
		MilliSleep(10);
		return;
	}

	if (active_modem) {
		try {
			scard->Open(O_WRONLY, active_modem->get_samplerate());
		}
		catch (const SndException& e) {
			put_status(e.what(), 1);
			MilliSleep(10);
			return;
		}

		push2talk->set(true);
		active_modem->tx_init(scard);

		while (trx_state == STATE_TX) {
			try {
				if (active_modem->tx_process() < 0)
					trx_state = STATE_RX;
			}
			catch (const SndException& e) {
				scard->Close();
				put_status(e.what(), 5);
				MilliSleep(10);
				return;
			}
		}
		if (!scard->full_duplex())
			scard->Close();
	} else
		MilliSleep(10);

	push2talk->set(false);
	REQ_SYNC(&waterfall::set_XmtRcvBtn, wf, false);

	if (progdefaults.useTimer == true) {
		trx_start_macro_timer();
	}
}

void trx_tune_loop()
{
	if (!scard) {
		MilliSleep(10);
		return;
	}
	if (active_modem) {
		try {
			scard->Open(O_WRONLY, active_modem->get_samplerate());
		}
		catch (const SndException& e) {
			put_status(e.what(), 1);
			MilliSleep(10);
			return;
		}

		push2talk->set(true);
		active_modem->tx_init(scard);

		try {
			while (trx_state == STATE_TUNE) {
				if (_trx_tune == 0) {
					REQ_SYNC(&waterfall::set_XmtRcvBtn, wf, true);
					xmttune::keydown(active_modem->get_txfreq_woffset(), scard);
					_trx_tune = 1;
				} else
					xmttune::tune(active_modem->get_txfreq_woffset(), scard);
			}
			xmttune::keyup(active_modem->get_txfreq_woffset(), scard);
		}
		catch (const SndException& e) {
			scard->Close();
			put_status(e.what(), 5);
			MilliSleep(10);
			return;
		}
		if (!scard->full_duplex())
			scard->Close();
		_trx_tune = 0;
	} else
		MilliSleep(10);

	push2talk->set(false);
	REQ_SYNC(&waterfall::set_XmtRcvBtn, wf, false);
}

void *trx_loop(void *args)
{
	SET_THREAD_ID(TRX_TID);

	for (;;) {
		if ( trx_state == STATE_ABORT) {
			trx_state = STATE_ENDED;
			return 0;
		}
		else if (trx_state == STATE_RESTART) {
			trx_reset_loop();
			MilliSleep(10);
		}
		else if (trx_state == STATE_NEW_MODEM) {
			trx_start_modem_loop();
			MilliSleep(10);
		} 
		else if (trx_state == STATE_TX)
			trx_trx_transmit_loop();
		else if (trx_state == STATE_TUNE)
			trx_tune_loop();
		else if (trx_state == STATE_RX)
			trx_trx_receive_loop();
		else
			MilliSleep(100);
	}
	trx_state = STATE_ENDED;
	return 0;
}

modem *trx_m;

void trx_start_modem_loop()
{
	if (active_modem)
		active_modem->shutdown();

	active_modem = trx_m;
	active_modem->init();
	trx_state = STATE_RX;
	signal_modem_ready();
	REQ(&waterfall::opmode, wf);
}

void trx_start_modem(modem *m)
{
	trx_m = m;
	trx_state = STATE_NEW_MODEM;
}

string trx_scdev;

void trx_reset_loop()
{
	if (scard)  {
		delete scard;
		scard = 0;
	}
#if USE_PORTAUDIO && USE_OSS
	if (progdefaults.btnAudioIOis == 1)
		scard = new SoundPort(trx_scdev.c_str());
	else
		scard = new SoundOSS(trx_scdev.c_str());
#else
#  if USE_PORTAUDIO
	scard = new SoundPort(trx_scdev.c_str());
#  endif
#  if USE_OSS
	scard = new SoundOSS(trx_scdev.c_str());
#  endif
#endif
	trx_state = STATE_RX;	
}

void trx_reset(const char *scdev)
{
	trx_scdev = scdev;
	trx_state = STATE_RESTART;
}

static char timermsg[80];
static void macro_timer(void *)
{
	if (progdefaults.useTimer == false)
		return;
	progdefaults.timeout--;
	if (progdefaults.timeout == 0) {
		progdefaults.useTimer = false;
		macros.execute(progdefaults.macronumber);
		FL_LOCK();
		btnMacroTimer->hide();
		FL_UNLOCK();
	} else {
		snprintf(timermsg, sizeof(timermsg), "Timer: %d", progdefaults.timeout);
		FL_LOCK();
		btnMacroTimer->label(timermsg);
		btnMacroTimer->redraw_label();
		FL_UNLOCK();
		Fl::repeat_timeout(1.0, macro_timer);
	}
}

void trx_start_macro_timer()
{
	Fl::add_timeout(1.0, macro_timer);
	snprintf(timermsg, sizeof(timermsg), "Timer: %d", progdefaults.timeout);
	FL_LOCK();
	btnMacroTimer->label(timermsg);
	btnMacroTimer->redraw_label();
	btnMacroTimer->show();
	FL_UNLOCK();
}

void trx_start(const char *scdev)
{
	if (trxrunning) {
		std::cout<< "trx already running!\n"; fflush(stdout);
		return;
	}
	
	if (scard) delete scard;
#if USE_PORTAUDIO && USE_OSS
	if (progdefaults.btnAudioIOis == 1)
		scard = new SoundPort(scdev);
	else
		scard = new SoundOSS(scdev);
#else
#  if USE_PORTAUDIO
	scard = new SoundPort(scdev);
#  endif
#  if USE_OSS
	scard = new SoundOSS(scdev);
#  endif
#endif

	trx_state = STATE_RX;
	_trx_tune = 0;
	active_modem = 0;
	if (fl_create_thread(trx_thread, trx_loop, &dummy) < 0) {
		std::cout <<  "trx pthread_create:" << std::endl; fflush(stdout);
		trxrunning = false;
		exit(1);
	} 
	trxrunning = true;
}

void trx_close() {
	trx_state = STATE_ABORT;
	do {
		MilliSleep(100);
	} while (trx_state != STATE_ENDED);
	if (scard) {
		delete scard;
		scard = 0;
	}
}

//---------------------------------------------------------------------

void wait_modem_ready_prep(void)
{
#ifndef NDEBUG
        if (GET_THREAD_ID() == TRX_TID)
                cerr << "trx thread called wait_modem_ready_prep!\n";
#endif

        fl_lock(&trx_cond_mutex);
}

void wait_modem_ready_cmpl(void)
{
#ifndef NDEBUG
        if (GET_THREAD_ID() == TRX_TID)
                cerr << "trx thread called wait_modem_ready_cmpl!\n";
#endif

        fl_cond_wait(&trx_cond, &trx_cond_mutex);
        fl_unlock(&trx_cond_mutex);
}


void signal_modem_ready(void)
{
#ifndef NDEBUG
        if (GET_THREAD_ID() != TRX_TID)
                cerr << "thread " << GET_THREAD_ID()
                     << " called signal_modem_ready!\n";
#endif

        fl_lock(&trx_cond_mutex);
        fl_cond_bcast(&trx_cond);
        fl_unlock(&trx_cond_mutex);
}
