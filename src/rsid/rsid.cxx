#include <config.h>

#include <math.h>

#include "rsid.h"
#include "filters.h"
#include "main.h"
#include "trx.h"
#include "configuration.h"
#include "confdialog.h"
#include "qrunner.h"

#include "rsid_fft.cxx"

enum {
	RSID_NONE = 0,

	RSID_BPSK31 = 1, RSID_QPSK31 = 110, RSID_BPSK63 = 2, RSID_QPSK63 = 3,
	RSID_BPSK125 = 4, RSID_QPSK125 = 5, RSID_BPSK250 = 126, RSID_QPSK250 = 127,

	RSID_UNKNOWN_1 = 7, RSID_UNKNOWN_2 = 8,

	RSID_MT63_500_LG = 9, RSID_MT63_500_ST = 10, RSID_MT63_500_VST = 11,
	RSID_MT63_1000_LG = 12, RSID_MT63_1000_ST = 13, RSID_MT63_1000_VST = 14,
	RSID_MT63_2000_LG = 15, RSID_MT63_2000_ST = 17, RSID_MT63_2000_VST = 18,

	RSID_PSKAM10 = 19, RSID_PSKAM31 = 20, RSID_PSKAM50 = 21, RSID_PSK63F = 22,
	RSID_PSK220F = 23, RSID_CHIP64 = 24, RSID_CHIP128 = 25,

	RSID_CW = 26,

	RSID_CCW_OOK_12 = 27, RSID_CCW_OOK_24 = 28, RSID_CCW_OOK_48 = 29,
	RSID_CCW_FSK_12 = 30, RSID_CCW_FSK_24 = 31, RSID_CCW_FSK_48 = 33,

	RSID_PACTOR1_FEC = 34, RSID_PACKET_300 = 35, RSID_PACKET_1200 = 36,

	RSID_RTTY_ASCII_7 = 37, RSID_RTTY_ASCII_8 = 38, RSID_RTTY_45 = 39,
	RSID_RTTY_50 = 40, RSID_RTTY_75 = 41,

	RSID_AMTOR_FEC = 42,

	RSID_THROB_1 = 43, RSID_THROB_2 = 44, RSID_THROB_4 = 45,
	RSID_THROBX_1 = 46, RSID_THROBX_2 = 47, RSID_THROBX_4 = 146,

	RSID_CONTESTIA_8_250 = 49, RSID_CONTESTIA_16_500 = 50, RSID_CONTESTIA_32_1000 = 51,
	RSID_CONTESTIA_8_500 = 52, RSID_CONTESTIA_16_1000 = 53, RSID_CONTESTIA_4_500 = 54,
	RSID_CONTESTIA_4_250 = 55,

	RSID_VOICE = 56,

	RSID_MFSK8 = 60, RSID_MFSK16 = 57, RSID_MFSK32 = 147,
	RSID_MFSK11 = 148, RSID_MFSK22 = 152,

	RSID_RTTYM_8_250 = 61, RSID_RTTYM_16_500 = 62, RSID_RTTYM_32_1000 = 63,
	RSID_RTTYM_8_500 = 65, RSID_RTTYM_16_1000 = 66, RSID_RTTYM_4_500 = 67,
	RSID_RTTYM_4_250 = 68,

	RSID_OLIVIA_8_250 = 69, RSID_OLIVIA_16_500 = 70, RSID_OLIVIA_32_1000 = 71,
	RSID_OLIVIA_8_500 = 72, RSID_OLIVIA_16_1000 = 73, RSID_OLIVIA_4_500 = 74,
	RSID_OLIVIA_4_250 = 75,

	RSID_PAX = 76, RSID_PAX2 = 77, RSID_DOMINOF = 78, RSID_FAX = 79, RSID_SSTV = 81,

	RSID_DOMINOEX_4 = 84, RSID_DOMINOEX_5 = 85, RSID_DOMINOEX_8 = 86,
	RSID_DOMINOEX_11 = 87, RSID_DOMINOEX_16 = 88, RSID_DOMINOEX_22 = 90,
	RSID_DOMINOEX_4_FEC = 92, RSID_DOMINOEX_5_FEC = 93, RSID_DOMINOEX_8_FEC = 97,
	RSID_DOMINOEX_11_FEC = 98, RSID_DOMINOEX_16_FEC = 99, RSID_DOMINOEX_22_FEC = 101,

	RSID_FELD_HELL = 104, RSID_PSK_HELL = 105, RSID_HELL_80 = 106,
	RSID_FM_HELL_105 = 107, RSID_FM_HELL_245 = 108,

	RSID_THOR_4 = 136, RSID_THOR_5 = 139, RSID_THOR_8 = 137,
	RSID_THOR_11 = 143, RSID_THOR_16 = 138, RSID_THOR_22 = 145,
};

RSIDs cRsId::rsid_ids[] = {
	{ RSID_BPSK31, MODE_BPSK31 },
	{ RSID_QPSK31, MODE_QPSK31 },
	{ RSID_BPSK63, MODE_PSK63 },
	{ RSID_QPSK63, MODE_QPSK63 },
	{ RSID_BPSK125, MODE_PSK125 },
	{ RSID_QPSK125, MODE_QPSK125 },
	{ RSID_BPSK250, MODE_PSK250 },
	{ RSID_QPSK250, MODE_QPSK250 },

	{ RSID_UNKNOWN_1, NUM_MODES },
	{ RSID_UNKNOWN_2, NUM_MODES },

	{ RSID_MT63_500_LG, MODE_MT63_500 },
	{ RSID_MT63_500_ST, MODE_MT63_500 },
	{ RSID_MT63_500_VST, MODE_MT63_500 },
	{ RSID_MT63_1000_LG, MODE_MT63_1000 },
	{ RSID_MT63_1000_ST, MODE_MT63_1000 },
	{ RSID_MT63_1000_VST, MODE_MT63_1000 },
	{ RSID_MT63_2000_LG, MODE_MT63_2000 },
	{ RSID_MT63_2000_ST, MODE_MT63_2000 },
	{ RSID_MT63_2000_VST, MODE_MT63_2000 },

	{ RSID_PSKAM10,  NUM_MODES },
	{ RSID_PSKAM31, NUM_MODES },
	{ RSID_PSKAM50, NUM_MODES },
	{ RSID_PSK63F, NUM_MODES },
	{ RSID_PSK220F, NUM_MODES },
	{ RSID_CHIP64, NUM_MODES },
	{ RSID_CHIP128, NUM_MODES },

	{ RSID_CW, MODE_CW },

	{ RSID_CCW_OOK_12, NUM_MODES },
	{ RSID_CCW_OOK_24, NUM_MODES },
	{ RSID_CCW_OOK_48, NUM_MODES },
	{ RSID_CCW_FSK_12, NUM_MODES },
	{ RSID_CCW_FSK_24, NUM_MODES },
	{ RSID_CCW_FSK_48, NUM_MODES },

	{ RSID_PACTOR1_FEC, NUM_MODES },
	{ RSID_PACKET_300, NUM_MODES },
	{ RSID_PACKET_1200, NUM_MODES },

	{ RSID_RTTY_ASCII_7, MODE_RTTY },
	{ RSID_RTTY_ASCII_8, MODE_RTTY },
	{ RSID_RTTY_45, MODE_RTTY },
	{ RSID_RTTY_50 , MODE_RTTY },
	{ RSID_RTTY_75 , MODE_RTTY },

	{ RSID_AMTOR_FEC, NUM_MODES },

	{ RSID_THROB_1, MODE_THROB1 },
	{ RSID_THROB_2, MODE_THROB2 },
	{ RSID_THROB_4, MODE_THROB4 },
	{ RSID_THROBX_1, MODE_THROBX1 },
	{ RSID_THROBX_2, MODE_THROBX2 },
	{ RSID_THROBX_4, MODE_THROBX4 },

	{ RSID_CONTESTIA_8_250, NUM_MODES },
	{ RSID_CONTESTIA_16_500, NUM_MODES },
	{ RSID_CONTESTIA_32_1000, NUM_MODES },
	{ RSID_CONTESTIA_8_500, NUM_MODES },
	{ RSID_CONTESTIA_16_1000, NUM_MODES },
	{ RSID_CONTESTIA_4_500, NUM_MODES },
	{ RSID_CONTESTIA_4_250, NUM_MODES },

	{ RSID_VOICE, NUM_MODES },

	{ RSID_MFSK8, MODE_MFSK8 },
	{ RSID_MFSK16, MODE_MFSK16 },
	{ RSID_MFSK32, MODE_MFSK32 },
#ifdef EXPERIMENTAL
	{ RSID_MFSK11, MODE_MFSK11 },
	{ RSID_MFSK22, MODE_MFSK22 },
#else
	{ RSID_MFSK11, NUM_MODES },
	{ RSID_MFSK22, NUM_MODES },
#endif

	{ RSID_RTTYM_8_250, NUM_MODES },
	{ RSID_RTTYM_16_500, NUM_MODES },
	{ RSID_RTTYM_32_1000, NUM_MODES },
	{ RSID_RTTYM_8_500, NUM_MODES },
	{ RSID_RTTYM_16_1000, NUM_MODES },
	{ RSID_RTTYM_4_500, NUM_MODES },
	{ RSID_RTTYM_4_250, NUM_MODES },

	{ RSID_OLIVIA_8_250, MODE_OLIVIA },
	{ RSID_OLIVIA_16_500, MODE_OLIVIA },
	{ RSID_OLIVIA_32_1000, MODE_OLIVIA },
	{ RSID_OLIVIA_8_500, MODE_OLIVIA },
	{ RSID_OLIVIA_16_1000, MODE_OLIVIA },
	{ RSID_OLIVIA_4_500, MODE_OLIVIA },
	{ RSID_OLIVIA_4_250, MODE_OLIVIA },

	{ RSID_PAX, NUM_MODES },
	{ RSID_PAX2, NUM_MODES },
	{ RSID_DOMINOF, NUM_MODES },
	{ RSID_FAX, NUM_MODES },
	{ RSID_SSTV, NUM_MODES },

	{ RSID_DOMINOEX_4, MODE_DOMINOEX4 },
	{ RSID_DOMINOEX_5, MODE_DOMINOEX5 },
	{ RSID_DOMINOEX_8, MODE_DOMINOEX8 },
	{ RSID_DOMINOEX_11, MODE_DOMINOEX11 },
	{ RSID_DOMINOEX_16, MODE_DOMINOEX16 },
	{ RSID_DOMINOEX_22, MODE_DOMINOEX22 },
	{ RSID_DOMINOEX_4_FEC, MODE_DOMINOEX4 },
	{ RSID_DOMINOEX_5_FEC, MODE_DOMINOEX5 },
	{ RSID_DOMINOEX_8_FEC, MODE_DOMINOEX8 },
	{ RSID_DOMINOEX_11_FEC, MODE_DOMINOEX11 },
	{ RSID_DOMINOEX_16_FEC, MODE_DOMINOEX16 },
	{ RSID_DOMINOEX_22_FEC, MODE_DOMINOEX22 },

	{ RSID_FELD_HELL, MODE_FELDHELL },
	{ RSID_PSK_HELL, NUM_MODES },
	{ RSID_HELL_80 , MODE_HELL80 },
	{ RSID_FM_HELL_105, MODE_FSKH105 },
	{ RSID_FM_HELL_245, NUM_MODES },

	{ RSID_THOR_4, MODE_THOR4 },
	{ RSID_THOR_8, MODE_THOR8 },
	{ RSID_THOR_16, MODE_THOR16 },
	{ RSID_THOR_5, MODE_THOR5 },
	{ RSID_THOR_11, MODE_THOR11 },
	{ RSID_THOR_22, MODE_THOR22 },

	{ RSID_NONE, NUM_MODES }
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

	if (progdefaults.rsidWideSearch) {
		nBinLow = RSID_RESOL + 1;
		nBinHigh = RSID_FFT_SIZE - 32;
	} else {
		nBinLow = (int)((centerfreq  - 100.0 * RSID_RESOL) * 2048.0 / 11025.0);
		nBinHigh = (int)((centerfreq  + 100.0 * RSID_RESOL) * 2048.0 / 11025.0);
	}
 	
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

void cRsId::apply(int iSymbol, int iBin)
{

	double freq = (iBin + (RSID_NSYMBOLS - 1) * RSID_RESOL / 2) * 11025.0 / 2048.0;

	int mbin = 0;
	for (int n = 0; n < rsid_ids_size; n++)
		if (rsid_ids[n].rs == iSymbol) {
			mbin = rsid_ids[n].mode;
			break;
		}
		
	REQ(toggleRSID);

	if (mbin == NUM_MODES) return;

	switch (iSymbol) {
	// rtty parameters
	case RSID_RTTY_ASCII_7:
		progdefaults.rtty_baud = 5;
		progdefaults.rtty_bits = 1;
		progdefaults.rtty_shift = 9;
		break;
	case RSID_RTTY_ASCII_8:
		progdefaults.rtty_baud = 5;
		progdefaults.rtty_bits = 2;
		progdefaults.rtty_shift = 9;
		break;
	case RSID_RTTY_45:
		progdefaults.rtty_baud = 1;
		progdefaults.rtty_bits = 0;
		progdefaults.rtty_shift = 3;
		break;
	case RSID_RTTY_50:
		progdefaults.rtty_baud = 2;
		progdefaults.rtty_bits = 0;
		progdefaults.rtty_shift = 3;
		break;
	case RSID_RTTY_75:
		progdefaults.rtty_baud = 4;
		progdefaults.rtty_bits = 0;
		progdefaults.rtty_shift = 9;
		break;
	// special MultiPsk FEC modes
	case RSID_DOMINOEX_4_FEC: case RSID_DOMINOEX_5_FEC: case RSID_DOMINOEX_8_FEC:
	case RSID_DOMINOEX_11_FEC: case RSID_DOMINOEX_16_FEC: case RSID_DOMINOEX_22_FEC:
		progdefaults.DOMINOEX_FEC = true;
		break;
	// olivia parameters
	case RSID_OLIVIA_8_250:
		progdefaults.oliviatones = 2;
		progdefaults.oliviabw = 1;
		break;
	case RSID_OLIVIA_16_500:
		progdefaults.oliviatones = 3;
		progdefaults.oliviabw = 2;
		break;
	case RSID_OLIVIA_32_1000:
		progdefaults.oliviatones = 4;
		progdefaults.oliviabw = 3;
		break;
	case RSID_OLIVIA_8_500:
		progdefaults.oliviatones = 2;
		progdefaults.oliviabw = 2;
		break;
	case RSID_OLIVIA_16_1000:
		progdefaults.oliviatones = 3;
		progdefaults.oliviabw = 3;
		break;
	case RSID_OLIVIA_4_500:
		progdefaults.oliviatones = 1;
		progdefaults.oliviabw = 2;
		break;
	case RSID_OLIVIA_4_250:
		progdefaults.oliviatones = 1;
		progdefaults.oliviabw = 1;
		break;
	// mt63
	case RSID_MT63_500_LG: case RSID_MT63_1000_LG: case RSID_MT63_2000_LG:
		progdefaults.mt63_interleave = 64;
		break;
	case RSID_MT63_500_ST: case RSID_MT63_1000_ST: case RSID_MT63_2000_ST:
	case RSID_MT63_500_VST: case RSID_MT63_1000_VST: case RSID_MT63_2000_VST:
		progdefaults.mt63_interleave = 32;
		break;

	default:
		break;
	}

	REQ(&configuration::loadDefaults, &progdefaults);

	active_modem->set_freq(freq);
	REQ(init_modem, mbin);
	
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
	
	trx_mode mode = active_modem->get_mode();
	uchar rmode = RSID_NONE;

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
			rmode = RSID_RTTY_45;; // 45 baud Baudot, shift 170
		break;

	case MODE_OLIVIA:
		if (progdefaults.oliviatones == 2 && progdefaults.oliviabw == 1)
			rmode = RSID_OLIVIA_8_250;
		else if (progdefaults.oliviatones == 3 && progdefaults.oliviabw == 2)
			rmode = RSID_OLIVIA_16_500;
		else if (progdefaults.oliviatones == 4 && progdefaults.oliviabw == 3)
			rmode = RSID_OLIVIA_32_1000;
		else if (progdefaults.oliviatones == 2 && progdefaults.oliviabw == 2)
			rmode = RSID_OLIVIA_8_500;
		else if (progdefaults.oliviatones == 3 && progdefaults.oliviabw == 3)
			rmode = RSID_OLIVIA_16_1000;
		else if (progdefaults.oliviatones == 1 && progdefaults.oliviabw == 2)
			rmode = RSID_OLIVIA_4_500;
		else if (progdefaults.oliviatones == 1 && progdefaults.oliviabw == 1)
			rmode = RSID_OLIVIA_4_250;
		else
			rmode = RSID_OLIVIA_16_500;
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

	case MODE_MT63_500:
		if (progdefaults.mt63_interleave == 32)
			rmode = RSID_MT63_500_ST;
		break;
	case MODE_MT63_1000:
		if (progdefaults.mt63_interleave == 32)
			rmode = RSID_MT63_1000_ST;
		break;
	case MODE_MT63_2000:
		if (progdefaults.mt63_interleave == 32)
			rmode = RSID_MT63_2000_ST;
		break;
	}

	// if rmode is still unset, look it up
	if (rmode == RSID_NONE) {
		for (size_t i = 0; i < sizeof(rsid_ids)/sizeof(*rsid_ids); i++) {
			if (mode == rsid_ids[i].mode) {
				rmode = rsid_ids[i].rs;
				break;
			}
		}
	}
	if (rmode == RSID_NONE)
		return;
	Encode(rmode, rsid);


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

