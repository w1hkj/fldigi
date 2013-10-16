// ----------------------------------------------------------------------------
//
//	rsid.cxx
//
// Copyright (C) 2008-2012
//		Dave Freese, W1HKJ
// Copyright (C) 2009-2012
//		Stelios Bounanos, M0GLD
// Copyright (C) 2012
//		John Douyere, VK2ETA
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
#include <cmath>
#include <cstring>
#include <float.h>
#include <samplerate.h>

#include "rsid.h"
#include "filters.h"
#include "misc.h"
#include "trx.h"
#include "fl_digi.h"
#include "configuration.h"
#include "confdialog.h"
#include "qrunner.h"
#include "notify.h"
#include "debug.h"

#include "main.h"
#include "arq_io.h"

LOG_FILE_SOURCE(debug::LOG_MODEM);

#include "rsid_defs.cxx"

#define RSWINDOW 1

const int cRsId::Squares[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
	0, 2, 4, 6, 8,10,12,14, 9,11,13,15, 1, 3, 5, 7,
	0, 3, 6, 5,12,15,10, 9, 1, 2, 7, 4,13,14,11, 8,
	0, 4, 8,12, 9,13, 1, 5,11,15, 3, 7, 2, 6,10,14,
	0, 5,10,15,13, 8, 7, 2, 3, 6, 9,12,14,11, 4, 1,
	0, 6,12,10, 1, 7,13,11, 2, 4,14, 8, 3, 5,15, 9,
	0, 7,14, 9, 5, 2,11,12,10,13, 4, 3,15, 8, 1, 6,
	0, 8, 9, 1,11, 3, 2,10,15, 7, 6,14, 4,12,13, 5,
	0, 9,11, 2,15, 6, 4,13, 7,14,12, 5, 8, 1, 3,10,
	0,10,13, 7, 3, 9,14, 4, 6,12,11, 1, 5,15, 8, 2,
	0,11,15, 4, 7,12, 8, 3,14, 5, 1,10, 9, 2, 6,13,
	0,12, 1,13, 2,14, 3,15, 4, 8, 5, 9, 6,10, 7,11,
	0,13, 3,14, 6,11, 5, 8,12, 1,15, 2,10, 7, 9, 4,
	0,14, 5,11,10, 4,15, 1,13, 3, 8, 6, 7, 9, 2,12,
	0,15, 7, 8,14, 1, 9, 6, 5,10, 2,13,11, 4,12, 3
};

const int cRsId::indices[] = {
	2, 4, 8, 9, 11, 15, 7, 14, 5, 10, 13, 3
};

cRsId::cRsId()
{
	int error;
	src_state = src_new(progdefaults.sample_converter, 1, &error);
	if (error) {
		LOG_ERROR("src_new error %d: %s", error, src_strerror(error));
		abort();
	}
	src_data.end_of_input = 0;

	reset();

	rsfft = new g_fft<rs_fft_type>(RSID_ARRAY_SIZE);

	memset(fftwindow, 0, sizeof(fftwindow));

	if (RSWINDOW) {
		for (int i = 0; i < RSID_ARRAY_SIZE; i++)
//		fftwindow[i] = blackman ( 1.0 * i / RSID_ARRAY_SIZE );
		fftwindow[i] = hamming ( 1.0 * i / RSID_ARRAY_SIZE );
//		fftwindow[i] = hanning ( 1.0 * i / RSID_ARRAY_SIZE );
//		fftwindow[i] = 1.0;
	}

	pCodes1 = new unsigned char[rsid_ids_size1 * RSID_NSYMBOLS];
	memset(pCodes1, 0, sizeof(pCodes1) * sizeof(unsigned char));

	pCodes2 = new unsigned char[rsid_ids_size2 * RSID_NSYMBOLS];
	memset(pCodes2, 0, sizeof(pCodes2) * sizeof(unsigned char));

	// Initialization  of assigned mode/submode IDs.
	unsigned char* c;
	for (int i = 0; i < rsid_ids_size1; i++) {
		c = pCodes1 + i * RSID_NSYMBOLS;
		Encode(rsid_ids_1[i].rs, c);
	}

	for (int i = 0; i < rsid_ids_size2; i++) {
		c = pCodes2 + i * RSID_NSYMBOLS;
		Encode(rsid_ids_2[i].rs, c);
	}

#if 0
	printf("pcode 1\n");
	printf(",rs, name, mode,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14\n");
	for (int i = 0; i < rsid_ids_size1; i++) {
		printf("%d,%d,%s,%d", i, rsid_ids_1[i].rs, rsid_ids_1[i].name, rsid_ids_1[i].mode);
		for (int j = 0; j < RSID_NSYMBOLS + 1; j++)
			printf(",%d", pCodes1[i*(RSID_NSYMBOLS + 1) + j]);
		printf("\n");
	}
	printf("\npcode 2\n");
	printf(", rs, name, mode,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14\n");
	for (int i = 0; i < rsid_ids_size2; i++) {
		printf("%d,%d,%s,%d", i, rsid_ids_2[i].rs, rsid_ids_2[i].name, rsid_ids_2[i].mode);
		for (int j = 0; j < RSID_NSYMBOLS + 1; j++)
			printf(",%d", pCodes2[i*(RSID_NSYMBOLS+ 1) + j]);
		printf("\n");
	}
#endif

	nBinLow = 3;
	nBinHigh = RSID_FFT_SIZE - 32; // - RSID_NTIMES - 2

	outbuf = 0;
	symlen = 0;

	reset();

}

cRsId::~cRsId()
{
	delete [] pCodes1;
	delete [] pCodes2;

	delete [] outbuf;
	delete rsfft;
	src_delete(src_state);
}

void cRsId::reset()
{
	iPrevDistance = iPrevDistance2 = 99;
	bPrevTimeSliceValid = bPrevTimeSliceValid2 = false;
	found1 = found2 = false;
	rsid_secondary_time_out = 0;

	memset(aInputSamples, 0, (RSID_ARRAY_SIZE * 2) * sizeof(float));
	memset(aFFTcmplx, 0, RSID_ARRAY_SIZE * sizeof(rs_cpx_type));
	memset(aFFTAmpl, 0, RSID_FFT_SIZE * sizeof(rs_fft_type));
	memset(fft_buckets, 0, RSID_NTIMES * RSID_FFT_SIZE * sizeof(int));

	int error = src_reset(src_state);
	if (error)
		LOG_ERROR("src_reset error %d: %s", error, src_strerror(error));
	src_data.src_ratio = 0.0;
	inptr = RSID_FFT_SIZE;
	hamming_resolution = progdefaults.RsID_label_type;
}

void cRsId::Encode(int code, unsigned char *rsid)
{
	rsid[0] = code >> 8;
	rsid[1] = (code >> 4) & 0x0f;
	rsid[2] = code & 0x0f;
	for (int i = 3; i < RSID_NSYMBOLS; i++)
		rsid[i] = 0;
	for (int i = 0; i < 12; i++) {
		for (int j = RSID_NSYMBOLS - 1; j > 0; j--)
			rsid[j] = rsid[j - 1] ^ Squares[(rsid[j] << 4) + indices[i]];
		rsid[0] = Squares[(rsid[0] << 4) + indices[i]];
	}
}

void cRsId::CalculateBuckets(const rs_fft_type *pSpectrum, int iBegin, int iEnd)
{
	rs_fft_type Amp = 0.0, AmpMax = 0.0;
	int iBucketMax = iBegin - 2;
	int j;

	for (int i = iBegin; i < iEnd; i += 2) {
		if (iBucketMax == i - 2) {
			AmpMax = pSpectrum[i];
			iBucketMax = i;
			for (j = i + 2; j < i + RSID_NTIMES + 2; j += 2) {
				Amp = pSpectrum[j];
				if (Amp > AmpMax) {
					AmpMax = Amp;
					iBucketMax = j;
				}
			}
		}
		else {
			j = i + RSID_NTIMES;
			Amp = pSpectrum[j];
			if (Amp > AmpMax) {
				AmpMax	= Amp;
				iBucketMax = j;
			}
		}
		fft_buckets[RSID_NTIMES - 1][i] = (iBucketMax - i) >> 1;
	}
}

void cRsId::receive(const float* buf, size_t len)
{

	if (len == 0) return;

	int srclen = static_cast<int>(len);
	double src_ratio = RSID_SAMPLE_RATE / active_modem->get_samplerate();

	if (rsid_secondary_time_out > 0) {
		rsid_secondary_time_out -= (int)(len / src_ratio);
		if (rsid_secondary_time_out <= 0) {
			LOG_INFO("%s", "Secondary RsID timed out");
			reset();
		}
	}

	if (src_data.src_ratio != src_ratio) {
		src_data.src_ratio = src_ratio;
		src_set_ratio(src_state, src_data.src_ratio);
	}

	while (srclen > 0) {
		src_data.data_in = const_cast<float*>(buf);
		src_data.input_frames = srclen;
		src_data.data_out = &aInputSamples[inptr];
		src_data.output_frames = RSID_ARRAY_SIZE * 2 - inptr;
		src_data.input_frames_used = 0;
		int error = src_process(src_state, &src_data);
		if (unlikely(error)) {
			LOG_ERROR("src_process error %d: %s", error, src_strerror(error));
			return;
		}
		size_t gend = src_data.output_frames_gen;
		size_t used = src_data.input_frames_used;
		inptr += gend;
		buf += used;
		srclen -= used;

		while (inptr >= RSID_ARRAY_SIZE) {
			search();
			memmove(&aInputSamples[0], &aInputSamples[RSID_FFT_SAMPLES],
					(RSID_BUFFER_SIZE - RSID_FFT_SAMPLES)*sizeof(float));
			inptr -= RSID_FFT_SAMPLES;
		}
	}
}

void cRsId::search(void)
{
	if (progdefaults.rsidWideSearch) {
		nBinLow = 3;
		nBinHigh = RSID_FFT_SIZE - 32;
	}
	else {
		float centerfreq = active_modem->get_freq();
		float bpf = 1.0 * RSID_ARRAY_SIZE / RSID_SAMPLE_RATE;
		nBinLow = (int)((centerfreq  - 100.0 * 2) * bpf);
		nBinHigh = (int)((centerfreq  + 100.0 * 2) * bpf);
	}
	if (nBinLow < 3) nBinLow = 3;
	if (nBinHigh > RSID_FFT_SIZE - 32) nBinHigh = RSID_FFT_SIZE - 32;

	bool bReverse = !(wf->Reverse() ^ wf->USB());
	if (bReverse) {
		nBinLow  = RSID_FFT_SIZE - nBinHigh;
		nBinHigh = RSID_FFT_SIZE - nBinLow;
	}

	if (RSWINDOW) {
		for (int i = 0; i < RSID_ARRAY_SIZE; i++)
			aFFTcmplx[i] = cmplx(aInputSamples[i] * fftwindow[i], 0);
	} else {
		for (int i = 0; i < RSID_ARRAY_SIZE; i++)
			aFFTcmplx[i] = cmplx(aInputSamples[i], 0);
	}

	rsfft->ComplexFFT(aFFTcmplx);

	memset(aFFTAmpl, 0, sizeof(aFFTAmpl));

	static const double pscale = 4.0 / (RSID_FFT_SIZE * RSID_FFT_SIZE);

	if (unlikely(bReverse)) {
		for (int i = 0; i < RSID_FFT_SIZE; i++)
			aFFTAmpl[RSID_FFT_SIZE - 1 - i] = norm(aFFTcmplx[i]) * pscale;
	} else {
		for (int i = 0; i < RSID_FFT_SIZE; i++)
			aFFTAmpl[i] = norm(aFFTcmplx[i]) * pscale;
	}

	int bucket_low = 3;
	int bucket_high = RSID_FFT_SIZE - 32;
	if (bReverse) {
		bucket_low  = RSID_FFT_SIZE - bucket_high;
		bucket_high = RSID_FFT_SIZE - bucket_low;
	}

	memmove(fft_buckets,
			&(fft_buckets[1][0]),
			(RSID_NTIMES - 1) * RSID_FFT_SIZE * sizeof(int));
	memset(&(fft_buckets[RSID_NTIMES - 1][0]), 0, RSID_FFT_SIZE * sizeof(int));

	CalculateBuckets ( aFFTAmpl, bucket_low,  bucket_high - RSID_NTIMES);
	CalculateBuckets ( aFFTAmpl, bucket_low + 1, bucket_high - RSID_NTIMES);

	int symbol_out_1 = -1;
	int bin_out_1    = -1;
	int symbol_out_2 = -1;
	int bin_out_2    = -1;

	if (rsid_secondary_time_out == 0) {
		found1 = search_amp(bin_out_1, symbol_out_1, pCodes1);
		if (found1) {
			if (symbol_out_1 != RSID_ESCAPE) {
				if (bReverse)
					bin_out_1 = 1024 - bin_out_1 - 31;
				apply(bin_out_1, symbol_out_1, 0);
				reset();
				return;
			} else {
				rsid_secondary_time_out = 3*15*1024;
				return;
			}
		} else
			return;
	}

	found2 = search_amp(bin_out_2, symbol_out_2, pCodes2);
	if (found2) {
		if (symbol_out_2 != RSID_NONE2) {
			if (bReverse)
				bin_out_2 = 1024 - bin_out_2 - 31;
			apply(bin_out_2, symbol_out_2, 1);
		}
		reset();
	}

}

void cRsId::setup_mode(int iSymbol)
{
	switch (iSymbol) {
	case RSID_RTTY_ASCII_7:
		progdefaults.rtty_baud = 5;
		progdefaults.rtty_bits = 1;
		progdefaults.rtty_shift = 9;
		REQ(&set_rtty_tab_widgets);
		break;
	case RSID_RTTY_ASCII_8:
		progdefaults.rtty_baud = 5;
		progdefaults.rtty_bits = 2;
		progdefaults.rtty_shift = 9;
		REQ(&set_rtty_tab_widgets);
		break;
	case RSID_RTTY_45:
		progdefaults.rtty_baud = 1;
		progdefaults.rtty_bits = 0;
		progdefaults.rtty_shift = 3;
		REQ(&set_rtty_tab_widgets);
		break;
	case RSID_RTTY_50:
		progdefaults.rtty_baud = 2;
		progdefaults.rtty_bits = 0;
		progdefaults.rtty_shift = 3;
		REQ(&set_rtty_tab_widgets);
		break;
	case RSID_RTTY_75:
		progdefaults.rtty_baud = 4;
		progdefaults.rtty_bits = 0;
		progdefaults.rtty_shift = 9;
		REQ(&set_rtty_tab_widgets);
		break;
// DominoEX / FEC
	case RSID_DOMINOEX_4: case RSID_DOMINOEX_5: case RSID_DOMINOEX_8:
	case RSID_DOMINOEX_11: case RSID_DOMINOEX_16: case RSID_DOMINOEX_22:
		progdefaults.DOMINOEX_FEC = false;
		REQ(&set_dominoex_tab_widgets);
		break;
	case RSID_DOMINOEX_4_FEC: case RSID_DOMINOEX_5_FEC: case RSID_DOMINOEX_8_FEC:
	case RSID_DOMINOEX_11_FEC: case RSID_DOMINOEX_16_FEC: case RSID_DOMINOEX_22_FEC:
		progdefaults.DOMINOEX_FEC = true;
		REQ(&set_dominoex_tab_widgets);
		break;
// olivia parameters
	case RSID_OLIVIA_4_125:
		progdefaults.oliviatones = 1;
		progdefaults.oliviabw = 0;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_4_250:
		progdefaults.oliviatones = 1;
		progdefaults.oliviabw = 1;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_4_500:
		progdefaults.oliviatones = 1;
		progdefaults.oliviabw = 2;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_4_1000:
		progdefaults.oliviatones = 1;
		progdefaults.oliviabw = 3;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_4_2000:
		progdefaults.oliviatones = 1;
		progdefaults.oliviabw = 4;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_8_125:
		progdefaults.oliviatones = 2;
		progdefaults.oliviabw = 0;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_8_250:
		progdefaults.oliviatones = 2;
		progdefaults.oliviabw = 1;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_8_500:
		progdefaults.oliviatones = 2;
		progdefaults.oliviabw = 2;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_8_1000:
		progdefaults.oliviatones = 2;
		progdefaults.oliviabw = 3;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_8_2000:
		progdefaults.oliviatones = 2;
		progdefaults.oliviabw = 4;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_16_500:
		progdefaults.oliviatones = 3;
		progdefaults.oliviabw = 2;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_16_1000:
		progdefaults.oliviatones = 3;
		progdefaults.oliviabw = 3;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_16_2000:
		progdefaults.oliviatones = 3;
		progdefaults.oliviabw = 4;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_32_1000:
		progdefaults.oliviatones = 4;
		progdefaults.oliviabw = 3;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_32_2000:
		progdefaults.oliviatones = 4;
		progdefaults.oliviabw = 4;
		REQ(&set_olivia_tab_widgets);
		break;
	case RSID_OLIVIA_64_2000:
		progdefaults.oliviatones = 5;
		progdefaults.oliviabw = 4;
		REQ(&set_olivia_tab_widgets);
		break;
// contestia parameters
	case RSID_CONTESTIA_4_125:
		progdefaults.contestiatones = 1;
		progdefaults.contestiabw = 0;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_4_250:
		progdefaults.contestiatones = 1;
		progdefaults.contestiabw = 1;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_4_500:
		progdefaults.contestiatones = 1;
		progdefaults.contestiabw = 2;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_4_1000:
		progdefaults.contestiatones = 1;
		progdefaults.contestiabw = 3;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_4_2000:
		progdefaults.contestiatones = 1;
		progdefaults.contestiabw = 4;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_8_125:
		progdefaults.contestiatones = 2;
		progdefaults.contestiabw = 0;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_8_250:
		progdefaults.contestiatones = 2;
		progdefaults.contestiabw = 1;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_8_500:
		progdefaults.contestiatones = 2;
		progdefaults.contestiabw = 2;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_8_1000:
		progdefaults.contestiatones = 2;
		progdefaults.contestiabw = 3;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_8_2000:
		progdefaults.contestiatones = 2;
		progdefaults.contestiabw = 4;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_16_500:
		progdefaults.contestiatones = 3;
		progdefaults.contestiabw = 2;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_16_1000:
		progdefaults.contestiatones = 3;
		progdefaults.contestiabw = 3;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_16_2000:
		progdefaults.contestiatones = 3;
		progdefaults.contestiabw = 4;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_32_1000:
		progdefaults.contestiatones = 4;
		progdefaults.contestiabw = 3;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_32_2000:
		progdefaults.contestiatones = 4;
		progdefaults.contestiabw = 4;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_64_500:
		progdefaults.contestiatones = 5;
		progdefaults.contestiabw = 2;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_64_1000:
		progdefaults.contestiatones = 5;
		progdefaults.contestiabw = 3;
		REQ(&set_contestia_tab_widgets);
		break;
	case RSID_CONTESTIA_64_2000:
		progdefaults.contestiatones = 5;
		progdefaults.contestiabw = 4;
		REQ(&set_contestia_tab_widgets);
		break;
	default:
		break;
	} // switch (iSymbol)
}

void cRsId::apply(int iBin, int iSymbol, int extended)
{
	ENSURE_THREAD(TRX_TID);

	double rsidfreq = 0, currfreq = 0;
	int n, mbin = NUM_MODES;

	int tblsize;
	const RSIDs *p_rsid;

	if (extended) {
		tblsize = rsid_ids_size2;
		p_rsid = rsid_ids_2;
	}
	else {
		tblsize = rsid_ids_size1;
		p_rsid = rsid_ids_1;
	}

	currfreq = active_modem->get_freq();
	rsidfreq = (iBin + RSID_NSYMBOLS - 0.5) * RSID_SAMPLE_RATE / 2048.0;

	for (n = 0; n < tblsize; n++) {
		if (p_rsid[n].rs == iSymbol) {
			mbin = p_rsid[n].mode;
			break;
		}
	}

	if (mbin == NUM_MODES) {
		char msg[50];
		if (n < tblsize) // RSID known but unimplemented
			snprintf(msg, sizeof(msg), "RSID: %s unimplemented",
				p_rsid[n].name);
		else // RSID unknown; shouldn't  happen
			snprintf(msg, sizeof(msg), "RSID: code %d unknown", iSymbol);
		put_status(msg, 4.0);
		LOG_VERBOSE("%s", msg);
		return;
	}

	if (progdefaults.rsid_rx_modes.test(mbin)) {
		LOG_VERBOSE("RSID: %s @ %0.1f Hz",
			p_rsid[n].name, rsidfreq);
	}
	else {
		LOG_DEBUG("Ignoring RSID: %s @ %0.1f Hz",
			p_rsid[n].name, rsidfreq);
		return;
	}

	if (mailclient || mailserver)
		REQ(pskmail_notify_rsid, mbin);

	if (progdefaults.rsid_auto_disable)
		REQ(toggleRSID);

	if (!progdefaults.disable_rsid_warning_dialog_box)
		REQ(notify_rsid, mbin, rsidfreq);

	if (progdefaults.rsid_notify_only) return;

	if (progdefaults.rsid_mark) // mark current modem & freq
		REQ(note_qrg, false, "\nBefore RSID: ", "\n",
			active_modem->get_mode(), 0LL, currfreq);

	if(active_modem) // Currently only effects Olivia, Contestia and MT63.
		active_modem->rx_flush();

	setup_mode(iSymbol);

	if (progdefaults.rsid_squelch)
		REQ(init_modem_squelch, mbin, progdefaults.disable_rsid_freq_change ? currfreq : rsidfreq);
	else
		REQ(init_modem, mbin, progdefaults.disable_rsid_freq_change ? currfreq : rsidfreq);

}

inline int cRsId::HammingDistance(int iBucket, unsigned char *p2)
{
	int dist = 0;
	for (int i = 0, j = 1; i < RSID_NSYMBOLS; i++, j += 2) {
		if (fft_buckets[j][iBucket] != p2[i]) {
			++dist;
			if (dist > hamming_resolution)
				break;
		}
	}
	return dist;
}

bool cRsId::search_amp( int &bin_out, int &symbol_out, unsigned char *pcode)
{
	int i, j;
	int iDistanceMin = 1000;  // infinity
	int iDistance    = 1000;
	int iBin         = -1;
	int iSymbol      = -1;

	int tblsize;
	const RSIDs *prsid;

	if (pcode == pCodes1) {
		tblsize = rsid_ids_size1;
		prsid = rsid_ids_1;
	} else {
		tblsize = rsid_ids_size2;
		prsid = rsid_ids_2;
	}

	unsigned char *pc = 0;
	for (i = 0; i < tblsize; i++) {
		pc = pcode + i * RSID_NSYMBOLS;
		for (j = nBinLow; j < nBinHigh - RSID_NTIMES; j++) {
			iDistance = HammingDistance(j, pc);
			if (iDistance < iDistanceMin) {
				iDistanceMin = iDistance;
				iSymbol  	 = prsid[i].rs;
				iBin		 = j;
				if (iDistanceMin == 0) break;
			}
		}
	}

	if (iDistanceMin <= hamming_resolution) {
		symbol_out	= iSymbol;
		bin_out		= iBin;
		return true;
	}

	return false;
}

//=============================================================================
// transmit rsid code for current mode
//=============================================================================

bool cRsId::assigned(trx_mode mode) {

	rmode = RSID_NONE;
	rmode2 = RSID_NONE2;

	switch (mode) {
	case MODE_RTTY :
		if (progdefaults.rtty_baud == 5 && progdefaults.rtty_bits == 1 && progdefaults.rtty_shift == 9)
			rmode = RSID_RTTY_ASCII_7;
		else if (progdefaults.rtty_baud == 5 && progdefaults.rtty_bits == 1 && progdefaults.rtty_shift == 9)
			rmode = RSID_RTTY_ASCII_8;
		else if (progdefaults.rtty_baud == 1 && progdefaults.rtty_bits == 0 && progdefaults.rtty_shift == 3)
			rmode = RSID_RTTY_45;
		else if (progdefaults.rtty_baud == 2 && progdefaults.rtty_bits == 0 && progdefaults.rtty_shift == 3)
			rmode = RSID_RTTY_50;
		else if (progdefaults.rtty_baud == 4 && progdefaults.rtty_bits == 0 && progdefaults.rtty_shift == 9)
			rmode = RSID_RTTY_75;
		else
			return false;
		return true;
		break;

	case MODE_OLIVIA:
	case MODE_OLIVIA_4_250:
	case MODE_OLIVIA_8_250:
	case MODE_OLIVIA_4_500:
	case MODE_OLIVIA_8_500:
	case MODE_OLIVIA_16_500:
	case MODE_OLIVIA_8_1000:
	case MODE_OLIVIA_16_1000:
	case MODE_OLIVIA_32_1000:
	case MODE_OLIVIA_64_2000:
		if (progdefaults.oliviatones == 1 && progdefaults.oliviabw == 0)
			rmode = RSID_OLIVIA_4_125;
		else if (progdefaults.oliviatones == 1 && progdefaults.oliviabw == 1)
			rmode = RSID_OLIVIA_4_250;
		else if (progdefaults.oliviatones == 1 && progdefaults.oliviabw == 2)
			rmode = RSID_OLIVIA_4_500;
		else if (progdefaults.oliviatones == 1 && progdefaults.oliviabw == 3)
			rmode = RSID_OLIVIA_4_1000;
		else if (progdefaults.oliviatones == 1 && progdefaults.oliviabw == 4)
			rmode = RSID_OLIVIA_4_2000;

		else if (progdefaults.oliviatones == 2 && progdefaults.oliviabw == 0)
			rmode = RSID_OLIVIA_8_125;
		else if (progdefaults.oliviatones == 2 && progdefaults.oliviabw == 1)
			rmode = RSID_OLIVIA_8_250;
		else if (progdefaults.oliviatones == 2 && progdefaults.oliviabw == 2)
			rmode = RSID_OLIVIA_8_500;
		else if (progdefaults.oliviatones == 2 && progdefaults.oliviabw == 3)
			rmode = RSID_OLIVIA_8_1000;
		else if (progdefaults.oliviatones == 2 && progdefaults.oliviabw == 4)
			rmode = RSID_OLIVIA_8_2000;

		else if (progdefaults.oliviatones == 3 && progdefaults.oliviabw == 2)
			rmode = RSID_OLIVIA_16_500;
		else if (progdefaults.oliviatones == 3 && progdefaults.oliviabw == 3)
			rmode = RSID_OLIVIA_16_1000;
		else if (progdefaults.oliviatones == 3 && progdefaults.oliviabw == 4)
			rmode = RSID_OLIVIA_16_2000;

		else if (progdefaults.oliviatones == 4 && progdefaults.oliviabw == 3)
			rmode = RSID_OLIVIA_32_1000;
		else if (progdefaults.oliviatones == 4 && progdefaults.oliviabw == 4)
			rmode = RSID_OLIVIA_32_2000;

		else if (progdefaults.oliviatones == 5 && progdefaults.oliviabw == 4)
			rmode = RSID_OLIVIA_64_2000;

		else
			return false;
		return true;
		break;

	case MODE_CONTESTIA:

		if (progdefaults.contestiatones == 1 && progdefaults.contestiabw == 0)
			rmode = RSID_CONTESTIA_4_125;
		else if (progdefaults.contestiatones == 1 && progdefaults.contestiabw == 1)
			rmode = RSID_CONTESTIA_4_250;
		else if (progdefaults.contestiatones == 1 && progdefaults.contestiabw == 2)
			rmode = RSID_CONTESTIA_4_500;
		else if (progdefaults.contestiatones == 1 && progdefaults.contestiabw == 3)
			rmode = RSID_CONTESTIA_4_1000;
		else if (progdefaults.contestiatones == 1 && progdefaults.contestiabw == 4)
			rmode = RSID_CONTESTIA_4_2000;

		else if (progdefaults.contestiatones == 2 && progdefaults.contestiabw == 0)
			rmode = RSID_CONTESTIA_8_125;
		else if (progdefaults.contestiatones == 2 && progdefaults.contestiabw == 1)
			rmode = RSID_CONTESTIA_8_250;
		else if (progdefaults.contestiatones == 2 && progdefaults.contestiabw == 2)
			rmode = RSID_CONTESTIA_8_500;
		else if (progdefaults.contestiatones == 2 && progdefaults.contestiabw == 3)
			rmode = RSID_CONTESTIA_8_1000;
		else if (progdefaults.contestiatones == 2 && progdefaults.contestiabw == 4)
			rmode = RSID_CONTESTIA_8_2000;

		else if (progdefaults.contestiatones == 3 && progdefaults.contestiabw == 2)
			rmode = RSID_CONTESTIA_16_500;
		else if (progdefaults.contestiatones == 3 && progdefaults.contestiabw == 3)
			rmode = RSID_CONTESTIA_16_1000;
		else if (progdefaults.contestiatones == 3 && progdefaults.contestiabw == 4)
			rmode = RSID_CONTESTIA_16_2000;

		else if (progdefaults.contestiatones == 4 && progdefaults.contestiabw == 3)
			rmode = RSID_CONTESTIA_32_1000;
		else if (progdefaults.contestiatones == 4 && progdefaults.contestiabw == 4)
			rmode = RSID_CONTESTIA_32_2000;

		else if (progdefaults.contestiatones == 5 && progdefaults.contestiabw == 2)
			rmode = RSID_CONTESTIA_64_500;
		else if (progdefaults.contestiatones == 5 && progdefaults.contestiabw == 3)
			rmode = RSID_CONTESTIA_64_1000;
		else if (progdefaults.contestiatones == 5 && progdefaults.contestiabw == 4)
			rmode = RSID_CONTESTIA_64_2000;

		else
			return false;
		return true;
		break;

	case MODE_DOMINOEX4:
		if (progdefaults.DOMINOEX_FEC)
			rmode = RSID_DOMINOEX_4_FEC;
		break;
	case MODE_DOMINOEX5:
		if (progdefaults.DOMINOEX_FEC)
			rmode = RSID_DOMINOEX_5_FEC;
		break;
	case MODE_DOMINOEX8:
		if (progdefaults.DOMINOEX_FEC)
			rmode = RSID_DOMINOEX_8_FEC;
		break;
	case MODE_DOMINOEX11:
		if (progdefaults.DOMINOEX_FEC)
			rmode = RSID_DOMINOEX_11_FEC;
		break;
	case MODE_DOMINOEX16:
		if (progdefaults.DOMINOEX_FEC)
			rmode = RSID_DOMINOEX_16_FEC;
		break;
	case MODE_DOMINOEX22:
		if (progdefaults.DOMINOEX_FEC)
			rmode = RSID_DOMINOEX_22_FEC;
		break;

	case MODE_MT63_500S:
		rmode = RSID_MT63_500_ST;
		break;
	case MODE_MT63_500L:
		rmode = RSID_MT63_500_LG;
		break;
	case MODE_MT63_1000S:
		rmode = RSID_MT63_1000_ST;
		break;
	case MODE_MT63_1000L:
		rmode = RSID_MT63_1000_LG;
		break;
	case MODE_MT63_2000S:
		rmode = RSID_MT63_2000_ST;
		break;
	case MODE_MT63_2000L:
		rmode = RSID_MT63_2000_LG;
		break;
	}

// if rmode is still unset, look it up
// Try secondary table first
	if (rmode == RSID_NONE) {
		for (size_t i = 0; i < sizeof(rsid_ids_2)/sizeof(*rsid_ids_2); i++) {
			if (mode == rsid_ids_2[i].mode) {
				rmode = RSID_ESCAPE;
				rmode2 = rsid_ids_2[i].rs;
				break;
			}
		}
		if (rmode2 == RSID_NONE2) {
			for (size_t i = 0; i < sizeof(rsid_ids_1)/sizeof(*rsid_ids_1); i++) {
				if (mode == rsid_ids_1[i].mode) {
					rmode = rsid_ids_1[i].rs;
					break;
				}
			}
		}
	}
	if (rmode == RSID_NONE) {
		LOG_DEBUG("%s mode is not assigned an RSID", mode_info[mode].sname);
		return false;
	}
	return true;
}

void cRsId::send(bool preRSID)
{
	trx_mode mode = active_modem->get_mode();

	if (!progdefaults.rsid_tx_modes.test(mode)) {
		LOG_DEBUG("Mode %s excluded, not sending RSID", mode_info[mode].sname);
		return;
	}

	if (!progdefaults.rsid_post && !preRSID)
		return;

	if (!assigned(mode)) return;

	unsigned char rsid[RSID_NSYMBOLS];
	double sr;
	size_t len;
	int iTone;
	double freq, phaseincr;
	double fr;
	double phase;

	Encode(rmode, rsid);
	sr = active_modem->get_samplerate();
	len = (size_t)floor(RSID_SYMLEN * sr);
	if (unlikely(len != symlen)) {
		symlen = len;
		delete [] outbuf;
		outbuf = new double[symlen];
	}

// transmit 5 symbol periods of silence at beginning of rsid
	memset(outbuf, 0, symlen * sizeof(*outbuf));
	for (int i = 0; i < 5; i++)
		active_modem->ModulateXmtr(outbuf, symlen);

// transmit sequence of 15 symbols (tones)
	fr = 1.0 * active_modem->get_txfreq() - (RSID_SAMPLE_RATE * 7 / 1024);
	phase = 0.0;

	for (int i = 0; i < 15; i++) {
		iTone = rsid[i];
		if (active_modem->get_reverse())
		iTone = 15 - iTone;
		freq = fr + iTone * RSID_SAMPLE_RATE / 1024;
		phaseincr = 2.0 * M_PI * freq / sr;

		for (size_t j = 0; j < symlen; j++) {
			phase += phaseincr;
			if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
			outbuf[j] = sin(phase);
		}
		active_modem->ModulateXmtr(outbuf, symlen);
	}

	if (rmode == RSID_ESCAPE && rmode2 != RSID_NONE2) {
// transmit 10 symbol periods of silence between rsid sequences
		memset(outbuf, 0, symlen * sizeof(*outbuf));
		for (int i = 0; i < 10; i++)
			active_modem->ModulateXmtr(outbuf, symlen);

		Encode(rmode2, rsid);
		sr = active_modem->get_samplerate();
		len = (size_t)floor(RSID_SYMLEN * sr);
		if (unlikely(len != symlen)) {
			symlen = len;
			delete [] outbuf;
			outbuf = new double[symlen];
		}
// transmit sequence of 15 symbols (tones)
		fr = 1.0 * active_modem->get_txfreq() - (RSID_SAMPLE_RATE * 7 / 1024);
		phase = 0.0;

		for (int i = 0; i < 15; i++) {
			iTone = rsid[i];
			if (active_modem->get_reverse())
			iTone = 15 - iTone;
			freq = fr + iTone * RSID_SAMPLE_RATE / 1024;
			phaseincr = 2.0 * M_PI * freq / sr;

			for (size_t j = 0; j < symlen; j++) {
				phase += phaseincr;
				if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
				outbuf[j] = sin(phase);
			}
			active_modem->ModulateXmtr(outbuf, symlen);
		}
	}

	// 5 symbol periods of silence at end of transmission
	// and between RsID and the data signal
	int nperiods = 5;
	memset(outbuf, 0, symlen * sizeof(*outbuf));
	for (int i = 0; i < nperiods; i++)
		active_modem->ModulateXmtr(outbuf, symlen);

}

