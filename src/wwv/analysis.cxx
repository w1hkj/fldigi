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
	delete bpfilt;
	delete ffilt;
	delete favg;
}

void anal::restart()
{
	double fhi = ANAL_BW * 1.1 / samplerate;
	double flo = 0.0;
	if (bpfilt)
		bpfilt->create_filter(flo, fhi);
	else
		bpfilt = new fftfilt(flo, fhi, 2048);

	set_bandwidth(ANAL_BW);

	ffilt->reset();

	elapsed = 0.0;
	fout = 0.0;
	wf_freq = frequency;

	if (clock_gettime(CLOCK_REALTIME, &start_time) == -1) {
		LOG_PERROR("clock_gettime");
		abort();
	}

	passno = 0;
	dspcnt = DSP_CNT;
	for (int i = 0; i < PIPE_LEN; i++) pipe[i] = 0;

	if (write_to_csv) stop_csv();

	start_csv();

}

anal::anal()
{
	mode = MODE_ANALYSIS;

	samplerate = ANAL_SAMPLERATE;

	bpfilt = (fftfilt *)0;
	ffilt = new Cmovavg(FILT_LEN * samplerate);

	analysisFilename = TempDir;
	analysisFilename.append("analysis.csv");

	cap &= ~CAP_TX;
	write_to_csv = false;

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
	if (phaseacc < 0) phaseacc += TWOPI;

	return z;
}

void anal::start_csv()
{
	FILE *out = fl_fopen(analysisFilename.c_str(), "w");
	if (unlikely(!out)) {
		LOG_PERROR("fl_fopen");
		return;
	}
	fprintf(out, "Clock,Elapsed Time,Freq Error,RF\n");
	fclose(out);

	put_status("Writing csv file");

	write_to_csv = true;
}

void anal::stop_csv()
{
	write_to_csv = false;
	put_status("");
}

void anal::writeFile()
{
	if (!write_to_csv) return;

	struct timespec now;
	struct tm tm;

	// calculate wall clock time using the realtime clock
	if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
		LOG_PERROR("clock_gettime");
		abort();
	}
	gmtime_r(&now.tv_sec, &tm);

	FILE* out = fl_fopen(analysisFilename.c_str(), "a");
	if (unlikely(!out)) {
		LOG_PERROR("fl_fopen");
		return;
	}
	fprintf(out, "%02d:%02d:%02d, %8.3f, %8.3f, %12.3f\n",
		tm.tm_hour, tm.tm_min, tm.tm_sec, elapsed, fout,
		(wf->rfcarrier() + (wf->USB() ? 1.0 : -1.0) * (frequency + fout)));
	fclose(out);

	put_status("Writing csv file");

}

int anal::rx_process(const double *buf, int len)
{
	cmplx z, *zp;
	double fin;
	int n = 0;

	if (wf_freq != frequency) {
		restart();
		set_scope(pipe, PIPE_LEN, false);
	}

	for (int i = 0; i < len; i++) {
// create analytic signal from sound card input samples
		z = cmplx( *buf, *buf );
		buf++;
// mix it with the audio carrier frequency to create a baseband signal
		z = mixer(z);
// low pass filter using Windowed Sinc - Overlap-Add convolution filter
		n = bpfilt->run(z, &zp);

		if (n) {
			for (int j = 0; j < n; j++) {
// measure phase difference between successive samples to determine
// the frequency of the baseband signal (+anal_baud or -anal_baud)
// see class cmplx definiton for operator %
			fin = arg( conj(prevsmpl) * zp[j] ) * samplerate / TWOPI;
			prevsmpl = zp[j];
// filter using moving average filter
			fout = ffilt->run(fin);
			}
		} //else prevsmpl = z;
	}

	if (passno++ > 10) {
		dspcnt -= (1.0 * n / samplerate);

		if (dspcnt <= 0) {
			for (int i = 0; i < PIPE_LEN -1; i++)
				pipe[i] = pipe[i+1];

			double fdsp = fout / 4.0;
			if (fabs(fdsp) < 2.6) {
				elapsed += DSP_CNT - dspcnt;
				pipe[PIPE_LEN - 1] = fout / 4.0;
				set_scope(pipe, PIPE_LEN, false);

				if (wf->USB())
					snprintf(msg1, sizeof(msg1), "%12.2f", wf->rfcarrier() + frequency + fout );
				else
					snprintf(msg1, sizeof(msg1), "%12.2f", wf->rfcarrier() - frequency - fout );
				put_Status2(msg1, 2.0);
				writeFile();
			}
// reset the display counter & the pipe pointer
			dspcnt = DSP_CNT;
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
