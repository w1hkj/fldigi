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

#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <string>
#include <sys/types.h>
#include "globals.h"

struct benchmark_params {
	trx_mode modem;
	int freq;
	bool afc, sql;
	double sqlevel;
	double src_ratio;
	int src_type;
	std::string input, output, buffer;
	size_t samples;
};
extern struct benchmark_params benchmark;

int setup_benchmark(void);
void do_benchmark(void);

#endif
