// ----------------------------------------------------------------------------
// psm/psm.cxx
//
// Support for Signal Montoring, CSMA, Transmit Inhibit (Busy Detection)
// When enabled effects all transmission types, Keybord, ARQ, and KISS.
//
// Copyright (c) 2016
//      Robert Stiles, KK5VD
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

#include "config.h"

#ifdef __MINGW32__
#  include "compat.h"
#endif

#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <errno.h>
#include <float.h>
#include <cstring>

#include <sys/types.h>
#include <sys/time.h>

#if !defined(__WOE32__) && !defined(__APPLE__)
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif

#include <signal.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Check_Button.H>

#include "main.h"
#include "fl_digi.h"
#include "trx.h"
#include "globals.h"
#include "threads.h"
#include "socket.h"
#include "debug.h"
#include "qrunner.h"
#include "data_io.h"
#include "configuration.h"
#include "status.h"
#include "confdialog.h"
#include "psm/psm.h"
#include "gettext.h"
#include "timeops.h"
#include "kiss_io.h"
#include "xmlrpc.h"
#include "arq_io.h"

#define HISTO_COUNT 256
static int HISTO_THRESH_HOLD = 48;

// In seconds
#define HISTO_RESET_TIME 180
#define HISTO_RESET_TX_TIME_INHIBIT 3

#define DISABLE_TX_INHIBIT_DURATION 5
#define EST_STATE_CHANGE_MS 25

static int histogram[HISTO_COUNT];
//static bool init_hist_flag = true;
static double threshold = 5.0;
static int kpsql_pl = 0;
static double kpsql_threshold = 0.0;
time_t inhibit_tx_seconds = 0;

// Used to scale the sensitivity of PSM
// Values range from 1/(largest int value) to 1/1
#define FGD_DEFAULT 2
static double fractional_gain = (1.0 / (1.0 * FGD_DEFAULT));

static pthread_t       psm_pthread;
static pthread_cond_t  psm_cond;
static pthread_mutex_t psm_mutex;

bool psm_thread_running   = false;
static bool psm_terminate_flag   = false;
static bool psm_thread_exit_flag = false;

static bool request_transmit_flag = false;

// A list of timers
static double timer_tramit_buffer_timeout = 0;
static double timer_slot_time = 0;
static double timer_inhibit_tx_seconds = 0;
static double timer_histrogram_reset_timer = 0;
static double timer_temp_disable_tx_inhibit = 0;
static double timer_sql_timer = 0;

static pthread_mutex_t external_access_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t millisleep_mutex      = PTHREAD_MUTEX_INITIALIZER;

static void update_sql_display(void);
static double detect_signal(int freq, int bw, double *low, double *high);
static void flush_tx_buffer(void);
static void process_psm(void);
static void * psm_loop(void *args);
static inline double current_double_time(void);
static void	psm_millisleep(int delay_time);

bool csma_idling = 0;

/**********************************************************************************
 * Use a local version of MilliSleep()
 **********************************************************************************/
static void	psm_millisleep(int delay_time)
{
	guard_lock _lock(&millisleep_mutex);

	size_t seconds      = 0;
	size_t nano_seconds = 0;

	struct timespec timeout = {0};
	double to_time = current_double_time();

	to_time += (delay_time * 0.001);

	seconds = (size_t) to_time;
	nano_seconds = (size_t) ((to_time - seconds) * 1000000000.0);

	timeout.tv_sec  = seconds;
	timeout.tv_nsec = nano_seconds;

	pthread_cond_timedwait(&psm_cond, &millisleep_mutex, &timeout);
}

/**********************************************************************************
 * Reset Histogram
 **********************************************************************************/
void psm_reset_histogram(void)
{
	guard_lock _lock(&external_access_mutex);

	memset(histogram, 0, sizeof(histogram));
	histogram[3] = 1;
	timer_inhibit_tx_seconds = current_double_time() + HISTO_RESET_TX_TIME_INHIBIT;
}

/**********************************************************************************
 *
 **********************************************************************************/
void update_kpsql_fractional_gain(int value)
{
	guard_lock _lock(&external_access_mutex);

	if(value > 1) {
		progdefaults.kpsql_attenuation = value;
		fractional_gain = 1.0 / ((double) value);
	} else {
		progdefaults.kpsql_attenuation = FGD_DEFAULT;
		fractional_gain = 1.0 / ( 1.0 * FGD_DEFAULT);
	}
}

/**********************************************************************************
 *
 **********************************************************************************/
static void update_sql_display(void)
{
	static int prev_power_level = 0;
	static double convert_scale = (1.0 / ((double)HISTO_COUNT));

	if(progdefaults.show_psm_btn && progStatus.kpsql_enabled) {
		double high_limit = 0;
		double low_limit = 0;

		if(kpsql_pl != prev_power_level) {
			prev_power_level = kpsql_pl;
			high_limit = sldrSquelch->maximum();
			low_limit = sldrSquelch->minimum();
			if(kpsql_pl > HISTO_COUNT) {
				REQ(callback_set_metric, low_limit);
			} else {
				double diff = high_limit - low_limit;
				double scaled_value = kpsql_pl * convert_scale;
				double convert_value = scaled_value * diff;
				double results = high_limit - convert_value;
				REQ(callback_set_metric, results);
			}
		}
	}
}

/**********************************************************************************
 * To deal with the AGC from radios we create a ratio between
 * the high and low signal levels.
 **********************************************************************************/
static double detect_signal(int freq, int bw, double *low, double *high)
{
	int freq_step = 10;
	int freq_pos = 0;
	int start_freq = freq - (bw >> 1);
	int end_freq = freq + (bw >> 1);
	int freq_half_step = freq_step >> 1;
	int i = 0;
	double low_value = FLT_MAX;
	double high_value = FLT_MIN;
	double ratio = 0.0;
	double pd = 0;
	double ratio_avg = 0.0;
	static double pratio0 = 0.0;
	static double pratio1 = 0.0;
	static double pratio2 = 0.0;
	static double pratio3 = 0.0;

	if(trx_state != STATE_RX) return ratio_avg;

	for(i = 0; start_freq <= end_freq; start_freq += freq_step, i++) {
		freq_pos = start_freq + freq_half_step;
		pd = wf->powerDensity((double) freq_pos, (double) freq_step);
		if(pd < low_value)  low_value = pd;
		if(pd > high_value) high_value = pd;
	}

	if(low)  *low = low_value;
	if(high) *high = high_value;

	ratio = high_value/low_value;
	ratio *= fractional_gain;

	kpsql_pl = ratio_avg = (ratio + pratio0 + pratio1 + pratio2 + pratio3) * 0.20;

	if((ratio_avg > 0.0) && (ratio_avg <= (double) HISTO_COUNT)) {
		i = (int) ratio_avg;
		i &= 0xFF;
		histogram[i]++;

		if(histogram[i] > HISTO_THRESH_HOLD) {
			for(i = 0; i < HISTO_COUNT; i++) {
				histogram[i] >>= 1;
			}
			return 0.0;
		}
	}

	pratio3 = pratio2;
	pratio2 = pratio1;
	pratio1 = pratio0;
	pratio0 = ratio;

	return ratio;
}

/**********************************************************************************
 * Clear all transmit buffers (ARQ/KISS/XMLRPC)
 **********************************************************************************/
static void flush_tx_buffer(void)
{
	if(kiss_text_available) {
		flush_kiss_tx_buffer();
		kiss_text_available = false;
	}

	if(arq_text_available) {
		flush_arq_tx_buffer();
		arq_text_available = false;
	}

	if(xmltest_char_available) {
		reset_xmlchars();
		xmltest_char_available = false;
	}
}

/**********************************************************************************
 * Set state for PSM transmit.
 **********************************************************************************/
void psm_transmit(void)
{
	guard_lock extern_lock(&external_access_mutex);
	request_transmit_flag = true;
}

/**********************************************************************************
 * Clear state for PSM transmit.
 **********************************************************************************/
void psm_transmit_ended(int flag)
{
	guard_lock extern_lock(&external_access_mutex);

	if(flag == PSM_ABORT) {
		flush_tx_buffer();
		abort_tx();
	}

	request_transmit_flag = false;
	REQ(set_xmtrcv_selection_color_transmitting);
}

/**********************************************************************************
 * Convert timespec difference to absolute double.
 **********************************************************************************/
#if 0
static double timespec_difference(timespec * ts_a, timespec * ts_b)
{
	if(!ts_a) return 0.0;
	if(!ts_b) return 0.0;

	double a = ts_a->tv_sec + (ts_a->tv_nsec * 0.000000001);
	double b = ts_b->tv_sec + (ts_b->tv_nsec * 0.000000001);

	if(a > b)
		return (a - b);

	return (b - a);
}
#endif // 0

/**********************************************************************************
 * Convert timespec to double.
 **********************************************************************************/
static inline double current_double_time(void)
{
	struct timespec current_timespec_time = {0};
	clock_gettime(CLOCK_REALTIME, &current_timespec_time);
	double a = current_timespec_time.tv_sec + (current_timespec_time.tv_nsec * 0.000000001);
	return a;
}

/**********************************************************************************
 * PSM processing. Sync's with Waterfall Display Update
 **********************************************************************************/
static void process_psm(void)
{
	if (!progdefaults.show_psm_btn) return;
	if (!progStatus.kpsql_enabled)  return;

	guard_lock psm_lock(&psm_mutex);

	bool   detected_signal = false;
	bool   transmit_authorized = true;

	double busyChannelSeconds = 0;
	double current_time = 0;
	double level = 0.0;
	double random_number = 0;

	int    bw = active_modem->get_bandwidth();
	int    bw_margin = progStatus.psm_minimum_bandwidth_margin;
	int    freq = active_modem->get_txfreq();

	static bool   histrogram_reset_timer = true;
	static bool   signal_recorded_flag   = false;
	static double signal_hit_time        = 0;
	static int    delay_time             = 0;

	current_time = current_double_time();

	level = detect_signal(freq, bw + bw_margin, 0, 0);

	if(!progStatus.enableBusyChannel) {
		timer_inhibit_tx_seconds = temp_disable_tx_inhibit = 0;
	}

	// Enabled on valid packet reception. Currently only available
	// to checksum verified protocols (HDLC).

	if(temp_disable_tx_inhibit) {
		timer_temp_disable_tx_inhibit = current_time + DISABLE_TX_INHIBIT_DURATION;
		temp_disable_tx_inhibit = 0;
	}

	random_number = (rand() & 0xFF) * 0.00390625; // Reduce value to 0 - 1.0

	if(current_time < timer_temp_disable_tx_inhibit) {
		busyChannelSeconds = 0.25 + (random_number * 0.75); // 0.25 - 1.0 Seconds
	} else {
		busyChannelSeconds = (double) progStatus.busyChannelSeconds + random_number;
	}

	if(timer_tramit_buffer_timeout == 0.0) {
		timer_tramit_buffer_timeout = current_time + (progStatus.psm_flush_buffer_timeout * 60); // Minutes to Seconds
	}

	// If busy for an extended time flush transmit buffer(s).
	if(progStatus.psm_flush_buffer_timeout) { // If set to zero no buffer flushing allowed.
		if(current_time > timer_tramit_buffer_timeout) {
			timer_tramit_buffer_timeout = current_time + (progStatus.psm_flush_buffer_timeout * 60);
			flush_tx_buffer();
			return;
		}
	}

	if(histrogram_reset_timer) {
		timer_histrogram_reset_timer = current_time + HISTO_RESET_TIME;
		histrogram_reset_timer = false;
	}

	if(current_time > timer_histrogram_reset_timer) {
		psm_reset_histogram();
		timer_histrogram_reset_timer = current_time + HISTO_RESET_TIME;
		timer_inhibit_tx_seconds = current_time + 2.0;  // Time to rebuild the histogram table.
		return;
	}

	// Histogram keeps the threshold 'x' number of units above the noise level.
	if(progStatus.psm_use_histogram) {
		int idx = 0;
		int first_value = 0;
		int offset = progStatus.psm_histogram_offset_threshold;
		int index = 0;
		if(offset > HISTO_COUNT) offset = HISTO_COUNT;

		for(index = 0; index < HISTO_COUNT; index++) {
			if(histogram[index]) {
				if(idx == 0) {
					first_value = index;
				}

				if(idx >= offset) {
					threshold = (double) index;
					break;
				}
				idx++;
			}
		}

		if(index > HISTO_COUNT) {
			threshold = (double) (first_value + offset);
		}
	} else {
		threshold = (int) (progStatus.sldrPwrSquelchValue * 2.56); // Histogram scaled.
	}

	kpsql_threshold = threshold;

	if(level < threshold) {
		detected_signal      = false;
		signal_recorded_flag = false;
	}
	else {
		detected_signal = true;
		if(!signal_recorded_flag) {
			signal_hit_time = current_double_time();
			signal_recorded_flag = true;
		}
	}

	if(progStatus.enableBusyChannel && detected_signal) {
		double signal_hit_time_test = (progStatus.psm_hit_time_window * 0.001);  // Milliseconds to seconds.
		double signal_hit_time_diff = (current_time - signal_hit_time);
		if(signal_hit_time_diff >= signal_hit_time_test) {
			timer_inhibit_tx_seconds = current_time + busyChannelSeconds;
		}
	}

	if(current_time < timer_inhibit_tx_seconds) {
		inhibit_tx_seconds = true;
	} else {
		inhibit_tx_seconds = false;
	}

	// Limit the number of times update_sql_display() is called per second.
	if(current_time > timer_sql_timer) {
		update_sql_display();
		timer_sql_timer = current_time + 0.06; // Eyeball tested value.
	}

	if(inhibit_tx_seconds || !request_transmit_flag ||
	   detected_signal || (current_time < timer_slot_time))
		return;

	delay_time = 0;

	if(progStatus.csma_enabled) {

		int rn_persistance = rand() & 0xFF;

		if(rn_persistance > progStatus.csma_persistance) {
			double _slot_time = ((progdefaults.csma_slot_time * 10) * 0.001);
			timer_slot_time = current_time + _slot_time;
			transmit_authorized = false;
		}

		if(progStatus.csma_transmit_delay > 0) {
			csma_idling = true;
			delay_time = progStatus.csma_transmit_delay * 10;
		}
	}

	if(transmit_authorized && (trx_state == STATE_RX)) {

		REQ(set_xmtrcv_selection_color_transmitting);

		trx_transmit_psm();
		active_modem->set_stopflag(false);

		// Transmit idle time plus START_RX to STATE_TX state change
		// delay.
		if(delay_time > 0) {
			psm_millisleep(delay_time + EST_STATE_CHANGE_MS);
			delay_time = 0;
			csma_idling = false;
		} else {
			psm_millisleep(EST_STATE_CHANGE_MS);
		}

		timer_tramit_buffer_timeout = current_time + (progStatus.psm_flush_buffer_timeout * 60);
		timer_slot_time = current_time + ((progdefaults.csma_slot_time * 10) * 0.001);
		timer_slot_time += (((rand() & 0xFF) * 0.00390625) * 0.20);
	}
}

/**********************************************************************************
 * PSM processing loop. Sync's with Waterfall Display Update
 **********************************************************************************/
static void * psm_loop(void *args)
{
	SET_THREAD_ID(PSM_TID);

	psm_thread_running   = true;
	psm_terminate_flag   = false;
	psm_thread_exit_flag = false;

	while(1) {
		pthread_mutex_lock(&psm_mutex);
		pthread_cond_wait(&psm_cond, &psm_mutex);
		pthread_mutex_unlock(&psm_mutex);

		if (psm_terminate_flag) break;

		if(trx_state == STATE_RX) {
			process_psm();
		}
	}

	psm_thread_exit_flag = true;

	return (void *)0;
}

/**********************************************************************************
 * Start PSM Thread
 **********************************************************************************/
void start_psm_thread(void)
{
	guard_lock extern_lock(&external_access_mutex);
	csma_idling = false;

	if(psm_thread_running) return;

	memset((void *) &psm_pthread, 0, sizeof(psm_pthread));
	memset((void *) &psm_cond,    0, sizeof(psm_cond));
	memset((void *) &psm_mutex,   0, sizeof(psm_mutex));

	if(pthread_cond_init(&psm_cond, NULL)) {
		LOG_ERROR("PSM thread create fail (pthread_cond_init)");
		return;
	}

	if(pthread_mutex_init(&psm_mutex, NULL)) {
		LOG_ERROR("PSM thread create fail (pthread_mutex_init)");
		pthread_cond_destroy(&psm_cond);
		return;
	}

	memset((void *) &psm_pthread, 0, sizeof(psm_pthread));

	if(!psm_thread_running) {
		if (pthread_create(&psm_pthread, NULL, psm_loop, NULL) < 0) {
			pthread_cond_destroy(&psm_cond);
			pthread_mutex_destroy(&psm_mutex);
			LOG_ERROR("PSM thread create fail (pthread_create)");
		}
	}

	MilliSleep(10); // Give the CPU time to set 'psm_thread_running'
}

/**********************************************************************************
 * Stop PSM Thread
 **********************************************************************************/
void stop_psm_thread(void)
{
	guard_lock extern_lock(&external_access_mutex);

	if(!psm_thread_running) return;

	psm_terminate_flag = true;
	pthread_cond_signal(&psm_cond);

	MilliSleep(10);

	if(psm_thread_exit_flag) {
		pthread_join(psm_pthread, NULL);
		LOG_INFO("%s", "psm thread - join");
	} else {
		CANCEL_THREAD(psm_pthread);
		LOG_INFO("%s", "psm thread - cancel");
	}

	pthread_cond_destroy(&psm_cond);
	pthread_mutex_destroy(&psm_mutex);

	memset((void *) &psm_pthread, 0, sizeof(psm_pthread));
	memset((void *) &psm_cond,    0, sizeof(psm_cond));
	memset((void *) &psm_mutex,   0, sizeof(psm_mutex));

	psm_thread_running   = false;
	psm_terminate_flag   = false;
	psm_thread_exit_flag = false;
	csma_idling          = false;

}

/**********************************************************************************
 * Signal PSM to process Waterfall power level information.
 **********************************************************************************/
void signal_psm(void)
{
	if(psm_thread_running) {
		pthread_cond_signal(&psm_cond);
	}
}
