// ----------------------------------------------------------------------------
// fmt.cxx  --  fmt modem
//
// Copyright (C) 2020
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
#include <iostream>
#include <fstream>
#include <cstdarg>
#include <unistd.h>
#include <string.h>

#include "configuration.h"
#include "confdialog.h"
#include "fmt.h"
#include "fmt_dialog.h"
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
#include "qrunner.h"

#include "status.h"

//using namespace std;

// RnA discriminator

#define fmt_DFT_LEN 51200
#define fmt_LPF_LEN 512
#define fmt_BPF_LEN 512

static int srs[]	= {8000, 11025, 12000, 16000, 22050, 24000, 44100, 48000};
//static int dftlen[] = {1024,  1536,  1536,  2048,  2560,  2560,  5120,  6144}; // ~8 DFTs per second
//static int dftlen[] = {2048,  2560,  3072,  4096,  5632,  6144, 11264, 12288}; // ~4 DFTs per second
//static int dftlen[] = {4096,  5632,  6144,  8192, 11264, 12288, 22528, 24576}; // ~2 DFTs per second
static int dftlen[] = {8192, 11264, 12288, 16384, 22528, 24576, 45056, 49152}; // ~1 DFTs per second
//static int dftblocks[] = {16, 22, 24, 32, 44, 48, 88, 96}; // # 512 sample blocks for 1 DFT / second

static char msg1[80];
static char msg1a[80];
static char msg2[80];
static char msg2a[80];

static double fmt_ref_frequency = 0;
static double fmt_ref_base_freq = 0;
static double fmt_ref_amp = 0;

static double fmt_unk_frequency = 0;
static double fmt_unk_base_freq = 0;
static double fmt_unk_amp = 0;

static std::string fmt_filename;
static std::string fmt_wav_filename;
static std::string fmt_wav_pathfname;
static char file_datetime_name[200];

pthread_mutex_t scope_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

//======================================================================
// FMT Thread loop
//======================================================================

static pthread_t FMT_thread;

static int rec_interval[] = { 10, 25, 50, 100, 200, 500, 1000 };

bool record_unk = false;
bool record_ref = false;
bool write_recs = false;

static bool FMT_exit = false;
static bool FMT_enabled = false;
static bool is_recording = false;
static char s_clk_time[40];

static int csvrow = 0;

static bool fmt_start = true;
static bool record_ok = false;
static time_t fmt_time = 0;
static time_t last_epoch = 0;
static time_t curr_epoch = 0;
static struct tm fmt_tm;
static struct timeval fmt_tv;
static double ufreq = 0, uamp = 0;
static double rfreq = 0, ramp = 0;

// formatting strings used by csv export method
static std::string csv_string;
static std::string buffered_csv_string;
// debugging data string
static std::string debug_csv_string;

static char comma_format[] = "\
\"T %02d:%02d:%02d.%02d\",\
\"%6.2f\",\
\"%13.3f\",\
\"%13.4f\",\
\"%0.4f\",\
\"%6.4f\",\
\"%s\",\
\"%6.2f\",,\
\"%13.4f\",\
\"%0.4f\",\
\"%s\",\
\"%6.2f\"\
,,\"=L%d-F%d-D%d\"\n";

static char tab_format[] = "\
\"T %02d:%02d:%02d.%02d\"\t\
\"%6.2f\"\t\
\"%13.3f\"\t\
\"%13.4f\"\t\
\"%0.4f\"\t\
\"%6.4f\"\t\
\"%s\"\t\
\"%6.2f\"\t\t\
\"%13.4f\"\t\
\"%0.4f\"\t\
\"%s\"\t\
\"%6.2f\"\
\t\t\"=L%d-F%d-D%d\"\n";

static char usb_ref_equation[] = "=C%d + (D%d + E%d + F%d)";
static char lsb_ref_equation[] = "=C%d - (D%d + E%d + F%d)";
static char usb_unk_equation[] = "=C%d + (D%d + J%d + K%d)";
static char lsb_unk_equation[] = "=C%d - (D%d + J%d + K%d)";
static char ref_equation[50];
static char unk_equation[50];

void set_button ( void *d )
{
	Fl_Button *b = (Fl_Button *)d;
	b->value(1);
}

void clear_button ( void *d )
{
	Fl_Button *b = (Fl_Button *)d;
	b->value(0);
}

std::string txtout;

void set_output (void *)
{
	txt_fmt_wav_filename->value(txtout.c_str());
}

void start_fmt_wav_record()
{
	time_t wav_time = time(NULL);
	struct tm File_Start_Date;
	gmtime_r(&wav_time, &File_Start_Date);

	static char temp[200];
	strftime(temp, sizeof(temp), "fmt_%Y.%m.%d.%H.%M.%S", 
		&File_Start_Date);

	fmt_wav_pathfname.assign(FMTDir).
		append(temp).
		append(".").append((progdefaults.myCall.empty() ? "nil" : progdefaults.myCall)).
		append(".wav");

	if(!RXscard->startCapture(fmt_wav_pathfname, SF_FORMAT_WAV | SF_FORMAT_PCM_16)) {
		Fl::awake( clear_button, btn_fmt_record_wav);
		txtout.clear();
		Fl::awake (set_output);
		is_recording = false;
		return;
	}
	is_recording = true;
	txtout = temp;
	Fl::awake (set_output);
}

void cb_fmt_record_wav(bool b)
{
	if (!b) {
		RXscard->stopCapture();
		txtout.clear();
		Fl::awake (set_output);
		is_recording = false;
		return;
	}
	if (!is_recording)
		start_fmt_wav_record();
}

static void show(void *) {
	btn_fmt_record_wav->value(1);
	txt_fmt_wav_filename->value(fmt_wav_pathfname.c_str());
}

static void noshow(void *) {
	btn_fmt_record_wav->value(0);
	txt_fmt_wav_filename->value("");
}

static std::ofstream csv_file;

static void fmt_create_file()
{
	struct tm File_Start_Date;
	gmtime_r(&curr_epoch, &File_Start_Date);
	std::string call = progdefaults.myCall.empty() ? "nil" : progdefaults.myCall;

	strftime((char*)file_datetime_name, sizeof(file_datetime_name),
		"%Y.%m.%d.%H.%M.%S",
		&File_Start_Date);

	fmt_filename.assign(FMTDir).
		append("fmt_").
		append(file_datetime_name).
		append(".").append(call).
		append(".csv");

	csv_file.open(fmt_filename.c_str());

	if (!csv_file.is_open()) {
		LOG_ERROR("fl_fopen: %s", fmt_filename.c_str());
		return;
	}

//   A  ,   B   ,    C   ,    D    ,     E     ,    F   ,     G      ,   H     ,I,     J     ,    K   ,     L      ,    M    ,N,      O         , P,  Q    ,    R
// Clock,Elapsed,Xcvr VFO,Freq Corr,Ref WF Freq,Ref Corr,Ref Est Freq,Ref dBVpk, ,Unk WF Freq,Unk Corr,Unk Est Freq,Unk dBVpk, ,Unk Compensated ,
//              ,        ,         ,           ,        ,            ,         , ,           ,        ,            ,         , ,                , ,Average:,=average(O:O)
//              ,        ,         ,           ,        ,            ,         , ,           ,        ,            ,         , ,                , ,Std Dev:,=stdev(O:O)

csv_string.assign("\
\"Clock\",\"Elapsed\",\"Xcvr VFO\",\"Freq Corr\",\
\"Ref WF Freq\",\"Ref Corr\",\"Ref Est Freq\",\"Ref dBVpk\",,\
\"Unk WF Freq\",\"Unk Corr\",\"Unk Est Freq\",\"Unk dBVpk\",,\
\"Unk Compensated\",");
csv_string.append(call).append(",").append(file_datetime_name).append("\n");
csv_string.append("\
,,,,,,,,,,,,,,,\"Average:\",\"=average(O:O)\"\n"),
csv_string.append("\
,,,,,,,,,,,,,,,\"Std Dev:\",\"=stdev(O:O)\"\n");

	if (progdefaults.FMT_use_tabs) {
		for (size_t n = 0; n < csv_string.length(); n++)
			if (csv_string[n] == ',') csv_string[n] = '\t';
	}

	csvrow = 4;

	if (progdefaults.fmt_sync_wav_file && !is_recording) {
		fmt_wav_pathfname.assign(FMTDir).
			append(file_datetime_name).
			append(".").append(call).
			append(".wav");

		if(!RXscard->startCapture(fmt_wav_pathfname, SF_FORMAT_WAV | SF_FORMAT_PCM_16)) {
			Fl::awake (noshow);
			is_recording = false;
		} else {
			Fl::awake (show);
			is_recording = true;
		}
	}

//	debug_csv_string = csv_string;

	put_status (file_datetime_name);
}

void fmt_reset_record()
{
	Fl::awake (clear_button, btn_fmt_record);

	if (progdefaults.fmt_sync_wav_file) {
		Fl::awake ( clear_button, btn_fmt_record_wav);
		cb_fmt_record_wav(false);
	}
	txtout.clear();
	Fl::awake (set_output);
}

void fmt_show_recording(void *on)
{
	if (on == (void *)1) {
		box_fmt_recording->color(FL_DARK_RED);
	} else {
		box_fmt_recording->color(FL_WHITE);
	}
	box_fmt_recording->redraw();
}

int fmt_auto_record = false;
int start_auto_record = 0;
int autorecord_time = 0;

void start_auto_tracking(void *)
{
	LOG_INFO("%s", "start auto record in 10 secs");
	btn_unk_enable->value(1);
	btn_unk_enable->redraw();
	btn_ref_enable->value(1);
	btn_ref_enable->redraw();
}

void start_auto_recording(void *)
{
	LOG_INFO("%s", "start auto record now");
	btn_fmt_record->value(1);
	btn_fmt_record->redraw();
	fmt_create_file();
	return;
}

void stop_auto_recording(void *)
{
	LOG_INFO("%s", "stop auto record");
	btn_unk_enable->value(0);
	btn_unk_enable->redraw();
	btn_ref_enable->value(0);
	btn_ref_enable->redraw();
	btn_fmt_record->value(0);
	btn_fmt_record->redraw();
	write_recs = false;
}

static char sz_temp[512];

// debugging
void write_debug_string()
{
	std::string debug_filename;
	debug_filename.assign(FMTDir).append("debug.csv");
	rotate_log(debug_filename);

	FILE *csv_debug = fopen(debug_filename.c_str(), "w");
	fprintf(csv_debug, "%s", debug_csv_string.c_str());
	fclose(csv_debug);
	debug_csv_string.clear();
}

void fmt_write_file()
{
	fmt_start = true;
	static int ticks = 0;
	static int hrs = 0, mins = 0, secs = 0;
	static bool reset_ticks = false;

	if (gettimeofday (&fmt_tv, NULL)) {
		return;
	}

	fmt_time = time(NULL);
	last_epoch = fmt_time + 1;
	while (fmt_time < last_epoch) {
		MilliSleep(1);
		fmt_time = time(NULL);
	}

	gmtime_r(&fmt_time, &fmt_tm);
	last_epoch = fmt_time;
	hrs = fmt_tm.tm_hour;
	mins = fmt_tm.tm_min;
	secs = fmt_tm.tm_sec;
	ticks = 0;

	for (;;) {

		if (FMT_exit || active_modem->get_mode() != MODE_FMT)
			break;

		curr_epoch = time(NULL);
		if (curr_epoch != last_epoch) {
			last_epoch = curr_epoch;
			if (++secs >= 60) {
				secs = 0;
				if (++mins >= 60) {
					mins = 0;
					if (++hrs >= 24)
						hrs = 0;
				}
			}
			if (fmt_auto_record == 2 && secs == start_auto_record) {
				Fl::awake(start_auto_recording);
				fmt_auto_record = 3;
				autorecord_time = -1;
			}
			if ((++autorecord_time >= cnt_fmt_auto_record_time->value()) && fmt_auto_record == 3) {
				Fl::awake(stop_auto_recording);
				fmt_auto_record = 0;
			}

			if (reset_ticks) {
				ticks = 0;
				reset_ticks = false;
			} else {
				if (ticks % 100 < 50)
					ticks = ticks - ticks % 100;
				else
					ticks = ticks - ticks % 100 + 100;
			}
			snprintf(s_clk_time, sizeof(s_clk_time), "%02d:%02d:%02d",
				hrs, mins, secs);
			put_Status1 (s_clk_time);
			record_ok = true;
		}

		if (btn_fmt_autorecord->value()) {
			if (fmt_auto_record == 1) {
				Fl::awake(start_auto_tracking);
				start_auto_record = secs + 10;
				if (start_auto_record > 60) start_auto_record -= 60;
				fmt_auto_record = 2;
			}
		} else
			fmt_auto_record = 0;

		if (!record_unk && !record_ref && !btn_fmt_autorecord->value()) {
			if (csv_file.is_open()) {
				buffered_csv_string.assign(csv_string);
				csv_file << buffered_csv_string;
				csv_file.flush();
				csv_file.close();
				debug_csv_string.append(csv_string);
				write_debug_string();
				csv_string.clear();
			}
			Fl::awake (fmt_show_recording, (void *)0);
			fmt_reset_record();
			put_Status1 ("");
			put_status ("");
			MilliSleep(50);
			break;
		}

		if (write_recs && !csv_file.is_open()) {
			fmt_create_file();
			record_ok = false;
			reset_ticks = true;
			Fl::awake (fmt_show_recording, (void *)1);
		} else if (!write_recs && csv_file.is_open()) {
			Fl::awake (fmt_show_recording, (void *)0);
			buffered_csv_string.assign(csv_string);
			csv_file << buffered_csv_string;
			csv_file.flush();
			csv_file.close();
			debug_csv_string.append(csv_string);
			write_debug_string();
			csv_string.clear();
			put_Status1 ("");
			put_status ("");
			fmt_reset_record();
		}

		if (ticks % rec_interval[progStatus.FMT_rec_interval] == 0) {

			if (record_ok && (record_unk || record_ref) && write_recs && csv_file.is_open()) {
				{
					guard_lock datalock (&data_mutex);
						ufreq = fmt_unk_base_freq;
						uamp = fmt_unk_amp;
						rfreq = fmt_ref_base_freq;
						ramp = fmt_ref_amp;
				}
				if (record_unk) {
					uamp = 20.0 * log10( (uamp == 0 ? 1e-6 : uamp) );
				} else {
					ufreq = 0;
					uamp = -160;
				}
				if (record_ref) {
					ramp = 20.0 * log10( (ramp == 0 ? 1e-6 : ramp) );
				} else {
					rfreq = 0;
					ramp = -160;
				}

				if (wf->USB()) {
					snprintf(ref_equation, sizeof(ref_equation), usb_ref_equation, csvrow, csvrow, csvrow, csvrow);
					snprintf(unk_equation, sizeof(unk_equation), usb_unk_equation, csvrow, csvrow, csvrow, csvrow);
				} else {
					snprintf(ref_equation, sizeof(ref_equation), lsb_ref_equation, csvrow, csvrow, csvrow, csvrow);
					snprintf(unk_equation, sizeof(unk_equation), lsb_unk_equation, csvrow, csvrow, csvrow, csvrow);
				}

				snprintf(sz_temp, sizeof(sz_temp),
					(progdefaults.FMT_use_tabs ? tab_format : comma_format),
					hrs, mins, secs, ticks % 100,
					ticks * 0.01,
					1.0 * qsoFreqDisp->value(),
					progdefaults.FMT_freq_corr,
					(record_ref ? progStatus.FMT_ref_freq : 0),
					rfreq, 
					(record_ref ? ref_equation : "0"),
					ramp,
					(record_unk ? progStatus.FMT_unk_freq : 0),
					ufreq, 
					(record_unk ? unk_equation : "0"),
					uamp,
					csvrow, csvrow, csvrow);
				csv_string.append(sz_temp);
				csvrow++;
				buffered_csv_string.assign(csv_string);
				csv_file << buffered_csv_string;
				csv_file.flush();

				debug_csv_string.append(csv_string);

				csv_string.clear();
			}
		}
		ticks++;
		MilliSleep(10);
	}
}

void *FMT_loop(void *args)
{
	SET_THREAD_ID(FMT_TID);
	while(1) {
		MilliSleep(50);
		if (FMT_exit) break;

		record_unk = btn_unk_enable->value();
		record_ref = btn_ref_enable->value();
		if (record_unk || record_ref || btn_fmt_autorecord->value()) {
			fmt_write_file();
		}
	}

// exit the FMT thread
	SET_THREAD_CANCEL();
	return NULL;
}

//======================================================================
//
//======================================================================
void FMT_thread_init(void)
{
	FMT_exit = false;

	if (pthread_create(&FMT_thread, NULL, FMT_loop, NULL) < 0) {
		LOG_ERROR("%s", "pthread_create failed");
		return;
	}

	LOG_INFO("%s", "FMT thread started");
	FMT_enabled = true;
}

//======================================================================
//
//======================================================================
void FMT_thread_close(void)
{
	if (!FMT_enabled) return;

	FMT_exit = true;
	pthread_join(FMT_thread, NULL);
	FMT_enabled = false;

	FMT_thread = 0;

	LOG_INFO("%s", "FMT thread closed");
}


void fmt::tx_init()
{
}

void fmt::rx_init()
{
	put_MODEstatus(mode);
}

void fmt::init()
{
	modem::init();
	rx_init();
}

fmt::~fmt()
{
	delete unk_ffilt;
	delete unk_afilt;

	delete ref_ffilt;
	delete ref_afilt;

	delete [] unk_pipe;
	delete [] ref_pipe;

// RnA
	delete [] fftbuff;
	delete [] unkbuff;
	delete [] refbuff;
	delete [] BLACKMAN;

	delete unk_bpfilter;
	delete ref_bpfilter;

	cb_fmt_record_wav(false);
}

bool clear_unknown_pipe = false;
void fmt::clear_unk_pipe()
{
	clear_unknown_pipe = true;
}

bool clear_reference_pipe = false;
void fmt::clear_ref_pipe()
{
	clear_reference_pipe = true;
}

double fmt::blackman(double omega) {
	return (0.42 - 0.50 * cos(twoPI * omega) + 0.08 * cos(4 * M_PI * omega));
}

void fmt::restart()
{
	if (progdefaults.FMT_sr < 0) progdefaults.FMT_sr = 0;
	if (progdefaults.FMT_sr > 7) progdefaults.FMT_sr = 7;
	sr = progdefaults.FMT_sr;

	samplerate = srs[sr];
	Ts = 1.0 / samplerate;

	set_samplerate(samplerate);

	unk_ffilt->setLength (movavg_len = 0);
	unk_afilt->setLength (movavg_len);

	ref_ffilt->setLength (movavg_len);
	ref_afilt->setLength (movavg_len);

	unk_freq = progStatus.FMT_unk_freq;

	unk_ffilt->reset();
	unk_afilt->reset();
	unk_count = 0;
	dspcnt = DSP_CNT;
	for (int i = 0; i < MAX_DATA_PTS; i++) {
		unk_pipe[i].x = i + 1;
		unk_pipe[i].y = 100;
	}

	ref_ffilt->reset();
	ref_afilt->reset();
	ref_freq = progStatus.FMT_ref_freq;
	ref_count = 0;
	{
		guard_lock datalock (&scope_mutex);
		for (int i = 0; i < MAX_DATA_PTS; i++) {
			ref_pipe[i].x = i + 1;
			ref_pipe[i].y = 100;
		}
	}
	double tau = 1.0 / (dftlen[sr] - 1);
	for (int i = 0; i < dftlen[sr]; i++) {
		BLACKMAN[i] = blackman( i * tau );
	}

	for (int i = 0; i < fmt_DFT_LEN; i++) {
		fftbuff[i] = 0;
		unkbuff[i] = 0;
		refbuff[i] = 0;
	}

	reset_bpf();

// delta conversion coefficients, only change with change in sample rate
	dmK = tan(M_PI / (2.0 * dftlen[sr]));
	srK = srs[sr] / M_PI;
}

fmt::fmt()
{
	mode = MODE_FMT;

	if (progdefaults.FMT_sr < 1) progdefaults.FMT_sr = 1;
	if (progdefaults.FMT_sr > 6) progdefaults.FMT_sr = 6;

	samplerate = srs[progdefaults.FMT_sr];

	unk_pipe = new PLOT_XY[MAX_DATA_PTS];
	ref_pipe = new PLOT_XY[MAX_DATA_PTS];

	movavg_len = progdefaults.FMT_movavg_len;

	unk_ffilt = new Cmovavg(movavg_len);
	unk_afilt = new Cmovavg(movavg_len);

	ref_ffilt = new Cmovavg(movavg_len);
	ref_afilt = new Cmovavg(movavg_len);

	cap &= ~CAP_TX;

// RnA
	BLACKMAN = new double[fmt_DFT_LEN];

	fftbuff = new double[fmt_DFT_LEN];
	unkbuff = new double[fmt_DFT_LEN];
	refbuff = new double[fmt_DFT_LEN];

	unk_bpfilter = new C_FIR_filter();
	ref_bpfilter = new C_FIR_filter();

	twoPI = 2.0 * M_PI;

	restart();

	FMT_thread_init();
}

void fmt::reset_unknown()
{
	unk_count = 0;
}

void fmt::reset_reference()
{
	ref_count = 0;
}

// ---------------------------------------------------------------------
// DFT estimator using T&R algorithm
// ---------------------------------------------------------------------

void fmt::reset_bpf()
{
	int fillen = fmt_BPF_LEN;
	double fhi = 0;
	double flo = 0;

	bpf_width = progdefaults.FMT_bpf_width;

	fhi = 1.0 * (unk_freq + bpf_width / 2) / samplerate;
	flo = 1.0 * (unk_freq - bpf_width / 2) / samplerate;
	unk_bpfilter->init_bandpass (fillen, 1, flo, fhi);

	fhi = 1.0 * (ref_freq + bpf_width / 2) / samplerate;
	flo = 1.0 * (ref_freq - bpf_width / 2) / samplerate;
	ref_bpfilter->init_bandpass (fillen, 1, flo, fhi);

	set_bandwidth(20);

}

double fmt::absdft (double *buff, double fm, double incr)
{
	double rval = 0;
	double ival = 0;

	double omega = fm * Ts + incr / dftlen[sr];

	for( int i = 0; i < dftlen[sr]; i++) {
		rval += buff[i] * cos(twoPI * i * omega);
		ival += buff[i] * sin(twoPI * i * omega);
	}
	return 2.0 * sqrt(rval * rval + ival * ival) / dftlen[sr];
}

double fmt::evaluate_dft(double &freq){
	for (int n = 0; n < 2; n++) {
		am = absdft (fftbuff, freq, -0.5);
		bm = absdft (fftbuff, freq,  0.5 );
		if (am + bm == 0)
			break;
		dm = (bm - am) / (bm + am);
		delta = srK * atan(dm * dmK);
		if (abs(delta) > progdefaults.FMT_HL_level) {
			if (progdefaults.FMT_HL_on) {
				LOG_ERROR("HDL: %f", delta);
				delta = (delta < 0 ? -1 : 1) * progdefaults.FMT_HL_level;
			} else if (progdefaults.FMT_dft_cull_on) {
				LOG_ERROR("CUL: %f", delta);
				delta = 0;
			}
		}
		freq += delta;
	}
	return 2.39883 * absdft(fftbuff, freq, 0);
}

int fmt::rx_process_dft()
{
	double amp;

	if (unk_count == 0 || 
		((fabs(dft_unk_base) > progdefaults.FMT_freq_err) && record_unk) ) {
		unk_freq = progStatus.FMT_unk_freq;
		unk_count = 1;
		LOG_VERBOSE("FMT unknown freq reset to track @ %f Hz", progStatus.FMT_unk_freq);
	}

	if (ref_count == 0 || 
		((fabs(dft_ref_base) > progdefaults.FMT_freq_err) && record_ref) ) {
		ref_freq = progStatus.FMT_ref_freq;
		ref_count = 1;
		LOG_VERBOSE("FMT reference freq reset to track @ %f Hz", progStatus.FMT_ref_freq);
	}

// unknown tracking

	for (int i = 0; i < dftlen[sr]; i++)
		fftbuff[i] = BLACKMAN[i] * unkbuff[i];

	amp = evaluate_dft(unk_freq);

	if (progdefaults.FMT_movavg_len > 0) {
		dft_unk_base = unk_ffilt->run (unk_freq - progStatus.FMT_unk_freq);
		dft_unk_amp  = unk_afilt->run (amp);
	} else {
		dft_unk_base = unk_freq - progStatus.FMT_unk_freq;
		dft_unk_amp = amp;
	}

// reference tracking
	for (int i = 0; i < dftlen[sr]; i++)
		fftbuff[i] = BLACKMAN[i] * refbuff[i];

	amp = evaluate_dft(ref_freq);

	if (progdefaults.FMT_movavg_len > 0) {
		dft_ref_base = ref_ffilt->run (ref_freq - progStatus.FMT_ref_freq);
		dft_ref_amp  = ref_afilt->run (amp);
	} else {
		dft_ref_base = ref_freq - progStatus.FMT_ref_freq;
		dft_ref_amp = amp;
	}
	return 0;
}

static int smpl_counter = 0;
static int blk_counter = 0;

int fmt::rx_process(const double *buf, int len)
{
	if (sr != progdefaults.FMT_sr) {
		restart();
		ref_count = 0;
		unk_count = 0;
	}

	if (movavg_len != progdefaults.FMT_movavg_len) {

		movavg_len = progdefaults.FMT_movavg_len;

		unk_ffilt->setLength (movavg_len * samplerate / len);
		unk_afilt->setLength (movavg_len * samplerate / len);

		ref_ffilt->setLength (movavg_len * samplerate / len);
		ref_afilt->setLength (movavg_len * samplerate / len);
	}

	if (clear_unknown_pipe) {
		guard_lock datalock (&scope_mutex);
		for (int i = 0; i < MAX_DATA_PTS; i++) {
			unk_pipe[i].x = i + 1;
			unk_pipe[i].y = 100;
		}
		clear_unknown_pipe = false;
		REQ (clear_unk_scope);
	}

	if (clear_reference_pipe) {
		guard_lock datalock (&scope_mutex);
		for (int i = 0; i < MAX_DATA_PTS; i++) {
			ref_pipe[i].x = i + 1;
			ref_pipe[i].y = 100;
		}
		clear_reference_pipe = false;
		REQ (clear_ref_scope);
	}

	double out, in;
	for (int i = 0; i < fmt_DFT_LEN - len; i++) {
		unkbuff[i] = unkbuff[i + len];
		refbuff[i] = refbuff[i + len];
	}

	for (int i = 0; i < len; i++) {
		in = buf[i];

		if (progdefaults.FMT_unk_bpf_on) {
			unk_bpfilter->Irun(in, out);
			unkbuff[fmt_DFT_LEN - len + i] = out;
		} else
			unkbuff[fmt_DFT_LEN - len + i] = in;

		if (progdefaults.FMT_ref_bpf_on) {
			ref_bpfilter->Irun(in, out);
			refbuff[fmt_DFT_LEN - len + i] = out;
		} else
			refbuff[fmt_DFT_LEN - len + i] = in;
	}

	if ((blk_counter += len) >= dftlen[sr] / (progdefaults.FMT_dft_rate + 1)) {
		if (bpf_width != progdefaults.FMT_bpf_width || unk_count == 0 || ref_count == 0)
			reset_bpf();
		rx_process_dft ();
		unk_base_freq = dft_unk_base;
		unk_amp = dft_unk_amp;
		ref_base_freq = dft_ref_base;
		ref_amp = dft_ref_amp;
		blk_counter = 0;
	}

	if (++smpl_counter >= (samplerate / len) / 20.0) { // 0.05 second
		guard_lock datalock (&data_mutex);
		fmt_unk_frequency = unk_freq;
		fmt_unk_base_freq = unk_base_freq;
		fmt_unk_amp = unk_amp;
		fmt_ref_frequency = ref_freq;
		fmt_ref_base_freq = ref_base_freq;
		fmt_ref_amp = ref_amp;
		smpl_counter = 0;
	}

	dspcnt -= (1.0 * len / samplerate);
	if (dspcnt <= 0) {
		{
			guard_lock datalock(&scope_mutex);
			for (int i = 1; i < MAX_DATA_PTS; i++) {
				unk_pipe[i-1].y = unk_pipe[i].y;
				ref_pipe[i-1].y = ref_pipe[i].y;
			}
			if (btn_unk_enable->value()) {
				unk_pipe[MAX_DATA_PTS -1].y = 1.0 * unk_base_freq + progdefaults.FMT_freq_corr;
				snprintf(msg1, sizeof(msg1), "%13.3f",
					wf->rfcarrier() + (unk_freq + progdefaults.FMT_freq_corr) * (wf->USB() ? 1 : -1));
				REQ (put_unk_value, msg1);
				snprintf(msg1a, sizeof(msg1a), "%6.1f",
					20.0 * log10( (fmt_unk_amp == 0 ? 1e-6 : fmt_unk_amp) ) );
				REQ (put_unk_amp, msg1a);
				fmt_plot->show_1(true);
			} else {
				unk_pipe[MAX_DATA_PTS -1].y = 0.0;
				REQ (put_unk_value, "");
				REQ (put_unk_amp, "");
				fmt_plot->show_1(false);
			}
			if (btn_ref_enable->value()) {
				ref_pipe[MAX_DATA_PTS -1].y = 1.0 * ref_base_freq + progdefaults.FMT_freq_corr;
				snprintf(msg2, sizeof(msg2), "%13.3f",
					wf->rfcarrier() + (ref_freq + progdefaults.FMT_freq_corr) * (wf->USB() ? 1 : -1));
				REQ (put_ref_value, msg2);
				snprintf(msg2a, sizeof(msg2a), "%6.1f",
					20.0 * log10( (fmt_ref_amp == 0 ? 1e-6 : fmt_ref_amp) ) );
				REQ (put_ref_amp, msg2a);
				fmt_plot->show_2(true);
			} else {
				ref_pipe[MAX_DATA_PTS -1].y = 0.0;
				REQ (put_ref_value, "");
				REQ (put_ref_amp, "");
				fmt_plot->show_2(false);
			}
		}
		REQ (set_fmt_scope);
		dspcnt = DSP_CNT;
	}
	return 0;
}

//=====================================================================
// fmt transmit
//=====================================================================

int fmt::tx_process()
{
	return -1;
}
