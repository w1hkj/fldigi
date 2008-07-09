// ----------------------------------------------------------------------------
// trx.h
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
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

#ifndef	TRX_H
#define	TRX_H

#include "threads.h"
#include "modem.h"

#include "misc.h"
#include "sound.h"
#include "waterfall.h"
#include "globals.h"
#include "fl_digi.h"
#include "rsid.h"

// ----------------------------------------------------------------------------

extern	void	trx_start_modem(modem *);
extern	void	trx_start(void);
extern	void	trx_close();

extern	void	trx_transmit();
extern	void	trx_tune();
extern	void	trx_receive();

extern	void	trx_reset(void);
extern	void	trx_start_macro_timer();

extern	void	wait_modem_ready_prep(void);
extern	void	wait_modem_ready_cmpl(void);
extern	void	signal_modem_ready(void);

extern Fl_Thread	trx_thread;
extern state_t		trx_state;
extern modem		*active_modem;
extern cRsId		*ReedSolomon;

extern	SoundBase 	*scard;

extern  bool bHistory;

#endif
