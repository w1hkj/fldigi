// ----------------------------------------------------------------------------
// rtty.cxx  --  RTTY modem
//
// Copyright (C) 2012
//		Dave Freese, W1HKJ
//		Stefan Fendt, DL1SMF
//
// This file is part of fldigi.
//
// This code bears some resemblance to code contained in gmfsk from which
// it originated.  Much has been changed, but credit should still be
// given to Tomi Manninen (oh2bns@sral.fi), who so graciously distributed
// his gmfsk modem under the GPL.
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
#include <iostream>
#include <fstream>

using namespace std;

//#include "rtty.h"
#include "view_rtty.h"
#include "fl_digi.h"
#include "digiscope.h"
#include "misc.h"
#include "waterfall.h"
#include "confdialog.h"
#include "configuration.h"
#include "status.h"
#include "digiscope.h"
#include "trx.h"
#include "debug.h"
#include "synop.h"
#include "main.h"
#include "modem.h"

#include "rtty.h"

#define FILTER_DEBUG 0

#define SHAPER_BAUD 150

//=====================================================================
// Baudot support
//=====================================================================

static char letters[32] = {
	'\0',	'E',	'\n',	'A',	' ',	'S',	'I',	'U',
	'\r',	'D',	'R',	'J',	'N',	'F',	'C',	'K',
	'T',	'Z',	'L',	'W',	'H',	'Y',	'P',	'Q',
	'O',	'B',	'G',	' ',	'M',	'X',	'V',	' '
};

/*
 * U.S. version of the figures case.
 */
static char figures[32] = {
	'\0',	'3',	'\n',	'-',	' ',	'\a',	'8',	'7',
	'\r',	'$',	'4',	'\'',	',',	'!',	':',	'(',
	'5',	'"',	')',	'2',	'#',	'6',	'0',	'1',
	'9',	'?',	'&',	' ',	'.',	'/',	';',	' '
};

int dspcnt = 0;

static char msg1[20];

const double	rtty::SHIFT[] = {23, 85, 160, 170, 182, 200, 240, 350, 425, 850};
// FILTLEN must be same size as BAUD
const double	rtty::BAUD[]  = {45, 45.45, 50, 56, 75, 100, 110, 150, 200, 300};
const int		rtty::FILTLEN[] = { 512, 512, 512, 512, 512, 512, 512, 256, 128, 64};
const int		rtty::BITS[]  = {5, 7, 8};
const int		rtty::numshifts = (int)(sizeof(SHIFT) / sizeof(*SHIFT));
const int		rtty::numbauds = (int)(sizeof(BAUD) / sizeof(*BAUD));

void rtty::tx_init(SoundBase *sc)
{
	scard = sc;
	phaseacc = 0;
	preamble = true;
	videoText();

	symbols = 0;
	acc_symbols = 0;
	ovhd_symbols = 0;
}

// Customizes output of Synop decoded data.
struct rtty_callback : public synop_callback {
	// Callback for writing decoded synop messages.
	void print(const char * str, size_t nb, bool bold ) const {
		// Could choose: FTextBase::CTRL,XMIT,RECV
		int style = bold ? FTextBase::XMIT : FTextBase::RECV;
		for( size_t i = 0; i < nb; ++i ) {
			unsigned char c = str[i];
			put_rx_char(progdefaults.rx_lowercase ? tolower(c) : c, style );
		}
	}
	// Should we log new Synop messages to the current Adif log file ?
	bool log_adif(void) const { return progdefaults.SynopAdifDecoding ;}
	// Should we log new Synop messages to KML file ?
	bool log_kml(void) const { return progdefaults.SynopKmlDecoding ;}

	bool interleaved(void) const { return progdefaults.SynopInterleaved ;}
};

void rtty::rx_init()
{
	rxstate = RTTY_RX_STATE_IDLE;
	rxmode = LETTERS;
	phaseacc = 0;
	FSKphaseacc = 0;

	for (int i = 0; i < MAXBITS; i++ ) bit_buf[i] = 0.0;

	mark_phase = 0;
	space_phase = 0;
	xy_phase = 0.0;

	mark_mag = 0;
	space_mag = 0;
	mark_env = 0;
	space_env = 0;

	inp_ptr = 0;

	lastchar = 0;

	// Synop file is reloaded each time we enter this modem. Ideally do that when the file is changed.
	static bool wmo_loaded = false ;
	if( wmo_loaded == false ) {
		wmo_loaded = true ;
		SynopDB::Init(PKGDATADIR);
	}
	/// Used by weather reports decoding.
	synop::setup<rtty_callback>();
	synop::instance()->init();
}

void rtty::init()
{
	bool wfrev = wf->Reverse();
	bool wfsb = wf->USB();
	// Probably not necessary because similar to modem::set_reverse
	reverse = wfrev ^ !wfsb;
	stopflag = false;

	if (progdefaults.StartAtSweetSpot)
		set_freq(progdefaults.RTTYsweetspot);
	else if (progStatus.carrier != 0) {
		set_freq(progStatus.carrier);
#if !BENCHMARK_MODE
		progStatus.carrier = 0;
#endif
	} else
		set_freq(wf->Carrier());

	rx_init();
	put_MODEstatus(mode);
	if ((rtty_baud - (int)rtty_baud) == 0)
		snprintf(msg1, sizeof(msg1), "%-3.0f/%-4.0f", rtty_baud, rtty_shift);
	else
		snprintf(msg1, sizeof(msg1), "%-4.2f/%-4.0f", rtty_baud, rtty_shift);
	put_Status1(msg1);
	if (progdefaults.PreferXhairScope)
		set_scope_mode(Digiscope::XHAIRS);
	else
		set_scope_mode(Digiscope::RTTY);
	for (int i = 0; i < MAXPIPE; i++) mark_history[i] = space_history[i] = cmplx(0,0);

	lastchar = 0;
}

rtty::~rtty()
{
	if (rttyviewer) delete rttyviewer;

	if (mark_filt) delete mark_filt;
	if (space_filt) delete space_filt;
	if (pipe) delete [] pipe;
	if (dsppipe) delete [] dsppipe;
	if (bits) delete bits;
	delete m_Osc1;
	delete m_Osc2;
	delete m_SymShaper1;
	delete m_SymShaper2;
}

void rtty::reset_filters()
{
	delete mark_filt;
	mark_filt = new fftfilt(rtty_baud/samplerate, filter_length);
	mark_filt->rtty_filter(rtty_baud/samplerate);
	delete space_filt;
	space_filt = new fftfilt(rtty_baud/samplerate, filter_length);
	space_filt->rtty_filter(rtty_baud/samplerate);
}

void rtty::restart()
{
	double stl;

	rtty_shift = shift = (progdefaults.rtty_shift < numshifts ?
				  SHIFT[progdefaults.rtty_shift] : progdefaults.rtty_custom_shift);
	if (progdefaults.rtty_baud > numbauds - 1) progdefaults.rtty_baud = numbauds - 1;
	rtty_baud = BAUD[progdefaults.rtty_baud];
	filter_length = FILTLEN[progdefaults.rtty_baud];

	nbits = rtty_bits = BITS[progdefaults.rtty_bits];
	if (rtty_bits == 5)
		rtty_parity = RTTY_PARITY_NONE;
	else
		switch (progdefaults.rtty_parity) {
			case 0 : rtty_parity = RTTY_PARITY_NONE; break;
			case 1 : rtty_parity = RTTY_PARITY_EVEN; break;
			case 2 : rtty_parity = RTTY_PARITY_ODD; break;
			case 3 : rtty_parity = RTTY_PARITY_ZERO; break;
			case 4 : rtty_parity = RTTY_PARITY_ONE; break;
			default : rtty_parity = RTTY_PARITY_NONE; break;
		}
	// (exists below already)  rtty_stop = progdefaults.rtty_stop;

	txmode = LETTERS;
	rxmode = LETTERS;
	symbollen = (int) (samplerate / rtty_baud + 0.5);
	set_bandwidth(shift);

	rtty_BW = progdefaults.RTTY_BW = rtty_baud * 2;

	wf->redraw_marker();

	reset_filters();

	if (bits)
		bits->setLength(symbollen / 8);//2);
	else
		bits = new Cmovavg(symbollen / 8);//2);
	mark_noise = space_noise = 0;
	bit = nubit = true;

// stop length = 1, 1.5 or 2 bits
	rtty_stop = progdefaults.rtty_stop;
	if (rtty_stop == 0) stl = 1.0;
	else if (rtty_stop == 1) stl = 1.5;
	else stl = 2.0;
	stoplen = (int) (stl * samplerate / rtty_baud + 0.5);
	freqerr = 0.0;
	pipeptr = 0;

	for (int i = 0; i < MAXBITS; i++ ) bit_buf[i] = 0.0;

	metric = 0.0;

	if ((rtty_baud - (int)rtty_baud) == 0)
		snprintf(msg1, sizeof(msg1), "%-3.0f/%-4.0f", rtty_baud, rtty_shift);
	else
		snprintf(msg1, sizeof(msg1), "%-4.2f/%-4.0f", rtty_baud, rtty_shift);
	put_Status1(msg1);
	put_MODEstatus(mode);
	for (int i = 0; i < MAXPIPE; i++)
		QI[i] = cmplx(0.0, 0.0);
	sigpwr = 0.0;
	noisepwr = 0.0;
	sigsearch = 0;
	dspcnt = 2*(nbits + 2);

	clear_zdata = true;

	// restart symbol-rtty_shaper
	m_SymShaper1->Preset(rtty_baud, samplerate);
	m_SymShaper2->Preset(rtty_baud, samplerate);

	mark_phase = 0;
	space_phase = 0;
	xy_phase = 0.0;

	mark_mag = 0;
	space_mag = 0;
	mark_env = 0;
	space_env = 0;

	inp_ptr = 0;

	for (int i = 0; i < MAXPIPE; i++) mark_history[i] = space_history[i] = cmplx(0,0);

//	if (::rttyviewer) ::rttyviewer->restart();
	if (rttyviewer) rttyviewer->restart();

	progStatus.rtty_filter_changed = false;

}

rtty::rtty(trx_mode tty_mode)
{
	cap |= CAP_AFC | CAP_REV;

	mode = tty_mode;

	samplerate = RTTY_SampleRate;

	mark_filt = (fftfilt *)0;
	space_filt = (fftfilt *)0;

	bits = (Cmovavg *)0;

	pipe = new double[MAXPIPE];
	dsppipe = new double [MAXPIPE];

//	if (::rttyviewer == 0) ::rttyviewer = new view_rtty(mode);

	rttyviewer = new view_rtty(mode);

	m_Osc1 = new Oscillator( samplerate );
	m_Osc2 = new Oscillator( samplerate );

	m_SymShaper1 = new SymbolShaper( 45, samplerate );
	m_SymShaper2 = new SymbolShaper( 45, samplerate );

	restart();

}

void rtty::Update_syncscope()
{
	int j;
	for (int i = 0; i < symbollen; i++) {
		j = pipeptr - i;
		if (j < 0) j += symbollen;
		dsppipe[i] = pipe[j];
	}
	set_scope(dsppipe, symbollen, false);
}

void rtty::Clear_syncscope()
{
	set_scope(0, 0, false);
}

cmplx rtty::mixer(double &phase, double f, cmplx in)
{
	cmplx z = cmplx( cos(phase), sin(phase)) * in;

	phase -= TWOPI * f / samplerate;
	if (phase < -TWOPI) phase += TWOPI;

	return z;
}

unsigned char rtty::Bit_reverse(unsigned char in, int n)
{
	unsigned char out = 0;

	for (int i = 0; i < n; i++)
		out = (out << 1) | ((in >> i) & 1);

	return out;
}

static int rparity(int c)
{
	int w = c;
	int p = 0;
	while (w) {
		p += (w & 1);
		w >>= 1;
	}
	return p & 1;
}

int rtty::rttyparity(unsigned int c)
{
	c &= (1 << nbits) - 1;

	switch (rtty_parity) {
	default:
	case RTTY_PARITY_NONE:
		return 0;

	case RTTY_PARITY_ODD:
		return rparity(c);

	case RTTY_PARITY_EVEN:
		return !rparity(c);

	case RTTY_PARITY_ZERO:
		return 0;

	case RTTY_PARITY_ONE:
		return 1;
	}
}

int rtty::decode_char()
{
	unsigned int parbit, par, data;

	parbit = (rxdata >> nbits) & 1;
	par = rttyparity(rxdata);

	if (rtty_parity != RTTY_PARITY_NONE && parbit != par)
		return 0;

	data = rxdata & ((1 << nbits) - 1);

	if (nbits == 5)
		return baudot_dec(data);

	return data;
}

bool rtty::is_mark_space( int &correction)
{
	correction = 0;
// test for rough bit position
	if (bit_buf[0] && !bit_buf[symbollen-1]) {
// test for mark/space straddle point
		for (int i = 0; i < symbollen; i++)
			correction += bit_buf[i];
		if (abs(symbollen/2 - correction) < 6) // too small & bad signals are not decoded
			return true;
	}
	return false;
}

bool rtty::is_mark()
{
	return bit_buf[symbollen / 2];
}

bool rtty::rx(bool bit) // original modified for probability test
{
	bool flag = false;
	unsigned char c = 0;
	int correction;

	for (int i = 1; i < symbollen; i++) bit_buf[i-1] = bit_buf[i];
	bit_buf[symbollen - 1] = bit;

	switch (rxstate) {
	case RTTY_RX_STATE_IDLE:
		if ( is_mark_space(correction)) {
			rxstate = RTTY_RX_STATE_START;
			counter = correction;
		}
		break;
	case RTTY_RX_STATE_START:
		if (--counter == 0) {
			if (!is_mark()) {
				rxstate = RTTY_RX_STATE_DATA;
				counter = symbollen;
				bitcntr = 0;
				rxdata = 0;
			} else {
				rxstate = RTTY_RX_STATE_IDLE;
			}
		}
		break;
	case RTTY_RX_STATE_DATA:
		if (--counter == 0) {
			rxdata |= is_mark() << bitcntr++;
			counter = symbollen;
		}
		if (bitcntr == nbits + (rtty_parity != RTTY_PARITY_NONE ? 1 : 0))
			rxstate = RTTY_RX_STATE_STOP;
		break;
	case RTTY_RX_STATE_STOP:
		if (--counter == 0) {
			if (is_mark()) {
				if ((metric >= progStatus.sldrSquelchValue && progStatus.sqlonoff) || !progStatus.sqlonoff) {
					c = decode_char();
					if( progdefaults.SynopAdifDecoding || progdefaults.SynopKmlDecoding ) {
						if (c != 0 && c != '\r')  {
							synop::instance()->add(c);
						} else {
							if( synop::instance()->enabled() )
								synop::instance()->flush(false);
							put_rx_char(c);
						}
					} else if ( c != 0 ) {
// supress <CR><CR> and <LF><LF> sequences
// these were observed during the RTTY contest 2/9/2013
						if (c == '\r' && lastchar == '\r');
						else if (c == '\n' && lastchar == '\n');
						else
							put_rx_char(progdefaults.rx_lowercase ? tolower(c) : c);
						lastchar = c;
					}
					flag = true;
				}
			}
			rxstate = RTTY_RX_STATE_IDLE;
		}
		break;
	default : break;
	}

	return flag;
}

char snrmsg[80];
void rtty::Metric()
{
	double delta = rtty_baud/8.0;
	double np = wf->powerDensity(frequency, delta) * 3000 / delta;
	double sp =
		wf->powerDensity(frequency - shift/2, delta) +
		wf->powerDensity(frequency + shift/2, delta) + 1e-10;
	double snr = 0;

	sigpwr = decayavg( sigpwr, sp, sp > sigpwr ? 2 : 8);
	noisepwr = decayavg( noisepwr, np, 16 );
	snr = 10*log10(sigpwr / noisepwr);

	snprintf(snrmsg, sizeof(snrmsg), "s/n %-3.0f dB", snr);
	put_Status2(snrmsg);
	metric = CLAMP((3000 / delta) * (sigpwr/noisepwr), 0.0, 100.0);
	display_metric(metric);
}

void rtty::searchDown()
{
	double srchfreq = frequency - shift -100;
	double minfreq = shift * 2 + 100;
	double spwrlo, spwrhi, npwr;
	while (srchfreq > minfreq) {
		spwrlo = wf->powerDensity(srchfreq - shift/2, 2*rtty_baud);
		spwrhi = wf->powerDensity(srchfreq + shift/2, 2*rtty_baud);
		npwr = wf->powerDensity(srchfreq + shift, 2*rtty_baud) + 1e-10;
		if ((spwrlo / npwr > 10.0) && (spwrhi / npwr > 10.0)) {
			frequency = srchfreq;
			set_freq(frequency);
			sigsearch = SIGSEARCH;
			break;
		}
		srchfreq -= 5.0;
	}
}

void rtty::searchUp()
{
	double srchfreq = frequency + shift +100;
	double maxfreq = IMAGE_WIDTH - shift * 2 - 100;
	double spwrhi, spwrlo, npwr;
	while (srchfreq < maxfreq) {
		spwrlo = wf->powerDensity(srchfreq - shift/2, 2*rtty_baud);
		spwrhi = wf->powerDensity(srchfreq + shift/2, 2*rtty_baud);
		npwr = wf->powerDensity(srchfreq - shift, 2*rtty_baud) + 1e-10;
		if ((spwrlo / npwr > 10.0) && (spwrhi / npwr > 10.0)) {
			frequency = srchfreq;
			set_freq(frequency);
			sigsearch = SIGSEARCH;
			break;
		}
		srchfreq += 5.0;
	}
}

#if FILTER_DEBUG == 1
int snum = 0;
int mnum = 0;
#define ook(sp) \
{ \
	value = sin(2.0*M_PI*( \
		(((sp / symbollen) % 2 == 0) ? (frequency + shift/2.0) : (frequency - shift/2.0))\
		/samplerate)*sp); \
}

std::fstream ook_signal("ook_signal.csv", std::ios::out );
#endif

int rtty::rx_process(const double *buf, int len)
{
	const double *buffer = buf;
	int length = len;
	static int showxy = symbollen;

	cmplx z, zmark, zspace, *zp_mark, *zp_space;

	int n_out = 0;
	static int bitcount = 5 * nbits * symbollen;

	if ( !progdefaults.report_when_visible ||
		 dlgViewer->visible() || progStatus.show_channels )
		if (!bHistory && rttyviewer) rttyviewer->rx_process(buf, len);

	if (progStatus.rtty_filter_changed) {
		progStatus.rtty_filter_changed = false;
		reset_filters();
	}

	Metric();
#if FILTER_DEBUG == 1
double value;
#endif
	while (length-- > 0) {

// Create analytic signal from sound card input samples

#if FILTER_DEBUG == 1
if (snum < 2 * filter_length) {
	frequency = 1000.0;
	ook(snum);
	z = cmplx(value, value);
	ook_signal << snum << "," << z.real() << ",";
//	snum++;
} else {
	z = cmplx(*buffer, *buffer);
}
#else
	z = cmplx(*buffer, *buffer);
#endif
	buffer++;

// Mix it with the audio carrier frequency to create two baseband signals
// mark and space are separated and processed independently
// lowpass Windowed Sinc - Overlap-Add convolution filters.
// The two fftfilt's are the same size and processed in sync
// therefore the mark and space filters will concurrently have the
// same size outputs available for further processing

		zmark = mixer(mark_phase, frequency + shift/2.0, z);
		mark_filt->run(zmark, &zp_mark);

		zspace = mixer(space_phase, frequency - shift/2.0, z);
		n_out = space_filt->run(zspace, &zp_space);
#if FILTER_DEBUG == 1
if (snum < 2 * filter_length) {
	ook_signal << abs(zmark) <<"\n";
	snum++;
}
#endif
		for (int i = 0; i < n_out; i++) {

			mark_mag = abs(zp_mark[i]);
			mark_env = decayavg (mark_env, mark_mag,
						(mark_mag > mark_env) ? symbollen / 4 : symbollen * 16);
			mark_noise = decayavg (mark_noise, mark_mag,
						(mark_mag < mark_noise) ? symbollen / 4 : symbollen * 48);
			space_mag = abs(zp_space[i]);
			space_env = decayavg (space_env, space_mag,
						(space_mag > space_env) ? symbollen / 4 : symbollen * 16);
			space_noise = decayavg (space_noise, space_mag,
						(space_mag < space_noise) ? symbollen / 4 : symbollen * 48);
#if FILTER_DEBUG == 1
if (mnum < 2 * filter_length)
	ook_signal << ",,," << mnum++ + filter_length / 2 << "," << mark_mag << "," << space_mag << "\n";
#endif
			noise_floor = min(space_noise, mark_noise);

// clipped if clipped decoder selected
			double mclipped = 0, sclipped = 0;
			mclipped = mark_mag > mark_env ? mark_env : mark_mag;
			sclipped = space_mag > space_env ? space_env : space_mag;
			if (mclipped < noise_floor) mclipped = noise_floor;
			if (sclipped < noise_floor) sclipped = noise_floor;

			switch (progdefaults.rtty_cwi) {
				case 1 : // mark only decode
					space_env = sclipped = noise_floor;
					break;
				case 2: // space only decode
					mark_env = mclipped = noise_floor;
				default : ;
			}

//			double v0, v1, v2, v3, v4, v5;
			double v3;

// no ATC
//			v0 = mark_mag - space_mag;
// Linear ATC
//			v1 = mark_mag - space_mag - 0.5 * (mark_env - space_env);
// Clipped ATC
//			v2  = (mclipped - noise_floor) - (sclipped - noise_floor) - 0.5 * (
//					(mark_env - noise_floor) - (space_env - noise_floor));
// Optimal ATC
			v3  = (mclipped - noise_floor) * (mark_env - noise_floor) -
					(sclipped - noise_floor) * (space_env - noise_floor) - 0.25 * (
					(mark_env - noise_floor) * (mark_env - noise_floor) -
					(space_env - noise_floor) * (space_env - noise_floor));
// Kahn Squarer with Linear ATC
//			v4 =  (mark_mag - noise_floor) * (mark_mag - noise_floor) -
//					(space_mag - noise_floor) * (space_mag - noise_floor) - 0.25 * (
//					(mark_env - noise_floor) * (mark_env - noise_floor) -
//					(space_env - noise_floor) * (space_env - noise_floor));
// Kahn Squarer with Clipped ATC
//			v5 =  (mclipped - noise_floor) * (mclipped - noise_floor) -
//					(sclipped - noise_floor) * (sclipped - noise_floor) - 0.25 * (
//					(mark_env - noise_floor) * (mark_env - noise_floor) -
//					(space_env - noise_floor) * (space_env - noise_floor));
//				switch (progdefaults.rtty_demodulator) {
//			switch (2) { // Optimal ATC
//			case 0: // linear ATC
//				bit = v1 > 0;
//				break;
//			case 1: // clipped ATC
//				bit = v2 > 0;
//				break;
//			case 2: // optimal ATC
				bit = v3 > 0;
//				break;
//			case 3: // Kahn linear ATC
//				bit = v4 > 0;
//				break;
//			case 4: // Kahn clipped
//				bit = v5 > 0;
//				break;
//			case 5: // No ATC
//			default :
//				bit = v0 > 0;
//			}

// XY scope signal generation

			if (progdefaults.true_scope) {
//----------------------------------------------------------------------
// "true" scope implementation------------------------------------------
//----------------------------------------------------------------------

// get the baseband-signal and...
				xy = cmplx(
						zp_mark[i].real() * cos(xy_phase) + zp_mark[i].imag() * sin(xy_phase),
						zp_space[i].real() * cos(xy_phase) + zp_space[i].imag() * sin(xy_phase) );

// if mark-tone has a higher magnitude than the space-tone,
// further reduce the scope's space-amplitude and vice versa
// this makes the scope looking a little bit nicer, too...
// aka: less noisy...
				if( abs(zp_mark[i]) > abs(zp_space[i]) ) {
// note ox x complex lib does not support xy.real(double) or xy.imag(double)
					xy = cmplx( xy.real(),
								xy.imag() * abs(zp_space[i])/abs(zp_mark[i]) );
//					xy.imag() *= abs(zp_space[i])/abs(zp_mark[i]);
				} else {
					xy = cmplx( xy.real() / ( abs(zp_space[i])/abs(zp_mark[i]) ),
								xy.imag() );
//					xy.real() /= abs(zp_space[i])/abs(zp_mark[i]);
				}

// now normalize the scope
				double const norm = 1.3*(abs(zp_mark [i]) + abs(zp_space[i]));
				xy /= norm;

			} else {
//----------------------------------------------------------------------
// "ortho" scope implementation-----------------------------------------
//----------------------------------------------------------------------
// get magnitude of the baseband-signal
				if (bit)
					xy = cmplx( mark_mag * cos(xy_phase), space_noise * sin(xy_phase) / 2.0);
				else
					xy = cmplx( mark_noise * cos(xy_phase) / 2.0, space_mag * sin(xy_phase));
// now normalize the scope
				double const norm = (mark_env + space_env);
				xy /= norm;
			}

// Rotate the scope x-y iaw frequency error.  Old scopes were not capable
// of this, but it should be very handy, so... who cares of realism anyways?
			double const rotate = 8 * TWOPI * freqerr / rtty_shift;
			xy = xy * cmplx(cos(rotate), sin(rotate));

			QI[inp_ptr] = xy;

// shift it to 128Hz(!) and not to it's original position.
// this makes it more pretty and does not remove it's other
// qualities. Reason is that this is a fraction of the used
// block-size.
			xy_phase += (TWOPI * (128.0 / samplerate));
// end XY signal generation

			mark_history[inp_ptr] = zp_mark[i];
			space_history[inp_ptr] = zp_space[i];

			inp_ptr = (inp_ptr + 1) % MAXPIPE;

			if (dspcnt && (--dspcnt % (nbits + 2) == 0)) {
				pipe[pipeptr] = bit - 0.5; //testbit - 0.5;
				pipeptr = (pipeptr + 1) % symbollen;
			}

// detect TTY signal transitions
// rx(...) returns true if valid TTY bit stream detected
// either character or idle signal
			if ( rx( reverse ? !bit : bit ) ) {
				dspcnt = symbollen * (nbits + 2);
				if (!bHighSpeed) Update_syncscope();
				clear_zdata = true;
				bitcount = 5 * nbits * symbollen;
				if (sigsearch) sigsearch--;
					int mp0 = inp_ptr - 2;
				int mp1 = mp0 + 1;
				if (mp0 < 0) mp0 += MAXPIPE;
				if (mp1 < 0) mp1 += MAXPIPE;
				double ferr = (TWOPI * samplerate / rtty_baud) *
						(!reverse ?
							arg(conj(mark_history[mp1]) * mark_history[mp0]) :
							arg(conj(space_history[mp1]) * space_history[mp0]));
				if (fabs(ferr) > rtty_baud / 2) ferr = 0;
				freqerr = decayavg ( freqerr, ferr / 8,
					progdefaults.rtty_afcspeed == 0 ? 8 :
					progdefaults.rtty_afcspeed == 1 ? 4 : 1 );
				if (progStatus.afconoff &&
					(metric > progStatus.sldrSquelchValue || !progStatus.sqlonoff))
					set_freq(frequency - freqerr);
			} else
				if (bitcount) --bitcount;
		}
		if (!bHighSpeed) {
			if (!bitcount) {
				if (clear_zdata) {
					clear_zdata = false;
					Clear_syncscope();
					for (int i = 0; i < MAXPIPE; i++)
						QI[i] = cmplx(0.0, 0.0);
				}
			}
			if (!--showxy) {
				set_zdata(QI, MAXPIPE);
				showxy = symbollen;
			}
		}
	}
	return 0;
}

//=====================================================================
// RTTY transmit
//=====================================================================
//double freq1;
double maxamp = 0;

double rtty::nco(double freq)
{
	phaseacc += TWOPI * freq / samplerate;

	if (phaseacc > TWOPI) phaseacc -= TWOPI;

	return cos(phaseacc);
}

double rtty::FSKnco()
{
	FSKphaseacc += TWOPI * 1000 / samplerate;

	if (FSKphaseacc > TWOPI) FSKphaseacc -= TWOPI;

	return sin(FSKphaseacc);

}


void rtty::send_symbol(int symbol, int len)
{
	acc_symbols += len;

//#if !SHAPER_BAUD
if (!progStatus.shaped_rtty) {
//if (rtty_baud > SHAPER_BAUD) {
	double freq;

	if (reverse) symbol = !symbol;

	if (symbol)
		freq = get_txfreq_woffset() + shift / 2.0;
	else
		freq = get_txfreq_woffset() - shift / 2.0;

	for (int i = 0; i < len; i++) {
		outbuf[i] = nco(freq);
		if (symbol)
			FSKbuf[i] = FSKnco();
		else
			FSKbuf[i] = 0.0 * FSKnco();
	}
} else {
//#else

	double const freq1 = get_txfreq_woffset() + shift / 2.0;
	double const freq2 = get_txfreq_woffset() - shift / 2.0;
	double mark = 0, space = 0;
	double signal = 0;

	if (reverse)
		symbol = !symbol;

	if (maxamp == 0) {
		int sym = 0;
		for (int j = 0; j < 100; j++) {
			if (sym) sym = 0;
			else sym = 1;
			for( int i = 0; i < 3*len; ++i ) {
				mark  = m_SymShaper1->Update( sym) * m_Osc1->Update( freq1 );
				space = m_SymShaper2->Update(!sym) * m_Osc2->Update( freq2 );
				signal = mark + space;

				if (maxamp < fabs(signal)) maxamp = fabs(signal);
			}
		}
	}

	for( int i = 0; i < len; ++i ) {
		mark  = m_SymShaper1->Update( symbol) * m_Osc1->Update( freq1 );
		space = m_SymShaper2->Update(!symbol) * m_Osc2->Update( freq2 );
		signal = mark + space;

		if (maxamp < fabs(signal)) {
			maxamp = fabs(signal);
		}
		outbuf[i] = maxamp ? (0.99 * signal / maxamp) : 0.0;

		if (symbol)
			FSKbuf[i] = FSKnco();
		else
			FSKbuf[i] = 0.0 * FSKnco();
	}
}
//#endif
	if (progdefaults.PseudoFSK)
		ModulateStereo(outbuf, FSKbuf, symbollen);
	else
		ModulateXmtr(outbuf, symbollen);
}

void rtty::send_stop()
{
//#if !SHAPER_BAUD
if (!progStatus.shaped_rtty) {
//if (rtty_baud >= SHAPER_BAUD) {
	double freq;
	bool invert = reverse;

	if (invert)
		freq = get_txfreq_woffset() - shift / 2.0;
	else
		freq = get_txfreq_woffset() + shift / 2.0;

	for (int i = 0; i < stoplen; i++) {
		outbuf[i] = nco(freq);
		if (invert)
			FSKbuf[i] = 0.0 * FSKnco();
		else
			FSKbuf[i] = FSKnco();
	}
} else {
//#else

	double const freq1 = get_txfreq_woffset() + shift / 2.0;
	double const freq2 = get_txfreq_woffset() - shift / 2.0;
	double mark = 0, space = 0, signal = 0;

	bool symbol = true;

	if (reverse)
		symbol = !symbol;

	for( int i = 0; i < stoplen; ++i ) {
		mark  = m_SymShaper1->Update( symbol)*m_Osc1->Update( freq1 );
		space = m_SymShaper2->Update(!symbol)*m_Osc2->Update( freq2 );
		signal = mark + space;

		if (maxamp < fabs(signal)) maxamp = fabs(signal);
		outbuf[i] = maxamp ? (0.99 * signal / maxamp) : 0.0;

		if (reverse)
			FSKbuf[i] = 0.0 * FSKnco();
		else
			FSKbuf[i] = FSKnco();
	}
}
//#endif
	if (progdefaults.PseudoFSK)
		ModulateStereo(outbuf, FSKbuf, stoplen);
	else
		ModulateXmtr(outbuf, stoplen);
}

void rtty::flush_stream()
{
	double const freq1 = get_txfreq_woffset() + shift / 2.0;
	double const freq2 = get_txfreq_woffset() - shift / 2.0;
	double mark = 0, space = 0, signal = 0;

	for( int i = 0; i < symbollen * 6; ++i ) {
		mark  = m_SymShaper1->Update(0)*m_Osc1->Update( freq1 );
		space = m_SymShaper2->Update(0)*m_Osc2->Update( freq2 );
		signal = mark + space;

		if (maxamp < fabs(signal)) maxamp = fabs(signal);
		outbuf[i] = maxamp ? (0.99 * signal / maxamp) : 0.0;

		FSKbuf[i] = 0.0;
	}

	if (progdefaults.PseudoFSK)
		ModulateStereo(outbuf, FSKbuf, symbollen * 6);
	else
		ModulateXmtr(outbuf, symbollen * 6);

}

void rtty::send_char(int c)
{
	int i;

	if (nbits == 5) {
		if (c == LETTERS)
			c = 0x1F;
		if (c == FIGURES)
			c = 0x1B;
	}

// start bit
	send_symbol(0, symbollen);
// data bits
	for (i = 0; i < nbits; i++) {
		send_symbol((c >> i) & 1, symbollen);
	}
// parity bit
	if (rtty_parity != RTTY_PARITY_NONE)
		send_symbol(rttyparity(c), symbollen);
// stop bit(s)
	send_stop();

	if (nbits == 5) {
		if (c == 0x1F || c == 0x1B)
			return;
		if (txmode == LETTERS)
			c = letters[c];
		else
			c = figures[c];
		if (c)
			put_echo_char(progdefaults.rx_lowercase ? tolower(c) : c);
	}
	else
		put_echo_char(c);
}

void rtty::send_idle()
{
	if (nbits == 5) {
		send_char(LETTERS);
		txmode = LETTERS;
	} else
		send_char(0);
}

static int line_char_count = 0;

int rtty::tx_process()
{
	int c;

	if (preamble) {
		m_SymShaper1->reset();
		m_SymShaper2->reset();
		for (int i = 0; i < nbits + 1; i++) send_symbol(0, symbollen);
		send_stop();
		for (int i = 0; i < nbits + 1; i++) send_symbol(1, symbollen);
		send_stop();
		send_idle();
		preamble = false;
//		freq1 = get_txfreq_woffset() + shift / 2.0;
	}
	c = get_tx_char();

// TX buffer empty
	if (c == GET_TX_CHAR_ETX || stopflag) {
		stopflag = false;
		line_char_count = 0;
		if (nbits != 5) {
			if (progdefaults.rtty_crcrlf) send_char('\r');
			send_char('\r');
			send_char('\n');
		} else {
			if (progdefaults.rtty_crcrlf) send_char(0x08);
			send_char(0x08);
			send_char(0x02);
		}
	if (progStatus.shaped_rtty) flush_stream();
//	if (rtty_baud <= SHAPER_BAUD) flush_stream();
//#if SHAPER_BAUD
//	flush_stream();
//#endif
		cwid();
		return -1;
	}

// send idle character if c == -1
	if (c == GET_TX_CHAR_NODATA) {
		send_idle();
		return 0;
	}

// if NOT Baudot
	if (nbits != 5) {
///
		acc_symbols = 0;
		send_char(c);
		xmt_samples = char_samples = acc_symbols;

		return 0;
	}

	if (isalpha(c) || isdigit(c) || isblank(c) || ispunct(c)) {
		++line_char_count;
	}

	if (progdefaults.rtty_autocrlf && (c != '\n' && c != '\r') &&
		(line_char_count == progdefaults.rtty_autocount ||
		 (line_char_count > progdefaults.rtty_autocount - 5 && c == ' '))) {
		line_char_count = 0;
		if (progdefaults.rtty_crcrlf)
			send_char(0x08); // CR-CR-LF triplet
		send_char(0x08);
		send_char(0x02);
		if (c == ' ')
			return 0;
	}
	if (c == '\r') {
		line_char_count = 0;
		send_char(0x08);
		return 0;
	}
	if (c == '\n') {
		line_char_count = 0;
		if (progdefaults.rtty_crcrlf)
			send_char(0x08); // CR-CR-LF triplet
		send_char(0x02);
		return 0;
	}


/* unshift-on-space */
	if (c == ' ') {
		if (progdefaults.UOStx) {
			send_char(LETTERS);
			send_char(0x04); // coded value for a space
			txmode = LETTERS;
		} else
			send_char(0x04);
		return 0;
	}

	if ((c = baudot_enc(c)) < 0)
		return 0;

// switch case if necessary

	if ((c & 0x300) != txmode) {
		if (txmode == FIGURES) {
			send_char(LETTERS);
			txmode = LETTERS;
		} else {
			send_char(FIGURES);
			txmode = FIGURES;
		}
	}
///
	acc_symbols = 0;
	send_char(c & 0x1F);
	xmt_samples = char_samples = acc_symbols;

	return 0;
}

int rtty::baudot_enc(unsigned char data)
{
	int i, c, mode;

	mode = 0;
	c = -1;

	if (islower(data))
		data = toupper(data);

	for (i = 0; i < 32; i++) {
		if (data == letters[i]) {
			mode |= LETTERS;
			c = i;
		}
		if (data == figures[i]) {
			mode |= FIGURES;
			c = i;
		}
		if (c != -1)
			return (mode | c);
	}

	return -1;
}

char rtty::baudot_dec(unsigned char data)
{
	int out = 0;

	switch (data) {
	case 0x1F:		/* letters */
		rxmode = LETTERS;
		break;
	case 0x1B:		/* figures */
		rxmode = FIGURES;
		break;
	case 0x04:		/* unshift-on-space */
		if (progdefaults.UOSrx)
			rxmode = LETTERS;
		return ' ';
		break;
	default:
		if (rxmode == LETTERS)
			out = letters[data];
		else
			out = figures[data];
		break;
	}

	return out;
}

//======================================================================
// methods for class Oscillator and class SymbolShaper
//======================================================================

Oscillator::Oscillator( double samplerate )
{
	m_phase = 0;
	m_samplerate = samplerate;
//	std::cerr << "samplerate for Oscillator:"<<m_samplerate<<"\n";
}

double Oscillator::Update( double frequency )
{
	m_phase += frequency/m_samplerate * TWOPI;
	if ( m_phase > TWOPI ) m_phase -= TWOPI;

	return ( sin( m_phase ) );
}

SymbolShaper::SymbolShaper(double baud, double sr)
{
	m_sinc_table = 0;
	Preset( baud, sr );
}

SymbolShaper::~SymbolShaper()
{
	delete [] m_sinc_table;
}

void SymbolShaper::reset()
{
	m_State = false;
	m_Accumulator = 0.0;
	m_Counter0 = 1024;
	m_Counter1 = 1024;
	m_Counter2 = 1024;
	m_Counter3 = 1024;
	m_Counter4 = 1024;
	m_Counter5 = 1024;
	m_Factor0 = 0.0;
	m_Factor1 = 0.0;
	m_Factor2 = 0.0;
	m_Factor3 = 0.0;
	m_Factor4 = 0.0;
	m_Factor5 = 0.0;
}

void SymbolShaper::Preset(double baud, double sr)
{
    double baud_rate = baud;
    double sample_rate = sr;

    LOG_INFO("Shaper::reset( %f, %f )",  baud_rate, sample_rate);

// calculate new table-size for six integrators ----------------------

    m_table_size = sample_rate / baud_rate * 5.49;
    LOG_INFO("Shaper::m_table_size = %d", m_table_size);

// kill old sinc-table and get memory for the new one -----------------

	if (m_sinc_table)
		delete [] m_sinc_table;
    m_sinc_table = new double[m_table_size];

// set up the new sinc-table based on the new parameters --------------

    long double sum = 0.0;

    for( int x=0; x<m_table_size; ++x ) {
        int const offset = m_table_size/2;
        double wfactor = 1.0 / 1.568; // optimal
// symbol-length in samples if wmultiple = 1.0
        double const T = wfactor * sample_rate / (baud_rate*2.0); 
// symbol-time relative to zero
        double const t = (x-offset);

        m_sinc_table[x] = rcos( t, T, 1.0 );

// calculate integral
        sum += m_sinc_table[x];
    }

// scale the values in the table so that the integral over it is as
// exactly 1.0000000 as we can do this...

    for( int x=0; x<m_table_size; ++x ) {
        m_sinc_table[x] *= 1.0 / sum;
    }

// reset internal states
    reset();
    maxamp = 0;
}

double SymbolShaper::Update( bool state )
{
	if( m_State != state ) {
		m_State = state;
		if( m_Counter0 >= m_table_size ) {
			m_Counter0 = 0;
			m_Factor0 = (state)? +1.0 : -1.0;
		} else if( m_Counter1 >= m_table_size ) {
			m_Counter1 = 0;
			m_Factor1 = (state)? +1.0 : -1.0;
		} else if( m_Counter2 >= m_table_size ) {
			m_Counter2 = 0;
			m_Factor2 = (state)? +1.0 : -1.0;
		} else if( m_Counter3 >= m_table_size ) {
			m_Counter3 = 0;
			m_Factor3 = (state)? +1.0 : -1.0;
		} else if( m_Counter4 >= m_table_size ) {
			m_Counter4 = 0;
			m_Factor4 = (state)? +1.0 : -1.0;
		} else  if( m_Counter5 >= m_table_size ) {
			m_Counter5 = 0;
			m_Factor5 = (state)? +1.0 : -1.0;
		}
	}

	if( m_Counter0 < m_table_size )
		m_Accumulator += m_Factor0 * m_sinc_table[m_Counter0++];

	if( m_Counter1 < m_table_size )
		m_Accumulator += m_Factor1 * m_sinc_table[m_Counter1++];

	if( m_Counter2 < m_table_size )
		m_Accumulator += m_Factor2 * m_sinc_table[m_Counter2++];

	if( m_Counter3 < m_table_size )
		m_Accumulator += m_Factor3 * m_sinc_table[m_Counter3++];

	if( m_Counter4 < m_table_size )
		m_Accumulator += m_Factor4 * m_sinc_table[m_Counter4++];

	if( m_Counter5 < m_table_size )
		m_Accumulator += m_Factor5 * m_sinc_table[m_Counter5++];

	return ( m_Accumulator / sqrt(2) );
}

void SymbolShaper::print_sinc_table()
{
	for (int i = 0; i < 1024; i++) printf("%f\n", m_SincTable[i]);
}
