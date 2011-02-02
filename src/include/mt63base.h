/*
 *	mt63base.h  --  MT63 transmitter and receiver in C++ for LINUX
 *
 *	Copyright (c) 2007, 2008 Dave Freese, W1HKJ
 *
 *	base class for use by fldigi
 *	modified from original
 *	excluded CW_ID which is a part of the base modem class for fldigi
 *
 *	based on mt63 code by Pawel Jalocha
 *	Copyright (C) 1999-2004 Pawel Jalocha, SP9VRC
 *	Copyright (c) 2007-2011 Dave Freese, W1HKJ
 *
 *	This file is part of fldigi.
 *
 *	Fldigi is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	Fldigi is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MT63BASE_H
#define MT63BASE_H

// ==========================================================================
// Character encoder and block interleaver for the MT63 modem
/*
How to use this class:
1. Create or declare an object like:
	 MT63encoder Encoder;
2. Preset the object for the given number of carriers and interleave:
	 err=Encoder.Preset(<carriers>,<interleave>,<pattern>);
   MT63 uses 64 carriers and interleave of 32 or 64
   - the corresponding interleave patterns can be found in mt63.dat.
   If Encode.Preset() returns non-zero you are in big troubles !
3. For each character to be encode you make the call:
	 Encoder.Process(<character>);
   and you should then take the bits from the Encode.Output
   - these are the bits to be sent out on the carriers.
   (for MT63 logical 0 means carrier flip, logical 1 is no flip).
4. At any time you can call Encoder.Preset() again to change the parameters
   (this will clean away the interleaver pipe).
*/

// MT63 modem specific routines, made to be compatible with the MT63ASC.ASM
// (c) 1999 Pawel Jalocha, SP9VRC, jalocha@hpdel1.ifj.edu.pl
// Date: 05-NOV-1999

class MT63encoder
{
public:
	MT63encoder();
	~MT63encoder();
	void	Free();
	int	 Preset(int Carriers, int Intlv, int *Pattern, int RandFill=0);
	int	 Process(char code);
	char	*Output;
private:
	int	 DataCarriers;
	char	CodeMask;
	int	 IntlvLen;
	int	 IntlvSize;
	int	 *IntlvPatt;
	char	*IntlvPipe;
	int	 IntlvPtr;
	double  *WalshBuff;
} ;

// ==========================================================================
// MT63 deinterleaver and decoder
/*
How to use this class:
1. Create or declare an object:
	 MT63decoder Decoder;
2. Preset given parameters with Decoder.Preset();
	 Decoder.Preset();
   Number of carriers and interleave are same as for MT63encoder.
   "Margin" is the number of extra carriers demodulated on the side
   because with the MT63 you cannot say with full confidence which
   is really the first carrier: the FEC decoder have to tell you this.
   "Integ" is the integration period to find the best FEC match.
   "Integ" is measured in MT63 symbols (at 1000 kHz we do 10 symbols/s).
3. For each symbol period feed the demodulated data into the object:
	 Decoder.Process(<data>);
   and then get the demodulated character code from Decoder.Output
   You can get as well the measured signal-to-noise ratio from Decoder.SNR
   and the index of the first carrier (according to the FEC match)
   from Decoder.CarrOfs
4. You can change the parameters at any time with Decoder.Preset()
   (this will clean the data pipes).
*/

class MT63decoder
{
public:
	MT63decoder();
	~MT63decoder();
	void	Free();
	int	 Preset(int Carriers, int Intlv, int *Pattern, int Margin, int Integ);
	int	 Process(double *Data);
	char	Output;
	double  SignalToNoise;
	int	 CarrOfs;

private:
	int	 DataCarriers;
	double  *IntlvPipe;
	int	 IntlvLen;
	int	 IntlvSize;
	int	 IntlvPtr;
	int	 *IntlvPatt;

	double  *WalshBuff;

	int	 ScanLen;
	int	 ScanSize;
	double  *DecodeSnrMid,*DecodeSnrOut;
	double  W1, W2, W5;
	char	*DecodePipe;
	int	 DecodeLen;
	int	 DecodeSize;
	int	 DecodePtr;

} ;

// ==========================================================================
// MT63 transmitter
/*
How to use this class:
1. Create or declare an object:
	 MT63tx Tx;
2. Preset parameters:
	 Tx.Preset(<bandwidth>,<interleave>);
   Allowed values are: bandwidth=500,1000,2000; interleave=0,1;
   Non-zero value returned means there was a problem...
3. For each character to be sent:
	 Tx.SendChar(<char>);
   After each call to SendChar() you must read the samples
   from the Tx.Comb.Output.Data, the number of samples to read
   is in Tx.Comb.Output.Len. They are in double floating point, so you should
   convert them to 16-bit integers and output them to your soundcard.
4. If you have nothing to transmit, you must not stop, because
   you have to keep the sound going. MT63 transmits NL characters (code=0)
   in this case.
5. When you are done with all the characters and you want to stop,
   you should still put some NL characters in to flush the interleave
   thus please call the Tx.SendChar() Tx.DataInterleave times
   (still add few more characters to flush the windowed IFFT buffers).
   After that the MT63 transmits a jamming dspSequence for some time
   to speed up carrier drop at the receiver: you do this by calling
	 Tx.SendJam();
6. You can transmit few symbols of silence by:
	 Tx.SendSilence()
   to make a gracefull switch-off.
   Remember: each time you call SendChar(), SendJam() or SendSilence()
   you must send the contains of Tx.Comb.Output out to your soundcard.
   Each Tx.SendXxx() produces the amount of sound corresponding to one
   symbol time that is 0.1 second for the 1000 Hz mode.
   The soundcard output rate must be 8000 Hz, rather precisely,
   that is the error should be below 1 Hz. If it is not you should
   use the rate converter: look into mt63tx for an example.
7. Inbetween transmissions you may change the settings by calling
   the Tx.Preset() again.
*/

class MT63tx
{
public:
	MT63tx();
	~MT63tx();
	void	Free(void);
	int		Preset(double freq, int BandWidth=1000, int LongInterleave=0);
	int		SendTune(bool twotones);
	int		SendChar(char ch);
	int		SendJam(void);
	int		SendSilence(void);

private:
	int		DataCarriers;		// the number of data carriers
	int		FirstDataCarr;		// the FFT index of the first data carrier
	int		WindowLen;			// FFT window and symbol shape length
	double	*TxWindow;			// The shape of the FFT window (=symbol shape)

	int		AliasFilterLen;		// anti-alias filter length
	double	*AliasShapeI,
			*AliasShapeQ;		// and shapes (for fixed lower freq of 500 Hz)
	int		DecimateRatio;		// decimation/interpolation after/before filter
	int		*InterleavePattern; // how the bits of one block are placed on data carriers
	double	TxAmpl;				// Amplitude applied to generate a carrier (before IFFT)
	long	CarrMarkCode;
	int		CarrMarkAmpl;

	MT63encoder		Encoder;		// data encode and interleaver
	int				*TxVect;		// modulator vector (dspPhases)
	int				*dspPhaseCorr;	// dspPhase corrections for each carrier
	dspCmpx_buff	WindowBuff;		// FFT/window buffer
	dsp_r2FFT		FFT;			// FFT engine

	dspCmpxMixer	txmixer;

	dspCmpxOverlapWindow Window;	// overlapping window

	int ProcessTxVect();

public:
	int				DataInterleave;
	dspQuadrComb	Comb;			// the output of this module is in Comb.Output
} ;

// ==========================================================================
// MT63 receiver
/*
How to use this class:
1. Declare the object:
	 MT63rx Rx;
2. Preset paramateres
	 Rx.Preset(<bandwidth>,<interleave>,<integration>);
   For weak signals I recommend integration of 32 or more,
   otherwise 16 is enough. By the way, 16 means 1.6 second for 1000 Hz mode
   because then we transmit 10 symbols per second.
3. After EACH new batch of samples
   you should look into Rx.Output for the decoded characters.
   You can egzamin the receiver status at any time by calling:
	 Rx.SYNC_LockStatus() => logical value 0 or 1
	 Rx.SYNC_Confidence() => lock confidence: a double between 0.0 and 1.0
	 Rx.FEC_SNR()		 => signal-to-noise seen by FEC
	 Rx.TotalFreqOffset() => measured frequency offset in [Hz]
				 assuming 8000 Hz sAmpling
*/

class MT63rx
{
public:
	MT63rx();
	~MT63rx();
	void	Free(void);
	int	 Preset( double freq,
					int BandWidth = 1000,
					int LongInterleave = 0,
					int Integ = 16,
					void (*Display)(double *Spectra, int Len) = NULL);
	int	 Process(double_buff *Input);
	char_buff Output;		// decoded characters

	int	SYNC_LockStatus(void); // 1 => locked, 0 => not locked
	double SYNC_Confidence(void); // lock confidence <0..1>
	double SYNC_FreqOffset(void);
	double SYNC_FreqDevdspRMS(void);
	double SYNC_TimeOffset(void);
	double TotalFreqOffset();	// Total frequency offset in [Hz]
	double FEC_SNR(void);		// signal-to-noise ratio at the FEC
	int	FEC_CarrOffset(void);

private:
	dspQuadrSplit   InpSplit;   // input filter, I/Q splitter, decimator
	dspCmpxMixer	TestOfs;	// frequency offset for tests

	dspDelayLine<dspCmpx> ProcLine; // processing pipe
	int	 ProcdspDelay;   // processing dspDelay for optimal symbol probing
	int	 SyncProcPtr;	// sAmpling pointer for the synchronizer
	int	 DataProcPtr;	// sAmpling pointer for the data demodulator

	dsp_r2FFT FFT;			// FFT engine
	int	 WindowLen;		// FFT window length = symbol shape length
	int	 WindowLenMask;	// WindowLen-1 for pointer wrapping
	double  *RxWindow;		// FFT window shape = symbol shape

	void (*SpectraDisplay)(double *Spectra, int Len);
	double *SpectradspPower;

	int	 AliasFilterLen; // anti-alias filter length
	double  *AliasShapeI,
			*AliasShapeQ;   // and shapes
	int	 DecimateRatio;	// decimation/interpolation after/before filter

// how the bits of one block are placed on data carriers
	int	 *InterleavePattern;
	int DataInterleave;	 // data interleave depth

	int	 DataCarriers;	// number of carriers
	int	 FirstDataCarr;	// the FFT index of the first data carrier
//  int	 DataCarrSepar;	// freq. separation between carriers [FFT bins]
	long	CarrMarkCode;   // code to mark carriers (not in use here)
//  int	 SymbolSepar;	// time separation between symbols [samples]
	int	 ScanMargin;		// How many carriers up and down to search
	int	 IntegLen;	   // Over how many symbols we integrate to synchronize

	int	 SymbolDiv;		// =4 we probe the input 4 times per symbol time
	int	 SyncStep;		// SymbolSepar/SymbolDiv
	int	 ScanFirst;		// first carrier to scan
	int	 ScanLen;		// number of carriers to scan

	dspCmpx *FFTbuff;
	dspCmpx *FFTbuff2;

// here starts the time/frequency synchronizer
	void	SyncProcess(dspCmpx *Slice);

	dspCmpx *SyncPipe[4];	// FFT result buffer for sync.
	int	 SyncPtr;		// wrapping pointer for SyncPipe and integrators
	int	 SymbPtr;		// points about where the symbol is

	dspCmpx *SyncPhCorr;	// dspPhase corrections for the sync. processor

	dspCmpx *CorrelMid[4],
			*CorrelOut[4];	// correlation integrator
	double  *dspPowerMid,
			*dspPowerOut;	// carrier dspPower integrator
	dspCmpx *CorrelNorm[4];	// normalized correlation
	double  W1, W2, W5;		// correlation integrator weights
	double  W1p, W2p, W5p;	// dspPower integrator weights

	dspCmpx *CorrelAver[4];	// sliding sum to fit the carrier pattern
	int	 FitLen;

	void	DoCorrelSum(dspCmpx *Correl1, dspCmpx *Correl2, dspCmpx *Aver);

	dspCmpx *SymbFit;		// vectors to match symbol shift and confidence
	int	 SymbFitPos;		// "smoothed" peak position

	double  *FreqPipe;		// smoothing pipe for frequency offset
	dspCmpx *SymbPipe;		// smoothing pipe for symbol shift
	int	 TrackPipeLen;	// tracking pipe length
	int	 TrackPipePtr;	// pipe pointer
	double  AverFreq;		// dspAveraged frequency
	dspCmpx AverSymb;	   // dspAveraged symbol dspPhase

	double  SyncLockThres;  // lock confidence threshold
	double  SyncHoldThres;  // minimal confidence to hold the lock

	int	 SyncLocked;	 // locked or not locked
	double  SyncSymbConf;   // current smoothed confidence
	double  SyncFreqOfs;	// current smoothed frequency offset
	double  SyncFreqDev;	// frequency deviation (dspRMS)
	double  SyncSymbShift;  // current smoothed symbol time shift

// here starts the data decoder
	void	DataProcess( dspCmpx *EvenSlice,
						 dspCmpx *OddSlice,
						 double FreqOfs,
						 int TimeDist);

	int	 DataScanMargin; // +/- data carriers to scan for best FEC match
	int	 DataScanLen;	// total number of data carriers being processed
	int	 DataScanFirst;

	dspCmpx *RefDataSlice;  // reference data slice for differential dspPhase decode
	dspCmpx *DataVect;	  // differentially decoded data vactor

	int	 DataPipeLen;	// pipe length
	int	 DataPipePtr;	// wrapping pointer
	dspCmpx **DataPipe;	 // decoded vectors pipe
	double  *DataPwrMid,
			*DataPwrOut;	// carrier dspPower integrator
	dspCmpx *DataSqrMid,
			*DataSqrOut;	// carrier complex square integrator
	double  dW1, dW2, dW5;  // integrator constants

	double  *DatadspPhase;  // differential decoded dspPhases
	double  *DatadspPhase2; // rather for debugging, not use otherwise

	MT63decoder Decoder;

} ;

#endif // MT63_BASE_H
