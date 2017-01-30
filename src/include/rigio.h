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

#ifndef RIGIO_H
#define RIGIO_H

#include <string>

#include "serial.h"

extern Cserial rigio;

extern bool hexout(const std::string&);

extern bool sendCommand(std::string, int retnbr, int waitval);

extern long long rigCAT_getfreq(int retries, bool &failed, int multiplier = 1);
extern void rigCAT_setfreq(long long);

extern std::string rigCAT_getmode();
extern void rigCAT_setmode(const std::string&);

extern std::string rigCAT_getwidth();
extern void rigCAT_setwidth(const std::string&);

extern void rigCAT_get_notch();
extern void rigCAT_set_notch(int val);
extern void rigCAT_get_smeter();
extern void rigCAT_get_pwrmeter();
extern void rigCAT_set_pwrlevel(int);
extern void rigCAT_get_pwrlevel();

extern void rigCAT_close();
extern bool rigCAT_init();
extern bool rigCAT_active();
extern void rigCAT_sendINIT(const std::string& icmd, int multiplier = 1);

extern void rigCAT_set_ptt(int);
extern void rigCAT_set_qsy(long long f);

extern void rigCAT_defaults();
//extern void rigCAT_restore_defaults();

// no xcvr 

extern long long noCAT_getfreq();
extern void noCAT_setfreq(long long f);
extern std::string noCAT_getmode();
extern void noCAT_setmode(const std::string &md);
extern std::string noCAT_getwidth();
extern void noCAT_setwidth(const std::string &w);
extern void noCAT_setPTT(bool val);
extern void noCAT_init();

#endif

