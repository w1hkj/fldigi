// ----------------------------------------------------------------------------
// fftmon.cxx  --  fftmon modem
//
// Copyright (C) 2017
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

#include <FL/Fl_Counter.H>

#include "fl_digi.h"
#include "modem.h"
#include "misc.h"
#include "filters.h"
#include "fftfilt.h"
#include "waterfall.h"
#include "main.h"
#include "fft-monitor.h"
#include "timeops.h"
#include "debug.h"
#include "digiscope.h"
#include "trx.h"
#include "spectrum_viewer.h"

#include "threads.h"

#include "configuration.h"

using namespace std;

//extern Digiscope	*fftscope;
//extern spectrum		*fftscope;

#include "confdialog.h"

extern Fl_Counter	*fftviewer_scans;
extern Fl_Counter	*fftviewer_fcenter;
extern Fl_Counter	*fftviewer_frng;

extern Fl_Button	*pause_button;
extern Fl_Box		*annunciator;

pthread_mutex_t fftmon_mutex     = PTHREAD_MUTEX_INITIALIZER;

bool b_write_fftfile = false;
static std::string fftmonFilename;

void toggle_scans(void *me)
{
	fftmon *mon = (fftmon *)(me);
	if (mon->scans_stable)
		fftviewer_scans->color(FL_GREEN);
	else
		fftviewer_scans->color(FL_BACKGROUND_COLOR);
	fftviewer_scans->redraw();
}

void fftmon::init()
{
}

fftmon::~fftmon()
{
	delete [] fftbuff;
	delete [] dftbuff;
	delete [] buffer;
	delete scanfft;
}

void fftmon::restart()
{
	fftmon_sr = active_modem->get_samplerate();

	memset(dftbuff, 0, fftmonFFT_LEN * sizeof(*dftbuff));
	memset(fftbuff, 0, fftmonFFT_LEN * sizeof(*fftbuff));

	for (int i = 0; i < LENdiv2; i++)
		fftfilt[i]->setLength(progdefaults.fftviewer_scans);

	scans_stable = false;
	numscans = 0;
	Fl::awake(toggle_scans, this);
}

fftmon::fftmon()
{
	fftbuff = new double[fftmonFFT_LEN];
	dftbuff = new double[fftmonFFT_LEN];
	buffer  = new double[fftmonFFT_LEN / 2];

	for (int i = 0; i < LENdiv2; i++) {
		fftfilt[i] = new Cmovavg(1000);
		fftfilt[i]->setLength(progdefaults.fftviewer_scans);
	}

	for (int i = 0; i < fftmonFFT_LEN; i++)
		bshape[i] = blackman(1.0 * i / fftmonFFT_LEN);

	scanfft = new g_fft<double>(fftmonFFT_LEN);

	fftmonFilename = TempDir;
	fftmonFilename.append("rx_spectrum.csv");

	cap &= ~CAP_TX;  // rx only modem
	restart();
}

//=======================================================================
//
//=======================================================================
//
static double scopebuff[LENdiv2];
static double filebuff[LENdiv2];

double goto_freq()
{
	int fc = fftscope->gofreq();
	int fr = progdefaults.fftviewer_frng;
	int rem = 0;

	if (fc < 100) fc = 100;
	if (fr < 200) fr = 200;
	if (fc + fr/2 > 4000) fr = (4000 - fc)/2;
	if (fc < fr/2) fr = fc * 2;
	rem = fr % 20;
	fr /= 20;
	if (rem >= 10) fr = fr + 1;
	fr *= 20;
	if (fr > 4000) fr = 4000;

	progdefaults.fftviewer_frng = fr;
	progdefaults.fftviewer_fcenter = fc;

	fftviewer_fcenter->value(fc);
	fftviewer_fcenter->redraw();

	fftviewer_frng->value(fr);
	fftviewer_frng->redraw();

	fftscope->gofreq(0);
	return fc;
}

void write_to_fftscope(void *)
{
// clear scope views
	fftscope->clear_axis();

	double f0 = progdefaults.fftviewer_fcenter - progdefaults.fftviewer_frng / 2;
	double f1 = progdefaults.fftviewer_fcenter + progdefaults.fftviewer_frng / 2;
	double sr = active_modem->get_samplerate();

	if (fftscope->gofreq()) f0 = goto_freq();

	int n0 = LENdiv2 * (8000.0 / sr) * (f0 / 4000.0);
	int n1 = LENdiv2 * (8000.0 / sr) * (f1 / 4000.0);

// vertical graticule, every 10 dB
	int N = progdefaults.fftviewer_range / 10;
	for (int i = 1; i < N; i++)
		fftscope->xaxis(i, 1.0 * i / N);

// horizontal graticule
	int incr = 500;
	if (progdefaults.fftviewer_frng <= 2000) incr = 200;
	if (progdefaults.fftviewer_frng <= 1000) incr = 100;
	if (progdefaults.fftviewer_frng <= 500 ) incr = 50;
	if (progdefaults.fftviewer_frng <= 250)  incr = 25;

	annunciator->label(
		(incr == 500) ? "10 db/div, 500 Hz/div" :
		(incr == 200) ? "10 db/div, 200 Hz/div" :
		(incr == 100) ? "10 db/div, 100 Hz/div" :
		(incr == 50) ? "10 db/div, 50 Hz/div" :
		"10 db/div, 25 Hz/div");
	annunciator->redraw_label();

	int xp = f0;
    int xpd = xp % incr;
	double xpos = 1.0 * (incr - xpd) / progdefaults.fftviewer_frng;
	double fincr = 1.0 * incr / progdefaults.fftviewer_frng;

	int n = 1;
	while (xpos < 1.0) {
		fftscope->yaxis(n, xpos);
		xpos += fincr;
		n++;
	}

	if (fftscope->paused()) pause_button->label("Paused");
	else pause_button->label("Running");
	pause_button->redraw_label();

	static char msg[100];
	snprintf(msg, sizeof(msg), " %.0f Hz, %.1f dB",
		fftscope->freq(),
		fftscope->db());
	values->value(msg);

	if (fftscope->db_diff()) {
		snprintf(msg, sizeof(msg), "%.0f dB", fftscope->db_diff());
		db_diffs->value(msg);
		snprintf(msg, sizeof(msg), "%.0f Hz", fabs(fftscope->f_diff()));
		f_diffs->value(msg);
	} else {
		db_diffs->value("");
		f_diffs->value("");
	}

	fftscope->data(&scopebuff[n0], n1 - n0, false);

	fftscope->redraw();
}

// add mutex lock
void write_to_fftfile(void *)
{
	guard_lock gl_filebuff(&fftmon_mutex);

	b_write_fftfile = false;

	double sr = active_modem->get_samplerate();

	FILE *out = fl_fopen(fftmonFilename.c_str(), "w");
	if (unlikely(!out)) {
		LOG_PERROR("fl_fopen");
		return;
	}

	fprintf(out, "Frequency,Magnitude\n");
	for (int i = 0; i < LENdiv2; i++)
		fprintf(out, "%0.1f, %f\n", i * sr / fftmonFFT_LEN, filebuff[i]);

	fclose(out);
}

void fftmon::update_fftscope()
{
	if (!fftscope) return;

	if (b_write_fftfile) Fl::awake(write_to_fftfile);

	if ( !fftscope->paused() ) {
		guard_lock gl_filebuff(&fftmon_mutex);

		double val = 0;
		for (int i = 0; i < LENdiv2; i++) {
			val = fftbuff[i] / LENdiv2;
			if (val < 1e-6) val = 1e-6;
			if (val > 1) val = 1.0;
			filebuff[i] = val;
			buffer[i] = 20 * log10f(val);
		}
	}

	for (int i = 0; i < LENdiv2; i++)
		scopebuff[i] = 1.0 + (buffer[i] - progdefaults.fftviewer_maxdb)/progdefaults.fftviewer_range;

	Fl::awake(write_to_fftscope);
}

static std::complex<double> fftmon_temp[fftmonFFT_LEN];

int fftmon::rx_process(const double *buf, int len)
{

	if (len > fftmonFFT_LEN) return 0; // if audio playback

	if (fftmon_sr != active_modem->get_samplerate())
		restart();

	for (int i = 0; i < fftmonFFT_LEN - len; i++)
		dftbuff[i] = dftbuff[i + len];
	for (int i = 0; i < len; i++) {
		dftbuff[fftmonFFT_LEN - len + i] = buf[i];
	}

	double val;
	for (int i = 0; i < fftmonFFT_LEN; i++) {
		val = dftbuff[i] * bshape[i];
		fftmon_temp[i] = std::complex<double>(val, 0);//val);
	}

	scanfft->ComplexFFT(fftmon_temp);

	for (int i = 0; i < fftmonFFT_LEN/2; i++)
		fftbuff[i] = fftfilt[i]->run(abs(fftmon_temp[i]));

	update_fftscope();

	if (!scans_stable) {
		if (numscans++ >= progdefaults.fftviewer_scans) {
			scans_stable = true;
			Fl::awake(toggle_scans, this);
		}
	}

	return 0;
}
