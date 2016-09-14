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
	delete [] buffer;
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
	buffer  = new double[fftscanFFT_LEN / 2];

	scanfft = new g_fft<double>(fftscanFFT_LEN);

	fftscanFilename = TempDir;
	fftscanFilename.append("fftscan.csv");

	_refresh = false;

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
	FILE *out = fl_fopen(fftscanFilename.c_str(), "w");
	if (unlikely(!out)) {
		LOG_PERROR("fl_fopen");
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
	int nyquist = fftscanFFT_LEN / 2;

	FILE *out = fl_fopen(fftscanFilename.c_str(), "w");//"a");
	if (unlikely(!out)) {
		LOG_PERROR("fl_fopen");
		return;
	}

	for (int i = 0; i < nyquist; i++) {
		buffer[i] = 20 * log10f(fftbuff[i]/nyquist);
	}

	if (progdefaults.dft_relative) {
		double maxdb = -120;
		for (int i = 0; i < nyquist; i++)
			if (buffer[i] > maxdb) maxdb = buffer[i];
		for (int i = 0; i < nyquist; i++)
			buffer[i] -= maxdb;
	}

	fprintf(out, "Freq,Signal,dB\n");
	for (int i = 0; i < fftscanFFT_LEN / 2; i++) {
		fprintf(out, "%0.1f, %f, %f\n",
			1.0 * i * fftscan_SampleRate/fftscanFFT_LEN,
			(fftbuff[i]/nyquist),
			buffer[i]);//20 * log10f(fftbuff[i]/nyquist));
	}
	fclose(out);
}

//=======================================================================
//update_syncscope()
//=======================================================================
//

void fftscan::update_syncscope()
{
	int nyquist = fftscanFFT_LEN/2;

	for (int i = 0; i < nyquist; i++) {
		buffer[i] = 20 * log10f(fftbuff[i]/nyquist);
	}

	if (progdefaults.dft_relative) {
		double maxdb = -120;
		for (int i = 0; i < nyquist; i++)
			if (buffer[i] > maxdb) maxdb = buffer[i];
		for (int i = 0; i < nyquist; i++)
			buffer[i] -= maxdb;
	}

	for (int i = 0; i < nyquist; i++)
		buffer[i] = 1.0 + buffer[i] / progdefaults.cnt_dft_range;

	double scopebuff[nyquist];
	for (int i = 1; i < nyquist - 1; i++)
		scopebuff[i] = max(max(buffer[i-1], buffer[i]), buffer[i+1]);
// clear scope views
	if (digiscope) digiscope->clear_axis();
	wf->wfscope->clear_axis();

// vertical graticule, every 10 dB
int N = progdefaults.cnt_dft_range / 10;
	for (int i = 1; i < N; i++) {
		if (digiscope) digiscope->xaxis(i, 1.0 * i / N);
		wf->wfscope->xaxis(i, 1.0 * i / N);
	}
// horizontal graticule, every 500 Hz
	for (int i = 1; i < 8; i++) {
		if (digiscope) digiscope->yaxis(i, i / 8.0);
		wf->wfscope->yaxis(i, i / 8.0);
	}
	set_scope(scopebuff, nyquist, false);
}

std::complex<double> tempbuff[fftscanFFT_LEN];

int fftscan::rx_process(const double *buf, int len)
{
	if (len > fftscanFFT_LEN) return 0; // if audio playback

	if (wf_freq != frequency)
		restart();

	scans++;

	if (scans > progdefaults.cnt_dft_scans) {
		if (_refresh) {
			_refresh = false;
			update_syncscope();
		}
		return 0;
	}

	for (int i = 0; i < fftscanFFT_LEN - len; i++)
		dftbuff[i] = dftbuff[i + len];
	for (int i = 0; i < len; i++) {
		dftbuff[fftscanFFT_LEN - len + i] = std::complex<double>(buf[i], 0);
	}

	for (int i = 0; i < fftscanFFT_LEN; i++)
		tempbuff[i] = dftbuff[i];

	scanfft->ComplexFFT(tempbuff);
	for (int i = 0; i < fftscanFFT_LEN/2; i++)
		fftbuff[i] = (fftbuff[i] * (scans - 1) + abs(tempbuff[i])) / scans;

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

