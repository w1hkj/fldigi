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

#define anal_BW     10
#define	anal_SampleRate	8000
#define analFFT_LEN 8192 // approximately 1 sec of bpf'd audio stream
#define analBPF_SIZE 1024 //2048

static char msg1[15];
static char msg2[15];

void anal::tx_init(SoundBase *sc)
{
}

void anal::rx_init()
{
	put_MODEstatus(mode);
}

void anal::init()
{
	modem::init();
	rx_init();
	set_scope_mode(Digiscope::SCOPE);
}

anal::~anal()
{
	delete hilbert;
	delete bpfilt;
	delete [] fftbuff;
	delete [] dftbuff;
}

void anal::restart()
{
	double fhi = 0, flo = 0;

	wf_freq = frequency;
	fhi = (wf_freq + anal_BW * 1.1) / samplerate;
	flo = (wf_freq - anal_BW * 1.1) / samplerate;

	if (bpfilt)
		bpfilt->create_filter(flo, fhi);
	else
		bpfilt = new fftfilt(flo, fhi, analBPF_SIZE);

	memset(dftbuff, 0, analFFT_LEN * sizeof(*dftbuff));
	memset(fftbuff, 0, analFFT_LEN * sizeof(*fftbuff));

	set_bandwidth(anal_BW);

	sig_level = 1e-10;
	freq_corr = 0.0;
	restart_count = analFFT_LEN;

	if (write_to_csv) stop_csv();

	start_csv();
}

anal::anal()
{
	mode = MODE_ANALYSIS;

	samplerate = anal_SampleRate;

	fftbuff = new std::complex<double>[analFFT_LEN];
	dftbuff = new std::complex<double>[analFFT_LEN];

	bpfilt = (fftfilt *)0;

	hilbert = new C_FIR_filter();
	hilbert->init_hilbert(37, 1);

	analysisFilename = TempDir;
	analysisFilename.append("analysis.csv");

	cap &= ~CAP_TX;
	restart();
}

std::complex<double> anal::dft (std::complex<double> *buff, double fm, double Ts, double offset)
{
	std::complex<double> val;
	val = std::complex<double>(0,0);

	double factor = 2.0 / analFFT_LEN;
//	double omega = fm * Ts + offset / (2.0 * analFFT_LEN);
	double omega = fm * Ts + offset / (2.0 * analFFT_LEN);

	for( int i = 0; i < analFFT_LEN; i++)
		val += buff[i] * std::complex<double>(
			cos(2 * M_PI * i * omega),
			sin(2 * M_PI * i * omega) );
	val *= factor;
	return val;
}

void anal::start_csv()
{
	FILE *out = fopen(analysisFilename.c_str(), "w");
	if (unlikely(!out)) {
		LOG_PERROR("fopen");
		return;
	}
	fprintf(out, "Clock,Error,Audio,RF,Signal,Noise\n");
	fclose(out);

	put_status("writing csv file");

	write_to_csv = true;
	ticks = 0;
	slen = 0;
}

void anal::stop_csv()
{
	write_to_csv = false;
	put_status("");
}

void anal::writeFile()
{
	if (!write_to_csv) return;

	// calculate elapsed time using the number of sample blocks
	
	FILE *out = fopen(analysisFilename.c_str(), "a");
	if (unlikely(!out)) {
		LOG_PERROR("fopen");
		return;
	}

	double elapsed = ticks * slen / anal_SampleRate;
	fprintf(out, "%.1f, %.3f, %.3f, %12.4f, %g, %g\n",
		elapsed, freq_corr, freq_corr + wf_freq,
		(wf->rfcarrier() + (wf->USB() ? 1.0 : -1.0) * (wf_freq + freq_corr)),
		sig_level,
		noise);
//printf("fm %f, sig %f, noise %g\n", tracking_freq, sig_level, noise);
	fclose(out);
}

int anal::rx_process(const double *buf, int len)
{
	if (len > analFFT_LEN) return 0;

	std::complex<double> z;//, *zp;
	double fm = wf_freq;
	double am, bm;
	double dm;
	double delta = 0;
	double Ts = 1.0 / samplerate;

	if (wf_freq != frequency) {
		restart();
	}

	for (int i = 0; i < analFFT_LEN - len; i++)
		dftbuff[i] = dftbuff[i + len];
	size_t ptr = analFFT_LEN - len;
	for (int i = 0; i < len; i++) {
		z = std::complex<double>(buf[i], buf[i]);
		dftbuff[ptr + i] = z;
	}

	ticks++;
	slen = len;
	if (ticks % ( analFFT_LEN / len ) == 0) {

		for (int i = 0; i < analFFT_LEN; i++)
			fftbuff[i] = 2.0 * blackman(1.0 * i / analFFT_LEN) * dftbuff[i];

		max = 1e-10;
		maxnom = wf_freq;
		maxlower = maxnom - anal_BW;
		maxupper = maxnom + anal_BW;
		if (maxlower < 0) maxlower = 0;
		if (maxupper > analFFT_LEN / 2 - 2) maxupper = analFFT_LEN / 2 - 2;

		for (double f = maxlower; f < maxupper; f += 1) {
			test = norm(dft(fftbuff, f, Ts, 0));
			if (test > max) {
				maxnom = f;
				max = test;
			}
		}
		fm = maxnom;

		for (int m = 0; m < 16; m++) {
			am = norm(dft(fftbuff, fm, Ts, -0.5));
			bm = norm(dft(fftbuff, fm, Ts, 0.5));
			dm = (bm - am) / (bm + am);
			delta = (atan(dm * tan(M_PI / (2 * analFFT_LEN))) / M_PI)/Ts;
			if (fabs(delta) < 5) fm += delta;
		}

		sig_level = decayavg(sig_level, norm(dft(fftbuff, fm, Ts, 0)), 8);
		noise = decayavg(
				noise,
				(norm(dft(fftbuff, fm - anal_BW * 2, Ts, 0)) +
				norm(dft(fftbuff, fm + anal_BW * 2, Ts, 0))) / 2.0, 
				8);

		if (!noise) noise = 1e-8;
		if (!sig_level) sig_level = 1e-10;

		snr = 10.0 * log10(sig_level/noise) - 38.0; // 1 Hz / 3000 Hz noise bandwidth
		freq_corr = decayavg(freq_corr, fm - wf_freq, 8);

		tracking_freq = (wf->rfcarrier() + (wf->USB() ? 1.0 : -1.0) * (wf_freq + freq_corr));

		snprintf(msg1, 
			sizeof(msg1), 
			"%12.1f", tracking_freq);
		snprintf(msg2, 
			sizeof(msg2), 
			"%5.1f, %6.4f", snr, sig_level);
		put_Status1(msg1, 2.0);
		put_Status2(msg2, 2.0);

		writeFile();
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

