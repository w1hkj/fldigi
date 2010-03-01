// ----------------------------------------------------------------------------
//      benchmark.cxx
//
// Copyright (C) 2009
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

#include <config.h>

#include <fstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <inttypes.h>
#include <sys/time.h>
#include <time.h>

#ifndef __MINGW32__
#  include <sys/resource.h>
#else
#  include "compat.h"
#endif

#if USE_SNDFILE
#  include <sndfile.h>
#endif

#include "fl_digi.h"
#include "modem.h"
#include "trx.h"
#include "timeops.h"
#include "configuration.h"
#include "status.h"
#include "debug.h"

#include "benchmark.h"

using namespace std;

struct benchmark_params benchmark = { MODE_PSK31, 1000, false, false, 0.0, 1.0, SRC_SINC_FASTEST };


int setup_benchmark(void)
{
	ENSURE_THREAD(FLMAIN_TID);

	if (benchmark.input.empty()) {
		LOG_ERROR("Missing input");
		return 1;
	}
	else {
		char* p;
		benchmark.samples = (size_t)strtol(benchmark.input.c_str(), &p, 10);
		if (*p != '\0') { // invalid char in input string
#if USE_SNDFILE
			// treat as filename
			benchmark.samples = 0;
#else
			LOG_ERROR("Bad input string, \"%s\"", benchmark.input.c_str());
			return 1;
#endif
		}
	}
	if (!benchmark.output.empty())
		benchmark.buffer.reserve(BUFSIZ);

	progdefaults.rsid = false;
	progdefaults.StartAtSweetSpot = false;

	if (benchmark.modem != NUM_MODES)
		progStatus.lastmode = benchmark.modem;
	if (benchmark.freq)
		progStatus.carrier = benchmark.freq;
	progStatus.afconoff = benchmark.afc;
	progStatus.sqlonoff = benchmark.sql;
	progStatus.sldrSquelchValue = benchmark.sqlevel;

	debug::level = debug::INFO_LEVEL;
	TRX_WAIT(STATE_ENDED, trx_start(); init_modem(progStatus.lastmode));
	if (!benchmark.output.empty()) {
		ofstream out(benchmark.output.c_str());
		if (out)
			out << benchmark.buffer;
	}

	return 0;
}

#if USE_SNDFILE
SNDFILE* infile = 0;
#endif

static size_t do_rx(struct rusage ru[2], struct timespec wall_time[2]);
static size_t do_rx_src(struct rusage ru[2], struct timespec wall_time[2]);

void do_benchmark(void)
{
	ENSURE_THREAD(TRX_TID);

	if (benchmark.src_ratio != 1.0)
		LOG_INFO("modem=%" PRIdPTR " (%s) rate=%d ratio=%f converter=%d (\"%s\")",
			 active_modem->get_mode(), mode_info[active_modem->get_mode()].sname,
			 active_modem->get_samplerate(),
			 benchmark.src_ratio, benchmark.src_type, src_get_name(benchmark.src_type));
	else
		LOG_INFO("modem=%" PRIdPTR " (%s) rate=%d", active_modem->get_mode(),
			 mode_info[active_modem->get_mode()].sname, active_modem->get_samplerate());

#if USE_SNDFILE
	if (!benchmark.samples) {
		SF_INFO info = { 0, 0, 0, 0, 0, 0 };
		if ((infile = sf_open(benchmark.input.c_str(), SFM_READ, &info)) == NULL) {
			LOG_ERROR("Could not open input file \"%s\"", benchmark.input.c_str());
			return;
		}
	}
#endif

	struct rusage ru[2];
	struct timespec wall_time[2];
	size_t nproc, nrx;
	if (benchmark.src_ratio == 1.0)
		nrx = nproc = do_rx(ru, wall_time);
	else {
		nproc = do_rx_src(ru, wall_time);
		nrx = (size_t)(nproc * benchmark.src_ratio);
	}
	ru[1].ru_utime -= ru[0].ru_utime;
	wall_time[1] -= wall_time[0];

#if USE_SNDFILE
	if (infile) {
		sf_close(infile);
		infile = 0;
	}
#endif

	LOG_INFO("processed: %" PRIuSZ " samples (decoded %" PRIuSZ ") in %.3f seconds", nproc, nrx,
		 wall_time[1].tv_sec + wall_time[1].tv_nsec / 1e9);
	double speed = nproc / (ru[1].ru_utime.tv_sec + ru[1].ru_utime.tv_usec / 1e6);
	LOG_INFO("cpu time : %" PRIdMAX ".%03" PRIdMAX "; speed=%.3f samples/s; factor=%.3f",
		 (intmax_t)ru[1].ru_utime.tv_sec, (intmax_t)ru[1].ru_utime.tv_usec / 1000,
		 speed, speed / active_modem->get_samplerate());
}

// ----------------------------------------------------------------------------

static size_t do_rx(struct rusage ru[2], struct timespec wall_time[2])
{
	size_t nread;
	size_t inlen = 1 << 19;
	double* inbuf = new double[inlen];

#if USE_SNDFILE
	if (infile) {
		nread = 0;
		clock_gettime(CLOCK_MONOTONIC, &wall_time[0]);
		getrusage(RUSAGE_SELF, &ru[0]);

		for (size_t n; (n = sf_readf_double(infile, inbuf, inlen)); nread += n)
			active_modem->rx_process(inbuf, n);
	}
	else
#endif
	{
		memset(inbuf, 0, sizeof(double) * inlen);
		clock_gettime(CLOCK_MONOTONIC, &wall_time[0]);
		getrusage(RUSAGE_SELF, &ru[0]);

		for (nread = benchmark.samples; nread > inlen; nread -= inlen)
			active_modem->rx_process(inbuf, inlen);
		if (nread)
			active_modem->rx_process(inbuf, nread);
		nread = benchmark.samples;
	}

	getrusage(RUSAGE_SELF, &ru[1]);
	clock_gettime(CLOCK_MONOTONIC, &wall_time[1]);

	delete [] inbuf;
	return nread;
}


size_t inlen = 1 << 19;
static float* inbuf = 0;
static long src_read(void* arg, float** data)
{
	*data = inbuf;
	return inlen;
}
#if USE_SNDFILE
static long src_readf(void* arg, float** data)
{
	long n = (long)sf_readf_float(infile, inbuf, inlen);
	*data = n ? inbuf : 0;
	return n;
}
#endif

static size_t do_rx_src(struct rusage ru[2], struct timespec wall_time[2])
{
	int err;
	SRC_STATE* src_state;

#if USE_SNDFILE
	if (infile)
		src_state = src_callback_new(src_readf, benchmark.src_type, 1, &err, NULL);
	else
#endif
		src_state = src_callback_new(src_read, benchmark.src_type, 1, &err, NULL);

	if (!src_state) {
		LOG_ERROR("src_callback_new error %d: %s", err, src_strerror(err));
		return 0;
	}

	inbuf = new float[inlen];
	size_t outlen = (size_t)floor(inlen * benchmark.src_ratio);
	float* outbuf = new float[outlen];
	double* rxbuf = new double[outlen];

	long n;
	size_t nread;
#if USE_SNDFILE
	if (infile) { // read until src returns 0
		nread = 0;
		clock_gettime(CLOCK_MONOTONIC, &wall_time[0]);
		getrusage(RUSAGE_SELF, &ru[0]);

		while ((n = src_callback_read(src_state, benchmark.src_ratio, outlen, outbuf))) {
			for (long i = 0; i < n; i++)
				rxbuf[i] = outbuf[i];
			active_modem->rx_process(rxbuf, n);
			nread += n;
		}

		nread = (size_t)round(nread * benchmark.src_ratio);
	}
	else
#endif
	{ // read benchmark.samples * benchmark.src_ratio
		nread = (size_t)round(benchmark.samples * benchmark.src_ratio);
		clock_gettime(CLOCK_MONOTONIC, &wall_time[0]);
		getrusage(RUSAGE_SELF, &ru[0]);

		while (nread > outlen) {
			if ((n = src_callback_read(src_state, benchmark.src_ratio, outlen, outbuf)) == 0)
				break;
			for (long i = 0; i < n; i++)
				rxbuf[i] = outbuf[i];
			active_modem->rx_process(rxbuf, n);
			nread -= (size_t)n;
		}
		if (nread) {
			if ((n = src_callback_read(src_state, benchmark.src_ratio, nread, outbuf))) {
				for (long i = 0; i < n; i++)
					rxbuf[i] = outbuf[i];
				active_modem->rx_process(rxbuf, n);
			}
		}
		nread = benchmark.samples;
	}

	getrusage(RUSAGE_SELF, &ru[1]);
	clock_gettime(CLOCK_MONOTONIC, &wall_time[1]);

	delete [] inbuf;
	delete [] outbuf;
	delete [] rxbuf;

	return nread;
}
