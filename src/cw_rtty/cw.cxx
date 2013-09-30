// ----------------------------------------------------------------------------
// cw.cxx  --  morse code modem
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
//		   (C) Mauri Niininen, AG1LE
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code
// distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
//  Copyright (C) 2004
//  Lawrence Glaister (ve7it@shaw.ca)
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

#include <cstring>
#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include "digiscope.h"
#include "waterfall.h"
#include "fl_digi.h"
#include "fftfilt.h"

#include "cw.h"
#include "misc.h"
#include "configuration.h"
#include "confdialog.h"
#include "status.h"
#include "debug.h"
#include "FTextRXTX.h"

#include "qrunner.h"

using namespace std;

const cw::SOM_TABLE cw::som_table[] = {
	/* Prosigns */
	{'=',	"<BT>",   {1.0,  0.33,  0.33,  0.33, 1.0,   0, 0}	}, // 0
	{'~',	"<AA>",   { 0.33, 1.0,  0.33, 1.0,   0,   0, 0}	}, // 1
	{'%',	"<AS>",   { 0.33, 1.0,  0.33,  0.33,  0.33,   0, 0} 	}, // 2
	{'+',	"<AR>",   { 0.33, 1.0,  0.33, 1.0,  0.33,   0, 0} 	}, // 3
	{'>',	"<SK>",   { 0.33,  0.33,  0.33, 1.0,  0.33, 1.0, 0}	}, // 4
	{'<',	"<KN>",   {1.0,  0.33, 1.0, 1.0,  0.33,   0, 0} 	}, // 5
	{'&',	"<INT>",  { 0.33,  0.33, 1.0,  0.33, 1.0,   0, 0}	}, // 6
	{'}',	"<HM>",   { 0.33,  0.33,  0.33,  0.33, 1.0, 1.0, 0}	}, // 7
	{'{',	"<VE>",   { 0.33,  0.33,  0.33, 1.0,  0.33,   0, 0}	}, // 8
	/* ASCII 7bit letters */
	{'A',	"A",	{ 0.33, 1.0,   0,   0,   0,   0, 0}	},
	{'B',	"B",	{1.0,  0.33,  0.33,  0.33,   0,   0, 0}	},
	{'C',	"C",	{1.0,  0.33, 1.0,  0.33,   0,   0, 0}	},
	{'D',	"D",	{1.0,  0.33,  0.33,   0,   0,   0, 0} 	},
	{'E',	"E",	{ 0.33,   0,   0,   0,   0,   0, 0}	},
	{'F',	"F",	{ 0.33,  0.33, 1.0,  0.33,   0,   0, 0}	},
	{'G',	"G",	{1.0, 1.0,  0.33,   0,   0,   0, 0}	},
	{'H',	"H",	{ 0.33,  0.33,  0.33,  0.33,   0,   0, 0}	},
	{'I',	"I",	{ 0.33,  0.33,   0,   0,   0,   0, 0}	},
	{'J',	"J",	{ 0.33, 1.0, 1.0, 1.0,   0,   0, 0}	},
	{'K',	"K",	{1.0,  0.33, 1.0,   0,   0,   0, 0}	},
	{'L',	"L",	{ 0.33, 1.0,  0.33,  0.33,   0,   0, 0}	},
	{'M',	"M",	{1.0, 1.0,   0,   0,   0,   0, 0}	},
	{'N',	"N",	{1.0,  0.33,   0,   0,   0,   0, 0}	},
	{'O',	"O",	{1.0, 1.0, 1.0,   0,   0,   0, 0}	},
	{'P',	"P",	{ 0.33, 1.0, 1.0,  0.33,   0,   0, 0}	},
	{'Q',	"Q",	{1.0, 1.0,  0.33, 1.0,   0,   0, 0}	},
	{'R',	"R",	{ 0.33, 1.0,  0.33,   0,   0,   0, 0}	},
	{'S',	"S",	{ 0.33,  0.33,  0.33,   0,   0,   0, 0}	},
	{'T',	"T",	{1.0,   0,   0,   0,   0,   0, 0}	},
	{'U',	"U",	{ 0.33,  0.33, 1.0,   0,   0,   0, 0}	},
	{'V',	"V",	{ 0.33,  0.33,  0.33, 1.0,   0,   0, 0}	},
	{'W',	"W",	{ 0.33, 1.0, 1.0,   0,   0,   0, 0}	},
	{'X',	"X",	{1.0,  0.33,  0.33, 1.0,   0,   0, 0}	},
	{'Y',	"Y",	{1.0,  0.33, 1.0, 1.0,   0,   0, 0}	},
	{'Z',	"Z",	{1.0, 1.0,  0.33,  0.33,   0,   0, 0}	},
	/* Numerals */
	{'0',	"0",	{1.0, 1.0, 1.0, 1.0, 1.0,   0, 0}	},
	{'1',	"1",	{ 0.33, 1.0, 1.0, 1.0, 1.0,   0, 0}	},
	{'2',	"2",	{ 0.33,  0.33, 1.0, 1.0, 1.0,   0, 0}	},
	{'3',	"3",	{ 0.33,  0.33,  0.33, 1.0, 1.0,   0, 0}	},
	{'4',	"4",	{ 0.33,  0.33,  0.33,  0.33, 1.0,   0, 0}	},
	{'5',	"5",	{ 0.33,  0.33,  0.33,  0.33,  0.33,   0, 0}	},
	{'6',	"6",	{1.0,  0.33,  0.33,  0.33,  0.33,   0, 0}	},
	{'7',	"7",	{1.0, 1.0,  0.33,  0.33,  0.33,   0, 0}	},
	{'8',	"8",	{1.0, 1.0, 1.0,  0.33,  0.33,   0, 0}	},
	{'9',	"9",	{1.0, 1.0, 1.0, 1.0,  0.33,   0, 0}	},
	/* Punctuation */
	{'\\',	"\\",	{ 0.33, 1.0,  0.33,  0.33, 1.0,  0.33, 0}	},
	{'\'',	"'",	{ 0.33, 1.0, 1.0, 1.0, 1.0,  0.33, 0}	},
	{'$',	"$",	{ 0.33,  0.33,  0.33, 1.0,  0.33,  0.33,1.0}	},
	{'(',	"(",	{1.0,  0.33, 1.0, 1.0,  0.33,   0, 0}	},
	{')',	")",	{1.0,  0.33, 1.0, 1.0,  0.33, 1.0, 0}	},
	{',',	",",	{1.0, 1.0,  0.33,  0.33, 1.0, 1.0, 0}	},
	{'-',	"-",	{1.0,  0.33,  0.33,  0.33,  0.33, 1.0, 0}	},
	{'.',	".",	{ 0.33, 1.0,  0.33, 1.0,  0.33, 1.0, 0}	},
	{'/',	"/",	{1.0,  0.33,  0.33, 1.0,  0.33,   0, 0}	},
	{':',	":",	{1.0, 1.0, 1.0,  0.33,  0.33,  0.33, 0}	},
	{';',	";",	{1.0,  0.33, 1.0,  0.33, 1.0,  0.33, 0}	},
	{'?',	"?",	{ 0.33,  0.33, 1.0, 1.0,  0.33,  0.33, 0}	},
	{'_',	"_",	{ 0.33,  0.33, 1.0, 1.0,  0.33, 1.0, 0}	},
	{'@',	"@",	{ 0.33, 1.0, 1.0,  0.33, 1.0,  0.33, 0}	},
	{'!',	"!",	{1.0,  0.33, 1.0,  0.33, 1.0, 1.0, 0}	},
	{0, NULL, {0.0}}
};

int cw::normalize(float *v, int n, int twodots)
{
	if( n == 0 ) return 0 ;

	float max = v[0];
	float min = v[0];
	int j;

	/* find max and min values */
	for (j=1; j<n; j++) {
		float vj = v[j];
		if (vj > max)	max = vj;
		else if (vj < min)	min = vj;
	}
	/* all values 0 - no need to normalize or decode */
	if (max == 0.0) return 0;

	/* scale values between  [0,1] -- if Max longer than 2 dots it was "dah" and should be 1.0, otherwise it was "dit" and should be 0.33 */
	float ratio = (max > twodots) ? 1.0 : 0.33 ;
	ratio /= max ;
	for (j=0; j<n; j++) v[j] *= ratio;
	return (1);
}


const char * cw::find_winner (float *inbuf, int twodots)
{
	float diffsf = 999999999999.0;

	if ( normalize (inbuf, WGT_SIZE, twodots) == 0) return NULL;

	const SOM_TABLE * winner = NULL;
	for ( const SOM_TABLE * som = som_table; som->chr != 0; som++) {
		 /* Compute the distance between codebook and input entry */
		float difference = 0.0;
	   	for (int i = 0; i < WGT_SIZE; i++) {
			float diff = (inbuf[i] - som->wgt[i]);
					difference += diff * diff;
					if (difference > diffsf) break;
	  		}

	 /* If distance is smaller than previous distances */
			if (difference < diffsf) {
	  			winner = som;
	  			diffsf = difference;
			}
	}

	if (winner != NULL)
		return winner->prt;
	else
		return NULL;
}

void cw::tx_init(SoundBase *sc)
{
	scard = sc;
	phaseacc = 0;
	lastsym = 0;
	qskphase = 0;
}

void cw::rx_init()
{
	cw_receive_state = RS_IDLE;
	smpl_ctr = 0;
	cw_rr_current = 0;
	cw_ptr = 0;
	agc_peak = 0;
	set_scope_mode(Digiscope::SCOPE);

	update_Status();
	usedefaultWPM = false;
	scope_clear = true;
}

void cw::init()
{
	bool wfrev = wf->Reverse();
	bool wfsb = wf->USB();
	reverse = wfrev ^ !wfsb;

	if (progdefaults.StartAtSweetSpot)
		set_freq(progdefaults.CWsweetspot);
	else if (progStatus.carrier != 0) {
		set_freq(progStatus.carrier);
#if !BENCHMARK_MODE
		progStatus.carrier = 0;
#endif
	} else
		set_freq(wf->Carrier());

	trackingfilter->reset();
	cw_adaptive_receive_threshold = (long int)trackingfilter->run(2 * cw_send_dot_length);
	put_cwRcvWPM(cw_send_speed);
	for (int i = 0; i < OUTBUFSIZE; i++)
		outbuf[i] = qskbuf[i] = 0.0;
	rx_init();
	use_paren = progdefaults.CW_use_paren;
	prosigns = progdefaults.CW_prosigns;
	stopflag = false;
}

cw::~cw() {
	if (hilbert) delete hilbert;
	if (cw_FIR_filter) delete cw_FIR_filter;
	if (cw_FFT_filter) delete cw_FFT_filter;
	if (bitfilter) delete bitfilter;
	if (trackingfilter) delete trackingfilter;
}

cw::cw() : modem()
{
	cap |= CAP_BW;

	mode = MODE_CW;
	freqlock = false;
	usedefaultWPM = false;
	frequency = progdefaults.CWsweetspot;
	tx_frequency = get_txfreq_woffset();
	risetime = progdefaults.CWrisetime;
	QSKshape = progdefaults.QSKshape;

	cw_ptr = 0;
	clrcount = CLRCOUNT;

	samplerate = CWSampleRate;
	fragmentsize = CWMaxSymLen;

	cw_speed  = progdefaults.CWspeed;
	bandwidth = progdefaults.CWbandwidth;

	cw_send_speed = cw_speed;
	cw_receive_speed = cw_speed;
	cw_adaptive_receive_threshold = 2 * DOT_MAGIC / cw_speed;
	cw_noise_spike_threshold = cw_adaptive_receive_threshold / 4;
	cw_send_dot_length = DOT_MAGIC / cw_send_speed;
	cw_send_dash_length = 3 * cw_send_dot_length;
	symbollen = (int)(samplerate * 1.2 / progdefaults.CWspeed);
	fsymlen = (int)(samplerate * 1.2 / progdefaults.CWfarnsworth);

	memset(rx_rep_buf, 0, sizeof(rx_rep_buf));

// block of variables that get updated each time speed changes
	pipesize = (22 * samplerate * 12) / (progdefaults.CWspeed * 160);
	if (pipesize < 0) pipesize = 512;
	if (pipesize > MAX_PIPE_SIZE) pipesize = MAX_PIPE_SIZE;

	cwTrack = true;
	phaseacc = 0.0;
	FFTphase = 0.0;
	FIRphase = 0.0;
	FFTvalue = 0.0;
	FIRvalue = 0.0;
	pipeptr = 0;
	clrcount = 0;

	upper_threshold = progdefaults.CWupper;
	lower_threshold = progdefaults.CWlower;
	for (int i = 0; i < MAX_PIPE_SIZE; clearpipe[i++] = 0.0);

	agc_peak = 1.0;
	in_replay = 0;

	use_fft_filter = progdefaults.CWuse_fft_filter;
	use_matched_filter = progdefaults.CWmfilt;

	bandwidth = progdefaults.CWbandwidth;
	if (use_matched_filter)
		progdefaults.CWbandwidth = bandwidth = 2.0 * progdefaults.CWspeed / 1.2;

	hilbert = new C_FIR_filter(); // hilbert transform used by FFT filter
	hilbert->init_hilbert(37, 1);

	cw_FIR_filter = new C_FIR_filter();
	cw_FIR_filter->init_lowpass (CW_FIRLEN, DEC_RATIO, progdefaults.CWspeed/(1.2 * samplerate));

//overlap and add filter length should be a factor of 2
// low pass implementation
	FilterFFTLen = 4096;
	cw_FFT_filter = new fftfilt(progdefaults.CWspeed/(1.2 * samplerate), FilterFFTLen);

// bit filter based on 10 msec rise time of CW waveform
	int bfv = (int)(samplerate * .010 / DEC_RATIO);
	bitfilter = new Cmovavg(bfv);

	trackingfilter = new Cmovavg(TRACKING_FILTER_SIZE);

	makeshape();
	sync_parameters();
	REQ(static_cast<void (waterfall::*)(int)>(&waterfall::Bandwidth), wf, (int)bandwidth);
	REQ(static_cast<int (Fl_Value_Slider2::*)(double)>(&Fl_Value_Slider2::value), sldrCWbandwidth, (int)bandwidth);
	update_Status();
}

// SHOULD ONLY BE CALLED FROM THE rx_processing loop
void cw::reset_rx_filter()
{
	if (use_fft_filter != progdefaults.CWuse_fft_filter ||
		use_matched_filter != progdefaults.CWmfilt ||
		cw_speed != progdefaults.CWspeed ||
		(bandwidth != progdefaults.CWbandwidth && !use_matched_filter)) {

		use_fft_filter = progdefaults.CWuse_fft_filter;
		use_matched_filter = progdefaults.CWmfilt;
		cw_send_speed = cw_speed = progdefaults.CWspeed;

		if (use_matched_filter)
			progdefaults.CWbandwidth = bandwidth = 2.0 * progdefaults.CWspeed / 1.2;
		else
			bandwidth = progdefaults.CWbandwidth;

		if (use_fft_filter) { // FFT filter
			cw_FFT_filter->create_lpf(progdefaults.CWspeed/(1.2 * samplerate));
			FFTphase = 0;
		} else { // FIR filter
			cw_FIR_filter->init_lowpass (CW_FIRLEN, DEC_RATIO, progdefaults.CWspeed/(1.2 * samplerate));
			FIRphase = 0;
		}
		REQ(static_cast<void (waterfall::*)(int)>(&waterfall::Bandwidth),
			wf, (int)bandwidth);
		REQ(static_cast<int (Fl_Value_Slider2::*)(double)>(&Fl_Value_Slider2::value),
			sldrCWbandwidth, (int)bandwidth);

		pipesize = (22 * samplerate * 12) / (progdefaults.CWspeed * 160);
		if (pipesize < 0) pipesize = 512;
		if (pipesize > MAX_PIPE_SIZE) pipesize = MAX_PIPE_SIZE;

		cw_adaptive_receive_threshold = 2 * DOT_MAGIC / cw_speed;
		cw_noise_spike_threshold = cw_adaptive_receive_threshold / 4;
		cw_send_dot_length = DOT_MAGIC / cw_send_speed;
		cw_send_dash_length = 3 * cw_send_dot_length;
		symbollen = (int)(samplerate * 1.2 / progdefaults.CWspeed);
		fsymlen = (int)(samplerate * 1.2 / progdefaults.CWfarnsworth);

		phaseacc = 0.0;
		FFTphase = 0.0;
		FIRphase = 0.0;
		FFTvalue = 0.0;
		FIRvalue = 0.0;
		pipeptr = 0;
		clrcount = 0;
		smpl_ctr = 0;

		memset(rx_rep_buf, 0, sizeof(rx_rep_buf));

		agc_peak = 0;
		clear_syncscope();
	}
	if (lower_threshold != progdefaults.CWlower ||
		upper_threshold != progdefaults.CWupper) {
		lower_threshold = progdefaults.CWlower;
		upper_threshold = progdefaults.CWupper;
		clear_syncscope();
	}
}

// sync_parameters()
// Synchronize the dot, dash, end of element, end of character, and end
// of word timings and ranges to new values of Morse speed, or receive tolerance.

void cw::sync_parameters()
{
	int lowerwpm, upperwpm, nusymbollen, nufsymlen;

	int wpm = usedefaultWPM ? progdefaults.defCWspeed : progdefaults.CWspeed;
	int fwpm = progdefaults.CWfarnsworth;

	cw_send_dot_length = DOT_MAGIC / progdefaults.CWspeed;

	cw_send_dash_length = 3 * cw_send_dot_length;

	nusymbollen = (int)(samplerate * 1.2 / wpm);
	nufsymlen = (int)(samplerate * 1.2 / fwpm);

	if (symbollen != nusymbollen ||
		nufsymlen != fsymlen ||
		risetime  != progdefaults.CWrisetime ||
		QSKshape  != progdefaults.QSKshape ) {
		risetime = progdefaults.CWrisetime;
		QSKshape = progdefaults.QSKshape;
		symbollen = nusymbollen;
		fsymlen = nufsymlen;
		makeshape();
	}

// check if user changed the tracking or the cw default speed
	if ((cwTrack != progdefaults.CWtrack) ||
		(cw_send_speed != progdefaults.CWspeed)) {
		trackingfilter->reset();
		cw_adaptive_receive_threshold = 2 * cw_send_dot_length;
		put_cwRcvWPM(cw_send_speed);
	}
	cwTrack = progdefaults.CWtrack;
	cw_send_speed = progdefaults.CWspeed;

// Receive parameters:
	lowerwpm = cw_send_speed - progdefaults.CWrange;
	upperwpm = cw_send_speed + progdefaults.CWrange;
	if (lowerwpm < progdefaults.CWlowerlimit)
		lowerwpm = progdefaults.CWlowerlimit;
	if (upperwpm > progdefaults.CWupperlimit)
		upperwpm = progdefaults.CWupperlimit;
	cw_lower_limit = 2 * DOT_MAGIC / upperwpm;
	cw_upper_limit = 2 * DOT_MAGIC / lowerwpm;

	if (cwTrack)
		cw_receive_speed = DOT_MAGIC / (cw_adaptive_receive_threshold / 2);
	else {
		cw_receive_speed = cw_send_speed;
		cw_adaptive_receive_threshold = 2 * cw_send_dot_length;
	}

	if (cw_receive_speed > 0)
		cw_receive_dot_length = DOT_MAGIC / cw_receive_speed;
	else
		cw_receive_dot_length = DOT_MAGIC / 5;

	cw_receive_dash_length = 3 * cw_receive_dot_length;

	cw_noise_spike_threshold = cw_receive_dot_length / 2;

}


//=======================================================================
// cw_update_tracking()
// This gets called everytime we have a dot dash sequence or a dash dot
// sequence. Since we have semi validated tone durations, we can try and
// track the cw speed by adjusting the cw_adaptive_receive_threshold variable.
// This is done with moving average filters for both dot & dash.
//=======================================================================

void cw::update_tracking(int idot, int idash)
{
	int dot, dash;
	if (idot > cw_lower_limit && idot < cw_upper_limit)
		dot = idot;
	else
		dot = cw_send_dot_length;
	if (idash > cw_lower_limit && idash < cw_upper_limit)
		dash = idash;
	else
		dash = cw_send_dash_length;

	cw_adaptive_receive_threshold = (long int)trackingfilter->run((dash + dot) / 2);

//	if (!use_matched_filter)
		sync_parameters();
}

//=======================================================================
//update_syncscope()
//Routine called to update the display on the sync scope display.
//For CW this is an o scope pattern that shows the cw data stream.
//=======================================================================

void cw::update_Status()
{
	put_MODEstatus("CW %s Rx %d", usedefaultWPM ? "*" : " ", cw_receive_speed);

}

//=======================================================================
//update_syncscope()
//Routine called to update the display on the sync scope display.
//For CW this is an o scope pattern that shows the cw data stream.
//=======================================================================
//

void cw::update_syncscope()
{
	if (pipesize < 0 || pipesize > MAX_PIPE_SIZE)
		return;

	for (int i = 0; i < pipesize; i++)
		scopedata[i] = 0.96*pipe[i]+0.02;

	set_scope_xaxis_1(progdefaults.CWupper);
	set_scope_xaxis_2(progdefaults.CWlower);

	set_scope(scopedata, pipesize, true);
	scopedata.next(); // change buffers

	clrcount = CLRCOUNT;
	put_cwRcvWPM(cw_receive_speed);
	update_Status();
}

void cw::clear_syncscope()
{
	set_scope_xaxis_1(upper_threshold);
	set_scope_xaxis_2(lower_threshold);
	set_scope(clearpipe, pipesize, false);
	clrcount = CLRCOUNT;
}

cmplx cw::mixer(cmplx in)
{
	cmplx z (cos(phaseacc), sin(phaseacc));
	z = z * in;

	phaseacc += TWOPI * frequency / samplerate;
	if (phaseacc > M_PI)
		phaseacc -= TWOPI;
	else if (phaseacc < M_PI)
		phaseacc += TWOPI;

	return z;
}

//=====================================================================
// cw_rxprocess()
// Called with a block (size SCBLOCKSIZE samples) of audio.
//
//======================================================================

void cw::decode_stream(double value)
{
	const char *c, *somc;
	char *cptr;


// Compute a variable threshold value for tone detection
// Fast attack and slow decay.
	if (value > agc_peak)
		agc_peak = decayavg(agc_peak, value, 20);
	else
		agc_peak = decayavg(agc_peak, value, 800);

	metric = clamp(agc_peak * 2e3 , 0.0, 100.0);

// save correlation amplitude value for the sync scope
// normalize if possible
	if (agc_peak)
		value /= agc_peak;
	else
		value = 0;

	pipe[pipeptr] = value;
	if (++pipeptr == pipesize) pipeptr = 0;

	if (!progStatus.sqlonoff || metric > progStatus.sldrSquelchValue ) {
// Power detection using hysterisis detector
// upward trend means tone starting
		if ((value > progdefaults.CWupper) && (cw_receive_state != RS_IN_TONE)) {
			handle_event(CW_KEYDOWN_EVENT, NULL);
		}
// downward trend means tone stopping
		if ((value < progdefaults.CWlower) && (cw_receive_state == RS_IN_TONE)) {
			handle_event(CW_KEYUP_EVENT, NULL);
		}
	}

	if (handle_event(CW_QUERY_EVENT, &c) == CW_SUCCESS) {
		update_syncscope();
		if (progdefaults.CWuseSOMdecoding) {
			somc = find_winner(cw_buffer, cw_adaptive_receive_threshold);
			cptr = (char*)somc;
			if (somc != NULL) {
				while (*cptr != '\0')
					put_rx_char(progdefaults.rx_lowercase ? tolower(*cptr++) : *cptr++,FTextBase::CTRL);
			}
			if (strlen(c) == 1 && *c == ' ')
				put_rx_char(progdefaults.rx_lowercase ? tolower(*c) : *c);
			cw_ptr = 0;
			memset(cw_buffer, 0, sizeof(cw_buffer));
		} else {
			if (strlen(c) == 1)
				put_rx_char(progdefaults.rx_lowercase ? tolower(*c) : *c);
			else while (*c)
				put_rx_char(progdefaults.rx_lowercase ? tolower(*c++) : *c++, FTextBase::CTRL);
		}
	}
}

void cw::rx_FFTprocess(const double *buf, int len)
{
	cmplx z, *zp;
	int n;

	while (len-- > 0) {

		z = cmplx ( *buf * cos(FFTphase), *buf * sin(FFTphase) );
		FFTphase += TWOPI * frequency / samplerate;
		if (FFTphase > M_PI)
			FFTphase -= TWOPI;
		else if (FFTphase < M_PI)
			FFTphase += TWOPI;

		buf++;

		n = cw_FFT_filter->run(z, &zp); // n = 0 or filterlen/2

		if (!n) continue;

		for (int i = 0; i < n; i++) {
// update the basic sample counter used for morse timing
			++smpl_ctr;

			if (smpl_ctr % DEC_RATIO) continue; // decimate by DEC_RATIO

// demodulate
			FFTvalue = abs(zp[i]);
			FFTvalue = bitfilter->run(FFTvalue);

			decode_stream(FFTvalue);

		} // for (i =0; i < n ...

	} //while (len-- > 0)
}

void cw::rx_FIRprocess(const double *buf, int len)
{
	cmplx z;

	while (len-- > 0) {
		z = cmplx ( *buf * cos(FIRphase), *buf * sin(FIRphase) );
		buf++;

		FIRphase += TWOPI * frequency / samplerate;
		if (FIRphase > M_PI)
			FIRphase -= TWOPI;
		else if (FIRphase < M_PI)
			FIRphase += TWOPI;

		if (cw_FIR_filter->run ( z, z )) {

// update the basic sample counter used for morse timing
			smpl_ctr += DEC_RATIO;
// demodulate
			FIRvalue = abs(z);
			FIRvalue = bitfilter->run(FIRvalue);

			decode_stream(FIRvalue);
		}
	}
}

static bool cwprocessing = false;

int cw::rx_process(const double *buf, int len)
{
	if (cwprocessing) return 0;
	cwprocessing = true;
	reset_rx_filter();

	if (use_fft_filter)
		rx_FFTprocess(buf, len);
	else
		rx_FIRprocess(buf, len);

	if (!clrcount--) clear_syncscope();

	display_metric(metric);
	cwprocessing = false;

	return 0;
}

// ----------------------------------------------------------------------

// Compare two timestamps, and return the difference between them in usecs.

int cw::usec_diff(unsigned int earlier, unsigned int later)
{
// Compare the timestamps.
// At 4 WPM, the dash length is 3*(1200000/4)=900,000 usecs, and
// the word gap is 2,100,000 usecs.
	if (earlier >= later) {
		return 0;
	} else
		return (int) (((double) (later - earlier) * USECS_PER_SEC) / samplerate);
}


//=======================================================================
// handle_event()
//	high level cw decoder... gets called with keyup, keydown, reset and
//	query commands.
//   Keyup/down influences decoding logic.
//	Reset starts everything out fresh.
//	The query command returns CW_SUCCESS and the character that has
//	been decoded (may be '*',' ' or [a-z,0-9] or a few others)
//	If there is no data ready, CW_ERROR is returned.
//=======================================================================

int cw::handle_event(int cw_event, const char **c)
{
	static int space_sent = true;	// for word space logic
	static int last_element = 0;	// length of last dot/dash
	int element_usec;		// Time difference in usecs

	switch (cw_event) {
	case CW_RESET_EVENT:
		sync_parameters();
		cw_receive_state = RS_IDLE;
		cw_rr_current = 0;			// reset decoding pointer
		cw_ptr = 0;
		memset(cw_buffer, 0, sizeof(cw_buffer));
		smpl_ctr = 0;					// reset audio sample counter
		memset(rx_rep_buf, 0, sizeof(rx_rep_buf));
		break;
	case CW_KEYDOWN_EVENT:
// A receive tone start can only happen while we
// are idle, or in the middle of a character.
		if (cw_receive_state == RS_IN_TONE)
			return CW_ERROR;
// first tone in idle state reset audio sample counter
		if (cw_receive_state == RS_IDLE) {
			smpl_ctr = 0;
			memset(rx_rep_buf, 0, sizeof(rx_rep_buf));
			cw_rr_current = 0;
			cw_ptr = 0;
		}
// save the timestamp
		cw_rr_start_timestamp = smpl_ctr;
// Set state to indicate we are inside a tone.
		old_cw_receive_state = cw_receive_state;
		cw_receive_state = RS_IN_TONE;
		return CW_ERROR;
		break;
	case CW_KEYUP_EVENT:
// The receive state is expected to be inside a tone.
		if (cw_receive_state != RS_IN_TONE)
			return CW_ERROR;
// Save the current timestamp
		cw_rr_end_timestamp = smpl_ctr;
		element_usec = usec_diff(cw_rr_start_timestamp, cw_rr_end_timestamp);

// make sure our timing values are up to date
		sync_parameters();
// If the tone length is shorter than any noise cancelling
// threshold that has been set, then ignore this tone.
		if (cw_noise_spike_threshold > 0
			&& element_usec < cw_noise_spike_threshold) {
			cw_receive_state = RS_IDLE;
 			return CW_ERROR;
		}

// Set up to track speed on dot-dash or dash-dot pairs for this test to work, we need a dot dash pair or a
// dash dot pair to validate timing from and force the speed tracking in the right direction. This method
// is fundamentally different than the method in the unix cw project. Great ideas come from staring at the
// screen long enough!. Its kind of simple really ... when you have no idea how fast or slow the cw is...
// the only way to get a threshold is by having both code elements and setting the threshold between them
// knowing that one is supposed to be 3 times longer than the other. with straight key code... this gets
// quite variable, but with most faster cw sent with electronic keyers, this is one relationship that is
// quite reliable. Lawrence Glaister (ve7it@shaw.ca)
		if (last_element > 0) {
// check for dot dash sequence (current should be 3 x last)
			if ((element_usec > 2 * last_element) &&
				(element_usec < 4 * last_element)) {
				update_tracking(last_element, element_usec);
			}
// check for dash dot sequence (last should be 3 x current)
			if ((last_element > 2 * element_usec) &&
				(last_element < 4 * element_usec)) {
				update_tracking(element_usec, last_element);
			}
		}
		last_element = element_usec;
// ok... do we have a dit or a dah?
// a dot is anything shorter than 2 dot times
		if (element_usec <= cw_adaptive_receive_threshold) {
			rx_rep_buf[cw_rr_current++] = CW_DOT_REPRESENTATION;
	//		printf("%d dit ", last_element/1000);  // print dot length
			cw_buffer[cw_ptr++] = (float)last_element;
		} else {
// a dash is anything longer than 2 dot times
			rx_rep_buf[cw_rr_current++] = CW_DASH_REPRESENTATION;
			cw_buffer[cw_ptr++] = (float)last_element;
		}
// We just added a representation to the receive buffer.
// If it's full, then reset everything as it probably noise
		if (cw_rr_current == RECEIVE_CAPACITY - 1) {
			cw_receive_state = RS_IDLE;
			cw_rr_current = 0;	// reset decoding pointer
			cw_ptr = 0;
			smpl_ctr = 0;		// reset audio sample counter
			return CW_ERROR;
		} else {
// zero terminate representation
			rx_rep_buf[cw_rr_current] = 0;
			cw_buffer[cw_ptr] = 0.0;
		}
// All is well.  Move to the more normal after-tone state.
		cw_receive_state = RS_AFTER_TONE;
		return CW_ERROR;
		break;
	case CW_QUERY_EVENT:
// this should be called quite often (faster than inter-character gap) It looks after timing
// key up intervals and determining when a character, a word space, or an error char '*' should be returned.
// CW_SUCCESS is returned when there is a printable character. Nothing to do if we are in a tone
		if (cw_receive_state == RS_IN_TONE)
			return CW_ERROR;
// in this call we expect a pointer to a char to be valid
		if (c == NULL) {
// else we had no place to put character...
			cw_receive_state = RS_IDLE;
			cw_rr_current = 0;
			cw_ptr = 0;
// reset decoding pointer
			return CW_ERROR;
		}
// compute length of silence so far
		sync_parameters();
		element_usec = usec_diff(cw_rr_end_timestamp, smpl_ctr);
// SHORT time since keyup... nothing to do yet
		if (element_usec < (2 * cw_receive_dot_length))
			return CW_ERROR;
// MEDIUM time since keyup... check for character space
// one shot through this code via receive state logic
// FARNSWOTH MOD HERE -->
		if (element_usec >= (2 * cw_receive_dot_length) &&
			element_usec <= (4 * cw_receive_dot_length) &&
			cw_receive_state == RS_AFTER_TONE) {
// Look up the representation
//cout << "CW_QUERY medium time after keyup: " << rx_rep_buf;
			*c = morse.rx_lookup(rx_rep_buf);
//cout <<": " << *c <<flush;
			if (*c == NULL) {
// invalid decode... let user see error
				*c = "*";
			}
			cw_receive_state = RS_IDLE;
			cw_rr_current = 0;	// reset decoding pointer
			space_sent = false;
			cw_ptr = 0;

			return CW_SUCCESS;
		}
// LONG time since keyup... check for a word space
// FARNSWOTH MOD HERE -->
		if ((element_usec > (4 * cw_receive_dot_length)) && !space_sent) {
			*c = " ";
			space_sent = true;
			return CW_SUCCESS;
		}
// should never get here... catch all
		return CW_ERROR;
		break;
	}
// should never get here... catch all
	return CW_ERROR;
}

//===========================================================================
// cw transmit routines
// Define the amplitude envelop for key down events (32 samples long)
// this is 1/2 cycle of a raised cosine
//===========================================================================

double keyshape[KNUM];

void cw::makeshape()
{
	for (int i = 0; i < KNUM; i++) keyshape[i] = 1.0;
	knum = (int)(8 * risetime);

	if (knum >= symbollen)
		knum = symbollen - 1;

	if (knum > KNUM)
		knum = KNUM;

	switch (QSKshape) {
		case 1: // blackman
			for (int i = 0; i < knum; i++)
				keyshape[i] = (0.42 - 0.50 * cos(M_PI * i/ knum) + 0.08 * cos(2 * M_PI * i / knum));
			break;
		case 0: // raised cosine (hanning)
		default:
			for (int i = 0; i < knum; i++)
				keyshape[i] = 0.5 * (1.0 - cos (M_PI * i / knum));
	}
}

inline double cw::nco(double freq)
{
	phaseacc += 2.0 * M_PI * freq / samplerate;

	if (phaseacc > M_PI)
		phaseacc -= 2.0 * M_PI;

	return sin(phaseacc);
}

inline double cw::qsknco()
{
	qskphase += 2.0 * M_PI * 1000 / samplerate;

	if (qskphase > M_PI)
		qskphase -= 2.0 * M_PI;

	return sin(qskphase);
}

//=====================================================================
// send_symbol()
// Sends a part of a morse character (one dot duration) of either
// sound at the correct freq or silence. Rise and fall time is controlled
// with a raised cosine shape.
//
// Left channel contains the shaped A2 CW waveform
// Right channel contains a square wave burst of 1600 Hz that is used
// to trigger a qsk switch.  Right channel has pre and post timings for
// proper switching of the qsk switch before and after the A2 element.
// If the Pre + Post timing exceeds the interelement spacing then the
// Pre and / or Post is only applied at the beginning and end of the
// character.
//=======================================================================

int q_carryover = 0, carryover = 0;

void cw::send_symbol(int bits, int len)
{
	double freq;
	int sample, qsample, i;
	int delta = 0;
	int keydown;
	int keyup;
	int kpre;
	int kpost;
	int duration = 0;
	int symlen = 0;
	float dsymlen = 0.0;
	int currsym = bits & 1;

	freq = get_txfreq_woffset();

	delta = (int) (len * (progdefaults.CWweight - 50) / 100.0);

	symlen = len;
	if (currsym == 1) {
   		dsymlen = len * (progdefaults.CWdash2dot - 3.0) / (progdefaults.CWdash2dot + 1.0);
		if (lastsym == 1 && currsym == 1)
			symlen += (int)(3 * dsymlen);
		else
			symlen -= (int)dsymlen;
	}

	if (delta < -(symlen - knum)) delta = -(symlen - knum);
	if (delta > (symlen - knum)) delta = symlen - knum;

	keydown = symlen + delta ;
	keyup = symlen - delta;

	kpre = (int)(progdefaults.CWpre * 8);
	if (kpre > symlen) kpre = symlen;
	if (progdefaults.QSK) {
		kpre = (int)(progdefaults.CWpre * 8);
		if (kpre > symlen) kpre = symlen;

		if (progdefaults.CWnarrow) {
			if (keydown - 2*knum < 0)
				kpost = knum + (int)(progdefaults.CWpost * 8);
			else
				kpost = keydown - knum + (int)(progdefaults.CWpost * 8);
		} else
			kpost = keydown + (int)(progdefaults.CWpost * 8);
			if (kpost < 0) kpost = 0;
		} else {
			kpre = 0;
			kpost = 0;
	}

	if (firstelement) {
		firstelement = false;
		return;
	}

	if (currsym == 1) { // keydown
		sample = 0;
		if (lastsym == 1) {
			for (i = 0; i < keydown; i++, sample++) {
				outbuf[sample] = nco(freq);
				qskbuf[sample] = qsknco();
			}
			duration = keydown;
		} else {
			if (carryover) {
				for (int i = carryover; i < knum; i++, sample++)
					outbuf[sample] = nco(freq) * keyshape[knum - i];
				while (sample < kpre)
					outbuf[sample++] = 0 * nco(freq);
			} else
				for (int i = 0; i < kpre; i++, sample++)
					outbuf[sample] = 0 * nco(freq);
			sample = 0;
			for (int i = 0; i < kpre; i++, sample++) {
				qskbuf[sample] = qsknco();
			}
			for (int i = 0; i < knum; i++, sample++) {
				outbuf[sample] = nco(freq) * keyshape[i];
				qskbuf[sample] = qsknco();
			}
			duration = kpre + knum;
		}
		carryover = 0;
	}
	else { // keyup
		if (lastsym == 0) {
			duration = keyup;
			sample = 0;
			if (carryover) {
				for (int i = carryover; i < knum; i++, sample++)
					outbuf[sample] = nco(freq) * keyshape[knum - i];
				while (sample < duration)
					outbuf[sample++] = 0 * nco(freq);
			} else
				while (sample < duration)
					outbuf[sample++] = 0 * nco(freq);
			carryover = 0;

			qsample = 0;
			if (q_carryover) {
				for (int i = 0; i < q_carryover; i++, qsample++) {
					qskbuf[qsample] = qsknco();
				}
				while (qsample < duration)
					qskbuf[qsample++] = 0 * qsknco();
			} else
				while (qsample < duration)
					qskbuf[qsample++] = 0 * qsknco();
			if (q_carryover > duration)
				q_carryover = duration - q_carryover;
			else
				q_carryover = 0;

		} else { // last symbol = 1
			duration = 2 * len - kpre - knum;
			carryover = 0;
			sample = 0;

			int next = keydown - knum;
			if (progdefaults.CWnarrow)
				next = keydown - 2*knum;

			for (int i = 0; i < next; i++, sample++)
				outbuf[sample] = nco(freq);

			for (int i = 0; i < knum; i++, sample++) {
				if (sample == duration) {
					carryover = i;
					break;
				}
				outbuf[sample] = nco(freq) * keyshape[knum - i];
			}
			while (sample < duration)
				outbuf[sample++] = 0 * nco(freq);

			q_carryover = 0;
			qsample = 0;

			for (int i = 0; i < kpost; i++, qsample++) {
				if (qsample == duration) {
					q_carryover = kpost - duration;
					break;
				}
				qskbuf[qsample] = qsknco();
			}
			while (qsample < duration)
				qskbuf[qsample++] = 0 * qsknco();
		}
	}

	if (duration > 0) {
		if (progdefaults.QSK)
			ModulateStereo(outbuf, qskbuf, duration);
		else
			ModulateXmtr(outbuf, duration);
	}

	lastsym = currsym;
	firstelement = false;
}

//=====================================================================
// send_ch()
// sends a morse character and the space afterwards
//=======================================================================

void cw::send_ch(int ch)
{
	int code;
	int chout = ch;
	int flen;

	sync_parameters();
// handle word space separately (7 dots spacing)
// last char already had 3 elements of inter-character spacing

	if ((chout == ' ') || (chout == '\n')) {
		firstelement = false;
		flen = 4 * symbollen;
		while (flen - symbollen > 0) {
			send_symbol(0, symbollen);
			flen -= symbollen;
		}
		if (flen) send_symbol(0, flen);
		put_echo_char(progdefaults.rx_lowercase ? tolower(ch) : ch);
		return;
	}

// convert character code to a morse representation
	if ((chout < 256) && (chout >= 0)) {
		code = morse.tx_lookup(chout); //cw_tx_lookup(ch);
		firstelement = true;
	} else {
		code = 0x04; 	// two extra dot spaces
		firstelement = false;
	}

// loop sending out binary bits of cw character
// at WPM or Farnsworth rate
	if (progdefaults.CWusefarnsworth && (progdefaults.CWspeed <= progdefaults.CWfarnsworth))
		flen = fsymlen;
	else
		flen = symbollen;

	int charlen = 0;
	while (code > 1) {
		send_symbol(code, flen);
		charlen++;
		code = code >> 1;
	}

// inter character space at WPM/FWPM rate
	flen = symbollen;
	if (progdefaults.CWusefarnsworth && (progdefaults.CWspeed <= progdefaults.CWfarnsworth))
		flen += (symbollen - fsymlen)*charlen;

	while(flen - symbollen > 0) {
		send_symbol(0, symbollen);
		flen -= symbollen;
	}
	if (flen) send_symbol(0, flen);

	FL_AWAKE();

	if (ch != -1) {
		string prtstr = morse.tx_print(ch);
		if (prtstr.length() == 1)
			put_echo_char(progdefaults.rx_lowercase ? tolower(prtstr[0]) : prtstr[0]);
		else
			for (size_t n = 0; n < prtstr.length(); n++)
				put_echo_char(progdefaults.rx_lowercase ? tolower(prtstr[n]) : prtstr[n], FTextBase::CTRL);
	}
}

//=====================================================================
// cw_txprocess()
// Read characters from screen and send them out the sound card.
// This is called repeatedly from a thread during tx.
//=======================================================================

int cw::tx_process()
{
	int c;

	if (use_paren != progdefaults.CW_use_paren ||
		prosigns != progdefaults.CW_prosigns) {
		use_paren = progdefaults.CW_use_paren;
		prosigns = progdefaults.CW_prosigns;
		morse.init();
	}

	c = get_tx_char();
	if (c == GET_TX_CHAR_ETX || stopflag) {
		send_symbol(0, symbollen);
		stopflag = false;
			return -1;
	}
	send_ch(c);
	return 0;
}

void cw::incWPM()
{

	if (usedefaultWPM) return;
	if (progdefaults.CWspeed < progdefaults.CWupperlimit) {
		progdefaults.CWspeed++;
		sync_parameters();
		set_CWwpm();
		update_Status();
	}
}

void cw::decWPM()
{

	if (usedefaultWPM) return;
	if (progdefaults.CWspeed > progdefaults.CWlowerlimit) {
		progdefaults.CWspeed--;
		set_CWwpm();
		sync_parameters();
		update_Status();
	}
}

void cw::toggleWPM()
{
	usedefaultWPM = !usedefaultWPM;
	sync_parameters();
	update_Status();
}

