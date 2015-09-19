// ----------------------------------------------------------------------------
// Copyright (C) 2014 David Freese, W1HKJ
// Copyright (C) 2015 Robert Stiles, KK5VD
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

#ifndef SOUNDCONF_H
#define SOUNDCONF_H

enum { SND_IDX_UNKNOWN = -1, SND_IDX_OSS, SND_IDX_PORT,
       SND_IDX_PULSE, SND_IDX_NULL, SND_IDX_END
};

enum {
	FLDIGI_SRC_BEST,
	FLDIGI_SRC_MEDIUM,
	FLDIGI_SRC_FASTEST,
#if !(defined(__ppc__) || defined(__powerpc__) || defined(__PPC__))
	FLDIGI_SRC_LINEAR,
#endif
	FLDIGI_NUM_SRC
};

enum {
	PA_DEV_NOT_FOUND = 0,
	PA_DEV_FOUND,
	PA_EXACT_DEV_FOUND
};

extern int sample_rate_converters[FLDIGI_NUM_SRC];

void sound_init(void);
void sound_close(void);
void sound_update(unsigned idx);

#include <string>

class Fl_Choice;
int pa_set_dev(Fl_Choice *choice, std::string dev_name, int dev_index);
extern std::string str_pa_devices;

#endif // SOUNDCONF_H
