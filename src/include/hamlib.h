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

#ifndef _HAMLIB_H
#define _HAMLIB_H

#include <hamlib/rig.h>

extern void hamlib_get_rigs(void);
extern size_t hamlib_get_index(rig_model_t model);
extern rig_model_t hamlib_get_rig_model(size_t i);
extern rig_model_t hamlib_get_rig_model_compat(const char* name);
extern void hamlib_get_rig_str(int (*func)(const char*));

extern void hamlib_close();
extern bool hamlib_init(bool bPtt);
extern void hamlib_set_ptt(int);
extern void hamlib_set_qsy(long long f);
extern int	hamlib_setfreq(long int);
extern int	hamlib_setmode(rmode_t);
extern rmode_t hamlib_getmode();
extern int	hamlib_setwidth(pbwidth_t);
extern pbwidth_t hamlib_getwidth();
extern void hamlib_get_defaults();
extern void hamlib_init_defaults();

#endif

