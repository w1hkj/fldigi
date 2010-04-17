// ----------------------------------------------------------------------------
//      spot.h
//
// Copyright (C) 2008-2009
//              Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
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

#ifndef SPOT_H_
#define SPOT_H_

#if HAVE_REGEX_H
#  include <regex.h>
#else
#  include "compat/regex.h"
#endif
#include <sys/time.h>
#include "globals.h"

typedef void (*spot_recv_cb_t)(trx_mode mode, int afreq, const char* str, const regmatch_t* sub, size_t len, void* data);
typedef void (*spot_log_cb_t)(const char* call, const char* loc, long long freq,
			      trx_mode mode, time_t rtime, void* data);

void spot_recv(char c, int decoder = -1, int afreq = 0, int md = 0);
void spot_log(const char* callsign, const char* locator = "", long long freq = 0LL,
	      trx_mode mode = NUM_MODES, time_t rtime = -1L);
void spot_manual(const char* callsign, const char* locator = "",
		 long long freq = 0LL, trx_mode mode = NUM_MODES, time_t rtime = -1L);

void spot_register_log(spot_log_cb_t lcb, void* ldata);
void spot_register_manual(spot_log_cb_t mcb, void* mdata);
void spot_register_recv(spot_recv_cb_t rcb, void* rdata, const char* re, int reflags);

void spot_unregister_log(spot_log_cb_t lcb, const void* ldata);
void spot_unregister_manual(spot_log_cb_t mcb, const void* mdata);
void spot_unregister_recv(spot_recv_cb_t rcb, const void* rdata);

#endif // SPOT_H_
