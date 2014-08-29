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

#ifndef FL_LOGBOOK_H
#define FL_LOGBOOK_H


#include <cstring>

#include "lgbook.h"
#include "logsupport.h"
#include "threads.h"

extern std::string log_checksum;

extern pthread_t logbook_thread;
extern pthread_mutex_t logbook_mutex;

extern void start_logbook();
extern void close_logbook();


#endif
