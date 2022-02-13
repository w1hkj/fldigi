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

#ifndef NOTIFY_H_
#define NOTIFY_H_

#include "globals.h"

extern void notify_start(void);
extern void notify_stop(void);
extern void notify_show(void);
extern void notify_dxcc_show(bool readonly = true);
extern void notify_change_callsign(void);
extern void notify_rsid(trx_mode mode, int afreq);
extern void notify_rsid_eot(trx_mode mode, int afreq);
extern void notify_create_rsid_event(bool val);

#endif // NOTIFY_H_
