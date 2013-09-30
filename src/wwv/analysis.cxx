// ----------------------------------------------------------------------------
// anal.cxx  --  anal modem
//
// Copyright (C) 2006-2009
//		Dave Freese, W1HKJ
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

#include <config.h>

#include <string>
#include <cstdio>
#include <ctime>

#include "analysis.h"
#include "modem.h"
#include "misc.h"
#include "filters.h"
#include "fftfilt.h"
#include "digiscope.h"
#include "waterfall.h"
#include "main.h"
#include "fl_digi.h"

#include "timeops.h"
#include "debug.h"

using namespace std;

#define anal_BW         4

static char msg1[80];

void anal::tx_init(SoundBase *sc)
{
}

void anal::rx_init()
{
	phaseacc = 0;
	put_MODEstatus(mode);
}

void anal::init()
{
	modem::init();
	rx_init();
	set_scope_mode(Digiscope::RTTY);
}

anal::~anal()
{
	delete hilbert;
	delete bpfilt;
	delete ffilt;
	delete favg;
}

void anal::restart()
{
	double fhi = anal_BW * 1.1 / samplerate;
	double flo = 0.0;
	if (bpfilt)
		bpfilt->create_filter(flo, fhi);
	else
		bpfilt = new fftfilt(flo, fhi, 2048);

	symbollen = analMaxSymLen;
	set_bandwidth(anal_BW);

	ffilt->setLength(4000); // average over last 1/2 second of samples
	favg->setLength(120);    // average over last minute of samples

	sum = 0.0;
	fout_1 = fout_2 = 0.0;
	restart_count = 64;
	wf_freq = frequency;

	if (clock_gettime(CLOCK_REALTIME, &start_time) == -1) {
		LOG_PERROR("clock_gettime");
		abort();
	}
	struct tm tm;
	gmtime_r(&start_time.tv_sec, &tm);

	FILE* out = fopen(analysisFilename.c_str(), "a");
	if (unlikely(!out)) {
		LOG_PERROR("fopen");
		goto ret;
	}
	fprintf(out, "Time,Clock,Track,Freq\n"
		",,,,rf track frequency:    %" PRIdMAX "   %s\n"
		",,,,audio track frequency: %.0f\n"
		",,,,%02d-%02d-%02d\n",
		(intmax_t)wf->rfcarrier(), (wf->USB() ? "USB" : "LSB"),
		frequency, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	fclose(out);

ret:
	if (clock_gettime(CLOCK_MONOTONIC, &start_time) == -1) {
		LOG_PERROR("clock_gettime");
		abort();
	}
}

anal::anal()
{
	mode = MODE_ANALYSIS;

	samplerate = anal_SampleRate;

	bpfilt = (fftfilt *)0;
	hilbert = new C_FIR_filter();
	hilbert->init_hilbert(37, 1);
	ffilt = new Cmovavg(512);
	favg = new Cmovavg(64);
	
	analysisFilename = HomeDir;
	analysisFilename.append("freqanalysis.csv");

	pipeptr = 0;
	cap &= ~CAP_TX;
	restart();
}

void anal::clear_syncscope()
{
	set_scope(0, 0, false);
}

cmplx anal::mixer(cmplx in)
{
	cmplx z = cmplx( cos(phaseacc), sin(phaseacc)) * in;

	phaseacc -= TWOPI * frequency / samplerate;
	if (phaseacc > M_PI) 
		phaseacc -= TWOPI;
	else if (phaseacc < M_PI) 
		phaseacc += TWOPI;

	return z;
}

void anal::writeFile()
{
	struct timespec elapsed, now;
	// calculate elapsed time using the monotonic clock
	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
		LOG_PERROR("clock_gettime");
		abort();
	}
	elapsed = now - start_time;
	// calculate wall clock time using the realtime clock
	if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
		LOG_PERROR("clock_gettime");
		abort();
	}
	struct tm tm;
	gmtime_r(&now.tv_sec, &tm);

	FILE* out = fopen(analysisFilename.c_str(), "a");
	if (unlikely(!out)) {
		LOG_PERROR("fopen");
		return;
	}
	fprintf(out, "%02d:%02d:%02d, %" PRIdMAX ".%03" PRIdMAX ", %f, %12.4f\n",
		tm.tm_hour, tm.tm_min, tm.tm_sec, (intmax_t)elapsed.tv_sec,
		(intmax_t)(elapsed.tv_nsec / 1000000), fout_2,
		(wf->rfcarrier() + (wf->USB() ? 1.0 : -1.0) * (frequency + fout_2)));
	fclose(out);
}

int anal::rx_process(const double *buf, int len)
{
	cmplx z, *zp;
	double fin;
	int n;
	static int dspcnt = symbollen;

	if (wf_freq != frequency) restart();

	while (len-- > 0) {
// create analytic signal from sound card input samples
		z = cmplx( *buf, *buf );
		buf++;
		hilbert->run(z, z);
// mix it with the audio carrier frequency to create a baseband signal
		z = mixer(z);
// low pass filter using Windowed Sinc - Overlap-Add convolution filter
		n = bpfilt->run(z, &zp);

		if (n) for (int i = 0; i < n; i++) {
// measure phase difference between successive samples to determine
// the frequency of the baseband signal (+anal_baud or -anal_baud)
// see class cmplx definiton for operator %
			fin = arg( conj(prevsmpl) * zp[i] ) * samplerate / TWOPI;
			prevsmpl = zp[i];
			if (restart_count) restart_count--;
			else {
// filter using moving average filter
				fout_1 = ffilt->run(fin);
// the values in the pipe are +/- 2 Hz ==> +/- 1.0			
				pipe[pipeptr] = fout_1 / 4;
				pipeptr = (pipeptr + 1) % symbollen;
				dspcnt--;
				if (dspcnt == 0) {
					set_scope(pipe, symbollen, false);
					pipe.next(); // change buffers
// filter using second moving average filter & display the result
					fout_2 = favg->run(fout_1);
					if (wf->USB())
						snprintf(msg1, sizeof(msg1), "Freq: %12.2f", wf->rfcarrier() + frequency + fout_2 ); 
					else
						snprintf(msg1, sizeof(msg1), "Freq: %12.2f", wf->rfcarrier() - frequency - fout_2 );
					put_status(msg1, 2.0);
					writeFile();
// reset the display counter & the pipe pointer
					dspcnt = symbollen;
					pipeptr = 0;
				}
			}
		}
	}
	return 0;
}

//=====================================================================
// anal transmit
//=====================================================================

int anal::tx_process()
{
	return -1;
}
