// ----------------------------------------------------------------------------
// trx.h
//
// Copyright (C) 2006-2009
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2009
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
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

#ifndef	TRX_H
#define	TRX_H

#include "threads.h"
#include "modem.h"
#include "sound.h"
#include "globals.h"
#include "rsid.h"

// ----------------------------------------------------------------------------

extern	void	trx_start_modem(modem* m, int f = 0);
extern	void	trx_start(void);
extern	void	trx_close();

extern	void	trx_transmit();
extern	void	trx_tune();
extern	void	trx_receive();

extern	void	trx_reset(void);

extern	void	trx_wait_state(void);

extern state_t		trx_state;
extern modem		*active_modem;
extern cRsId		*ReedSolomon;

extern	SoundBase 	*scard;

extern  bool bHistory;
extern  bool bHighSpeed;

#define TRX_WAIT(s_, code_)			\
	do {					\
		ENSURE_NOT_THREAD(TRX_TID);	\
		code_;				\
		while (trx_state != s_)		\
			trx_wait_state();	\
	} while (0)

void trx_xmit_wfall_queue(int samplerate, const double* buf, size_t len);

#endif
