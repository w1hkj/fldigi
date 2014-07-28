// ----------------------------------------------------------------------------
// view_cw.cxx
//
// Copyright
//		2008  Dave Freese, W1HKJ
// 		2014  Mauri Niininen, AG1LE
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

// view_cw is a multi channel CW decoder which allows the parallel processing
// of the complete audio spectrum from 200 to 3500 Hz in equal 100 Hz
// channels.  Each channel is separately decoded and the decoded characters
// passed to the user interface routines for presentation.  The number of
// channels can be up to and including 30.

#include <config.h>

#include <stdlib.h>
#include <stdio.h>

#include "bmorse.h"
#include "view_cw.h"

#include "misc.h"
#include "configuration.h"
#include "Viewer.h"
#include "qrunner.h"
#include "status.h"
#include "waterfall.h"
#include "debug.h"

extern waterfall *wf;

view_cw *cwviewer = (view_cw *)0;

view_cw::view_cw(trx_mode cwmode)
{
	int bfv = (int)(CWSampleRate * .010 / DECIMATE); // was .. /DEC_RATIO);

	for (int i = 0; i < MAXCHANNELS; i++) {
		channel[i].cw_FFT_filter = (fftfilt *)0;
		channel[i].cw_FIR_filter = (C_FIR_filter *)0;
		channel[i].bitfilter = new Cmovavg(bfv);
		channel[i].mp = new morse();
	}

	viewmode = MODE_PREV;
	restart(cwmode);
}

view_cw::~view_cw()
{
	for (int i = 0; i < MAXCHANNELS; i++) {
		if (channel[i].cw_FFT_filter) delete channel[i].cw_FFT_filter;
		if (channel[i].cw_FIR_filter) delete channel[i].cw_FIR_filter;
		if (channel[i].bitfilter) delete channel[i].bitfilter;
	}
}

void view_cw::init()
{


	nchannels = progdefaults.VIEWERchannels;
	lowfreq = progdefaults.LowFreqCutoff;

	for (int i = 0; i < MAXCHANNELS; i++) {
		channel[i].phaseacc = 0;
		channel[i].timeout = 0;
		channel[i].agc_peak = 0;
		channel[i].frequency = NULLFREQ;
		channel[i].reset = false;
		channel[i].smpl_ctr =0;
		channel[i].spdhat = 0.;
	}
	for (int i = 0; i < nchannels; i++)
		REQ(&viewclearchannel, i);

	reset_all = false;
}

void view_cw::restart(trx_mode cwmode)
{
	if (viewmode == cwmode) return;
	viewmode = cwmode;


	for (int i = 0; i < MAXCHANNELS; i++) {
		if (channel[i].cw_FFT_filter) delete channel[i].cw_FFT_filter;
		channel[i].cw_FFT_filter = new fftfilt(progdefaults.CWspeed/(1.2 * CWSampleRate), 4096);

		if (channel[i].cw_FIR_filter) delete channel[i].cw_FIR_filter;
		channel[i].cw_FIR_filter = new C_FIR_filter();
		channel[i].cw_FIR_filter->init_lowpass (CW_FIRLEN, DECIMATE, progdefaults.CWspeed/(1.2 * CWSampleRate));
//		channel[i].cw_FIR_filter->init_lowpass (CW_FIRLEN, DEC_RATIO, progdefaults.CWspeed/(1.2 * CWSampleRate));
	}

	bandwidth = progdefaults.CWbandwidth;  // check if need to be changed

	init();
}

//=============================================================================
//========================= view_cw receive routines ==========================
//=============================================================================

bool view_cw::decode_stream(int ch, double value)
{

	char *cptr;
	float  rn,  px, pmax, zout;
	long int  elmhat, xhat,imax;
	double metric;
	char buf[12];

// Compute a variable threshold value for tone detection
// Fast attack and slow decay.
	if (value > channel[ch].agc_peak)
		channel[ch].agc_peak = decayavg(channel[ch].agc_peak, value, 10);	// was 20 ms
	else
		channel[ch].agc_peak = decayavg(channel[ch].agc_peak, value, 800);  // was 800 ms


// normalize if possible
	if (channel[ch].agc_peak) {
		value /= channel[ch].agc_peak;
		value = clamp(value, 0.0, 1.0);
	//	printf("\nagc:%f\tvalue:%f\tnorm:%f", agc_peak, value, value/agc_peak);
		}
	else
		value = 0;

	metric = clamp(channel[ch].agc_peak * 2e3 , 0.0, 100.0);

	if (!progStatus.sqlonoff || metric > progStatus.sldrSquelchValue ) {
		if (progdefaults.CWuseSOMdecoding) {


			channel[ch].mp->noise_(value, &rn, &zout);
			zout = clamp(zout, 0.0, 1.0);
//		if (ch ==1) printf("\n%f",zout);
//printf("\nch[%d]:%f val:%f met:%f",ch,zout,value,metric);
			memset(buf,0,sizeof(buf));

			channel[ch].mp->proces_(zout, rn, &xhat, &px, &elmhat, &channel[ch].spdhat, &imax, &pmax, buf);

			if (channel[ch].spdhat < 0) {
LOG_ERROR("Error in Bayesian decoder: speed %f should not be < 0", channel[ch].spdhat);
				//exit(1);
			}
			cptr = buf;
			while (*cptr != '\0')
				REQ(&viewaddchr, ch, (int)channel[ch].frequency, (int)*cptr++, (int)MODE_CW);

		}
	}
	return TRUE;
}



void view_cw::clearch(int ch)
{
	channel[ch].reset = true;
	channel[ch].state = CW_IDLE;
}

void view_cw::clear()
{
	for (int i = 0; i < nchannels; i++) {
		channel[i].reset = true;
		channel[i].state = CW_IDLE;
	}
}

void view_cw::Metric(int ch)
{
	double bandwidth = 2.0 * (channel[ch].spdhat>0?channel[ch].spdhat:20) / 1.2;
	double np = wf->powerDensity(2000, 1999);
	double sp =	wf->powerDensity(channel[ch].frequency, bandwidth);

	channel[ch].sigpwr = decayavg( channel[ch].sigpwr, sp, sp - channel[ch].sigpwr > 0 ? 2 : 16);
	channel[ch].noisepwr = decayavg( channel[ch].noisepwr, np, 16 );
	channel[ch].metric = CLAMP(channel[ch].sigpwr/channel[ch].noisepwr, 0.0, 100.0);


	if (channel[ch].state == CW_RCVNG)

//printf("\nMetric[%d]:%f frq:%f bw:%f sp:%E np:%E SNR:%Eh sql:%f",ch, channel[ch].metric, channel[ch].frequency, bandwidth,channel[ch].sigpwr,channel[ch].noisepwr,channel[ch].sigpwr/channel[ch].noisepwr,cw_squelch); 

		if (channel[ch].metric < cw_squelch) {
			channel[ch].timeout = progdefaults.VIEWERtimeout * CWSampleRate / WFBLOCKSIZE;
			channel[ch].state = CW_WAITING;
		}
	if (channel[ch].timeout) {
		channel[ch].timeout--;
		if (!channel[ch].timeout) {
			channel[ch].frequency = NULLFREQ;
			channel[ch].metric = 0;
			channel[ch].state = CW_IDLE;
			REQ(&viewclearchannel, ch);
			REQ(&viewaddchr, ch, (int)NULLFREQ, 0, viewmode);
		}
	}
}





void view_cw::found_signal(int frq)
{
	int chf = round(frq/100);					// try to map found signal on corresponding channel 

	for (int ch = chf; ch < chf+2; ch++) {		// check 3 next channels
		if (channel[ch].frequency == frq) break;// we have already found this frequency before

		if (!ch && (channel[ch+1].state == CW_SRCHG || channel[ch+1].state == CW_RCVNG)) break;
		if ((ch == (progdefaults.VIEWERchannels -2)) &&	(channel[ch+1].state == CW_SRCHG || channel[ch+1].state == CW_RCVNG)) break;
		if (ch && (channel[ch-1].state == CW_SRCHG || channel[ch-1].state == CW_RCVNG)) break;
		if (ch > 3 && (channel[ch-2].state == CW_SRCHG || channel[ch-2].state == CW_RCVNG)) break;

		if (channel[ch].state == CW_IDLE) {		// if we found an idle channel - set it in SIGSEARCH mode 
			channel[ch].frequency = frq;
			channel[ch].sigsearch = SIGSEARCH;
			channel[ch].state = CW_SRCHG;
			REQ(&viewaddchr, ch, (int)channel[ch].frequency, 0, viewmode);
		}
	}
}

void view_cw::find_signals()
{
	double cwsquelch = pow(10, progStatus.VIEWER_cwsquelch / 10.0);
	double value;
	int mxpos = 0;
	int lookformax = 1;
	double mn = 1.0/0.0;
	double mx = -1.0/0.0;
	int	flower = lowfreq;
	int fupper = progdefaults.HighFreqCutoff;


	for (int fr=flower; fr< fupper; fr++) {
		  value = wf->Pwr(fr)*1e6;	// scale signal value by 1e6 - not sure if this is only needed in AG1LE system? 
		  if (value > mx) {mx = value; mxpos = fr;}
		  if (value < mn) {mn = value; }

		  if (lookformax){
				if (value < cwsquelch) {
				  mn = value;
				  lookformax = 0;
				};
			} else {
				if (value > cwsquelch) {
				  mx = value; mxpos = fr;
				  found_signal(mxpos);
				  lookformax = 1;
				};
			}
	}

	// remove duplicates less than 20 Hz bandwidth apart
	for (int i = 1; i < progdefaults.VIEWERchannels; i++ )
		if (fabs(channel[i].frequency - channel[i-1].frequency) < 20)
			clearch(i);

}


int view_cw::rx_process(const double *buf, int len)
{
	bool  rx_ok;
	cw_squelch = pow(10, progStatus.VIEWER_cwsquelch / 10.0);

	if (nchannels != progdefaults.VIEWERchannels || lowfreq != progdefaults.LowFreqCutoff)
		init();

// process all channels that have found signal peak
	for (int ch = 0; ch < progdefaults.VIEWERchannels; ch++) {
		if (channel[ch].state == CW_IDLE) continue;
		if (channel[ch].sigsearch) {
			channel[ch].sigsearch--;
			if (!channel[ch].sigsearch)
				channel[ch].state = CW_RCVNG;
				if (progdefaults.CWmfilt) { 		// use matched filter feature
					if (channel[ch].spdhat > 0.) {  // adjust filter to speed we are receiving
						if (channel[ch].cw_FIR_filter) delete channel[ch].cw_FIR_filter;
						channel[ch].cw_FIR_filter = new C_FIR_filter();
						channel[ch].cw_FIR_filter->init_lowpass (CW_FIRLEN, DECIMATE, channel[ch].spdhat/(1.2 * CWSampleRate));
	//printf("\nChannel[%d]:spd%f filter:%f",ch,channel[ch].spdhat,2*channel[ch].spdhat/(1.2 * CWSampleRate));
					}
				}
		}

		for (int ptr = 0; ptr < len; ptr++) {

			cmplx z, *zp;
			int n;
			double FFTvalue;

// Mix with the internal NCO for each channel
			z = cmplx ( buf[ptr] * cos(channel[ch].phaseacc), buf[ptr] * sin(channel[ch].phaseacc) );
			channel[ch].phaseacc += 2.0 * M_PI * channel[ch].frequency / CWSampleRate;
			if (channel[ch].phaseacc > M_PI)
				channel[ch].phaseacc -= TWOPI;
			else if (channel[ch].phaseacc < M_PI)
				channel[ch].phaseacc += TWOPI;

// filter & decimate
			n = channel[ch].cw_FFT_filter->run(z, &zp); // n = 0 or filterlen/2
			if (!n) continue;
			if (n) Metric(ch);

			for (int i = 0; i < n; i++) {
// update the basic sample counter used for morse timing
				channel[ch].smpl_ctr++;

				if (channel[ch].smpl_ctr % DECIMATE) continue; // decimate by 40  (for Bayesian decoder - requires 200 Hz input)
// demodulate
				FFTvalue = abs(zp[i]);
				FFTvalue = channel[ch].bitfilter->run(FFTvalue);
				rx_ok = decode_stream(ch,FFTvalue);
				if ((channel[ch].state == CW_RCVNG) && rx_ok ) {
					if (channel[ch].sigsearch) channel[ch].sigsearch--;
				}
			}
		}
	}

	find_signals();
	return 0;
}

int view_cw::get_freq(int n)
{
	return (int)channel[n].frequency;
}
