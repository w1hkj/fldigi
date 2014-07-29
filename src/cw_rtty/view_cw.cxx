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

	bandwidth = 20;  // check if need to be changed

	init();
}

//=============================================================================
//========================= view_cw receive routines ==========================
//=============================================================================

void view_cw::decode_stream(int ch, double value)
{

	char *cptr;
	float  rn,  px,  spdhat, pmax, zout;
	long int  elmhat, xhat,imax;
	double metric;
	int cw_receive_speed;
	char buf[12];

// Compute a variable threshold value for tone detection
// Fast attack and slow decay.
	if (value > channel[ch].agc_peak)
		channel[ch].agc_peak = decayavg(channel[ch].agc_peak, value, 10);	// was 20 ms
	else
		channel[ch].agc_peak = decayavg(channel[ch].agc_peak, value, 800);  // was 800 ms


// save correlation amplitude value for the sync scope
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

			channel[ch].mp->proces_(zout, rn, &xhat, &px, &elmhat, &spdhat, &imax, &pmax, buf);

//printf("\nch[%d].spdhat:%f",ch,spdhat);
			cw_receive_speed = spdhat;

			if (cw_receive_speed < 0) {
LOG_ERROR("Error in Bayesian decoder: speed %f should not be < 0", spdhat);
				//exit(1);
			}
			cptr = buf;
			while (*cptr != '\0')
				REQ(&viewaddchr, ch, (int)channel[ch].frequency, (int)*cptr++, (int)MODE_CW);

		}
	}

}



void view_cw::clearch(int n)
{
	channel[n].reset = true;

}

void view_cw::clear()
{
	struct PEAKS pks;

	for (int i = 0; i < nchannels; i++)
		channel[i].reset = true;

	findsignals(&pks);
//	for (int i = 0; i < pks.mxcount; i++)
//		printf("\nmx[%d]frq[%d] val:%f",i,pks.mxpos[i],pks.mx[i]);
}

inline void view_cw::timeout_check()
{
	for (int ch = 0; ch < nchannels; ch++) {
		if (channel[ch].timeout) channel[ch].timeout--;
		if (channel[ch].frequency == NULLFREQ) continue;
		if (channel[ch].reset || (!channel[ch].timeout) ||
			(ch && (fabs(channel[ch-1].frequency - channel[ch].frequency) < bandwidth))) {
			channel[ch].reset = false;
			channel[ch].frequency = NULLFREQ;
			REQ(&viewclearchannel, ch);
			REQ(&viewaddchr, ch, (int)NULLFREQ, 0, viewmode);
		}
	}
}


void view_cw::findsignals(struct PEAKS *p)
{

	double delta = progStatus.VIEWER_cwsquelch; //pow(10, progStatus.VIEWER_rttysquelch / 10.0);
	double value, mn,mx;
	int i, lookformax, mnpos,mxpos;
	int flower, fupper;
	

	flower = lowfreq; 		// 0 Hz min 
	fupper = progdefaults.HighFreqCutoff; 			//IMAGE_WIDTH =  4000 Hz max 
	p->mxcount = 0;
	p->mncount = 0;


	mn = 1.0/0.0;
	mx = -1.0/0.0;
	mnpos = -10000; mxpos = 10000;
	lookformax = 1;

	for (i=flower; i< fupper; i++) {
		  value = (20*log10(wf->Pwr(i)+1e-12) + 100);


		  if (value > mx) {mx = value; mxpos = i;}
		  if (value < mn) {mn = value; mnpos = i;}
		  if (p->mxcount > PEAKS_SIZE-1) break;
		  if (p->mncount > PEAKS_SIZE-1) break;

		  if (lookformax){
				if (value < delta) {
				  p->mx[p->mxcount] = mx;
				  p->mxpos[p->mxcount] = mxpos;
				  p->mxcount += 1;
				  mn = value; mnpos = i;
				  lookformax = 0;
				};
			} else {
				if (value > delta) {
				  p->mn[p->mncount] = mn;
				  p->mnpos[p->mncount] = mnpos;
				  p->mncount += 1;
				  mx = value; mxpos = i;
				  lookformax = 1;
				};
			}
	}
}


int view_cw::rx_process(const double *buf, int len)
{

	struct PEAKS pks;

	if (nchannels != progdefaults.VIEWERchannels || lowfreq != progdefaults.LowFreqCutoff)
		init();

// process all channels that have found signal peak
	for (int ch = 0; ch < nchannels; ch++) {
		if (channel[ch].frequency == NULLFREQ) continue;

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
			for (int i = 0; i < n; i++) {
// update the basic sample counter used for morse timing
				channel[ch].smpl_ctr++;

				if (channel[ch].smpl_ctr % DECIMATE) continue; // decimate by 40  (for Bayesian decoder - requires 200 Hz input)
// demodulate
				FFTvalue = abs(zp[i]);
				FFTvalue = channel[ch].bitfilter->run(FFTvalue);
				decode_stream(ch,FFTvalue);
			}
		}
	}

// find signal peaks
	findsignals(&pks);

//  store found signal peaks that exceed cw_squelch value to channels 0...N
	double delta = progStatus.VIEWER_cwsquelch;
	for (int i = 0; i < pks.mxcount; i++) {
		if (pks.mx[i] > delta) {
			// assumption: signals < 100 Hz apart are stored on same channel (maybe a problem?)
			int frq = pks.mxpos[i];
			int ch = round(frq/100);
			if ((channel[ch].frequency == NULLFREQ)&& (ch < nchannels) ){
				channel[ch].frequency = (double)frq;
				channel[ch].timeout = progdefaults.VIEWERtimeout*8000;
			}
		}

	}
// remove signals too close to each others - min separation  20 Hz 
	for (int i = 1; i < progdefaults.VIEWERchannels; i++ )
		if (fabs(channel[i].frequency - channel[i-1].frequency) < 20)
			clearch(i);

	timeout_check();
	return 0;
}

int view_cw::get_freq(int n)
{
	return (int)channel[n].frequency;
}
