// ----------------------------------------------------------------------------
// psm/psm.h
//
// Support for Signal Montoring, CSMA, Transmit Inhibit (Busy Detection)
// Effect all transmission types, Keybord, ARQ and KISS interface.
//
// Copyright (c) 2016
//		Robert Stiles, KK5VD
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

#ifndef __psm_h__
#define __psm_h__

enum {
	CSMA_PERSISTANCE = 0x01,
	CSMA_SLOT_TIME   = 0x02,
	CSMA_TX_DELAY    = 0x04,
	CSMA_ALL         = (CSMA_PERSISTANCE|CSMA_SLOT_TIME|CSMA_TX_DELAY)
};

enum {
	PSM_STOP = 1,
	PSM_ABORT
};

extern bool psm_thread_running;

extern void psm_reset_histogram(void);
extern void psm_transmit(void);
extern void psm_transmit_ended(int flag);
extern void start_psm_thread(void);
extern void stop_psm_thread(void);
extern void signal_psm(void);
extern void update_kpsql_fractional_gain(int value);
extern void update_csma_io_config(int update_this);

#endif /* __psm_h__ */
