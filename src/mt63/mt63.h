/*
 *    mt63.h  --  MT63 transmitter and receiver in C++ for LINUX
 *
 *    Copyright (C) 1999-2004 Pawel Jalocha, SP9VRC
 *
 *    This file is part of MT63.
 *
 *    MT63 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    MT63 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with MT63; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// ==========================================================================
// Morse Encoder

class MorseEncoder
{ public:
   MorseEncoder();
   ~MorseEncoder();
   void Free(void);
   int SetTxMsg(char *Msg); // set the message to be transmitted
   int NextKey(void);       // get the next key state (ON of OFF)
  private:
   char *TxMsg;
   int TxPtr;
   long Code;
} ;

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
{ public:
   MT63encoder();
   ~MT63encoder();
   void Free();
   int Preset(int Carriers, int Intlv, int *Pattern, int RandFill=0);
   int Process(char code);
   char *Output;
  private:
   int DataCarriers;
   char CodeMask;
   int IntlvLen;
   int IntlvSize;
   int *IntlvPatt;
   char *IntlvPipe;
   int IntlvPtr;
   float *WalshBuff;
} ;

// ==========================================================================
// MT63 envelope (non-synchronous) time and frequency synchronizer
// experimental status: looks like it's not good enough.
/*
class MT63sync
{ public:
   MT63sync();
   ~MT63sync();
   void Free(void);
   int Preset(int FFTlen, int FirstCarr, int CarrSepar, int Carriers, int Steps,
	      int Margin, int Integ);
   int Process(fcmpx *SpectraSlice);
   int SampleNow;
   int Locked;
   float FreqOfs;
   float TimeOfs;
  private:
   int FFTmask;
   int FirstDataCarr;
   int DataCarrSepar;
   int DataCarriers;
   int ScanMargin;
   int ScanFirst;
   int ScanLen;
   int StepsPerSymb;
   int ScanSize;
   double *PwrIntegMid,*PwrIntegOut;
   int IntegPtr;
   int NormSize;
   double *NormPwr;
   // int SymbPtr;
   float W1,W2,W5;
} ;
*/
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
{ public:
   MT63decoder();
   ~MT63decoder();
   void Free();
   int Preset(int Carriers, int Intlv, int *Pattern, int Margin, int Integ);
   int Process(float *Data);
   char Output;
   float SignalToNoise;
   int CarrOfs;

  private:
   int DataCarriers;
   float *IntlvPipe;
   int IntlvLen;
   int IntlvSize;
   int IntlvPtr;
   int *IntlvPatt;

   float *WalshBuff;

   int ScanLen;
   int ScanSize;
   double *DecodeSnrMid,*DecodeSnrOut;
   float W1,W2,W5;
   char *DecodePipe;
   int DecodeLen;
   int DecodeSize;
   int DecodePtr;

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
   is in Tx.Comb.Output.Len. They are in floating point, so you should
   convert them to 16-bit integers and output them to your soundcard.
4. If you have nothing to transmit, you must not stop, because
   you have to keep the sound going. MT63 transmits NL characters (code=0)
   in this case.
5. When you are done with all the characters and you want to stop,
   you should still put some NL characters in to flush the interleave
   thus please call the Tx.SendChar() Tx.DataInterleave times
   (still add few more characters to flush the windowed IFFT buffers).
   After that the MT63 transmits a jamming sequence for some time
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
7. Inbetween transmitions you may change the settings by calling
   the Tx.Preset() again.
*/

class MT63tx
{ public:
   MT63tx(); ~MT63tx();
   void Free(void);
   int Preset(int BandWidth=1000, int LongInterleave=0, char *ID=NULL);
   int SendTune(void);
   int SendChar(char ch);
   int SendJam(void);
   int SendSilence(void);

  private:
   int DataCarriers;	// the number of data carriers
   int FirstDataCarr;	// the FFT index of the first data carrier
   // int DataCarrSepar;	// separation [FFT bins] between data carriers
   int WindowLen;	// FFT window and symbol shape length
   float *TxWindow;	// The shape of the FFT window (=symbol shape)
   // int SymbolSepar;	// separation between symbols on a carrier
   int AliasFilterLen;  // anti-alias filter length
   float *AliasShapeI,*AliasShapeQ; // and shapes
   int DecimateRatio;	// decimation/interpolation after/before filter
   int *InterleavePattern; // how the bits of one block are placed on data carriers
   float TxAmpl;	// amplitude applied to generate a carrier (before IFFT)
   long CarrMarkCode;
   int CarrMarkAmpl;

   MorseEncoder CW_Coder; // CW encoder
   char *CW_ID;		// Morse Code identifier to be transmitted along the MT63 signal
   int CW_Carr;		// the carrier index to transmit the CW
   float CW_Ampl;	// CW amplitude
   int CW_Phase;	// CW phase
   int CW_PhaseCorr;	// CW phase correction

   MT63encoder Encoder; // data encode and interleaver
   int *TxVect;		// modulator vector (phases)
   int *PhaseCorr;	// phase corrections for each carrier
   fcmpx_buff WindowBuff; // FFT/window buffer
   r2FFT FFT;		// FFT engine
   CmpxOverlapWindow Window; // overlapping window
   int ProcessTxVect();

  public:
   int DataInterleave;
   QuadrComb Comb; // the output of this module is in Comb.Output
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
3. Feed floating point samples into the Rx.Process, if you have signed 16-bit
   samples, you should convert them first to floating point - look at how
   I do it in mt63rx.cc
4. After EACH new batch of samples
   you should look into Rx.Output for the decoded characters.
   You can egzamin the receiver status at any time by calling:
     Rx.SYNC_LockStatus() => logical value 0 or 1
     Rx.SYNC_Confidence() => lock confidence: a float between 0.0 and 1.0
     Rx.FEC_SNR()         => signal-to-noise seen by FEC
     Rx.TotalFreqOffset() => measured frequency offset in [Hz]
			     assuming 8000 Hz sampling
*/

class MT63rx
{ public:
   MT63rx(); ~MT63rx();
   void Free(void);
   int Preset(int BandWidth=1000, int LongInterleave=0, int Integ=16,
	      void (*Display)(float *Spectra, int Len)=NULL);
   int Process(float_buff *Input);
   char_buff Output;		// decoded characters

   int   SYNC_LockStatus(void); // 1 => locked, 0 => not locked
   float SYNC_Confidence(void); // lock confidence <0..1>
   float SYNC_FreqOffset(void);
   float SYNC_FreqDevRMS(void);
   float SYNC_TimeOffset(void);
   float TotalFreqOffset();	// Total frequency offset in [Hz]
   float FEC_SNR(void);		// signal-to-noise ratio at the FEC
   int FEC_CarrOffset(void);

  private:
   QuadrSplit InpSplit;    // input filter, I/Q splitter, decimator
   CmpxMixer TestOfs;	   // frequency offset for tests

   DelayLine<fcmpx> ProcLine; // processing pipe
   int ProcDelay;	// processing delay for optimal symbol probing
   int SyncProcPtr;     // sampling pointer for the synchronizer
   int DataProcPtr;	// sampling pointer for the data demodulator

   r2FFT FFT;		// FFT engine
   int WindowLen;	// FFT window length = symbol shape length
   int WindowLenMask;	// WindowLen-1 for pointer wrapping
   float *RxWindow;	// FFT window shape = symbol shape

   void (*SpectraDisplay)(float *Spectra, int Len);
   float *SpectraPower;

   int AliasFilterLen;  // anti-alias filter length
   float *AliasShapeI,*AliasShapeQ; // and shapes
   int DecimateRatio;	// decimation/interpolation after/before filter

   int *InterleavePattern; // how the bits of one block are placed on data carriers
   int DataInterleave;  // data interleave depth

   int DataCarriers;	// number of carriers
   int FirstDataCarr;	// the FFT index of the first data carrier
   // int DataCarrSepar;	// freq. separation between carriers [FFT bins]
   long CarrMarkCode;   // code to mark carriers (not in use here)
   // int SymbolSepar;	// time separation between symbols [samples]
   int ScanMargin;	// How many carriers up and down to search
   int IntegLen;        // Over how many symbols we integrate to synchronize

   int SymbolDiv;	// =4 we probe the input 4 times per symbol time
   int SyncStep;	// SymbolSepar/SymbolDiv
   int ScanFirst;	// first carrier to scan
   int ScanLen;		// number of carriers to scan

   fcmpx *FFTbuff;
   fcmpx *FFTbuff2;

   // here starts the time/frequency synchronizer
   void SyncProcess(fcmpx *Slice);

   fcmpx *SyncPipe[4];	// FFT result buffer for sync.
   int SyncPtr;		// wrapping pointer for SyncPipe and integrators
   int SymbPtr;		// points about where the symbol is

   fcmpx *SyncPhCorr;  // phase corrections for the sync. processor

   dcmpx  *CorrelMid[4], *CorrelOut[4];	// correlation integrator
   double *PowerMid, *PowerOut;		// carrier power integrator
   fcmpx *CorrelNorm[4];		// normalized correlation
   float W1,W2,W5;		// correlation integrator weights
   float W1p,W2p,W5p;		// power integrator weights

   fcmpx *CorrelAver[4];	// sliding sum to fit the carrier pattern
   int FitLen;

   void DoCorrelSum(fcmpx *Correl1, fcmpx *Correl2, fcmpx *Aver);

   fcmpx *SymbFit;	// vectors to match symbol shift and confidence
   int SymbFitPos;	// "smoothed" peak position

   float *FreqPipe;	// smoothing pipe for frequency offset
   fcmpx *SymbPipe;	// smoothing pipe for symbol shift
   int TrackPipeLen;	// tracking pipe length
   int TrackPipePtr;	// pipe pointer
   double AverFreq;	// averaged frequency
   dcmpx AverSymb;	// averaged symbol phase

   float SyncLockThres; // lock confidence threshold
   float SyncHoldThres; // minimal confidence to hold the lock

   int   SyncLocked;	// locked or not locked
   float SyncSymbConf;  // current smoothed confidence
   float SyncFreqOfs;	// current smoothed frequency offset
   float SyncFreqDev;   // frequency deviation (RMS)
   float SyncSymbShift; // current smoothed symbol time shift

   // here starts the data decoder
   void DataProcess(fcmpx *EvenSlice, fcmpx *OddSlice, float FreqOfs, int TimeDist);

   int DataScanMargin;	// +/- data carriers to scan for best FEC match
   int DataScanLen;	// total number of data carriers being processed
   int DataScanFirst;

   fcmpx *RefDataSlice;  // reference data slice for differential phase decode
   fcmpx *DataVect;	 // differentially decoded data vactor

   int DataPipeLen;	// pipe length
   int DataPipePtr;	// wrapping pointer
   fcmpx **DataPipe;	// decoded vectors pipe
   double *DataPwrMid,*DataPwrOut; // carrier power integrator
   dcmpx *DataSqrMid,*DataSqrOut; // carrier complex square integrator
   float dW1,dW2,dW5;	// integrator constants

   float *DataPhase;	 // differential decoded phases
   float *DataPhase2;	 // rather for debugging, not use otherwise

   MT63decoder Decoder;

//   MT63sync EnvSync;	// envelope synchronizer (experimental)

} ;
