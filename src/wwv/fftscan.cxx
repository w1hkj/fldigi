// ----------------------------------------------------------------------------
// fftscan.cxx  --  fftscan modem
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

#include "fftscan.h"
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
#include "configuration.h"

using namespace std;

static char msg[30];

void fftscan::tx_init(SoundBase *sc)
{
}

void fftscan::rx_init()
{
	put_MODEstatus(mode);
}

void fftscan::init()
{
	modem::init();
	rx_init();
	set_scope_mode(Digiscope::SCOPE);
}

fftscan::~fftscan()
{
	delete [] fftbuff;
	delete [] dftbuff;
	delete scanfft;
}

void fftscan::restart()
{
	wf_freq = frequency;

	memset(dftbuff, 0, fftscanFFT_LEN * sizeof(*dftbuff));
	memset(fftbuff, 0, fftscanFFT_LEN * sizeof(*fftbuff));

	set_bandwidth(fftscan_BW);

	scans = 0;
	ticks = 0;

	if (write_to_csv) stop_csv();

	start_csv();
}

fftscan::fftscan()
{
	mode = MODE_FFTSCAN;

	samplerate = fftscan_SampleRate;

	fftbuff = new double[fftscanFFT_LEN];
	dftbuff = new std::complex<double>[fftscanFFT_LEN];

	scanfft = new g_fft<double>(fftscanFFT_LEN);

	fftscanFilename = TempDir;
	fftscanFilename.append("fftscan.csv");

	cap &= ~CAP_TX;
	restart();
}

std::complex<double> fftscan::dft (std::complex<double> *buff, double fm, double Ts, double offset)
{
	std::complex<double> val;
	val = std::complex<double>(0,0);

	double factor = 2.0 / fftscanFFT_LEN;
	double omega = fm * Ts + offset / (2.0 * fftscanFFT_LEN);

	for( int i = 0; i < fftscanFFT_LEN; i++)
		val += buff[i] * std::complex<double>(
			cos(2 * M_PI * i * omega),
			sin(2 * M_PI * i * omega) );
	val *= factor;
	return val;
}

void fftscan::start_csv()
{
	FILE *out = fopen(fftscanFilename.c_str(), "w");
	if (unlikely(!out)) {
		LOG_PERROR("fopen");
		return;
	}
	fprintf(out, "Freq, |Amp|\n");
	fclose(out);

	write_to_csv = true;
	ticks = 0;
	scans = 0;
}

void fftscan::stop_csv()
{
	write_to_csv = false;
	put_status("");
}

void fftscan::writeFile()
{
	if (!write_to_csv) return;

	// calculate elapsed time using the number of sample blocks
	
	FILE *out = fopen(fftscanFilename.c_str(), "w");//"a");
	if (unlikely(!out)) {
		LOG_PERROR("fopen");
		return;
	}

	for (int i = 0; i < fftscanFFT_LEN / 2; i++) {
		fprintf(out, "%0.1f, %f\n", 
			1.0 * i * fftscan_SampleRate/fftscanFFT_LEN, (buffer[i] - 1) * progdefaults.cnt_dft_range);
	}
	fclose(out);
}

//=======================================================================
//update_syncscope()
//=======================================================================
//
void fftscan::update_syncscope()
{
	double max = -1e6;
	double amp[3];

	buffer.next();

	amp[0] = fftbuff[0];
	amp[1] = fftbuff[1];
	amp[2] = fftbuff[2];
	double AMP = amp[0] + amp[1] + amp[2];
	for (int i = 1; i < fftscanFFT_LEN / 2 - 1; i++) {
		buffer[i] = 20 * log10f(AMP);
		if (max < buffer[i]) max = buffer[i];
		AMP -= amp[0];
		amp[0] = amp[1];
		amp[1] = amp[2];
		amp[2] = fftbuff[i + 1];
		AMP += amp[2];
	}
	buffer[0] = buffer[1];
	buffer[fftscanFFT_LEN/2 - 1] = buffer[fftscanFFT_LEN/2 - 2];
	for (int i = 0; i < fftscanFFT_LEN / 2; i++) {
		buffer[i] -= max;
		buffer[i] /= progdefaults.cnt_dft_range;
		buffer[i] += 1.0;
	}

// clear scope views
	if (digiscope) digiscope->clear_axis();
	wf->wfscope->clear_axis();

// vertical graticule
int N = progdefaults.cnt_dft_range / 10;
	for (int i = 1; i < N; i++) {
		if (digiscope) digiscope->xaxis(i, 1.0 * i / N);
		wf->wfscope->xaxis(i, 1.0 * i / N);
	}
// horizontal graticule
	for (int i = 1; i < 8; i++) {
		if (digiscope) digiscope->yaxis(i, i / 8.0);
		wf->wfscope->yaxis(i, i / 8.0);
	}
	set_scope(buffer, fftscanFFT_LEN/2, false);
}

std::complex<double> tempbuff[fftscanFFT_LEN];

int fftscan::rx_process(const double *buf, int len)
{
	if (wf_freq != frequency)
		restart();

	scans++;
	if (scans > progdefaults.cnt_dft_scans) return 0;

	for (int i = 0; i < fftscanFFT_LEN - len; i++)
		dftbuff[i] = dftbuff[i + len];
	for (int i = 0; i < len; i++) {
		dftbuff[fftscanFFT_LEN - len + i] = std::complex<double>(buf[i], 0);
	}

	for (int i = 0; i < fftscanFFT_LEN; i++)
		tempbuff[i] = dftbuff[i];

	scanfft->ComplexFFT(tempbuff);
	for (int i = 0; i < fftscanFFT_LEN/2; i++)
		fftbuff[i] += abs(tempbuff[i]);

	update_syncscope();

	if (scans == progdefaults.cnt_dft_scans) {
		put_Status2("scan completed", 30.0);
		writeFile();
		return 0;
	}
	snprintf(msg, sizeof(msg), "scanning %d", (int)(progdefaults.cnt_dft_scans - scans + 1));
	put_Status2(msg, 5.0);

	return 0;
}

//=====================================================================
// fftscan transmit
//=====================================================================

int fftscan::tx_process()
{
	return -1;
}

