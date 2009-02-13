/*
 *    mt63base.cc  --  MT63 transmitter and receiver in C++ for LINUX
 * 
 *    Copyright (c) 2007, 2008 Dave Freese, W1HKJ
 * 
 *    base class for use by fldigi
 *    modified from original
 *    excluded CW_ID which is a part of the base modem class for fldigi
 *    changed all floats to double and removed all float functions/methods
 *    changed from int* to double* for all sound card buffer transfers
 *
 *    based on mt63 code by Pawel Jalocha
 *    Copyright (C) 1999-2004 Pawel Jalocha, SP9VRC
 *    Copyright (c) 2007-2008 Dave Freese, W1HKJ
 *
 *    This file is part of fldigi.
 *
 *    fldigi is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    fldigi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty oF
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with MT63; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>

#include <stdio.h> // only for control printf's
// #include <alloc.h>
#include <iostream>

#include "dsp.h"

#include "mt63base.h"

#include "symbol.dat"      // symbol shape
#include "mt63intl.dat" // interleave patterns
#include "alias_k5.dat" // anti-alias filter shapes
#include "alias_1k.dat" // for 500, 1000 and 2000 Hz modes
#include "alias_2k.dat"


// ==========================================================================
// MT63 transmitter code

MT63tx::MT63tx()
{ 
    TxVect = NULL; 
    dspPhaseCorr = NULL; 
}

MT63tx::~MT63tx()
{ 
    free(TxVect); 
    free(dspPhaseCorr); 
}

void MT63tx::Free(void)
{ 
    free(TxVect);
    TxVect = NULL;
    free(dspPhaseCorr); 
    dspPhaseCorr = NULL;
    Encoder.Free();
    FFT.Free();
    Window.Free();
    Comb.Free();
    WindowBuff.Free();
}

int MT63tx::Preset(int BandWidth, int LongInterleave)
{ 
    int i, p, step, incr, mask;

    switch(BandWidth) { 
        case 500:
            FirstDataCarr = 256;
            AliasShapeI = Alias_k5_I;
            AliasShapeQ = Alias_k5_Q;
            AliasFilterLen = Alias_k5_Len;
            DecimateRatio = 8;
            break;
        case 1000:
            FirstDataCarr = 128;
            AliasShapeI = Alias_1k_I;
            AliasShapeQ = Alias_1k_Q;
            AliasFilterLen = Alias_1k_Len;
            DecimateRatio = 4;
            break;
        case 2000:
            FirstDataCarr = 64;
            AliasShapeI = Alias_2k_I;
            AliasShapeQ = Alias_2k_Q;
            AliasFilterLen = Alias_2k_Len;
            DecimateRatio = 2;
            break;
          default: 
              return -1;
    }

    DataCarriers = 64;
// DataCarrSepar=4;
// SymbolSepar=200;
    WindowLen = SymbolLen;
    TxWindow = SymbolShape;
    TxdspAmpl = 4.0 / DataCarriers; // for maximum output level we can set TxdspAmpl=4.0/DataCarriers
    CarrMarkCode = 0x16918BBEL;
    CarrMarkdspAmpl = 0; // WindowLen/32;

    if (LongInterleave) { 
        DataInterleave = 64; 
        InterleavePattern = LongIntlvPatt; 
    }
    else { 
        DataInterleave = 32; 
        InterleavePattern = ShortIntlvPatt; 
    }

    if (dspRedspAllocArray(&TxVect, DataCarriers))
        goto Error;
    if (dspRedspAllocArray(&dspPhaseCorr, DataCarriers))
        goto Error;
    if (WindowBuff.EnsureSpace(2 * WindowLen))
        goto Error;
    WindowBuff.Len = 2 * WindowLen;

    if (Encoder.Preset(DataCarriers, DataInterleave, InterleavePattern, 1))
        goto Error;
    if (FFT.Preset(WindowLen))
        goto Error;
    if (Window.Preset(WindowLen, SymbolSepar / 2, TxWindow))
        goto Error;
    if (Comb.Preset(AliasFilterLen, AliasShapeI, AliasShapeQ, DecimateRatio))
        goto Error;

    mask = FFT.Size - 1;

// Preset the initial dspPhase for each data carrier.
// Here we only compute indexes to the FFT twiddle factors
// so the actual vector is FFT.Twiddle[TxVect[i]]

    for (step = 0, incr = 1, p = 0, i = 0; i < DataCarriers; i++) { 
        TxVect[i] = p; 
        step += incr; 
        p = (p + step) & mask; 
    }

// compute dspPhase correction between successive FFTs separated by SymbolSepar
// Like above we compute indexes to the FFT.Twiddle[]

    incr = (SymbolSepar * DataCarrSepar) & mask;
    for (p = (SymbolSepar * FirstDataCarr) & mask, i = 0; i < DataCarriers; i++) { 
        dspPhaseCorr[i] = p; 
        p = (p + incr) & mask;
    }
    return 0;
Error: 
    Free(); 
    return -1;
}

int MT63tx::SendTune(void)
{
    int i, c, r, mask;
    double dspAmpl;

    mask = FFT.Size - 1;
    dspAmpl = TxdspAmpl * sqrt(DataCarriers / 2);

    for (i = 0; i < DataCarriers; i++)
        TxVect[i] = (TxVect[i] + dspPhaseCorr[i]) & mask;

    for (i = 0; i < 2 * WindowLen; i++) 
        WindowBuff.Data[i].im = WindowBuff.Data[i].re = 0.0;

    i = 0;
    c = FirstDataCarr;
    r = FFT.BitRevIdx[c];
    WindowBuff.Data[r].re = dspAmpl * FFT.Twiddle[TxVect[i]].re;
    WindowBuff.Data[r].im = (-dspAmpl) * FFT.Twiddle[TxVect[i]].im;

    i = DataCarriers - 1;
    c = FirstDataCarr + i * DataCarrSepar;
    c &= mask;
    r = WindowLen + FFT.BitRevIdx[c];
    WindowBuff.Data[r].re = dspAmpl * FFT.Twiddle[TxVect[i]].re;
    WindowBuff.Data[r].im = (-dspAmpl) * FFT.Twiddle[TxVect[i]].im;

// inverse FFT: WindowBuff is already scrmabled
    FFT.CoreProc(WindowBuff.Data);
    FFT.CoreProc(WindowBuff.Data + WindowLen);
// negate the imaginary part for the IFFT
    for (i = 0; i < 2 * WindowLen; i++) 
        WindowBuff.Data[i].im *= (-1.0);

    Window.Process(&WindowBuff);

    Comb.Process(&Window.Output);

    return 0;
}

int MT63tx::SendChar(char ch)
{
    int i,mask,flip;

    Encoder.Process(ch); // encode and interleave the character

// print the character and the DataBlock being sent
//    printf("0x%02x [%c] => ", ch, ch>=' ' ? ch : '.');
//    for (i=0; i<DataCarriers; i++) printf("%c",'0'+Encoder.Output[i]);
//  printf("\n");

// here we encode the Encoder.Output into dspPhase flips

    mask = FFT.Size - 1;
    flip = FFT.Size / 2;
    for (i = 0; i < DataCarriers; i++) {
// data bit = 1 => only dspPhase correction
        if (Encoder.Output[i])
            TxVect[i] = (TxVect[i] + dspPhaseCorr[i]) & mask;
// data bit = 0 => dspPhase flip + dspPhase correction
        else
            TxVect[i] = (TxVect[i] + dspPhaseCorr[i] + flip) & mask;
    }

    ProcessTxVect();
    return 0;
}

int MT63tx::SendJam(void)
{ 
    int i,mask,left,right;

    mask = FFT.Size-1;
    left = FFT.Size / 4;
    right = 3 * (FFT.Size / 4);
    for (i = 0; i < DataCarriers; i++) {
        if (rand() & 0x100) // turn left 90 degrees
            TxVect[i] = (TxVect[i] + dspPhaseCorr[i] + left) & mask; 
        else                // turn right 90 degrees
            TxVect[i] = (TxVect[i] + dspPhaseCorr[i] + right) & mask;
    }

    ProcessTxVect();
    return 0;
}

int MT63tx::ProcessTxVect(void)
{ 
    int i, c, r, mask;

     mask = FFT.Size - 1;

    for (i = 0; i < 2 * WindowLen; i++) 
        WindowBuff.Data[i].im = WindowBuff.Data[i].re = 0.0;

    for ( c = FirstDataCarr, i = 0; 
          i < DataCarriers; 
          i += 2, c = (c + 2 * DataCarrSepar) & mask) { 
        r = FFT.BitRevIdx[c];
        WindowBuff.Data[r].re = TxdspAmpl*FFT.Twiddle[TxVect[i]].re;
        WindowBuff.Data[r].im = (-TxdspAmpl)*FFT.Twiddle[TxVect[i]].im; 
    }
    for ( c = FirstDataCarr + DataCarrSepar, i = 1; 
          i < DataCarriers; 
          i += 2, c = (c + 2 * DataCarrSepar) & mask) {
        r = WindowLen + FFT.BitRevIdx[c];
        WindowBuff.Data[r].re = TxdspAmpl * FFT.Twiddle[TxVect[i]].re;
        WindowBuff.Data[r].im = (-TxdspAmpl) * FFT.Twiddle[TxVect[i]].im; 
    }
    FFT.CoreProc(WindowBuff.Data);
    FFT.CoreProc(WindowBuff.Data + WindowLen);

// negate the imaginary part for the IFFT
    for (i = 0; i < 2 * WindowLen; i++) 
        WindowBuff.Data[i].im *= (-1.0);

// we could be faster by avoiding Scramble and using the FFT.RevIdx[]
    Window.Process(&WindowBuff);

// audio output to be sent out is in Comb.Output
    Comb.Process(&Window.Output);

  return 0;
}

int MT63tx::SendSilence(void)
{ 
    Window.ProcessSilence(2);
    Comb.Process(&Window.Output);
    return 0;
}

// ==========================================================================
// Character encoder and block interleave for the MT63 modem

MT63encoder::MT63encoder()
{ 
    IntlvPipe = NULL;
    WalshBuff = NULL;
    Output = NULL;
    IntlvPatt=NULL;
}

MT63encoder::~MT63encoder()
{
    free(IntlvPipe);
    free(WalshBuff);
    free(Output);
    free(IntlvPatt);
}

void MT63encoder::Free()
{ 
    free(IntlvPipe);
    free(WalshBuff);
    free(Output);
    free(IntlvPatt);
    IntlvPipe = NULL; 
    WalshBuff = NULL;
    Output = NULL;
    IntlvPatt = NULL;
}

int MT63encoder::Preset(int Carriers, int Intlv, int *Pattern, int PreFill)
{
    int i, p;
    if (!dspPowerOf2(Carriers)) goto Error;

    DataCarriers = Carriers;
    IntlvLen = Intlv;
    IntlvSize = IntlvLen * DataCarriers;
    if (IntlvLen) {
        if (dspRedspAllocArray(&IntlvPipe, IntlvSize)) goto Error;
        if (PreFill) 
            for (i = 0; i < IntlvSize; i++) 
                IntlvPipe[i] = rand() & 1;
        else 
            dspClearArray(IntlvPipe,IntlvSize);
        if (dspRedspAllocArray(&IntlvPatt, DataCarriers)) goto Error;
        IntlvPtr = 0;
    }
    if (dspRedspAllocArray(&WalshBuff, DataCarriers)) goto Error;
    if (dspRedspAllocArray(&Output, DataCarriers)) goto Error;
    CodeMask = 2 * DataCarriers - 1;

    for (p = 0, i = 0; i < DataCarriers; i++) {
        IntlvPatt[i] = p * DataCarriers;
        p += Pattern[i];
        if (p >= IntlvLen) p -= IntlvLen;
    }
    return 0;

Error: 
    Free();
    return -1;
}

int MT63encoder::Process(char code) // encode an ASCII character "code"
{ 
    int i, k;
    code &= CodeMask;
    for (i = 0; i < DataCarriers; i++) 
        WalshBuff[i] = 0;
    if (code < DataCarriers) 
        WalshBuff[(int)code] = 1.0;
    else WalshBuff[code-DataCarriers] = (-1.0);
    
    dspWalshInvTrans(WalshBuff, DataCarriers);

    if (IntlvLen) {
        for (i = 0; i < DataCarriers; i++)
            IntlvPipe[IntlvPtr + i] = (WalshBuff[i] < 0.0);
        for (i = 0; i < DataCarriers; i++) {
            k = IntlvPtr + IntlvPatt[i];
            if (k >= IntlvSize) 
                k -= IntlvSize;
            Output[i] = IntlvPipe[k+i];
        }
        IntlvPtr += DataCarriers;
        if (IntlvPtr >= IntlvSize)
            IntlvPtr -= IntlvSize;
    } else
        for (i = 0; i < DataCarriers; i++) 
            Output[i] = (WalshBuff[i] < 0.0);

    return 0;
}

// After encoding the "Output" array contains the bits to be transmitted

// ==========================================================================
// MT63 decoder and deinterleaver

MT63decoder::MT63decoder()
{ 
    IntlvPipe = NULL;
    IntlvPatt = NULL;
    WalshBuff = NULL;
    DecodeSnrMid = NULL;
    DecodeSnrOut = NULL;
    DecodePipe = NULL;
}

MT63decoder::~MT63decoder()
{
    free(IntlvPipe);
    free(IntlvPatt);
    free(WalshBuff);
    free(DecodeSnrMid);
    free(DecodeSnrOut);
    free(DecodePipe);
}

void MT63decoder::Free()
{
    free(IntlvPipe);
    IntlvPipe = NULL;
    free(IntlvPatt);
    IntlvPatt = NULL;
    free(WalshBuff);
    WalshBuff = NULL;
    free(DecodeSnrMid);
    free(DecodeSnrOut);
    DecodeSnrMid = NULL;
    DecodeSnrOut = NULL;
    free(DecodePipe);
    DecodePipe = NULL;
}

int MT63decoder::Preset(int Carriers, int Intlv, int *Pattern, int Margin, int Integ)
{ 
    int i,p;

    if (!dspPowerOf2(Carriers)) goto Error;
    DataCarriers = Carriers; 
    ScanLen = 2 * Margin + 1;
    ScanSize = DataCarriers + 2 * Margin;

    dspLowPass2Coeff(Integ,W1,W2,W5);
    DecodeLen = Integ / 2;
    DecodeSize = DecodeLen * ScanLen;
    if (dspRedspAllocArray(&DecodePipe, DecodeSize)) goto Error;
    dspClearArray(DecodePipe, DecodeSize);
    DecodePtr = 0;

    IntlvLen = Intlv; // printf("%d:",IntlvLen);
    if (dspRedspAllocArray(&IntlvPatt, DataCarriers)) goto Error;
    for (p = 0, i = 0; i < DataCarriers; i++) {
        IntlvPatt[i] = p * ScanSize; // printf(" %2d",p);
        p += Pattern[i];
        if (p >= IntlvLen) p -= IntlvLen; 
    }
  // printf("\n");

    IntlvSize = (IntlvLen + 1) * ScanSize;
    if (dspRedspAllocArray(&IntlvPipe, IntlvSize)) goto Error;
    dspClearArray(IntlvPipe, IntlvSize);
    IntlvPtr = 0;

    if (dspRedspAllocArray(&WalshBuff, DataCarriers)) goto Error;

    if (dspRedspAllocArray(&DecodeSnrMid, ScanLen)) goto Error;
    if (dspRedspAllocArray(&DecodeSnrOut, ScanLen)) goto Error;
    dspClearArray(DecodeSnrMid, ScanLen);
    dspClearArray(DecodeSnrOut, ScanLen);

    SignalToNoise = 0.0;
    CarrOfs = 0;

    return 0;
Error:
    Free();
    return -1;
}

int MT63decoder::Process(double *data)
{
    int s, i, k;
    double  Min, Max, Sig, Noise, SNR;
    int MinPos,MaxPos,code;

    dspCopyArray(IntlvPipe + IntlvPtr, data, ScanSize);

// printf("Decoder [%d/%d/%d]: \n",IntlvPtr,IntlvSize,ScanSize);
    for (s = 0; s < ScanLen; s++) {
// printf(" %2d:",s);
        for (i = 0; i < DataCarriers; i++) {
            k = IntlvPtr - ScanSize - IntlvPatt[i];
            if (k < 0) k += IntlvSize;
            if ((s & 1) && (i & 1)) { 
                k += ScanSize; 
                if (k >= IntlvSize) k-=IntlvSize;
            }
            WalshBuff[i] = IntlvPipe[k + s + i]; 
// printf(" %4d",k/ScanSize);
        } 
// printf("\n");
        dspWalshTrans(WalshBuff, DataCarriers);
        Min = dspFindMin(WalshBuff, DataCarriers, MinPos);
        Max = dspFindMax(WalshBuff, DataCarriers, MaxPos);
        if (fabs(Max) > fabs(Min)) {
            code = MaxPos + DataCarriers;
            Sig = fabs(Max);
            WalshBuff[MaxPos] = 0.0;
        } else {
            code = MinPos;
            Sig = fabs(Min);
            WalshBuff[MinPos] = 0.0;
        }
        Noise = dspRMS(WalshBuff, DataCarriers);
        if (Noise > 0.0) 
            SNR = Sig/Noise;
        else SNR = 0.0;
        dspLowPass2(SNR, DecodeSnrMid[s], DecodeSnrOut[s], W1, W2, W5);
// printf("%2d: %02x => %c,  %5.2f/%5.2f=>%5.2f  <%5.2f>\n",
//       s,code,code<' ' ? '.' : (char)code,
//       Sig,Noise,SNR,DecodeSnrOut[s]);
        DecodePipe[DecodePtr+s]=code;
    }
    IntlvPtr += ScanSize;
    if (IntlvPtr >= IntlvSize) IntlvPtr = 0;
    DecodePtr += ScanLen; 
    if (DecodePtr >= DecodeSize) DecodePtr = 0;
    Max = dspFindMax(DecodeSnrOut, ScanLen, MaxPos);
    Output = DecodePipe[DecodePtr + MaxPos];
    SignalToNoise = Max;
    CarrOfs = MaxPos - (ScanLen - 1) / 2;
/*
  code=Output;
  if ((code>=' ')||(code=='\n')||(code=='\r')) printf("%c",code);
  else if (code!='\0') printf("<%02X>",code);
*/
    return 0;
}

// ==========================================================================
// MT63 receiver code

MT63rx::MT63rx()
{ 
    int s;

    FFTbuff = NULL;
    FFTbuff2 = NULL;

    for (s = 0; s < 4; s++) 
        SyncPipe[s] = NULL;
    SyncPhCorr = NULL;
    for (s = 0; s < 4; s++) {
        CorrelMid[s] = NULL;
        CorrelOut[s] = NULL;
    }
    dspPowerMid = NULL;
    dspPowerOut = NULL;
    for (s = 0; s < 4; s++)
        CorrelNorm[s] = NULL;
    for (s = 0; s < 4; s++)
        CorrelAver[s] = NULL;
    SymbFit = NULL;
    SymbPipe = NULL;
    FreqPipe = NULL;

    RefDataSlice = NULL;

    DataPipeLen = 0;
    DataPipe = NULL;
    DataPwrMid = NULL;
    DataPwrOut = NULL;
    DataSqrMid = NULL;
    DataSqrOut = NULL;

    DataVect = NULL;

    DatadspPhase = NULL;
    DatadspPhase2 = NULL;

    SpectradspPower = NULL;
}

MT63rx::~MT63rx()
{ 
    int s;

    free(FFTbuff);
    free(FFTbuff2);

    for (s = 0; s < 4; s++) 
        free(SyncPipe[s]);
    free(SyncPhCorr);
    for (s = 0; s < 4; s++) { 
        free(CorrelMid[s]);
        free(CorrelOut[s]);
    }
    free(dspPowerMid);
    free(dspPowerOut);
    for (s = 0; s < 4; s++)
        free(CorrelNorm[s]);
    for (s = 0; s < 4; s++)
        free(CorrelAver[s]);
    free(SymbFit);
    free(SymbPipe);
    free(FreqPipe);

    free(RefDataSlice);

    dspFreeArray2D(DataPipe, DataPipeLen);
// for (s=0; s<DataPipeLen; s++) free(DataPipe[s]); free(DataPipe);
    free(DataPwrMid);
    free(DataPwrOut);
    free(DataSqrMid);
    free(DataSqrOut);

    free(DataVect);

    free(DatadspPhase);
    free(DatadspPhase2);

    free(SpectradspPower);
}

void MT63rx::Free(void)
{
    int s;
    FFT.Free(); 
    InpSplit.Free();
    TestOfs.Free();
    ProcLine.Free();

    free(FFTbuff);
    FFTbuff = NULL;
    free(FFTbuff2);
    FFTbuff2 = NULL;

    for (s = 0; s < 4; s++) { 
        free(SyncPipe[s]); 
        SyncPipe[s] = NULL;
    }
    free(SyncPhCorr); 
    SyncPhCorr = NULL;
    for (s = 0; s < 4; s++) {
        free(CorrelMid[s]);
        CorrelMid[s] = NULL;
        free(CorrelOut[s]);
        CorrelOut[s] = NULL;
    }
    free(dspPowerMid);
    dspPowerMid = NULL;
    free(dspPowerOut);
    dspPowerOut = NULL;
    for (s = 0; s < 4; s++) {
        free(CorrelNorm[s]);
        CorrelNorm[s] = NULL;
    }
    for (s = 0; s < 4; s++) {
        free(CorrelAver[s]);
        CorrelAver[s] = NULL;
    }
    free(SymbFit);
    SymbFit = NULL;
    free(SymbPipe);
    SymbPipe = NULL;
    free(FreqPipe);
    FreqPipe = NULL;

    free(RefDataSlice);
    RefDataSlice = NULL;

    dspFreeArray2D(DataPipe, DataPipeLen);
// for (s=0; s<DataPipeLen; s++) free(DataPipe[s]); free(DataPipe);

    DataPipeLen = 0;
    DataPipe = NULL;

    free(DataPwrMid);
    free(DataPwrOut);
    DataPwrMid = NULL;
    DataPwrOut = NULL;
    free(DataSqrMid);
    free(DataSqrOut);
    DataSqrMid = NULL;
    DataSqrOut = NULL;

    free(DataVect);
    DataVect = NULL;

    free(DatadspPhase);
    DatadspPhase = NULL;
    free(DatadspPhase2);
    DatadspPhase2 = NULL;

    Decoder.Free();

    free(SpectradspPower); 
    SpectradspPower = NULL;
}

int MT63rx::Preset(int BandWidth, int LongInterleave, int Integ,
           void (*Display)(double *Spectra, int Len))
{ 
    int err,s,i,c;

    switch(BandWidth) {
    case 500:
        FirstDataCarr = 256;
        AliasShapeI = Alias_k5_I;
        AliasShapeQ = Alias_k5_Q;
        AliasFilterLen = Alias_k5_Len;
        DecimateRatio = 8;
        break;
    case 1000:
        FirstDataCarr = 128;
        AliasShapeI = Alias_1k_I;
        AliasShapeQ = Alias_1k_Q;
        AliasFilterLen = Alias_1k_Len;
        DecimateRatio = 4;
        break;
    case 2000:
        FirstDataCarr = 64;
        AliasShapeI = Alias_2k_I;
        AliasShapeQ = Alias_2k_Q;
        AliasFilterLen = Alias_2k_Len;
        DecimateRatio = 2;
        break;
    default:
        return -1;
    }

    DataCarriers = 64;    // 64 carriers

// DataCarrSepar=4;    // carrier each 4 FFT bins
// SymbolSepar=200;    // symbol each 200 decimated sdspAmples

    WindowLen = SymbolLen;    // the symbol length
    RxWindow = SymbolShape;    // the symbol shape

// RxWindow, WindowLen, SymbolSepar, DataCarrSepar are tuned one for another
// to minimize inter-symbol interference (ISI) and one should not change
// them independently or ISI will increase.

    CarrMarkCode = 0x16918BBEL;

    IntegLen = Integ;    // sync. integration period
    SymbolDiv = 4;        // don't change this
    ScanMargin = 8;        // we look 8 data carriers up and down
    SyncStep = SymbolSepar/SymbolDiv;

  // EnvSync.Preset(WindowLen,FirstDataCarr,DataCarrSepar,DataCarriers,SymbolDiv,ScanMargin,IntegLen);

// under MSDOS (or at least under Borland C++ 3.1) we can't make
// a long dspDelay pipe due to the 64K limit for an array
//#ifdef __MSDOS__
//  if (IntegLen<=16) ProcdspDelay=IntegLen*SymbolSepar;
//          else ProcdspDelay=16*SymbolSepar;
//#else
    ProcdspDelay = IntegLen * SymbolSepar;
//#endif

    TrackPipeLen = IntegLen;

    if (LongInterleave) {
        DataInterleave = 64;
        InterleavePattern = LongIntlvPatt;
    } else {
        DataInterleave = 32;
        InterleavePattern = ShortIntlvPatt;
    }

    DataScanMargin = 8;

//  printf("[1] Coreleft=%lu\n",coreleft());

    err = FFT.Preset(WindowLen);
    if (err) goto Error;

    if (dspRedspAllocArray(&FFTbuff, WindowLen))  goto Error;
    if (dspRedspAllocArray(&FFTbuff2, WindowLen)) goto Error;
    WindowLenMask = WindowLen - 1;

    err = InpSplit.Preset(AliasFilterLen, AliasShapeI, AliasShapeQ, DecimateRatio);
    if (err) goto Error;

    err = TestOfs.Preset(-0.25 * (2.0 * M_PI / WindowLen)); // for decoder tests only
    if (err) goto Error;

    err = ProcLine.Preset(ProcdspDelay + WindowLen + SymbolSepar);
    if (err) goto Error;
    SyncProcPtr = 0;

//  printf("[2] Coreleft=%lu\n",coreleft());

    ScanFirst = FirstDataCarr - ScanMargin * DataCarrSepar; // first FFT bin to scan
    if (ScanFirst < 0) ScanFirst += WindowLen;
    ScanLen = (DataCarriers + 2 * ScanMargin) * DataCarrSepar; // number of FFT bins to scan

    for (s = 0; s < SymbolDiv; s++) {
        if (dspRedspAllocArray(&SyncPipe[s], ScanLen)) goto Error;
        dspClearArray(SyncPipe[s], ScanLen);
    }
    SyncPtr = 0;

    if (dspRedspAllocArray(&SyncPhCorr, ScanLen)) goto Error;
    
    for (c = (ScanFirst * SymbolSepar) & WindowLenMask, i = 0; i < ScanLen; i++) {
        SyncPhCorr[i].re = FFT.Twiddle[c].re * FFT.Twiddle[c].re - 
                           FFT.Twiddle[c].im * FFT.Twiddle[c].im;
        SyncPhCorr[i].im = 2 * FFT.Twiddle[c].re * FFT.Twiddle[c].im;
        c = (c + SymbolSepar) & WindowLenMask;
    }

//  printf("[3] Coreleft=%lu\n",coreleft());

    for (s = 0; s < SymbolDiv; s++) {
        if (dspRedspAllocArray(&CorrelMid[s], ScanLen)) goto Error;
        dspClearArray(CorrelMid[s], ScanLen);
        if (dspRedspAllocArray(&CorrelOut[s], ScanLen)) goto Error;
        dspClearArray(CorrelOut[s], ScanLen);
    }
    dspLowPass2Coeff(IntegLen, W1, W2, W5);

    if (dspRedspAllocArray(&dspPowerMid, ScanLen)) goto Error;
    dspClearArray(dspPowerMid, ScanLen);
    if (dspRedspAllocArray(&dspPowerOut, ScanLen)) goto Error;
    dspClearArray(dspPowerOut, ScanLen);
    dspLowPass2Coeff(IntegLen * SymbolDiv, W1p, W2p, W5p);

//  printf("[4] Coreleft=%lu\n",coreleft());

    for (s = 0; s < SymbolDiv; s++) {
        if (dspRedspAllocArray(&CorrelNorm[s], ScanLen)) goto Error;
    }

    FitLen = 2 * ScanMargin * DataCarrSepar;

//  printf("[5] Coreleft=%lu\n",coreleft());

    for (s = 0; s < SymbolDiv; s++) {
        if (dspRedspAllocArray(&CorrelAver[s], FitLen)) goto Error;
    }

//  printf("[6] Coreleft=%lu\n",coreleft());

    if (dspRedspAllocArray(&SymbFit, FitLen)) goto Error;

    if (dspRedspAllocArray(&SymbPipe, TrackPipeLen)) goto Error;
    dspClearArray(SymbPipe, TrackPipeLen);
    if (dspRedspAllocArray(&FreqPipe, TrackPipeLen)) goto Error;
    dspClearArray(FreqPipe, TrackPipeLen);
    TrackPipePtr = 0;

//  printf("[7] Coreleft=%lu\n",coreleft());

    SymbFitPos = ScanMargin * DataCarrSepar;
    SyncLocked = 0;
    SyncSymbConf = 0.0;
    SyncFreqOfs = 0.0;
    SyncFreqDev = 0.0;
    SymbPtr = 0;
    SyncSymbShift = 0.0;

    SyncHoldThres = 1.5 * sqrt(1.0 / (IntegLen * DataCarriers));
    SyncLockThres = 1.5 * SyncHoldThres;

    DataProcPtr = (-ProcdspDelay);

//  printf("SyncLockThres=%5.2f, SyncHoldThres=%5.2f\n",
//     SyncLockThres,SyncHoldThres);

    DataScanLen = DataCarriers + 2 * DataScanMargin;
    DataScanFirst = FirstDataCarr - DataScanMargin * DataCarrSepar;

    if (dspRedspAllocArray(&RefDataSlice, DataScanLen)) goto Error;
    dspClearArray(RefDataSlice, DataScanLen);

    dspFreeArray2D(DataPipe, DataPipeLen);
    DataPipeLen = IntegLen / 2;
    dspLowPass2Coeff(IntegLen, dW1, dW2, dW5);
    if (dspAllocArray2D(&DataPipe, DataPipeLen, DataScanLen)) {
        DataPipeLen = 0;
        goto Error;
    }
    dspClearArray2D(DataPipe, DataPipeLen, DataScanLen);

/*
  if (dspRedspAllocArray(&DataPipe,DataPipeLen)) goto Error;
  for (s=0; s<DataPipeLen; s++) DataPipe[s]=NULL;
  for (s=0; s<DataPipeLen; s++)
  { if (dspRedspAllocArray(&DataPipe[s],DataScanLen)) goto Error;
    dspClearArray(DataPipe[s],DataScanLen); }
*/

    DataPipePtr = 0;

    if (dspRedspAllocArray(&DataPwrMid, DataScanLen)) goto Error;
    dspClearArray(DataPwrMid, DataScanLen);
    if (dspRedspAllocArray(&DataPwrOut, DataScanLen)) goto Error;
    dspClearArray(DataPwrOut, DataScanLen);

    if (dspRedspAllocArray(&DataSqrMid, DataScanLen)) goto Error;
    dspClearArray(DataSqrMid, DataScanLen);
    if (dspRedspAllocArray(&DataSqrOut, DataScanLen)) goto Error;
    dspClearArray(DataSqrOut, DataScanLen);

    if (dspRedspAllocArray(&DataVect, DataScanLen)) goto Error;

    if (dspRedspAllocArray(&DatadspPhase, DataScanLen)) goto Error;
    if (dspRedspAllocArray(&DatadspPhase2, DataScanLen)) goto Error;

    err = Decoder.Preset(DataCarriers, DataInterleave, 
                         InterleavePattern, DataScanMargin, IntegLen);
    if (err) goto Error;

    SpectraDisplay = Display;
    if (SpectraDisplay) {
        if (dspRedspAllocArray(&SpectradspPower, WindowLen))
            goto Error;
    }

//  printf("[8] Coreleft=%lu\n",coreleft());

    return 0;

Error:
    Free();
    return -1;
}

int MT63rx::Process(double_buff *Input)
{ 
    int s1,s2;

//  TestOfs.Omega+=(-0.005*(2.0*M_PI/512)); // simulate frequency drift

    Output.Len = 0;

    InpSplit.Process(Input);

    ProcLine.Process(&InpSplit.Output);
//  TestOfs.Process(&InpSplit.Output);
//  ProcLine.Process(&TestOfs.Output);

// printf("New input, Len=%d/%d\n",Input->Len,ProcLine.InpLen);

    while((SyncProcPtr+WindowLen) < ProcLine.InpLen) {
        SyncProcess(ProcLine.InpPtr + SyncProcPtr);
// printf("SyncSymbConf=%5.2f, SyncLock=%d, SyncProcPtr=%d, SyncPtr=%d, SymbPtr=%d, SyncSymbShift=%5.1f, SyncFreqOfs=%5.2f =>",
//        SyncSymbConf,SyncLocked,SyncProcPtr,SyncPtr,SymbPtr,SyncSymbShift,SyncFreqOfs);
        if (SyncPtr == SymbPtr) {
            s1 = SyncProcPtr - ProcdspDelay + 
                 ((int)SyncSymbShift - SymbPtr * SyncStep);
        s2 = s1 + SymbolSepar / 2;
//      printf(" SdspAmple at %d,%d (SyncProcPtr-%d), time diff.=%d\n",s1,s2,SyncProcPtr-s1,s1-DataProcPtr);
        DataProcess(ProcLine.InpPtr + s1, ProcLine.InpPtr + s2,
                    SyncFreqOfs, s1 - DataProcPtr);
        DataProcPtr = s1;
    }
// printf("\n");
    SyncProcPtr += SyncStep;
    }
    SyncProcPtr -= ProcLine.InpLen;
    DataProcPtr -= ProcLine.InpLen;
    return 0;
}

void MT63rx::DoCorrelSum(dspCmpx *Correl1, dspCmpx *Correl2, dspCmpx *Aver)
{
    dspCmpx sx;
    int i, s, d;
  
    s = 2 * DataCarrSepar;
    d = DataCarriers * DataCarrSepar;
    sx.re = sx.im = 0.0;
    for (i = 0; i < d; i+=s) {
        sx.re += Correl1[i].re;
        sx.im += Correl1[i].im;
        sx.re += Correl2[i].re;
        sx.im += Correl2[i].im;
    }
    Aver[0].re = sx.re / DataCarriers;
    Aver[0].im = sx.im / DataCarriers;
    for (i = 0; i < (FitLen-s); ) {
        sx.re -= Correl1[i].re;
        sx.im -= Correl1[i].im;
        sx.re -= Correl2[i].re;
        sx.im -= Correl2[i].im;
        sx.re += Correl1[i+d].re;
        sx.im -= Correl1[i+d].im;
        sx.re += Correl2[i+d].re;
        sx.im -= Correl2[i+d].im;
        i += s;
        Aver[i].re = sx.re / DataCarriers;
        Aver[i].im = sx.im / DataCarriers; }
}

void MT63rx::SyncProcess(dspCmpx *Slice)
{
    int i, j, k, r, s, s2;
    double pI, pQ;
    dspCmpx Correl;
    dspCmpx *PrevSlice;
    double I, Q;
    double dI, dQ;
    double P,A;
    double w0, w1;
    double Fl, F0, Fu;
    dspCmpx SymbTime;
    double SymbConf, SymbShift, FreqOfs;
    double dspRMS;
    int Loops, Incl;

    SyncPtr = (SyncPtr + 1) & (SymbolDiv - 1); // increment the correlators pointer

    for (i = 0; i < WindowLen; i++) {
        r = FFT.BitRevIdx[i];
        FFTbuff[r].re = Slice[i].re * RxWindow[i];
        FFTbuff[r].im = Slice[i].im * RxWindow[i];
    }
    FFT.CoreProc(FFTbuff);

    if (SpectraDisplay) {
        for ( i = 0, 
                j = FirstDataCarr + (DataCarriers / 2) * DataCarrSepar - 
                WindowLen / 2;
              (i < WindowLen) && ( j <WindowLen); i++,j++)
            SpectradspPower[i] = dspPower(FFTbuff[j]);
        for (j = 0; (i < WindowLen) && (j < WindowLen); i++,j++)
            SpectradspPower[i] = dspPower(FFTbuff[j]);
        (*SpectraDisplay)(SpectradspPower, WindowLen);
    }

// EnvSync.Process(FFTbuff); // experimental synchronizer

    PrevSlice = SyncPipe[SyncPtr];
    for (i = 0; i < ScanLen; i++) {
        k = (ScanFirst+i) & WindowLenMask;
        I = FFTbuff[k].re;
        Q = FFTbuff[k].im;
        P = I * I + Q * Q;
        A = sqrt(P);
        if (P > 0.0) {
            dI = (I * I - Q * Q) / A;
            dQ = (2 * I * Q) / A;
        } else { 
            dI = dQ = 0.0;
        }
        dspLowPass2(P, dspPowerMid[i], dspPowerOut[i], W1p, W2p, W5p);
        pI = PrevSlice[i].re * SyncPhCorr[i].re - 
             PrevSlice[i].im * SyncPhCorr[i].im;
        pQ = PrevSlice[i].re * SyncPhCorr[i].im +
             PrevSlice[i].im * SyncPhCorr[i].re;
        Correl.re = dQ * pQ + dI * pI;
        Correl.im = dQ * pI - dI * pQ;
        dspLowPass2(&Correl, CorrelMid[SyncPtr] + i,
                    CorrelOut[SyncPtr] + i, W1, W2, W5);
        PrevSlice[i].re = dI;
        PrevSlice[i].im = dQ;
    }

    if (SyncPtr == (SymbPtr^2)) {
        for (s = 0; s < SymbolDiv; s++) { // normalize the correlations
            for (i = 0; i < ScanLen; i++) {
                if (dspPowerOut[i] > 0.0) {
                    CorrelNorm[s][i].re = CorrelOut[s][i].re / dspPowerOut[i];
                    CorrelNorm[s][i].im = CorrelOut[s][i].im / dspPowerOut[i]; 
                } else 
                    CorrelNorm[s][i].im = CorrelNorm[s][i].re = 0.0;
            }
        }

/*
    // another way to normalize - a better one ?
    for (i=0; i<ScanLen; i++)
    { for (P=0.0,s=0; s<SymbolDiv; s++)
        P+=dspPower(CorrelOut[s][i]);
      if (P>0.0)
      { for (s=0; s<SymbolDiv; s++)
        { CorrelNorm[s][i].re=CorrelOut[s][i].re/P;
          CorrelNorm[s][i].im=CorrelOut[s][i].im/P; }
      } else
      { for (s=0; s<SymbolDiv; s++)
          CorrelNorm[s][i].re=CorrelNorm[s][i].im=0.0; }    
    }
*/
// make a sum for each possible carrier positions
        for (s = 0; s < SymbolDiv; s++) { 
            s2 = (s + SymbolDiv / 2) & (SymbolDiv - 1);
            for (k = 0; k < 2 * DataCarrSepar; k++)
                DoCorrelSum( CorrelNorm[s] + k,
                             CorrelNorm[s2] + k + DataCarrSepar,
                             CorrelAver[s] + k);
        }
// symbol-shift dspPhase fitting
        for (i = 0; i < FitLen; i++)  {
            SymbFit[i].re = dspAmpl(CorrelAver[0][i]) - 
                            dspAmpl(CorrelAver[2][i]);
            SymbFit[i].im = dspAmpl(CorrelAver[1][i]) - 
                            dspAmpl(CorrelAver[3][i]);
        }

//    P=dspFindMaxdspPower(SymbFit+30,4,j); j+=30;
        P = dspFindMaxdspPower(SymbFit + 2, FitLen- 4 , j);
        j += 2;
//    printf("[%2d,%2d]",j,SymbFitPos);
        k = (j - SymbFitPos) / DataCarrSepar;
        if (k > 1)
            j -= (k - 1) * DataCarrSepar; 
        else if (k < (-1)) 
            j -= (k + 1) * DataCarrSepar;
        SymbFitPos = j;
//    printf(" => %2d",j);
        if (P > 0.0) {
            SymbConf = dspAmpl(SymbFit[j]) + 
                       0.5 * (dspAmpl(SymbFit[j + 1]) + dspAmpl(SymbFit[j - 1]));
            SymbConf *= 0.5;
            I = SymbFit[j].re + 0.5 * (SymbFit[j - 1].re + SymbFit[j + 1].re);
            Q = SymbFit[j].im + 0.5 * (SymbFit[j - 1].im + SymbFit[j + 1].im);
            SymbTime.re = I;
            SymbTime.im = Q;
            SymbShift = (dspPhase(SymbTime) / (2 * M_PI)) * SymbolDiv;
            if (SymbShift < 0)
                SymbShift += SymbolDiv;
      // for (i=j-1; i<=j+1; i++) printf(" [%+5.2f,%+5.2f]",SymbFit[i].re,SymbFit[i].im);
      // make first estimation of FreqOfs
      // printf(" -> [%+5.2f,%+5.2f] =>",I,Q);
      // for (i=j-2; i<=j+2; i++) printf(" %+6.3f",I*SymbFit[i].re+Q*SymbFit[i].im);
            pI =  dspScalProd(I, Q, SymbFit[j])
                + 0.7 * dspScalProd(I, Q, SymbFit[j - 1])
                + 0.7 * dspScalProd(I, Q, SymbFit[j + 1]);
            pQ =  0.7 * dspScalProd(I, Q, SymbFit[j + 1])
                - 0.7 * dspScalProd(I, Q, SymbFit[j - 1])
                + 0.5 * dspScalProd(I, Q, SymbFit[j + 2])
                - 0.5 * dspScalProd(I, Q, SymbFit[j - 2]);
            FreqOfs = j + dspPhase(pI, pQ) / (2.0 * M_PI / 8);
/* SYNC TEST */
      // refine the FreqOfs
            i = (int)floor(FreqOfs + 0.5);
            s = (int)floor(SymbShift);
            s2 = (s + 1) & (SymbolDiv - 1);
//      printf(" [%5.2f,%2d,%d,%d] ",FreqOfs,i,s,s2);
            w0 = (s + 1 - SymbShift);
            w1 = (SymbShift - s);
//      printf(" [%4.2f,%4.2f] ",w0,w1);
            A = (0.5 * WindowLen) / SymbolSepar;
            I = w0 * CorrelAver[s][i].re + w1 * CorrelAver[s2][i].re;
            Q = w0 * CorrelAver[s][i].im + w1 * CorrelAver[s2][i].im;
//      printf(" [%5.2f,%2d] -> [%+5.2f,%+5.2f]",FreqOfs,i,I,Q);
//      FreqOfs=i+dspPhase(I,Q)/(2.0*M_PI)*0.5*A;
//      printf(" => %5.2f",FreqOfs);
            F0 = i + dspPhase(I, Q) / (2.0 * M_PI) * A - FreqOfs;
            Fl = F0 - A;
            Fu = F0 + A;
            if (fabs(Fl) < fabs(F0)) 
                FreqOfs += (fabs(Fu) < fabs(Fl)) ? Fu : Fl;
            else 
                FreqOfs += (fabs(Fu) < fabs(F0)) ? Fu : F0;
//      printf(" => (%5.2f,%5.2f,%5.2f) => %5.2f",Fl,F0,Fu,FreqOfs);

        } else { 
            SymbTime.re = SymbTime.im = 0.0;
            SymbConf = 0.0;
            SymbShift = 0.0;
            FreqOfs = 0.0;
        }

    // here we have FreqOfs and SymbTime.re/im

    // printf("FreqOfs=%5.2f",FreqOfs);

        if (SyncLocked) { // flip the SymbTime if it doesn't agree with the dspAverage
            if (dspScalProd(SymbTime, AverSymb) < 0.0) {
                SymbTime.re = (-SymbTime.re);
                SymbTime.im = (-SymbTime.im);
                FreqOfs -= DataCarrSepar;
            }
      // reduce the freq. offset towards the dspAverage offset
            A = 2 * DataCarrSepar;
            k = (int)floor((FreqOfs - AverFreq) / A + 0.5);
            FreqOfs -= k * A;
/* SYNC TEST */
            A = (0.5 * WindowLen) / SymbolSepar;
            F0 = FreqOfs - AverFreq; // correct freq. auto-correlator wrap
            Fl = F0 - A;
            Fu = F0 + A;
            if (fabs(Fl) < fabs(F0))
                FreqOfs += (fabs(Fu) < fabs(Fl)) ? A : -A;
            else
                FreqOfs += (fabs(Fu) < fabs(F0)) ? A : 0.0;
// printf(" => (%5.2f,%5.2f,%5.2f) => %5.2f",Fl,F0,Fu,FreqOfs);

        } else { // of if (SyncLocked)
// flip SymbTime if it doesn't agree with the previous
            if (dspScalProd(SymbTime, SymbPipe[TrackPipePtr]) < 0.0) {
                SymbTime.re = (-SymbTime.re);
                SymbTime.im = (-SymbTime.im);
                FreqOfs -= DataCarrSepar;
            }
// reduce the FreqOfs towards zero
            A = 2 * DataCarrSepar;
            k = (int)floor(FreqOfs / A + 0.5);
            FreqOfs -= k * A;
/* SYNC TEST */
            F0 = FreqOfs - FreqPipe[TrackPipePtr];
            Fl = F0 - A;
            Fu = F0 + A;
            if (fabs(Fl) < fabs(F0)) 
                FreqOfs += (fabs(Fu) < fabs(Fl)) ? A : -A;
            else 
                FreqOfs += (fabs(Fu) < fabs(F0)) ? A : 0.0;
        }

// printf(" => [%+5.2f,%+5.2f], %5.2f",SymbTime.re,SymbTime.im,FreqOfs);

        TrackPipePtr += 1;
        if (TrackPipePtr >= TrackPipeLen) 
            TrackPipePtr -= TrackPipeLen;
        SymbPipe[TrackPipePtr] = SymbTime;  // put SymbTime and FreqOfs into pipes
        FreqPipe[TrackPipePtr] = FreqOfs;   // for averaging

// find dspAverage symbol time
        Loops = dspSelFitAver( SymbPipe,
                               TrackPipeLen,
                               (double)3.0,
                               4,
                               AverSymb,
                               dspRMS,
                               Incl);
// printf(" AverSymb=[%+5.2f,%+5.2f], dspRMS=%5.3f/%2d",
//         AverSymb.re,AverSymb.im,dspRMS,Incl);
// find dspAverage freq. offset
        Loops = dspSelFitAver( FreqPipe,
                               TrackPipeLen,
                               (double)2.5,
                               4,
                               AverFreq,
                               dspRMS,
                               Incl);
        SyncFreqDev = dspRMS;
// printf(" AverFreq=%+5.2f, dspRMS=%5.3f/%2d",AverFreq,dspRMS,Incl);

        SymbConf = dspAmpl(AverSymb);
        SyncSymbConf = SymbConf;
        SyncFreqOfs = AverFreq;
        if (SymbConf > 0.0) {
            SymbShift = dspPhase(AverSymb) / (2 * M_PI) * SymbolSepar;
            if (SymbShift < 0.0) 
                SymbShift += SymbolSepar;
            SymbPtr = (int)floor((dspPhase(AverSymb) / (2 * M_PI)) * SymbolDiv);
            if (SymbPtr < 0)
                SymbPtr += SymbolDiv;
            SyncSymbShift = SymbShift;
        }

        if (SyncLocked) {
            if ((SyncSymbConf < SyncHoldThres) || (SyncFreqDev > 0.250))
                SyncLocked = 0;
        } else {
            if ((SyncSymbConf > SyncLockThres) && (SyncFreqDev < 0.125))
                SyncLocked = 1;
        }

        SyncSymbConf *= 0.5;

// printf(" => SyncLocked=%d, SyncSymbShift=%5.1f, SymbPtr=%d",
//        SyncLocked,SyncSymbShift,SymbPtr);

// printf("\n");

    } // enf of if (SyncPtr==(SymbPtr^2))

}

void MT63rx::DataProcess(dspCmpx *EvenSlice, dspCmpx *OddSlice, double FreqOfs, int TimeDist)
{
    int i, c, r;
    dspCmpx Freq, Phas;
    int incr, p;
    double I, Q, P;
    dspCmpx Dtmp;
    dspCmpx Ftmp;

//  double Aver,dspRMS; int Loops,Incl;

// Here we pickup a symbol in the data history. The time/freq. synchronizer
// told us where it is in time and at which frequency offset (FreqOfs)
// TimeDist is the distance in sdspAmples from the symbol we analyzed
// in the previous call to this routine

//  FreqOfs=0.0; // for DEBUG only !

//  printf("DataProcess: FreqOfs=%5.3f, TimeDist=%d, Locked=%d\n",
//     FreqOfs,TimeDist,SyncLocked);

    P = (-2 * M_PI * FreqOfs) / WindowLen;    // make ready for frequency correction
    Freq.re = cos(P);
    Freq.im = sin(P);
    Phas.re = 1.0;
    Phas.im = 0.0;
    for (i = 0; i < WindowLen; i++) {        // prepare slices for the FFT
        r = FFT.BitRevIdx[i];            // multiply by window and pre-scramble
//    if (i==2*ScanMargin)
//      printf("%3d: [%5.2f,%5.2f] [%5.2f,%5.2f]\n",
//        i, dspPhase.re,dspPhase.im, EvenSlice[i].re,EvenSlice[i].im);
        CdspcmpxMultAxB(I, Q, EvenSlice[i], Phas);
        FFTbuff[r].re = I * RxWindow[i];
        FFTbuff[r].im = Q * RxWindow[i];
        CdspcmpxMultAxB(I, Q, OddSlice[i], Phas);
        FFTbuff2[r].re = I * RxWindow[i];
        FFTbuff2[r].im = Q * RxWindow[i];
        CdspcmpxMultAxB(Dtmp, Phas, Freq);
        Phas = Dtmp;
    }
    FFT.CoreProc(FFTbuff);
    FFT.CoreProc(FFTbuff2);
/*
  printf("FFTbuff [%3d...]:",FirstDataCarr-16);
  for (i=FirstDataCarr-16; i<=FirstDataCarr+32; i++)
    printf(" %+3d/%4.2f",i-FirstDataCarr,dspAmpl(FFTbuff[i]));
  printf("\n");

  printf("FFTbuff2[%3d...]:",FirstDataCarr-16);
  for (i=FirstDataCarr-16; i<=FirstDataCarr+32; i++)
    printf(" %+3d/%4.2f",i-FirstDataCarr,dspAmpl(FFTbuff2[i]));
  printf("\n");
*/
//  printf(" FreqOfs=%5.2f: ",FreqOfs);

//  printf("Symbol vectors:\n");
    incr = (TimeDist * DataCarrSepar) & WindowLenMask;    // correct FFT dspPhase shift
    p = (TimeDist * DataScanFirst) & WindowLenMask;    // due to time shift by
    for (c = DataScanFirst, i = 0; i < DataScanLen; ) {    // TimeDist
// printf("%2d,%3d:",i,c);
// printf("  [%6.3f,%6.3f]  [%6.3f,%6.3f]",
//     FFTbuff[c].re,FFTbuff[c].im,
//    FFTbuff2[c+DataCarrSepar].re,FFTbuff2[c+DataCarrSepar].im);
// printf("   [%6.3f,%6.3f]/[%6.3f,%6.3f]",
//    FFTbuff2[c].re,FFTbuff2[c].im,
//    FFTbuff[c+DataCarrSepar].re,FFTbuff[c+DataCarrSepar].im);
// printf(" %5.3f/%5.3f",dspAmpl(FFTbuff[c]),dspAmpl(FFTbuff[c+DataCarrSepar]));
// printf(" %5.3f/%5.3f",dspAmpl(FFTbuff2[c+DataCarrSepar]),dspAmpl(FFTbuff2[c]));
// printf("\n");
        Phas = FFT.Twiddle[p];
        CdspcmpxMultAxB(Dtmp, RefDataSlice[i], Phas);
        CdspcmpxMultAxBs(DataVect[i], FFTbuff[c], Dtmp);
//    printf("%3d,%2d: [%8.5f,%8.5f] / %8.5f\n",
//       c,i,FFTbuff[c].re,FFTbuff[c].im,DataPwrOut[i]);
        dspLowPass2( dspPower(FFTbuff[c]),
                     DataPwrMid[i], 
                     DataPwrOut[i], dW1, dW2, dW5);
        RefDataSlice[i++] = FFTbuff[c];
        c = (c + DataCarrSepar) & WindowLenMask;
        p = (p + incr) & WindowLenMask;
    
        Phas = FFT.Twiddle[p];
        CdspcmpxMultAxB(Dtmp, RefDataSlice[i], Phas);
        CdspcmpxMultAxBs(DataVect[i], FFTbuff2[c], Dtmp);
//    printf("%3d,%2d: [%8.5f,%8.5f] / %8.5f\n",
//       c,i,FFTbuff2[c].re,FFTbuff2[c].im,DataPwrOut[i]);
        dspLowPass2( dspPower(FFTbuff2[c]),
                     DataPwrMid[i],
                     DataPwrOut[i], dW1, dW2, dW5);
        RefDataSlice[i++] = FFTbuff2[c];
        c = (c + DataCarrSepar) & WindowLenMask;
        p = (p + incr) & WindowLenMask;
    }

    P = (-TimeDist * 2 * M_PI * FreqOfs) / WindowLen;
    Freq.re = cos(P);
    Freq.im = sin(P);   
    for (i = 0; i < DataScanLen; i++) {
        CdspcmpxMultAxB(Ftmp, DataVect[i], Freq);
// dspLowPass2(dspPower(Ftmp),DataPwrMid[i],DataPwrOut[i],dW1,dW2,dW5);
// CdspcmpxMultAxB(Dtmp,Ftmp,Ftmp);
// Dtmp.re=Ftmp.re*Ftmp.re-Ftmp.im*Ftmp.im; Dtmp.im=2*Ftmp.re*Ftmp.im;
// dspLowPass2(&Dtmp,DataSqrMid+i,DataSqrOut+i,dW1,dW2,dW5);
        DataVect[i] = DataPipe[DataPipePtr][i];
        DataPipe[DataPipePtr][i] = Ftmp;
    }
    DataPipePtr += 1;
    if (DataPipePtr >= DataPipeLen)
        DataPipePtr = 0;

    for (i = 0; i < DataScanLen; i++) {
        if (DataPwrOut[i] > 0.0) {
            P = DataVect[i].re / DataPwrOut[i];
            if (P > 1.0) 
                P = 1.0; 
            else if (P < (-1.0)) 
                P = (-1.0);
            DatadspPhase[i] = P;
        } else 
            DatadspPhase[i] = 0.0;
    }
    Decoder.Process(DatadspPhase);
    Output.EnsureSpace(Output.Len + 1);
    Output.Data[Output.Len] = Decoder.Output;
    Output.Len += 1;
/*
  printf("Demodulator output vectors:\n");
  for (i=0; i<DataScanLen; i++)
  { printf("%2d: [%8.5f,%8.5f] / %8.5f => %8.5f\n",
       i,DataVect[i].re,DataVect[i].im,DataPwrOut[i], DatadspPhase[i]);
  }
*/
/*
  for (i=0; i<DataScanLen; i++)
  { // printf("%2d: [%8.5f,%8.5f]\n",i,DataVect[i].re,DataVect[i].im);
    if (dspPower(DataVect[i])>0.0) P=dspPhase(DataVect[i]); else P=0.0;
    DatadspPhase[i]=P;
    P*=2; if (P>M_PI) P-=2*M_PI; else if (P<(-M_PI)) P+=2*M_PI;
    DatadspPhase2[i]=P;
    printf("%2d: %6.3f [%6.3f,%6.3f]  [%8.5f,%8.5f], %5.2f, %5.2f",
       i, DataPwrOut[i], DataSqrOut[i].re,DataSqrOut[i].im,
          DataVect[i].re,DataVect[i].im, DatadspPhase[i],DatadspPhase2[i]);
    if (DataPwrOut[i]>0.0)
      printf(" %6.3f",dspAmpl(DataSqrOut[i])/DataPwrOut[i]);
    printf("\n");
  }
  Loops=dspSelFitAver(DatadspPhase2,DataScanLen,(double)2.5,4,Aver,dspRMS,Incl);
  printf("Aver=%5.2f, dspRMS=%5.2f, Incl=%d\n",Aver,dspRMS,Incl);
*/
}

int MT63rx::SYNC_LockStatus(void) { 
    return SyncLocked;
}

double MT63rx::SYNC_Confidence(void) {
    return SyncSymbConf <= 1.0 ? SyncSymbConf : 1.0;
}

double MT63rx::SYNC_FreqOffset(void) {
    return SyncFreqOfs / DataCarrSepar;
}

double MT63rx::SYNC_FreqDevdspRMS(void) {
    return SyncFreqDev / DataCarrSepar;
}

double MT63rx::SYNC_TimeOffset(void) {
    return SyncSymbShift / SymbolSepar;
}

double MT63rx::FEC_SNR(void) {
    return Decoder.SignalToNoise;
}

int MT63rx::FEC_CarrOffset(void) {
    return Decoder.CarrOfs;
}

double MT63rx::TotalFreqOffset(void) {
    return ( SyncFreqOfs + DataCarrSepar * Decoder.CarrOfs) * 
           (8000.0 / DecimateRatio) / WindowLen;
}

