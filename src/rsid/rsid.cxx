#include <config.h>

#include <math.h>

#include "rsid.h"
#include "filters.h"
#include "main.h"
#include "trx.h"
#include "configuration.h"

#include "rsid_fft.cxx"
	
RSIDs cRsId::rsid_ids[] = {
	{ 1, MODE_BPSK31 },
	{ 110, MODE_QPSK31 },
	{ 2, MODE_PSK63 },
	{ 3, MODE_QPSK63 },
	{ 4, MODE_PSK125 },
	{ 5, MODE_QPSK125 },
	{ 126, MODE_PSK250 },
	{ 127, MODE_QPSK250 },
		
	{ 7, NUM_MODES },
	{ 8, NUM_MODES },
	{ 9, MODE_MT63_500 }, // MT63-500-LG
	{ 10, MODE_MT63_500 }, // MT63-500-ST
	{ 11, MODE_MT63_500 }, // MT63-500-VST
	{ 12, MODE_MT63_1000 }, // MT63-1000-LG
	{ 13, MODE_MT63_1000 }, // MT63-1000-ST
	{ 14, MODE_MT63_1000 }, // MT63-1000-VST
	{ 15, MODE_MT63_2000 }, // MT63-2000-LG
	{ 17, MODE_MT63_2000 }, // MT63-2000-ST
	{ 18, MODE_MT63_2000 }, // MT63-2000-VST
	{ 19,  NUM_MODES }, // PSKAM10
	{ 20, NUM_MODES }, // PSKAM31
	{ 21, NUM_MODES }, // PSKAM50
	{ 22, NUM_MODES }, // PSK63F
	{ 23, NUM_MODES }, // PSK220F
	{ 24, NUM_MODES }, // CHIP-64
	{ 25, NUM_MODES }, // CHIP-128
	{ 26, MODE_CW }, // CW
	{ 27, NUM_MODES }, // CCW-OOK-12
	{ 28, NUM_MODES }, // CCW-OOK-24
	{ 29, NUM_MODES }, // CCW-OOK-48
	{ 30, NUM_MODES }, // CCW-FSK-12
	{ 31, NUM_MODES }, // CCW-FSK-24
	{ 33, NUM_MODES }, // CCW-FSK-48
	{ 34, NUM_MODES }, // PACTOR1-FEC
	{ 35, NUM_MODES }, // PACKET-300
	{ 36, NUM_MODES }, // PACKET-1200
	{ 37, MODE_RTTY }, // ASCII-7
	{ 38, MODE_RTTY }, // ASCII-8
	{ 39, MODE_RTTY }, // RTTY-45
	{ 40, MODE_RTTY }, // RTTY-50
	{ 41, MODE_RTTY }, // RTTY-75
	{ 42, NUM_MODES }, // AMTOR FEC
	{ 43, MODE_THROB1 }, // THROB-1
	{ 44, MODE_THROB2 }, // THROB-2
	{ 45, MODE_THROB4 }, // THROB-4
	{ 46, MODE_THROBX1 }, // THROBX-1
	{ 47, MODE_THROBX2 }, // THROBX-2
	{ 146, MODE_THROBX4 }, // THROBX-4
	{ 49, NUM_MODES }, // CONTESTIA-8-250
	{ 50, NUM_MODES }, // CONTESTIA-16-500
	{ 51, NUM_MODES }, // CONTESTIA-32-1000
	{ 52, NUM_MODES }, // CONTESTIA-8-500
	{ 53, NUM_MODES }, // CONTESTIA-16-1000
	{ 54, NUM_MODES }, // CONTESTIA-4-500
	{ 55, NUM_MODES }, // CONTESTIA-4-250
	{ 56, NUM_MODES }, // VOICE

	{ 57, MODE_MFSK16 }, // MFSK16
	{ 60, MODE_MFSK8 }, // MFSK8

	{ 61, NUM_MODES }, // RTTYM-8-250
	{ 62, NUM_MODES }, // RTTYM-16-500
	{ 63, NUM_MODES }, // RTTYM-32-1000
	{ 65, NUM_MODES }, // RTTYM-8-500
	{ 66, NUM_MODES }, // RTTYM-16-1000
	{ 67, NUM_MODES }, // RTTYM-4-500
	{ 68, NUM_MODES }, // RTTYM-4-250
	
	{ 69, MODE_OLIVIA }, // OLIVIA-8-250
	{ 70, MODE_OLIVIA }, // OLIVIA-16-500
	{ 71, MODE_OLIVIA }, // OLIVIA-32-1000
	{ 72, MODE_OLIVIA }, // OLIVIA-8-500
	{ 73, MODE_OLIVIA }, // OLIVIA-16-1000
	{ 74, MODE_OLIVIA }, // OLIVIA-4-500
	{ 75, MODE_OLIVIA }, // OLIVIA-4-250

	{ 76, NUM_MODES }, // PAX
	{ 77, NUM_MODES }, // PAX2
	{ 78, NUM_MODES }, // DOMINOF
	{ 79, NUM_MODES }, // FAX
	{ 81, NUM_MODES }, // SSTV

	{ 84, MODE_DOMINOEX4 }, // DOMINOEX-4
	{ 85, MODE_DOMINOEX5 }, // DOMINOEX-5
	{ 86, MODE_DOMINOEX8 }, // DOMINOEX-8
	{ 87, MODE_DOMINOEX11 }, // DOMINOEX-11
	{ 88, MODE_DOMINOEX16 }, // DOMINOEX-16
	{ 90, MODE_DOMINOEX22 }, // DOMINOEX-22
	{ 92, MODE_DOMINOEX4 }, // DOMINOEX-4-FEC
	{ 93, MODE_DOMINOEX5 }, // DOMINOEX-5-FEC
	{ 97, MODE_DOMINOEX8 }, // DOMINOEX-8-FEC
	{ 98, MODE_DOMINOEX11 }, // DOMINOEX-11-FEC
	{ 99, MODE_DOMINOEX16 }, // DOMINOEX-16-FEC
	{ 101, MODE_DOMINOEX22 }, // DOMINOEX-22-FEC

	{ 104, MODE_FELDHELL }, // FELD HELL
	{ 105, NUM_MODES }, // PSK HELL
	{ 106, MODE_HELL80 }, // HELL 80
	{ 107, MODE_FSKH105 }, // FM HELL-105
	{ 108, NUM_MODES }, // FM HELL-245

	{ 110, MODE_QPSK31 },
		
	{ 136, MODE_THOR4 },
	{ 137, MODE_THOR8 },
	{ 138, MODE_THOR16 },
	{ 139, MODE_THOR5 },
	{ 143, MODE_THOR11 },
	{ 145, MODE_THOR22 },
		
	{ 0,  NUM_MODES }
};

const int cRsId::Squares[256] = {
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

cRsId :: cRsId()
{
	memset (aInputSamples, 0, RSID_ARRAY_SIZE * sizeof(double));	
	memset (aFFTReal, 0, RSID_ARRAY_SIZE * sizeof(double));	
	memset (aFFTAmpl, 0, RSID_FFT_SIZE * sizeof(double));
	memset (fftwindow, 0, RSID_ARRAY_SIZE * sizeof(double));
	
	memset (aHashTable1, 255, 256);
	memset (aHashTable2, 255, 256);
	
// compute current size of rsid_ids
	rsid_ids_size = 0;
	while (rsid_ids[rsid_ids_size].rs) rsid_ids_size++;

	pCodes = new uchar[rsid_ids_size * RSID_NSYMBOLS];
	memset (pCodes, 0, rsid_ids_size * RSID_NSYMBOLS);

// Initialization  of assigned mode/submode IDs.
// HashTable is used for finding a code with lowest Hamming distance.
	
	for (int i = 0; i < rsid_ids_size; i++) {
		uchar *c = pCodes + i * RSID_NSYMBOLS;
		int    hash1;
		int    hash2;
		Encode(rsid_ids[i].rs, c);
		hash1 = c[11] | (c[12] << 4);
		hash2 = c[13] | (c[14] << 4);

		aHashTable1[hash1] = i;
		aHashTable2[hash2] = i;
	}

	for (int i = 0; i < RSID_NTIMES; i++)
		for (int j = 0; j < RSID_FFT_SIZE; j++)
			aBuckets[i][j] = 0;

	iPrevDistance = 99;

	BlackmanWindow(fftwindow, RSID_FFT_SIZE);

	nBinLow = RSID_RESOL + 1;
	nBinHigh = RSID_FFT_SIZE - 32;
	iTime = 0;
	bPrevTimeSliceValid = false;
	
	_samplerate = 11025;
}

cRsId::~cRsId()
{
	delete [] pCodes;
}

void cRsId::reset()
{
	iPrevDistance = 99;
	bPrevTimeSliceValid = false;
	iTime = 0;
	memset (aInputSamples, 0, RSID_ARRAY_SIZE * sizeof(double));	
	memset (aFFTReal, 0, RSID_ARRAY_SIZE * sizeof(double));	
	memset (aFFTAmpl, 0, RSID_FFT_SIZE * sizeof(double));
	for (int i = 0; i < RSID_NTIMES; i++)
		for (int j = 0; j < RSID_FFT_SIZE; j++)
			aBuckets[i][j] = 0;
}

void cRsId::Encode(int code, uchar *rsid)
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


void cRsId::CalculateBuckets(const double *pSpectrum, int iBegin, int iEnd)
{
	double   Amp = 0.0, AmpMax = 0.0;
	int   iBucketMax = iBegin - RSID_RESOL;
	int   i, j;
	bool  firstpass = true;

	for (i = iBegin; i < iEnd; i += RSID_RESOL) {
		if (firstpass) {
			AmpMax		= pSpectrum[i];
			iBucketMax	= i;
			for (j = i + RSID_RESOL; j < i + RSID_NTIMES + RSID_RESOL; j += RSID_RESOL) {
				Amp = pSpectrum[j];
				if (Amp > AmpMax) {
					AmpMax    = Amp;
					iBucketMax = j;
				}
			}
			firstpass = false;
		} else {
			j    = i + RSID_NTIMES;
			Amp = pSpectrum[j];
			if (Amp > AmpMax) {
				AmpMax    = Amp;
				iBucketMax = j;
			}
		}
		aBuckets[iTime][i] = (iBucketMax - i) >> 1;
	}
}


void cRsId::search( const double *pSamples, int nSamples )
{
	int i, ns;
	bool bReverse = false;
	double Real, Imag;
	double centerfreq = active_modem->get_freq();

	nBinLow = (int)((centerfreq  - 100.0 * RSID_RESOL) * 2048.0 / 11025.0);
	nBinHigh = (int)((centerfreq  + 100.0 * RSID_RESOL) * 2048.0 / 11025.0);
 	
 	if (wf->Reverse() == true && wf->USB() == true) bReverse = true;
 	if (wf->Reverse() == false && wf->USB() == false) bReverse = true;
 	
 	ns = nSamples;
 	if (ns > RSID_ARRAY_SIZE / 4) {
 		ns = RSID_ARRAY_SIZE / 4;
 	}
	
	if (bReverse) {
		nBinLow  = RSID_FFT_SIZE - nBinHigh;
		nBinHigh = RSID_FFT_SIZE - nBinLow;
	}

	memmove (aInputSamples, aInputSamples + ns, ns * sizeof(double));
	memcpy  (aInputSamples + ns, pSamples, ns * sizeof(double));
	
	memset  (aFFTReal, 0, RSID_ARRAY_SIZE * sizeof(double));
	memcpy  (aFFTReal, aInputSamples, RSID_FFT_SIZE * sizeof(double));
// or 
//	for (int i = 0; i < RSID_FFT_SIZE; i++)
//		aFFTReal[i] = aInputSamples[i] * fftwindow[i];

	rsrfft( aFFTReal, 11);

	memset(aFFTAmpl, 0, RSID_FFT_SIZE * sizeof(double));
	for (i = 1; i < RSID_FFT_SIZE; i++) {
		if (bReverse) {
			Real = aFFTReal[RSID_FFT_SIZE - i];
			Imag = aFFTReal[RSID_FFT_SIZE + i];
		} else {
			Real = aFFTReal[i];
			Imag = aFFTReal[2 * RSID_FFT_SIZE - i];
		}
		aFFTAmpl[i] = Real * Real + Imag * Imag;
	}

	int SymbolOut = -1, 
	    BinOut = -1;
	
	if (search_amp ( SymbolOut, BinOut ) ){
		if (bReverse)
			BinOut = 1024 - BinOut - 31;
		apply(SymbolOut, BinOut);
	}
}

// change the current mode and frequency to the rsid detected values
trx_mode newmode;

void changemode(void*) {
	init_modem(newmode);
}

void cRsId::apply(int iSymbol, int iBin)
{

//	double freq = (iBin + 14) * 11025.0 / 2048.0;
	double freq = (iBin + (RSID_NSYMBOLS - 1) * RSID_RESOL / 2) * 11025.0 / 2048.0;

	int mbin = 0;
	for (int n = 0; n < rsid_ids_size; n++)
		if (rsid_ids[n].rs == iSymbol) {
			mbin = rsid_ids[n].mode;
			break;
		}
		
//	std::cout << iBin
//	          << ", Mode: " << mode_info[mbin].sname 
//	          << ", Frequency " << (int)(freq  + 0.5)
//	          << std::endl;
	
//	bw_rsid_toggle(wf);
	toggleRSID();

	if (mbin == NUM_MODES) return;

	newmode = mbin;
	
	if (iSymbol == 37) {
		progdefaults.rtty_baud = 5;
		progdefaults.rtty_bits = 1;
		progdefaults.rtty_shift = 9;
	}
	if (iSymbol == 38) {
		progdefaults.rtty_baud = 5;
		progdefaults.rtty_bits = 2;
		progdefaults.rtty_shift = 9;
	}
	if (iSymbol == 39) {
		progdefaults.rtty_baud = 1;
		progdefaults.rtty_bits = 0;
		progdefaults.rtty_shift = 3;
	}
	if (iSymbol == 40) {
		progdefaults.rtty_baud = 2;
		progdefaults.rtty_bits = 0;
		progdefaults.rtty_shift = 3;
	}
	if (iSymbol == 41) {
		progdefaults.rtty_baud = 4;
		progdefaults.rtty_bits = 0;
		progdefaults.rtty_shift = 9;
	}
		
	if (iSymbol == 92 || iSymbol == 93 ||
	    iSymbol == 97 || iSymbol == 98 ||
	    iSymbol == 99 || iSymbol == 101 ) // special MultiPsk FEC modes
		progdefaults.DOMINOEX_FEC = true;

	active_modem->set_freq(freq);
	Fl::add_timeout(0.05, changemode);	
	
	
/*
	int submode = 0;

	switch (iSymbol) {
//	37, // ASCII-7
//	38, // ASCII-8

	case 39: mode = MODE_RTTY; submode = ID_SUBMODE_RTTY_HAM_45_170; break;

	case 43: mode = MODE_THROB; submode = ID_SUBMODE_TPS1;	break;
	case 44: mode = MODE_THROB; submode = ID_SUBMODE_TPS2;	break;
	case 45: mode = MODE_THROB; submode = ID_SUBMODE_TPS4;	break;

	case 46: mode = MODE_THROBX; submode = ID_SUBMODE_TPS1;	break;
	case 47: mode = MODE_THROBX; submode = ID_SUBMODE_TPS2;	break;

	case 49: mode = MODE_CONTESTIA; submode = ID_SUBMODE_8_250;	break;
	case 50: mode = MODE_CONTESTIA; submode = ID_SUBMODE_16_500; break;
	case 51: mode = MODE_CONTESTIA; submode = ID_SUBMODE_32_1K;	break;
	case 52: mode = MODE_CONTESTIA; submode = ID_SUBMODE_8_500;	break;
	case 53: mode = MODE_CONTESTIA; submode = ID_SUBMODE_16_1K;	break;
	case 54: mode = MODE_CONTESTIA; submode = ID_SUBMODE_4_500;	break;
	case 55: mode = MODE_CONTESTIA; submode = ID_SUBMODE_4_250;	break;
	case 57: mode = MODE_MFSK16; break;

	case 61: mode = MODE_RTTYM; submode = ID_SUBMODE_8_250;		break;
	case 62: mode = MODE_RTTYM; submode = ID_SUBMODE_16_500;	break;
	case 63: mode = MODE_RTTYM; submode = ID_SUBMODE_32_1K;		break;
	case 65: mode = MODE_RTTYM; submode = ID_SUBMODE_8_500;		break;
	case 66: mode = MODE_RTTYM; submode = ID_SUBMODE_16_1K;		break;
	case 67: mode = MODE_RTTYM; submode = ID_SUBMODE_4_500;		break;
	case 68: mode = MODE_RTTYM; submode = ID_SUBMODE_4_250;		break;
	
	case 69: mode = MODE_OLIVIA; submode = ID_SUBMODE_8_250;	break;
	case 70: mode = MODE_OLIVIA; submode = ID_SUBMODE_16_500;	break;
	case 71: mode = MODE_OLIVIA; submode = ID_SUBMODE_32_1K;	break;
	case 72: mode = MODE_OLIVIA; submode = ID_SUBMODE_8_500;	break;
	case 73: mode = MODE_OLIVIA; submode = ID_SUBMODE_16_1K;	break;
	case 74: mode = MODE_OLIVIA; submode = ID_SUBMODE_4_500;	break;
	case 75: mode = MODE_OLIVIA; submode = ID_SUBMODE_4_250;	break;

	case 110: mode = MODE_PSK; submode = ID_SUBMODE_PSK_QPSK31; break;

	default: _ASSERT(false);
	}
*/
}

bool cRsId::encode(trx_mode mode, int submode, uchar *rsid)
{
	int ptr = 0, code;
	while ( (rsid_ids[ptr].mode != mode) && (rsid_ids[ptr].rs != 0) ) 
		ptr++;
	code = rsid_ids[ptr].rs;

	if (code == 0)
		return false;

	Encode(code + submode, rsid);

	return true;
}

//=============================================================================
// search_amp routine #1
//=============================================================================

int cRsId::HammingDistance(int iBucket, uchar *p2)
{
	int dist = 0;
	int j = iTime - RSID_NTIMES + 1; // first value
	if (j < 0)
		j += RSID_NTIMES;
	for (int i = 0; i < RSID_NSYMBOLS; i++) {
		if (aBuckets[j][iBucket] != p2[i])//*p2++)
			if (++dist == 2)
				return dist;
		j += RSID_RESOL;//2;
		if (j >= RSID_NTIMES)
			j -= RSID_NTIMES;
	}
	return dist;
}

bool cRsId::search_amp( int &SymbolOut,	int &BinOut)
{
	int i, j;
	int iDistanceMin = 99;  // infinity
	int iDistance;
	int iBin		 = -1;
	int iSymbol		 = -1;
	int iEnd		 = nBinHigh - RSID_NTIMES;//30;
	int i1, i2, i3;

	if (++iTime == RSID_NTIMES)
		iTime = 0;

	i1 = iTime - 3 * RSID_RESOL;//6;
	i2 = i1 + RSID_RESOL;//2;
	i3 = i2 + RSID_RESOL;//2;

	if (i1 < 0) {
		i1 += RSID_NTIMES;
		if (i2 < 0) {
			i2 += RSID_NTIMES;
			if (i3 < 0)
				i3 += RSID_NTIMES;
		}
	}

	CalculateBuckets ( aFFTAmpl, nBinLow,     iEnd);//nBinHigh - 30);
	CalculateBuckets ( aFFTAmpl, nBinLow + 1, iEnd);//nBinHigh - 30);

	for (i = nBinLow; i < iEnd; ++ i) {
		j = aHashTable1[aBuckets[i1][i] | (aBuckets[i2][i] << 4)];
		if (j < rsid_ids_size)  { //!= 255) {
			iDistance = HammingDistance(i, pCodes + j * RSID_NSYMBOLS);
			if (iDistance < 2 && iDistance < iDistanceMin) {
				iDistanceMin = iDistance;
				iSymbol  	 = rsid_ids[j].rs;
				iBin		 = i;
			}
		}
		j = aHashTable2[aBuckets[i3][i] | (aBuckets[iTime][i] << 4)];
		if (j < rsid_ids_size)  { //!= 255) {
			iDistance = HammingDistance (i, pCodes + j * RSID_NSYMBOLS);
			if (iDistance < 2 && iDistance < iDistanceMin) {
				iDistanceMin = iDistance;
				iSymbol		 = rsid_ids[j].rs;
				iBin		 = i;
			}
		}
	}

	if (iSymbol == -1) {
		// No RSID found in this time slice.
		// If there is a code stored from the previous time slice, return it.
		if (bPrevTimeSliceValid) {
			SymbolOut			= iPrevSymbol;
			BinOut				= iPrevBin;
			DistanceOut	    	= iPrevDistance;
			MetricsOut			= 0;
			bPrevTimeSliceValid = false;
			return true;
		}
		return false;
	}

	if (! bPrevTimeSliceValid || 
		iDistanceMin <= iPrevDistance) {
		iPrevSymbol		= iSymbol;
		iPrevBin		= iBin;
		iPrevDistance	= iDistanceMin;
	}
	bPrevTimeSliceValid = true;
	return false;
}

//=============================================================================
// transmit rsid code for current mode
//=============================================================================

void cRsId::send()
{
	int iTone;
	uchar rsid[RSID_NSYMBOLS];
	double phaseincr;
	double freq, fr;
 	double sr;
	int symlen;
	
	sr = active_modem->get_samplerate();
	symlen = (int)floor(RSID_SYMLEN * sr);
	fr = 1.0 * active_modem->get_txfreq() - (11025.0 * 7 / 1024);
	
	trx_mode m = active_modem->get_mode();
	int submode = 0;
	switch (m) {
		case MODE_RTTY : 
			if (progdefaults.rtty_baud == 5 && 
			    progdefaults.rtty_bits == 1 &&
			    progdefaults.rtty_shift == 9) submode = 0;
			else if (progdefaults.rtty_baud == 5 &&
			         progdefaults.rtty_bits == 1 &&
			         progdefaults.rtty_shift == 9) submode = 1;
			else if (progdefaults.rtty_baud == 1 &&
			         progdefaults.rtty_bits == 0 &&
			         progdefaults.rtty_shift == 3) submode = 2;
			else if (progdefaults.rtty_baud == 2 &&
			         progdefaults.rtty_bits == 0 &&
			         progdefaults.rtty_shift == 3) submode = 3;
			else if (progdefaults.rtty_baud == 4 &&
			         progdefaults.rtty_bits == 0 &&
			         progdefaults.rtty_shift == 9) submode = 4;
			else submode = 2; // 45 baud Baudot, shift 170
			break;
		default:
			submode = 0;
	}
	
	if (!encode (active_modem->get_mode(), submode, rsid))
		return;
		
	outbuf = new double[symlen];
		
// transmit sequence of 15 symbols (tones)
	phase = 0.0;
	for (int i = 0; i < 15; i++) {
		iTone = rsid[i];
		if (active_modem->get_reverse())
			iTone = 15 - iTone;
		freq = fr + iTone * 11025.0 / 1024;
		phaseincr = 2.0 * M_PI * freq / sr;

		for (int j = 0; j < symlen; j++) {
			phase += phaseincr;
			if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
			outbuf[j] = sin(phase);
		}
		active_modem->ModulateXmtr(outbuf, symlen);

	}
// transmit 3 symbol periods of silence
	for (int j = 0; j < symlen; j++) outbuf[j] = 0.0;
	active_modem->ModulateXmtr(outbuf, symlen);
	active_modem->ModulateXmtr(outbuf, symlen);
	active_modem->ModulateXmtr(outbuf, symlen);
// clean up

	delete [] outbuf;
}

