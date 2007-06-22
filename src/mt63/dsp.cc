/*
 *    dsp.cc  --  various DSP algorithms
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

// Please note, that you should not rely on the correctness
// of these routines. They generally work well, but you may find
// differences in respect to the mathematical formulas: signs flipped,
// orders swapped, etc.

#include <stdio.h> // only when we do some control printf's
#include <stdlib.h>
#include <math.h>
#include "dsp.h"

// ----------------------------------------------------------------------------

double Power(float *X, int Len)
{ double Sum;
  for(Sum=0.0; Len; Len--,X++)
    Sum+=(*X)*(*X);
  return Sum; }

double Power(double *X, int Len)
{ double Sum;
  for(Sum=0.0; Len; Len--,X++)
    Sum+=(*X)*(*X);
  return Sum; }

double Power(float *I, float *Q, int Len)
{ double Sum;
  for(Sum=0.0; Len; Len--,I++,Q++)
    Sum+=(*I)*(*I)+(*Q)*(*Q);
  return Sum; }

double Power(double *I, double *Q, int Len)
{ double Sum;
  for(Sum=0.0; Len; Len--,I++,Q++)
    Sum+=(*I)*(*I)+(*Q)*(*Q);
  return Sum; }

double Power(fcmpx *X, int Len)
{ double Sum;
  for(Sum=0.0; Len; Len--,X++)
    Sum+=(X->re)*(X->re)+(X->im)*(X->im);
  return Sum; }

double Power(dcmpx *X, int Len)
{ double Sum;
  for(Sum=0.0; Len; Len--,X++)
    Sum+=(X->re)*(X->re)+(X->im)*(X->im);
  return Sum; }

// ----------------------------------------------------------------------------
// average, extremes, fitting

double Average(float *Data, int Len)
{ double Sum; int i;
  for(Sum=0.0,i=0; i<Len; i++) Sum+=Data[i];
  return Sum/Len; }

double Average(double *Data, int Len)
{ double Sum; int i;
  for(Sum=0.0,i=0; i<Len; i++) Sum+=Data[i];
  return Sum/Len; }

int CountInRange(float *Data, int Len, float Low, float Upp)
{ int count,i; float D;
  for(count=i=0; i<Len; i++)
  { D=Data[i]; count+=((Low<=D)&&(D<=Upp)); }
  return count; }

int CountInRange(double *Data, int Len, double Low, double Upp)
{ int count,i; double D;
  for(count=i=0; i<Len; i++)
  { D=Data[i]; count+=((Low<=D)&&(D<=Upp)); }
  return count; }

/*
double RMS(float *Data, int Len)
{ double Sum; int i;
  for(Sum=0.0,i=0; i<Len; i++) Sum+=Data[i]*Data[i];
  return sqrt(Sum/Len); }

double RMS(double *Data, int Len)
{ double Sum; int i;
  for(Sum=0.0,i=0; i<Len; i++) Sum+=Data[i]*Data[i];
  return sqrt(Sum/Len); }

double RMS(fcmpx *Data, int Len)
{ double Sum; int i;
  for(Sum=0.0,i=0; i<Len; i++)
    Sum+=Data[i].re*Data[i].re+Data[i].im*Data[i].im;
  return sqrt(Sum/Len); }

double RMS(dcmpx *Data, int Len)
{ double Sum; int i;
  for(Sum=0.0,i=0; i<Len; i++)
    Sum+=Data[i].re*Data[i].re+Data[i].im*Data[i].im;
  return sqrt(Sum/Len); }
*/

double FindMaxPower(fcmpx *Data, int Len)
{ double Max,Pwr; int i;
  Max=Power(Data[0]);
  for(i=1; i<Len; i++)
  { Pwr=Power(Data[i]); if(Pwr>Max) Max=Pwr;
  } return Max; }

double FindMaxPower(fcmpx *Data, int Len, int &MaxPos)
{ double Max,Pwr; int i,pos;
  Max=Power(Data[0]); pos=0;
  for(i=1; i<Len; i++)
  { Pwr=Power(Data[i]); if(Pwr>Max) { Max=Pwr; pos=i; }
  } MaxPos=pos; return Max; }

double FitPoly1(float *Data, int Len, double &A, double &B)
{ double Sum; int i;
  A=(Data[Len-1]-Data[0])/(Len-1);
  for(Sum=0.0,i=0; i<Len; i++)
    Sum+=Data[i]-A*i;
  B=Sum/Len;
  for(Sum=0.0,i=0; i<Len; i++)
    Sum+=Power(Data[i]-(A*i+B));
  return sqrt(Sum/Len);
}

double FitPoly1(double *Data, int Len, double &A, double &B)
{ double Sum; int i;
  A=(Data[Len-1]-Data[0])/(Len-1);
  for(Sum=0.0,i=0; i<Len; i++)
    Sum+=Data[i]-A*i;
  B=Sum/Len;
  for(Sum=0.0,i=0; i<Len; i++)
    Sum+=Power(Data[i]-(A*i+B));
  return sqrt(Sum/Len);
}

double FitPoly2(float *Data, int Len, double &A, double &B, double &C)
{ double Sum; int i;
//  if(Len==0) { A=0.0; B=0.0; C=0.0; return 0; }
//  if(Len==1) { A=0.0; B=0.0; C=Data[0]; return 0; }
//  if(Len==2) { A=0.0; B=Data[1]-Data[0]; C=Data[0]; return 0; }
//  if(Len==3)
//  { C=Data[0]; A=(Data[0]-2*Data[1]+Data[2])/2; B-(Data[1]-Data[0])-A;
//    return 0.0; }
  A=((Data[Len-1]-Data[Len-2])-(Data[1]-Data[0]))/(Len-2)/2;
  B=(Data[Len-1]-A*(Len-1)*(Len-1)-Data[0])/(Len-1);
  for(Sum=0.0,i=0; i<Len; i++)
    Sum+=Data[i]-(A*i*i+B*i);
  C=Sum/Len;
  for(Sum=0.0,i=0; i<Len; i++)
    Sum+=Power(Data[i]-(A*i*i+B*i+C));
  return sqrt(Sum/Len);
}

double FitPoly2(double *Data, int Len, double &A, double &B, double &C)
{ double Sum; int i;
  A=((Data[Len-1]-Data[Len-2])-(Data[1]-Data[0]))/(Len-2)/2;
  B=(Data[Len-1]-A*(Len-1)*(Len-1)-Data[0])/(Len-1);
  for(Sum=0.0,i=0; i<Len; i++)
    Sum+=Data[i]-(A*i*i+B*i);
  C=Sum/Len;
  for(Sum=0.0,i=0; i<Len; i++)
    Sum+=Power(Data[i]-(A*i*i+B*i+C));
  return sqrt(Sum/Len);
}

void FitPoly2(float Data[3], double &A, double &B, double &C)
{ C=Data[0]; A=(Data[0]-2*Data[1]+Data[2])/2; B=(Data[1]-Data[0])-A; }

void FitPoly2(double Data[3], double &A, double &B, double &C)
{ C=Data[0]; A=(Data[0]-2*Data[1]+Data[2])/2; B=(Data[1]-Data[0])-A; }

// ----------------------------------------------------------------------------
// various window shapes (for the FFT and FIR filters)
// these functions are supposed to be called with the argument "phase"
// between -PI and +PI. Most (or even all) will return zero for input
// euqal -PI or +PI.

double WindowHamming(double phase) { return cos(phase/2); } // not exactly ...

double WindowHanning(double phase) { return (1.0+cos(phase))/2; }

double WindowBlackman2(double phase) // from Freq 5.1 FFT analyzer
{ return 0.42+0.5*cos(phase)+0.08*cos(2*phase); }

double WindowBlackman3(double phase) // from the Motorola BBS
{ return 0.35875+0.48829*cos(phase)
	+0.14128*cos(2*phase)+0.01168*cos(3*phase); }

// ----------------------------------------------------------------------------
// FIR shape calculation for a flat response from FreqLow to FreqUpp

void WinFirI(float LowOmega, float UppOmega,
       float *Shape, int Len, double (*Window)(double), float shift)
{ int i; double time,phase,shape;
// printf("WinFirI: %5.3f %5.3f %d\n",LowOmega,UppOmega,Len);
  for(i=0; i<Len; i++)
  { time=i+(1.0-shift)-(float)Len/2; phase=2*M_PI*time/Len;
    if(time==0) shape=UppOmega-LowOmega;
	   else shape=(sin(UppOmega*time)-sin(LowOmega*time))/time;
// printf("%2d %5.1f %5.2f %7.4f %7.4f\n",i,time,phase,shape,(*Window)(phase));
    Shape[i]=shape*(*Window)(phase)/M_PI; }
}

void WinFirQ(float LowOmega, float UppOmega,
       float *Shape, int Len, double (*Window)(double), float shift)
{ int i; double time,phase,shape;
// printf("WinFirQ: %5.3f %5.3f %d\n",LowOmega,UppOmega,Len);
  for(i=0; i<Len; i++)
  { time=i+(1.0-shift)-(float)Len/2; phase=2*M_PI*time/Len;
    if(time==0) shape=0.0;
	   else shape=(-cos(UppOmega*time)+cos(LowOmega*time))/time;
// printf("%2d %5.1f %5.2f %7.4f %7.4f\n",i,time,phase,shape,(*Window)(phase));
    Shape[i]=(-shape)*(*Window)(phase)/M_PI; }
} // we put minus for the Q-part because the FIR shapes must be placed
  // in reverse order for simpler indexing

// ----------------------------------------------------------------------------
// convert 16-bit signed or 8-bit unsigned into floats

void ConvS16toFloat(s16 *S16, float *Float, int Len, float Gain)
{ for(; Len; Len--) (*Float++)=(*S16++)*Gain; }
int ConvS16toFloat(short int *S16, float_buff *Float, int Len, float Gain)
{ int err=Float->EnsureSpace(Len); if(err) return -1;
  ConvS16toFloat(S16,Float->Data,Len,Gain); Float->Len=Len; return 0; }

void ConvFloatToS16(float *Float, s16 *S16, int Len, float Gain)
{ float out; for(; Len; Len--)
  { out=(*Float++)*Gain;
    if(out>32767.0) out=32767.0;
    else if(out<(-32767.0)) out=(-32767.0);
    (*S16++)=(short int)floor(out+0.5); }
} // we could count the over/underflows ?

/* transformed into "inline" - moved to dsp.h
int ConvFloatToS16(float_buff *Float, s16_buff *S16, float Gain)
{ int err=S16->EnsureSpace(Float->Len); if(err) return -1;
  ConvFloatToS16(Float->Data,S16->Data,Float->Len,Gain);
  S16->Len=Float->Len; return 0; }
*/
void ConvU8toFloat(unsigned char *U8, float *Float, int Len, float Gain)
{ for(; Len; Len--) (*Float++)=((int)(*U8++)-128)*Gain; }

int  ConvU8toFloat(unsigned char *U8, float_buff *Float, int Len, float Gain)
{ int err=Float->EnsureSpace(Len); if(err) return -1;
  ConvU8toFloat(U8,Float->Data,Len,Gain); Float->Len=Len; return 0; }

// ----------------------------------------------------------------------------
// other converts

void ConvCmpxToPower(fcmpx *Inp, int Len, float *Out)
{ for(; Len; Len--) (*Out++)=Power(*Inp++); }

int ConvCmpxToPower(fcmpx_buff *Input, float_buff *Output)
{ int err=Output->EnsureSpace(Input->Len); if(err) return err;
  ConvCmpxToPower(Input->Data,Input->Len,Output->Data);
  Output->Len=Input->Len; return 0; }

void ConvCmpxToAmpl(fcmpx *Inp, int Len, float *Out)
{ for(; Len; Len--) (*Out++)=sqrt(Power(*Inp++)); }

int ConvCmpxToAmpl(fcmpx_buff *Input, float_buff *Output)
{ int err=Output->EnsureSpace(Input->Len); if(err) return err;
  ConvCmpxToAmpl(Input->Data,Input->Len,Output->Data);
  Output->Len=Input->Len; return 0; }

void ConvCmpxToPhase(fcmpx *Inp, int Len, float *Out)
{ for(; Len; Len--) (*Out++)=Phase(*Inp++); }

int ConvCmpxToPhase(fcmpx_buff *Input, float_buff *Output)
{ int err=Output->EnsureSpace(Input->Len); if(err) return err;
  ConvCmpxToPhase(Input->Data,Input->Len,Output->Data);
  Output->Len=Input->Len; return 0; }

// ----------------------------------------------------------------------------
// Pulse noise limiter

PulseLimiter::PulseLimiter() { Tap=NULL; }

PulseLimiter::~PulseLimiter() { free(Tap); }

void PulseLimiter::Free(void) { free(Tap); Tap=NULL; }

int PulseLimiter::Preset(int TapLen, float LimitThres)
{ Len=TapLen; Thres=LimitThres*LimitThres;
  if(ReallocArray(&Tap,Len)) return -1;
  ClearArray(Tap,Len); Ptr=0; PwrSum=0.0; return 0; }

int PulseLimiter::Process(float *Inp, int InpLen, float *Out)
{ int i,o; double Lim;
  for(i=0; i<InpLen; i++)
  { PwrSum-=Tap[Ptr]*Tap[Ptr];
    Tap[Ptr++]=Inp[i]; PwrSum+=Inp[i]*Inp[i];
    if(Ptr>=Len) Ptr-=Len;
    o=Ptr+(Len/2); if(o>=Len) o-=Len;
    Lim=Thres*PwrSum/Len;
    if(Tap[o]*Tap[o]<=Lim) Out[i]=Tap[o];
    else { if(Tap[o]>0.0) Out[i]=sqrt(Lim); else Out[i]=(-sqrt(Lim)); }
  }
  for(PwrSum=0.0, i=0; i<Len; i++) PwrSum+=Tap[i]*Tap[i];
  RMS=sqrt(PwrSum/Len);
  return 0; }

int PulseLimiter::Process(float_buff *Input)
{ int err=Output.EnsureSpace(Input->Len); if(err) return -1;
  Process(Input->Data,Input->Len,Output.Data);
  Output.Len=Input->Len; return 0; }

// ----------------------------------------------------------------------------
// Signal level monitor

LevelMonitor::LevelMonitor()
{ }

LevelMonitor::~LevelMonitor()
{ }

int LevelMonitor::Preset(float Integ, float Range)
{ LowPass2Coeff(Integ,W1,W2,W5);
  MaxSqr=Range*Range;
  PwrMid=0.0; PwrOut=0.0; RMS=0.0;
  OutOfRangeMid=0.0; OutOfRange=0.0;
  return 0; }

int LevelMonitor::Process(float *Inp, int Len)
{ int i,Out; double Sqr,Sum;
  if(Len<=0) return 0;
  for(Sum=0.0,Out=0,i=0; i<Len; i++)
  { Sum+=Sqr=Inp[i]*Inp[i]; Out+=(Sqr>MaxSqr); }
  LowPass2(Sum/Len,PwrMid,PwrOut,W1,W2,W5);
  LowPass2((float)Out/Len,OutOfRangeMid,OutOfRange,W1,W2,W5);
  if(OutOfRange<0.0) OutOfRange=0.0;
  if(PwrOut<=0.0) RMS=0.0; else RMS=sqrt(PwrOut);
  return 0; }

int LevelMonitor::Process(float_buff *Input)
{ return Process(Input->Data,Input->Len); }

// ----------------------------------------------------------------------------
// Automatic Gain/Level Control for the Mixer

MixerAutoLevel::MixerAutoLevel()
{ MinMS=0.01; MaxMS=0.05;
  IntegLen=8000; PeakHold=4000; MinHold=800;
  MinLevel=0; MaxLevel=100; AdjStep=1;
  Level=75; Hold=(-IntegLen); AverMS=0.0;
}

int MixerAutoLevel::Process(float *Inp, int InpLen)
{ double MS=Power(Inp,InpLen)/IntegLen;
  double W=1.0-((double)InpLen)/IntegLen;

  AverMS = AverMS*W + MS;
  Hold+=InpLen; if(Hold<MinHold) return 0;

  if(AverMS>MaxMS)
  { Level-=AdjStep; if(Level<MinLevel) Level=MinLevel;
    Hold=0; return 1; }

  if(Hold<PeakHold) return 0;

  if(AverMS<MinMS)
  { Level+=AdjStep; if(Level>MaxLevel) Level=MaxLevel;
    Hold=0; return 1; }

  return 0;
}

// ----------------------------------------------------------------------------

void LowPass2(fcmpx *Inp, dcmpx *Mid, dcmpx *Out,
		float W1, float W2, float W5)
{ double Sum, Diff;
//  printf("\n[LowPass2] %6.3f %6.3f %6.3f",Inp->re,Mid->re,Out->re);
  Sum=Mid->re+Out->re; Diff=Mid->re-Out->re;
  Mid->re+=W2*Inp->re-W1*Sum; Out->re+=W5*Diff;
//  printf(" => %6.3f %6.3f\n",Mid->re,Out->re);
  Sum=Mid->im+Out->im; Diff=Mid->im-Out->im;
  Mid->im+=W2*Inp->im-W1*Sum; Out->im+=W5*Diff; }

void LowPass2(dcmpx *Inp, dcmpx *Mid, dcmpx *Out,
		float W1, float W2, float W5)
{ double Sum, Diff;
//  printf("[LowPass2] %6.3f %6.3f %6.3f",Inp->re,Mid->re,Out->re);
  Sum=Mid->re+Out->re; Diff=Mid->re-Out->re;
  Mid->re+=W2*Inp->re-W1*Sum; Out->re+=W5*Diff;
//  printf(" => %6.3f %6.3f\n",Mid->re,Out->re);
  Sum=Mid->im+Out->im; Diff=Mid->im-Out->im;
  Mid->im+=W2*Inp->im-W1*Sum; Out->im+=W5*Diff; }

// ----------------------------------------------------------------------------
// periodic low pass

PeriodLowPass2::PeriodLowPass2() { TapMid=NULL; TapOut=NULL; }

PeriodLowPass2::~PeriodLowPass2() { free(TapMid); free(TapOut); Output.Free(); }

void PeriodLowPass2::Free(void)
{ free(TapMid); TapMid=NULL;
  free(TapOut); TapOut=NULL; }

int PeriodLowPass2::Preset(int Period, float IntegLen)
{ int i;
  Len=Period;
  if(ReallocArray(&TapMid,Len)) goto Error;
  if(ReallocArray(&TapOut,Len)) goto Error;
  for(i=0; i<Len; i++) { TapMid[i]=0.0; TapOut[i]=0.0; }
  TapPtr=0;
  LowPass2Coeff(IntegLen,W1,W2,W5);
  return 0;
  Error: Free(); return -1;
}

int PeriodLowPass2::Process(float Inp, float &Out)
{ LowPass2(Inp,TapMid[TapPtr],TapOut[TapPtr],W1,W2,W5);
  Out=TapOut[TapPtr++]; if(TapPtr>=Len) TapPtr=0;
  return 0; }

int PeriodLowPass2::Process(float *Inp, int InpLen, float *Out)
{ int i,batch;
  for(i=0; i<InpLen; )
  { for(batch=imin(InpLen-i,Len-TapPtr),i+=batch; batch; batch--)
    { LowPass2(*Inp++,TapMid[TapPtr],TapOut[TapPtr],W1,W2,W5);
      (*Out++)=TapOut[TapPtr++]; }
    if(TapPtr>=Len) TapPtr=0; }
  return 0;
}

int PeriodLowPass2::Process(float_buff *Input)
{ int err=Output.EnsureSpace(Input->Len); if(err) return -1;
  Process(Input->Data,Input->Len,Output.Data);
  Output.Len=Input->Len; return 0; }

// ----------------------------------------------------------------------------
// Low pass "moving box" FIR filter
// very unpure spectral response but complexity very low
// and independent on the integration time

BoxFilter::BoxFilter() { Tap=NULL; }

BoxFilter::~BoxFilter() { free(Tap); }

void BoxFilter::Free(void) { free(Tap); Tap=NULL; Output.Free(); }

int BoxFilter::Preset(int BoxLen)
{ int i;
  if(ReallocArray(&Tap,BoxLen)) return -1;
  for(i=0; i<BoxLen; i++) Tap[i]=0;
  Len=BoxLen; TapPtr=0; Sum=0; return 0; }

int BoxFilter::Process(float *Inp, int InpLen, float *Out)
{ int i,batch;
  for( i=0; i<InpLen; )
  { for(batch=imin(InpLen-i,Len-TapPtr), i+=batch; batch; batch--)
    { Sum-=Tap[TapPtr]; Out[i]=(Sum+=Tap[TapPtr++]=Inp[i]); }
    if(TapPtr>=Len) TapPtr=0; }
  for(Sum=0, i=0; i<Len; i++) Sum+=Tap[i]; return InpLen;
}

void BoxFilter::Recalibrate()
{ int i; for(Sum=0, i=0; i<Len; i++) Sum+=Tap[i]; }

int BoxFilter::Process(float_buff *Input)
{ int err=Output.EnsureSpace(Input->Len); if(err) return err;
  Process(Input->Data,Input->Len,Output.Data);
  Output.Len=Input->Len; return 0; }


CmpxBoxFilter::CmpxBoxFilter() { Tap=NULL; }

CmpxBoxFilter::~CmpxBoxFilter() { free(Tap); }

void CmpxBoxFilter::Free(void) { free(Tap); Tap=NULL; Output.Free(); }

int CmpxBoxFilter::Preset(int BoxLen)
{ int i;
  if(ReallocArray(&Tap,BoxLen)) return -1;
  for(i=0; i<BoxLen; i++) Tap[i].re=Tap[i].im=0.0;
  Len=BoxLen; TapPtr=0; Sum.re=0.0; Sum.im=0.0; return 0; }

int CmpxBoxFilter::Process(fcmpx *Inp, int InpLen, fcmpx *Out)
{ int i,batch;
  for( i=0; i<InpLen; )
  { for(batch=imin(InpLen-i,Len-TapPtr), i+=batch; batch; batch--)
    { Sum.re-=Tap[TapPtr].re; Sum.im-=Tap[TapPtr].im;
      Tap[TapPtr]=Inp[i]; Sum.re+=Inp[i].re; Sum.im+=Inp[i].im;
      Out[i].re=Sum.re; Out[i].im=Sum.im; }
    if(TapPtr>=Len) TapPtr=0; }
  for(Sum.re=Sum.im=0.0, i=0; i<Len; i++)
  { Sum.re+=Tap[i].re; Sum.im+=Tap[i].im; }
  return InpLen;
}

void CmpxBoxFilter::Recalibrate()
{ int i;
  for(Sum.re=Sum.im=0.0, i=0; i<Len; i++)
  { Sum.re+=Tap[i].re; Sum.im+=Tap[i].im; }
}

int CmpxBoxFilter::Process(fcmpx_buff *Input)
{ int err=Output.EnsureSpace(Input->Len); if(err) return err;
  Process(Input->Data,Input->Len,Output.Data);
  Output.Len=Input->Len; return 0; }

// ----------------------------------------------------------------------------
// FIR filter with a given shape

FirFilter::FirFilter() { Tap=NULL; ExternShape=1; }
FirFilter::~FirFilter() { free(Tap); if(!ExternShape) free(Shape); }

void FirFilter::Free(void)
{ free(Tap); Tap=NULL;
  if(!ExternShape) free(Shape);
  Shape=NULL; Output.Free(); }

int FirFilter::Preset(int FilterLen, float *FilterShape)
{ int i;
  if(ReallocArray(&Tap,FilterLen)) return -1;
  for(i=0; i<FilterLen; i++) Tap[i]=0;
  Len=FilterLen; TapPtr=0;
  if(!ExternShape) free(Shape);
  Shape=FilterShape; return 0; }

int FirFilter::ComputeShape(float LowOmega, float UppOmega,
			double (*Window)(double))
{ if(ExternShape) { Shape=NULL; ExternShape=0; }
  if(ReallocArray(&Shape,Len)) return -1;
  WinFirI(LowOmega,UppOmega,Shape,Len,Window); return 0; }

int FirFilter::Process(float *Inp, int InpLen, float *Out)
{ int i,s,t; double Sum;
  if(InpLen<Len)
  { for(i=0; i<InpLen; i++)
    { for(Sum=0.0,t=0,s=i; s<Len; s++)
	Sum+=Tap[s]*Shape[t++];
      for(s-=Len; t<Len; s++)
	Sum+=Inp[s]*Shape[t++];
      Out[i]=Sum; }
    memmove(Tap,Tap+InpLen,(Len-InpLen)*sizeof(float));
    memcpy(Tap+(Len-InpLen),Inp,InpLen*sizeof(float));
  } else
  { for(i=0; i<Len; i++)
    { for(Sum=0.0,t=0,s=i; s<Len; s++)
	Sum+=Tap[s]*Shape[t++];
      for(s-=Len; t<Len; s++)
	Sum+=Inp[s]*Shape[t++];
      Out[i]=Sum; }
    for(; i<InpLen; i++)
    { for(Sum=0.0,t=0,s=i-Len; t<Len; s++)
	Sum+=Inp[s]*Shape[t++];
      Out[i]=Sum; }
    memcpy(Tap,Inp+(InpLen-Len),Len*sizeof(float));
  }
  return InpLen;
}

/*
int FirFilter::Process(float *Inp, int InpLen, float *Out)
{ int i,s,t; double Sum;
  for(i=0; i<InpLen; i++)
  { Tap[TapPtr++]=(*Inp++); if(TapPtr>=Len) TapPtr=0;
    for(Sum=0,s=0,t=TapPtr; t<Len; ) Sum+=Shape[s++]*Tap[t++];
    for(t=0; t<TapPtr; ) Sum+=Shape[s++]*Tap[t++];
    (*Out++)=Sum; }
  return InpLen;
}
*/

int FirFilter::Process(float_buff *Input)
{ int err=Output.EnsureSpace(Input->Len); if(err) return err;
  Process(Input->Data,Input->Len,Output.Data);
  Output.Len=Input->Len; return 0; }

// ----------------------------------------------------------------------------
// a pair of FIR filters. for quadrature split & decimate
// the decimation rate must be an integer

QuadrSplit::QuadrSplit() { ExternShape=1; }

QuadrSplit::~QuadrSplit()
{ if(!ExternShape) { free(ShapeI); free(ShapeQ); } }

void QuadrSplit::Free(void)
{ Tap.Free();
  if(!ExternShape) { free(ShapeI); free(ShapeQ); }
  ShapeI=NULL; ShapeQ=NULL;
  Output.Free(); }

int QuadrSplit::Preset(int FilterLen,
		       float *FilterShape_I, float *FilterShape_Q,
		       int DecimateRate)
{ Len=FilterLen;
  if(!ExternShape) { free(ShapeI); free(ShapeQ); }
  ShapeI=FilterShape_I; ShapeQ=FilterShape_Q; ExternShape=1;
  Tap.EnsureSpace(Len); Tap.Len=Len;
  ClearArray(Tap.Data,Tap.Len);
  Rate=DecimateRate;
  return 0; }

int QuadrSplit::ComputeShape(float LowOmega,float UppOmega,
			 double (*Window)(double))
{ if(ExternShape)
  { ShapeI=NULL; ShapeQ=NULL; ExternShape=0; }
  if(ReallocArray(&ShapeI,Len)) return -1;
  if(ReallocArray(&ShapeQ,Len)) return -1;
  WinFirI(LowOmega,UppOmega,ShapeI,Len,Window);
  WinFirQ(LowOmega,UppOmega,ShapeQ,Len,Window);
  return 0; }

int QuadrSplit::Process(float_buff *Input)
{ int err,i,s,t,o,l; double SumI,SumQ;
  float *Inp; fcmpx *Out; int InpLen;
  InpLen=Input->Len;
  err=Tap.EnsureSpace(Tap.Len+InpLen); if(err) return err;
  CopyArray(Tap.Data+Tap.Len,Input->Data,InpLen);
  // printf("QuadrSplit: InpLen=%d, Tap.Len=%d",InpLen,Tap.Len);
  Tap.Len+=InpLen; Inp=Tap.Data;
  // printf(" -> %d",Tap.Len);
  err=Output.EnsureSpace(InpLen/Rate+2); if(err) return err;
  Out=Output.Data;
  for(l=Tap.Len-Len,o=0,i=0; i<l; i+=Rate)
  { for(SumI=SumQ=0.0,s=i,t=0; t<Len; t++,s++)
    { SumI+=Inp[s]*ShapeI[t]; SumQ+=Inp[s]*ShapeQ[t]; }
    Out[o].re=SumI; Out[o++].im=SumQ; }
  Tap.Len-=i; MoveArray(Tap.Data,Tap.Data+i,Tap.Len); Output.Len=o;
  // printf(" => Tap.Len=%d\n",Tap.Len);
  return 0;
/*
  if(InpLen<Len)
  { for(o=0,i=RateCount; i<InpLen; i+=Rate)
    { for(SumI=SumQ=0.0,t=0,s=i; s<Len; s++)
	{ SumI+=Tap[s]*ShapeI[t]; SumQ+=Tap[s]*ShapeQ[t++]; }
      for(s-=Len; t<Len; s++)
	{ SumI+=Inp[s]*ShapeI[t]; SumQ+=Inp[s]*ShapeQ[t++]; }
      Out[o].re=SumI; Out[o++].im=SumQ; }
    memmove(Tap,Tap+InpLen,(Len-InpLen)*sizeof(float));
    memcpy(Tap+(Len-InpLen),Inp,InpLen*sizeof(float));
  } else
  { for(o=0,i=RateCount; i<Len; i+=Rate)
    { for(SumI=SumQ=0.0,t=0,s=i; s<Len; s++)
	{ SumI+=Tap[s]*ShapeI[t]; SumQ+=Tap[s]*ShapeQ[t++]; }
      for(s-=Len; t<Len; s++)
	{ SumI+=Inp[s]*ShapeI[t]; SumQ+=Inp[s]*ShapeQ[t++]; }
      Out[o].re=SumI; Out[o++].im=SumQ; }
    for(; i<InpLen; i+=Rate)
    { for(SumI=SumQ=0.0,t=0,s=i-Len; t<Len; s++)
	{ SumI+=Inp[s]*ShapeI[t]; SumQ+=Inp[s]*ShapeQ[t++]; }
      Out[o].re=SumI; Out[o++].im=SumQ; }
    memcpy(Tap,Inp+(InpLen-Len),Len*sizeof(float));
  }
  RateCount=i-InpLen; Output.Len=o; return 0;
*/
}

// ----------------------------------------------------------------------------
// reverse of QuadrSplit: interpolates and combines the I/Q
// back into 'real' signal.

QuadrComb::QuadrComb() { Tap=NULL; ExternShape=1; }

QuadrComb::~QuadrComb()
{ free(Tap); if(!ExternShape) { free(ShapeI); free(ShapeQ); } }

void QuadrComb::Free(void)
{ free(Tap); Tap=NULL;
  if(!ExternShape) { free(ShapeI); free(ShapeQ); }
  ShapeI=NULL; ShapeQ=NULL;
  Output.Free(); }

int QuadrComb::Preset(int FilterLen,
		      float *FilterShape_I, float *FilterShape_Q,
		      int DecimateRate)
{ int i;
  Len=FilterLen;
  if(ReallocArray(&Tap,Len)) return -1;
  if(!ExternShape) { free(ShapeI); free(ShapeQ); }
  ShapeI=FilterShape_I; ShapeQ=FilterShape_Q; ExternShape=1;
  for(i=0; i<FilterLen; i++) Tap[i]=0.0;
  TapPtr=0; Rate=DecimateRate;
  return 0; }

int QuadrComb::ComputeShape(float LowOmega,float UppOmega,
			    double (*Window)(double))
{ if(ExternShape)
  { ShapeI=NULL; ShapeQ=NULL; ExternShape=0; }
  if(ReallocArray(&ShapeI,Len)) return -1;
  if(ReallocArray(&ShapeQ,Len)) return -1;
  WinFirI(LowOmega,UppOmega,ShapeI,Len,Window);
  WinFirQ(LowOmega,UppOmega,ShapeQ,Len,Window);
  return 0; }

int QuadrComb::Process(fcmpx_buff *Input)
{ int err,i,o,r,t,len;
  fcmpx *Inp; float *Out; int InpLen; float I,Q;
  InpLen=Input->Len;
  err=Output.EnsureSpace(InpLen*Rate); if(err) return err;
  Inp=Input->Data; Out=Output.Data; Output.Len=InpLen*Rate;
  for(o=0,i=0; i<InpLen; i++)
  { I=Inp[i].re; Q=Inp[i].im;
    for(r=0,t=TapPtr; t<Len; t++,r++) Tap[t]+=I*ShapeI[r]+Q*ShapeQ[r];
    for(    t=0;   t<TapPtr; t++,r++) Tap[t]+=I*ShapeI[r]+Q*ShapeQ[r];
    len=Len-TapPtr;
    if(len<Rate)
    { for(r=0; r<len; r++) { Out[o++]=Tap[TapPtr]; Tap[TapPtr++]=0.0; }
      TapPtr=0;
      for( ; r<Rate; r++) { Out[o++]=Tap[TapPtr]; Tap[TapPtr++]=0.0; }
    } else
    { for(r=0; r<Rate; r++) { Out[o++]=Tap[TapPtr]; Tap[TapPtr++]=0.0; } }
  } return 0;
}

// ----------------------------------------------------------------------------
// complex mix with an oscilator (carrier)

CmpxMixer::CmpxMixer() { Phase=0.0; Omega=0.0; }

void CmpxMixer::Free(void) { Output.Free(); }

int CmpxMixer::Preset(double CarrierOmega)
{ Omega=CarrierOmega; return 0; }

/*
int CmpxMixer::Process(float *InpI, float *InpQ, int InpLen,
		       float *OutI, float *OutQ)
{ int i; double I,Q;
  for(i=0; i<InpLen; i++)
  { I=cos(Phase); Q=sin(Phase);
    OutI[i]=I*InpI[i]+Q*InpQ[i]; OutQ[i]=I*InpQ[i]-Q*InpI[i];
    Phase+=Omega; if(Phase>=2*M_PI) Phase-=2*M_PI; }
  return InpLen;
}

int CmpxMixer::ProcessFast(float *InpI, float *InpQ, int InpLen,
			   float *OutI, float *OutQ)
{ int i; double dI,dQ,I,Q,nI,nQ, N;
  dI=cos(Omega); dQ=sin(Omega);
  I=cos(Phase); Q=sin(Phase);
  for(i=0; i<InpLen; i++)
  { OutI[i]=I*InpI[i]+Q*InpQ[i]; OutQ[i]=I*InpQ[i]-Q*InpI[i];
    nI=I*dI-Q*dQ; nQ=Q*dI+I*dQ; I=nI; Q=nQ; }
  Phase+=InpLen*Omega; N=floor(Phase/(2*M_PI)); Phase-=N*2*M_PI;
  return InpLen;
}
*/

int CmpxMixer::Process(fcmpx *Inp, int InpLen, fcmpx *Out)
{ int i; double I,Q;
  for(i=0; i<InpLen; i++)
  { I=cos(Phase); Q=sin(Phase);
    Out[i].re=I*Inp[i].re+Q*Inp[i].im; Out[i].im=I*Inp[i].im-Q*Inp[i].re;
    Phase+=Omega; if(Phase>=2*M_PI) Phase-=2*M_PI; }
  return InpLen;
}

int CmpxMixer::ProcessFast(fcmpx *Inp, int InpLen, fcmpx *Out)
{ int i; double dI,dQ,I,Q,nI,nQ, N;
  dI=cos(Omega); dQ=sin(Omega);
  I=cos(Phase); Q=sin(Phase);
  for(i=0; i<InpLen; i++)
  { Out[i].re=I*Inp[i].re+Q*Inp[i].im; Out[i].im=I*Inp[i].im-Q*Inp[i].re;
    nI=I*dI-Q*dQ; nQ=Q*dI+I*dQ; I=nI; Q=nQ; }
  Phase+=InpLen*Omega; N=floor(Phase/(2*M_PI)); Phase-=N*2*M_PI;
  return InpLen;
}

int CmpxMixer::Process(fcmpx_buff *Input)
{ int err=Output.EnsureSpace(Input->Len); if(err) return err;
  Process(Input->Data,Input->Len,Output.Data);
  Output.Len=Input->Len; return 0; }

int CmpxMixer::ProcessFast(fcmpx_buff *Input)
{ int err=Output.EnsureSpace(Input->Len); if(err) return err;
  ProcessFast(Input->Data,Input->Len,Output.Data);
  Output.Len=Input->Len; return 0; }

// ----------------------------------------------------------------------------
// FM demodulator (phase rotation speed meter)

FMdemod::FMdemod() { PrevPhase=0.0; RefOmega=0.0; }

int FMdemod::Preset(double CenterOmega) { RefOmega=CenterOmega; return 0; }

int FMdemod::Process(float *InpI, float *InpQ, int InpLen, float *Out)
{ int i; float Phase,PhaseDiff;
  for(i=0; i<InpLen; i++)
  { if((InpI[i]==0.0)&&(InpQ[i]==0.0)) Phase=0;
      else Phase=atan2(InpQ[i],InpI[i]);
    PhaseDiff=Phase-PrevPhase-RefOmega;
    if(PhaseDiff>=M_PI) PhaseDiff-=2*M_PI;
    else if(PhaseDiff<(-M_PI)) PhaseDiff+=2*M_PI;
    Out[i]=PhaseDiff; PrevPhase=Phase;
  } return InpLen;
}

int FMdemod::Process(fcmpx *Inp, int InpLen, float *Out)
{ int i; float Phase,PhaseDiff;
  for(i=0; i<InpLen; i++)
  { if((Inp[i].re==0.0)&&(Inp[i].im==0.0)) Phase=PrevPhase;
      else Phase=atan2(Inp[i].im,Inp[i].re);
    PhaseDiff=Phase-PrevPhase-RefOmega;
    if(PhaseDiff>=M_PI) PhaseDiff-=2*M_PI;
    else if(PhaseDiff<(-M_PI)) PhaseDiff+=2*M_PI;
    Out[i]=PhaseDiff; PrevPhase=Phase;
  } return InpLen;
}

int FMdemod::Process(fcmpx_buff *Input)
{ int err=Output.EnsureSpace(Input->Len); if(err) return err;
  Process(Input->Data,Input->Len,Output.Data);
  Output.Len=Input->Len; return 0; }

// ----------------------------------------------------------------------------
// Rate converter - real input/output, linear interpolation
// expect large error when high frequency components are present
// thus the best place to convert rates is after a low pass filter
// of a demodulator.

RateConvLin::RateConvLin() { PrevSample=0; OutPhase=0; OutStep=1.0; }

void RateConvLin::SetOutVsInp(float OutVsInp) { OutStep=1.0/OutVsInp; }

void RateConvLin::SetInpVsOut(float InpVsOut) { OutStep=InpVsOut; }

int RateConvLin::Process(float_buff *Input)
{ int err,i,o; float *Inp,*Out; int InpLen;
  Inp=Input->Data; InpLen=Input->Len;
  err=Output.EnsureSpace((int)ceil(InpLen/OutStep)+2); if(err) return err;
  Out=Output.Data;
  for(o=0; OutPhase<0; )
  { Out[o++]=Inp[0]*(1.0+OutPhase)-OutPhase*PrevSample; OutPhase+=OutStep; }
  for(i=0; i<(InpLen-1); )
  { if(OutPhase>=1.0) { OutPhase-=1.0; i++; }
    else { Out[o++]=Inp[i]*(1.0-OutPhase)+Inp[i+1]*OutPhase;
	   OutPhase+=OutStep; }
  } Output.Len=o; PrevSample=Inp[i]; OutPhase-=1.0;
  return 0;
}

// ----------------------------------------------------------------------------
// Rate converter - real input/output, quadratic interpolation
// similar limits like for RateConv1

RateConvQuadr::RateConvQuadr()
{ int i; for(i=0; i<4; i++) Tap[i]=0;
  OutStep=1.0; OutPhase=0; TapPtr=0; }

void RateConvQuadr::SetOutVsInp(float OutVsInp) { OutStep=1.0/OutVsInp; }

void RateConvQuadr::SetInpVsOut(float InpVsOut) { OutStep=InpVsOut; }

int RateConvQuadr::Process(float *Inp, int InpLen,
		  float *Out, int MaxOutLen, int *OutLen)
{ int i,o,t; float Ref0,Ref1,Diff0,Diff1;
  for(o=i=0; (i<InpLen)&&(o<MaxOutLen); )
  { if(OutPhase>=1.0)
    { Tap[TapPtr]=(*Inp++); i++; TapPtr=(TapPtr+1)&3; OutPhase-=1.0; }
    else
    { t=TapPtr; Diff0=(Tap[t^2]-Tap[t])/2; Ref1=Tap[t^2];
      t=(t+1)&3; Diff1=(Tap[t^2]-Tap[t])/2; Ref0=Tap[t];
      (*Out++)=Ref0*(1.0-OutPhase)+Ref1*OutPhase // linear piece
	      -(Diff1-Diff0)*OutPhase*(1.0-OutPhase)/2; // quadr. piece
      o++; OutPhase+=OutStep; }
  } (*OutLen)=o; return i;
}

int RateConvQuadr::Process(float_buff *Input)
{ int err,i,o,t; float Ref0,Ref1,Diff0,Diff1; float *Inp,*Out; int InpLen;
  Inp=Input->Data; InpLen=Input->Len;
  err=Output.EnsureSpace((int)ceil(InpLen/OutStep)+2); if(err) return err;
  Out=Output.Data;
  for(o=i=0; i<InpLen; )
  { if(OutPhase>=1.0)
    { Tap[TapPtr]=(*Inp++); i++; TapPtr=(TapPtr+1)&3; OutPhase-=1.0; }
    else
    { t=TapPtr; Diff0=(Tap[t^2]-Tap[t])/2; Ref1=Tap[t^2];
      t=(t+1)&3; Diff1=(Tap[t^2]-Tap[t])/2; Ref0=Tap[t];
      (*Out++)=Ref0*(1.0-OutPhase)+Ref1*OutPhase // linear piece
	      -(Diff1-Diff0)*OutPhase*(1.0-OutPhase)/2; // quadr. piece
      o++; OutPhase+=OutStep; }
  } Output.Len=o;
  return 0;
}

// ----------------------------------------------------------------------------
// Rate converter, real input/output,
// bandwidth-limited interpolation, several shifted FIR filters

RateConvBL::RateConvBL()
{ Tap=NULL; Shape=NULL; ExternShape=1; }

RateConvBL::~RateConvBL() { Free(); }

void RateConvBL::Free(void)
{ int s;
  free(Tap); Tap=NULL; if(ExternShape) return;
  if(Shape)
  { for(s=0; s<ShapeNum; s++) free(Shape[s]);
    free(Shape); Shape=NULL; }
}

int RateConvBL::Preset(int FilterLen, float **FilterShape, int FilterShapeNum)
{ int i;
  Free();
  Len=FilterLen;
  if(ReallocArray(&Tap,Len)) return -1;
  TapSize=Len; for(i=0; i<Len; i++) Tap[i]=0.0;
  Shape=FilterShape; ShapeNum=FilterShapeNum; ExternShape=1;
  OutStep=1.0; OutPhase=0.0;
  return 0;
}

int RateConvBL::ComputeShape(float LowOmega, float UppOmega, double (*Window)(double))
{ int idx;
  if(ExternShape)
  { if(AllocArray(&Shape,ShapeNum)) return -1;
    for(idx=0; idx<ShapeNum; idx++)
    { if(AllocArray(&Shape[idx],Len)) return -1; }
    ExternShape=0; }
  for(idx=0; idx<ShapeNum; idx++)
    WinFirI(LowOmega,UppOmega,Shape[idx],Len,Window,(float)idx/ShapeNum);
/*
  { int t,s; printf("Shape[][] dump:\n");
    for(t=0; t<Len; t++)
    { for(s=0; s<ShapeNum; s++)
	printf(" %+6.4f",Shape[s][t]);
      printf("\n");
    }
  }
*/
  return 0; }

void RateConvBL::SetOutVsInp(float OutVsInp) { OutStep=1.0/OutVsInp; }

void RateConvBL::SetInpVsOut(float InpVsOut) { OutStep=InpVsOut; }

int RateConvBL::Process(float_buff *Input)
{ int i,o,idx,t,err; float *shape; double Sum; float *Inp,*Out; int InpLen;
  Inp=Input->Data; InpLen=Input->Len;
  err=Output.EnsureSpace((int)ceil(InpLen/OutStep)+2); if(err) return err;
  Out=Output.Data;
  if((Len+InpLen)>TapSize)
  { Tap=(float*)realloc(Tap,(Len+InpLen)*sizeof(float)); if(Tap==NULL) return -1;
    TapSize=Len+InpLen; }
  memcpy(Tap+Len,Inp,InpLen*sizeof(float));
  for(o=i=0; i<InpLen; )
  { if(OutPhase>=1.0) { OutPhase-=1.0; i++; }
    else { idx=(int)floor(OutPhase*ShapeNum); shape=Shape[idx];
	   for(Sum=0.0,t=0; t<Len; t++) Sum+=Tap[i+t]*shape[t];
	   Out[o++]=Sum;
	   OutPhase+=OutStep; }
  } Output.Len=o;
  memmove(Tap,Tap+InpLen,Len*sizeof(float));
  return 0;
}

int RateConvBL::ProcessLinI(float_buff *Input)
{ int i,o,idx,t,err;
  float *Inp,*Out; int InpLen;
  double Sum0,Sum1; float *shape; float d;
  Inp=Input->Data; InpLen=Input->Len;
  err=Output.EnsureSpace((int)ceil(InpLen/OutStep)+2); if(err) return err;
  Out=Output.Data;
  if((Len+InpLen)>TapSize)
  { Tap=(float*)realloc(Tap,(Len+InpLen)*sizeof(float)); if(Tap==NULL) return -1;
    TapSize=Len+InpLen; }
  memcpy(Tap+Len,Inp,InpLen*sizeof(float));
  for(o=i=0; i<InpLen; )
  { if(OutPhase>=1.0) { OutPhase-=1.0; i++; }
    else { idx=(int)floor(OutPhase*ShapeNum); d=OutPhase*ShapeNum-idx;
	   shape=Shape[idx];
	   for(Sum0=0.0,t=0; t<Len; t++) Sum0+=Tap[i+t]*shape[t];
	   idx+=1; if(idx>=ShapeNum) { idx=0; i++; }
	   shape=Shape[idx];
	   for(Sum1=0.0,t=0; t<Len; t++) Sum1+=Tap[i+t]*shape[t];
	   if(idx==0) i--;
	   Out[o++]=(1.0-d)*Sum0+d*Sum1;
	   OutPhase+=OutStep; }
  } Output.Len=o;
  memmove(Tap,Tap+InpLen,Len*sizeof(float));
  return 0;
}

// ----------------------------------------------------------------------------
// Sliding window (for FFT input)

CmpxSlideWindow::CmpxSlideWindow()
{ Buff=NULL; Window=NULL; ExternWindow=1; }

CmpxSlideWindow::~CmpxSlideWindow()
{ free(Buff); if(!ExternWindow) free(Window); }

void CmpxSlideWindow::Free(void)
{ free(Buff); Buff=NULL;
  if(!ExternWindow) free(Window);
  Window=NULL; }

int CmpxSlideWindow::Preset(int WindowLen, int SlideDist, float *WindowShape)
{ int i;
  if(SlideDist>WindowLen) return -1;
  Len=WindowLen; Dist=SlideDist;
  if(ReallocArray(&Buff,Len)) return -1;
  for(i=0; i<Len; i++) Buff[i].re=Buff[i].im=0.0;
  Ptr=0;
  if(!ExternWindow) free(Window);
  Window=WindowShape; ExternWindow=1;
  return 0; }

int CmpxSlideWindow::SetWindow(double (*NewWindow)(double phase), double Scale)
{ int idx;
  if(NewWindow==NULL)
  { if(!ExternWindow) free(Window); Window=NULL; ExternWindow=1; return 0; }
  if(ExternWindow) { Window=NULL; ExternWindow=0; }
  if(ReallocArray(&Window,Len)) return -1;
  for(idx=0; idx<Len; idx++)
    Window[idx]=(*NewWindow)(2*M_PI*(idx-Len/2+0.5)/Len)*Scale;
  return 0;
}

int CmpxSlideWindow::Process(fcmpx_buff *Input)
{ fcmpx *Inp=Input->Data; int InpLen=Input->Len;
  int i,len,err;
  Output.Len=0;
  while(InpLen>0)
  { len=imin(Len-Ptr,InpLen);
    memcpy(Buff+Ptr,Inp,len*sizeof(fcmpx)); Ptr+=len; Inp+=len; InpLen-=len;
    if(Ptr>=Len)
    { len=Output.Len; err=Output.EnsureSpace(len+Len); if(err) return err;
      if(Window==NULL) memcpy(Output.Data,Buff,Len*sizeof(fcmpx));
	     else for(i=0; i<Len; i++)
		  { Output.Data[len+i].re=Buff[i].re*Window[i];
		    Output.Data[len+i].im=Buff[i].im*Window[i]; }
      Output.Len+=Len;
      memmove(Buff,Buff+Dist,(Len-Dist)*sizeof(fcmpx)); Ptr-=Dist; }
  } return 0;
}

// ----------------------------------------------------------------------------
// Overlaping window (for IFFT output)

CmpxOverlapWindow::CmpxOverlapWindow()
{ Buff=NULL; Window=NULL; ExternWindow=1; }

CmpxOverlapWindow::~CmpxOverlapWindow()
{ free(Buff); if(!ExternWindow) free(Window); }

void CmpxOverlapWindow::Free(void)
{ free(Buff); Buff=NULL;
  if(!ExternWindow) free(Window);
  Window=NULL; }

int CmpxOverlapWindow::Preset(int WindowLen, int SlideDist, float *WindowShape)
{ int i;
  if(SlideDist>WindowLen) return -1;
  Len=WindowLen; Dist=SlideDist;
  if(ReallocArray(&Buff,Len)) return -1;
  for(i=0; i<Len; i++) Buff[i].re=Buff[i].im=0.0;
  if(!ExternWindow) free(Window);
  Window=WindowShape; ExternWindow=1;
  return 0; }

int CmpxOverlapWindow::SetWindow(double (*NewWindow)(double phase), double Scale)
{ int idx;
  if(NewWindow==NULL)
  { if(!ExternWindow) free(Window); Window=NULL; ExternWindow=1; return 0; }
  if(ExternWindow) { Window=NULL; ExternWindow=0; }
  if(ReallocArray(&Window,Len)) return -1;
  for(idx=0; idx<Len; idx++)
    Window[idx]=(*NewWindow)(2*M_PI*(idx-Len/2+0.5)/Len)*Scale;
  return 0;
}

int CmpxOverlapWindow::Process(fcmpx_buff *Input)
{ int i,err;
  Output.Len=0;
  for(i=0; i<Input->Len; i+=Len)
  { err=Output.EnsureSpace(Output.Len+Dist); if(err) return err;
    Process(Input->Data+i,Output.Data+Output.Len);
    Output.Len+=Dist; }
  return 0;
}

int CmpxOverlapWindow::Process(fcmpx *Input)
{ int err;
  err=Output.EnsureSpace(Dist); if(err) return err;
  Process(Input,Output.Data);
  Output.Len=Dist;
  return 0;
}

void CmpxOverlapWindow::Process(fcmpx *Inp, fcmpx *Out)
{ int i;
  if(Window==NULL)
  { for(i=0; i<Dist; i++)
    { Out[i].re=Buff[i].re+Inp[i].re;
      Out[i].im=Buff[i].im+Inp[i].im; }
    for( ; i<Len-Dist; i++)
    { Buff[i-Dist].re=Buff[i].re+Inp[i].re;
      Buff[i-Dist].im=Buff[i].im+Inp[i].im; }
    for( ; i<Len; i++)
    { Buff[i-Dist].re=Inp[i].re;
      Buff[i-Dist].im=Inp[i].im; }
//    memcpy(Buff+i-Dist,Input+Dist,Dist*sizeof(fcmpx));
  } else
  { for(i=0; i<Dist; i++)
    { Out[i].re=Buff[i].re+Inp[i].re*Window[i];
      Out[i].im=Buff[i].im+Inp[i].im*Window[i]; }
    for( ; i<Len-Dist; i++)
    { Buff[i-Dist].re=Buff[i].re+Inp[i].re*Window[i];
      Buff[i-Dist].im=Buff[i].im+Inp[i].im*Window[i]; }
    for( ; i<Len; i++)
    { Buff[i-Dist].re=Inp[i].re*Window[i];
      Buff[i-Dist].im=Inp[i].im*Window[i]; }
  }
}

int CmpxOverlapWindow::ProcessSilence(int Slides)
{ int err,slide;
  err=Output.EnsureSpace(Slides*Dist); if(err) return err;
  Output.Len=0;
  for(slide=0; slide<Slides; slide++)
  { memcpy(Output.Data+Output.Len,Buff,Dist*sizeof(fcmpx));
    memcpy(Buff,Buff+Dist,(Len-Dist)*sizeof(fcmpx));
    Output.Len+=Dist; }
  return 0; }

// ----------------------------------------------------------------------------
// FFT phase corrector

FFT_TimeShift::FFT_TimeShift() { FreqTable=NULL; }

FFT_TimeShift::~FFT_TimeShift() { free(FreqTable); }

void FFT_TimeShift::Free(void) { free(FreqTable); FreqTable=NULL; }

int FFT_TimeShift::Preset(int FFTlen, int Backwards)
{ int i; double p;
  Phase=0; Len=FFTlen; LenMask=FFTlen-1;
  if((LenMask^Len)!=(2*Len-1)) return -1;
  if(ReallocArray(&FreqTable,Len)) return -1;
  for(i=0; i<Len; i++)
  { p=(2*M_PI*i)/Len; if(Backwards) p=(-p);
    FreqTable[i].re=cos(p); FreqTable[i].im=sin(p); }
  return 0;
}

int FFT_TimeShift::Process(fcmpx *Data, int Time)
{ float nI,nQ; int i,p;
  Phase=(Phase+Time)&LenMask;
  for(p=i=0; i<Len; i++)
  { nI=Data[i].re*FreqTable[i].re-Data[i].im*FreqTable[i].im;
    nQ=Data[i].re*FreqTable[i].im+Data[i].im*FreqTable[i].re;
    Data[i].re=nI; Data[i].im=nQ;
    p=(p+Phase)&LenMask; }
  return 0;
}

// ----------------------------------------------------------------------------
// bit synchronizer, the bit rate is the input rate divided by four

DiffBitSync4::DiffBitSync4(int IntegBits)
{ int i;
  IntegLen=IntegBits;
  InpTapLen=4*IntegLen+8;
  InpTap=(float*)malloc(InpTapLen*sizeof(float));
  for(i=0; i<InpTapLen; i++) InpTap[i]=0; InpTapPtr=0;
  for(i=0; i<4; i++) { DiffInteg[i]=DiffInteg0[i]=0.0; }
  DiffTapPtr=0; BitPtr=0; SyncPhase=0.0;
  SyncDrift=SyncDrift0=0; SyncConfid=0.0;
  LowPass2Coeff((float)IntegLen*2,W1,W2,W5);
}

DiffBitSync4::~DiffBitSync4() { free(InpTap); }

void DiffBitSync4::Free() { free(InpTap); InpTap=NULL; }

int DiffBitSync4::Process(float *Inp, int InpLen,
         float *BitOut, float *IbitOut,
         int MaxOutLen, int *OutLen)
{ int i,o,t,step; float diff; double Sum,SumI,SumQ,phase;
  for(step=0,o=i=0; (i<InpLen)&&(o<MaxOutLen); i++)
  { diff=(-InpTap[InpTapPtr++]); if(InpTapPtr>=InpTapLen) InpTapPtr=0;
    diff+=(InpTap[InpTapPtr]=(*Inp++)); DiffTapPtr=(DiffTapPtr+1)&3;
    LowPass2(diff*diff,DiffInteg0[DiffTapPtr],DiffInteg[DiffTapPtr],W1,W2,W5);
    if(DiffTapPtr==BitPtr)
    { for(Sum=0,t=0; t<4; t++) Sum+=DiffInteg[t];
      t=DiffTapPtr; SumI = DiffInteg[t]-DiffInteg[t^2];
      t=(t+1)&3; SumQ = DiffInteg[t]-DiffInteg[t^2];
      if((Sum==0.0)||((SyncConfid=(SumI*SumI+SumQ*SumQ)/(Sum*Sum))==0.0))
      { (*BitOut++)=0; (*IbitOut++)=0; o++; continue; }
      phase=atan2(-SumQ,-SumI)*(4/(2*M_PI));
      LowPass2(phase-SyncPhase,SyncDrift0,SyncDrift,W1,W2,W5); SyncPhase=phase;
      if(phase>0.52) { step=1; SyncPhase-=1.0; }
       else if(phase<(-0.52)) { step=(-1); SyncPhase+=1.0; }
	else step=0;
float Samp[5],bit,ibit,dx; int p;
      p=InpTapPtr-4*IntegLen-2; if(p<0) p+=InpTapLen;
      for(t=0; t<5; t++) { Samp[t]=InpTap[p++]; if(p>=InpTapLen) p=0; }
      dx=phase-0.5;
      // bit=Samp[2]+dx*(Samp[2]-Samp[1]); // linear interpolation
      bit=Samp[2]*(1.0+dx)-Samp[1]*dx // or quadratic
      +((Samp[3]-Samp[1])-(Samp[2]-Samp[0]))/2*dx*(1.0+dx)/2;
      ibit=Samp[4]+dx*(Samp[4]-Samp[3]); //linear interpolation is enough
      (*BitOut++)=bit; (*IbitOut++)=ibit; o++;
    } else if(DiffTapPtr==(BitPtr^2))
    { BitPtr=(BitPtr+step)&3; step=0; }
  } (*OutLen)=o; return i;
}

float DiffBitSync4::GetSyncConfid() { return 4*SyncConfid; }

float DiffBitSync4::GetSyncDriftRate() { return SyncDrift/4; }

// ----------------------------------------------------------------------------
// bit slicer, SNR/Tune meter

BitSlicer::BitSlicer(int IntegBits)
{ int i;
  TapLen=IntegLen=IntegBits;
  Tap=(float *)malloc(TapLen*sizeof(float));
  for(i=0; i<TapLen; i++) Tap[i]=0; TapPtr=0;
  for(i=0; i<2; i++)
  { Sum[i]=Sum0[i]=0.0;     SumSq[i]=SumSq0[i]=0.0;
    TimeAsym=TimeAsym0=0.0; AmplAsym=AmplAsym0=0.0; Noise[i]=0; }
  LowPass2Coeff((float)IntegLen*2,W1,W2,W5);
  PrevBit=PrevIBit=0.0; OptimThres=0.0;
}

BitSlicer::~BitSlicer()
{ free(Tap); }

int BitSlicer::Process(float *Bits, float *IBits, int InpLen, float *OutBits)
{ int i,l; float Bit,soft;
  for(i=0; i<InpLen; i++)
  { Bit=Bits[i]; l=Bit>0;
    LowPass2(Bit,Sum0[l],Sum[l],W1,W2,W5);
    LowPass2(Bit*Bit,SumSq0[l],SumSq[l],W1,W2,W5);
    Noise[l]=sqrt(SumSq[l]-Sum[l]*Sum[l]);
    if(Noise[0]+Noise[1]<=0) OptimThres=0;
    else OptimThres=(Sum[0]*Noise[1]+Sum[1]*Noise[0])/(Noise[0]+Noise[1]);
    soft=Tap[TapPtr]-OptimThres; // we could do a better soft-decision
    if(Bit*PrevBit<0)
    { LowPass2(PrevIBit,AmplAsym0,AmplAsym,W1,W2,W5);
      if(Bit>0) PrevIBit=(-PrevIBit);
      LowPass2(PrevIBit,TimeAsym0,TimeAsym,W1,W2,W5); }
    (*OutBits++)=soft; PrevBit=Bit; PrevIBit=IBits[i];
    Tap[TapPtr]=Bit; TapPtr++; if(TapPtr>=TapLen) TapPtr=0;
  } return InpLen;
}

float BitSlicer::GetSigToNoise()
{ return Noise[1]>0 ? (Sum[1]-OptimThres)/Noise[1] : 0.0; }

float BitSlicer::GetAmplAsym()
{ float Sweep=Sum[1]-Sum[0]; return Sweep>0 ? 2*AmplAsym/Sweep : 0.0; }

float BitSlicer::GetTimeAsym()
{ float Sweep=Sum[1]-Sum[0]; return Sweep>0 ? 2*TimeAsym/Sweep : 0.0; }

// ----------------------------------------------------------------------------
// The decoder for the HDLC frames,
// makes no AX.25 CRC check, only the length in bytes against MinLen and MaxLen
// however it does not pass frames with non-complete bytes.

HDLCdecoder::HDLCdecoder(int minlen, int maxlen, int diff, int invert,
                         int chan, int (*handler)(int, char *, int))
{ MinLen=minlen; MaxLen=maxlen; RxDiff=diff; RxInvert=invert;
  ChanId=chan; FrameHandler=handler;
  Buff=(char *)malloc(MaxLen); Len=(-1);
  PrevLev=0; ShiftReg=0; BitCount=0; Count1s=0;
  AllFrameCount=0; BadFrameCount=0;
}

HDLCdecoder::~HDLCdecoder()
{ free(Buff); }

int HDLCdecoder::Process(float *Inp, int InpLen)
{ int i,lev,bit,Flag;

  for(i=0; i<InpLen; i++)
  { lev=Inp[i]>0;
    bit=(lev^(PrevLev&RxDiff))^RxInvert; PrevLev=lev;
    ShiftReg=(ShiftReg>>1)|(bit<<7); BitCount+=1; Flag=0;
    if(bit) Count1s+=1; else
    { if(Count1s>=7) Len=(-1);
      else if(Count1s==6) Flag=1;
      else if(Count1s==5) { ShiftReg<<=1; BitCount-=1; }
      Count1s=0; }
    if(Flag)
    { if((Len>=MinLen)&&(BitCount==8)) (*FrameHandler)(ChanId,Buff,Len);
      Len=0; BitCount=0; }
    else if(Len>=0)
    { if(BitCount==8)
      { if(Len<MaxLen) Buff[Len++]=(char)ShiftReg; else Len=(-1);
	BitCount=0; }
    }
  }
  return InpLen;
}

// ----------------------------------------------------------------------------
// AX.25 CRC, adress decoding, etc.

short unsigned int AX25CRCtable[256] = {
	 0U,  4489U,  8978U, 12955U, 17956U, 22445U, 25910U, 29887U,
     35912U, 40385U, 44890U, 48851U, 51820U, 56293U, 59774U, 63735U,
      4225U,   264U, 13203U,  8730U, 22181U, 18220U, 30135U, 25662U,
     40137U, 36160U, 49115U, 44626U, 56045U, 52068U, 63999U, 59510U,
      8450U, 12427U,   528U,  5017U, 26406U, 30383U, 17460U, 21949U,
     44362U, 48323U, 36440U, 40913U, 60270U, 64231U, 51324U, 55797U,
     12675U,  8202U,  4753U,   792U, 30631U, 26158U, 21685U, 17724U,
     48587U, 44098U, 40665U, 36688U, 64495U, 60006U, 55549U, 51572U,
     16900U, 21389U, 24854U, 28831U,  1056U,  5545U, 10034U, 14011U,
     52812U, 57285U, 60766U, 64727U, 34920U, 39393U, 43898U, 47859U,
     21125U, 17164U, 29079U, 24606U,  5281U,  1320U, 14259U,  9786U,
     57037U, 53060U, 64991U, 60502U, 39145U, 35168U, 48123U, 43634U,
     25350U, 29327U, 16404U, 20893U,  9506U, 13483U,  1584U,  6073U,
     61262U, 65223U, 52316U, 56789U, 43370U, 47331U, 35448U, 39921U,
     29575U, 25102U, 20629U, 16668U, 13731U,  9258U,  5809U,  1848U,
     65487U, 60998U, 56541U, 52564U, 47595U, 43106U, 39673U, 35696U,
     33800U, 38273U, 42778U, 46739U, 49708U, 54181U, 57662U, 61623U,
      2112U,  6601U, 11090U, 15067U, 20068U, 24557U, 28022U, 31999U,
     38025U, 34048U, 47003U, 42514U, 53933U, 49956U, 61887U, 57398U,
      6337U,  2376U, 15315U, 10842U, 24293U, 20332U, 32247U, 27774U,
     42250U, 46211U, 34328U, 38801U, 58158U, 62119U, 49212U, 53685U,
     10562U, 14539U,  2640U,  7129U, 28518U, 32495U, 19572U, 24061U,
     46475U, 41986U, 38553U, 34576U, 62383U, 57894U, 53437U, 49460U,
     14787U, 10314U,  6865U,  2904U, 32743U, 28270U, 23797U, 19836U,
     50700U, 55173U, 58654U, 62615U, 32808U, 37281U, 41786U, 45747U,
     19012U, 23501U, 26966U, 30943U,  3168U,  7657U, 12146U, 16123U,
     54925U, 50948U, 62879U, 58390U, 37033U, 33056U, 46011U, 41522U,
     23237U, 19276U, 31191U, 26718U,  7393U,  3432U, 16371U, 11898U,
     59150U, 63111U, 50204U, 54677U, 41258U, 45219U, 33336U, 37809U,
     27462U, 31439U, 18516U, 23005U, 11618U, 15595U,  3696U,  8185U,
     63375U, 58886U, 54429U, 50452U, 45483U, 40994U, 37561U, 33584U,
     31687U, 27214U, 22741U, 18780U, 15843U, 11370U,  7921U,  3960U } ;

short unsigned int AX25CRC(char *Data, int Len)
{ int i,idx; short unsigned int CRC;
  for(CRC=0xFFFF, i=0; i<Len; i++)
  { idx=(unsigned char)CRC^(unsigned char)Data[i];
    CRC=(CRC>>8)^AX25CRCtable[idx];
  } CRC^=0xFFFF;
  return CRC;
}

// ----------------------------------------------------------------------------
// radix-2 FFT

// constructor
r2FFT::r2FFT() { BitRevIdx=NULL; Twiddle=NULL; /* Window=NULL; */ }

// destructor: free twiddles, bit-reverse lookup and window tables
r2FFT::~r2FFT() { free(BitRevIdx); free(Twiddle); /* free(Window); */ }

void r2FFT::Free(void)
{ free(BitRevIdx); BitRevIdx=NULL;
  free(Twiddle); Twiddle=NULL; }

// ..........................................................................

// a radix-2 FFT bufferfly
inline void r2FFT::FFTbf(fcmpx &x0, fcmpx &x1, dcmpx &W)
{ fcmpx x1W;
  x1W.re=x1.re*W.re+x1.im*W.im;    // x1W.re=x1.re*W.re-x1.im*W.im;
  x1W.im=(-x1.re*W.im)+x1.im*W.re; // x1W.im=x1.re*W.im+x1.im*W.re;
  x1.re=x0.re-x1W.re;
  x1.im=x0.im-x1W.im;
  x0.re=x0.re+x1W.re;
  x0.im=x0.im+x1W.im;
}

inline void r2FFT::FFTbf(dcmpx &x0, dcmpx &x1, dcmpx &W)
{ dcmpx x1W;
  x1W.re=x1.re*W.re+x1.im*W.im;    // x1W.re=x1.re*W.re-x1.im*W.im;
  x1W.im=(-x1.re*W.im)+x1.im*W.re; // x1W.im=x1.re*W.im+x1.im*W.re;
  x1.re=x0.re-x1W.re;
  x1.im=x0.im-x1W.im;
  x0.re=x0.re+x1W.re;
  x0.im=x0.im+x1W.im;
}

// 2-point FFT
inline void r2FFT::FFT2(dcmpx &x0, dcmpx &x1)
{ dcmpx x1W;
  x1W.re=x1.re;
  x1W.im=x1.im;
  x1.re=x0.re-x1.re;
  x1.im=x0.im-x1.im;
  x0.re+=x1W.re;
  x0.im+=x1W.im;
}

inline void r2FFT::FFT2(fcmpx &x0, fcmpx &x1)
{ fcmpx x1W;
  x1W.re=x1.re;
  x1W.im=x1.im;
  x1.re=x0.re-x1.re;
  x1.im=x0.im-x1.im;
  x0.re+=x1W.re;
  x0.im+=x1W.im;
}

// 4-point FFT
// beware: these depend on the convention for the twiddle factors !
inline void r2FFT::FFT4(fcmpx &x0, fcmpx &x1, fcmpx &x2, fcmpx &x3)
{ dcmpx x1W;
  x1W.re=x2.re;
  x1W.im=x2.im;
  x2.re=x0.re-x1W.re;
  x2.im=x0.im-x1W.im;
  x0.re=x0.re+x1W.re;
  x0.im=x0.im+x1W.im;
  x1W.re=x3.im;
  x1W.im=(-x3.re);
  x3.re=x1.re-x1W.re;
  x3.im=x1.im-x1W.im;
  x1.re=x1.re+x1W.re;
  x1.im=x1.im+x1W.im;
}

inline void r2FFT::FFT4(dcmpx &x0, dcmpx &x1, dcmpx &x2, dcmpx &x3)
{ dcmpx x1W;
  x1W.re=x2.re;
  x1W.im=x2.im;
  x2.re=x0.re-x1W.re;
  x2.im=x0.im-x1W.im;
  x0.re=x0.re+x1W.re;
  x0.im=x0.im+x1W.im;
  x1W.re=x3.im;
  x1W.im=(-x3.re);
  x3.re=x1.re-x1W.re;
  x3.im=x1.im-x1W.im;
  x1.re=x1.re+x1W.re;
  x1.im=x1.im+x1W.im;
}
// ..........................................................................

// bit reverse (in place) the sequence (before the actuall FTT)
void r2FFT::Scramble(fcmpx x[])
{ int idx,ridx; fcmpx tmp;
  for(idx=0; idx<Size; idx++)
   if((ridx=BitRevIdx[idx])>idx)
    { tmp=x[idx]; x[idx]=x[ridx]; x[ridx]=tmp;
      /* printf("%d <=> %d\n",idx,ridx); */ }
}

// bit reverse the sequence - double precision
void r2FFT::Scramble(dcmpx x[])
{ int idx,ridx; dcmpx tmp;
  for(idx=0; idx<Size; idx++)
   if((ridx=BitRevIdx[idx])>idx)
    { tmp=x[idx]; x[idx]=x[ridx]; x[ridx]=tmp;
      /* printf("%d <=> %d\n",idx,ridx); */ }
}

// Preset for given processing size
int r2FFT::Preset(int size)
{ int err,idx,ridx,mask,rmask; double phase;
  if(!PowerOf2(size)) goto Error;
  Size=size;
  err=ReallocArray(&BitRevIdx,Size); if(err) goto Error;
  err=ReallocArray(&Twiddle,Size); if(err) goto Error;
  for(idx=0; idx<Size; idx++)
  { phase=(2*M_PI*idx)/Size;
    Twiddle[idx].re=cos(phase); Twiddle[idx].im=sin(phase);
    /* printf("%2d => %6.4f => %6.4f %6.4f\n",
	idx,phase,Twiddle[idx].re,Twiddle[idx].im); */ }
  for(ridx=0,idx=0; idx<Size; idx++)
  { for(ridx=0,mask=Size/2,rmask=1; mask; mask>>=1,rmask<<=1)
    { if(idx&mask) ridx|=rmask; }
    BitRevIdx[idx]=ridx; /* printf("%04x %04x\n",idx,ridx); */ }
//  free(Window); Window=NULL; WinInpScale=1.0/Size; WinOutScale=0.5;
  return 0;

  Error: Free(); return -1;
}

// compute the window, set input/output scaling
/*
int r2FFT::SetWindow(double (*NewWindow)(double phase),
		   double InpScale, double OutScale)
{ int idx;
  if(NewWindow==NULL) { free(Window); Window=NULL; }
  else
  { Window=(double *)realloc(Window,Size*sizeof(double)); if(Window==NULL) return -1;
    for(idx=0; idx<Size; idx++)
      Window[idx]=(*NewWindow)(2*M_PI*(idx-Size/2+0.5)/Size);
  } WinInpScale=InpScale; WinOutScale=OutScale;
  return 0;
}
*/
// apply the window and input scaling, copy and bit reverse
/*
void r2FFT::InpWinAndScr(fcmpx Inp[], fcmpx Out[])
{ int idx;
  if(Window==NULL)
    for(idx=0; idx<Size; idx++)
    { Out[BitRevIdx[idx]].re=Inp[idx].re*WinInpScale;
      Out[BitRevIdx[idx]].im=Inp[idx].im*WinInpScale; }
  else
    for(idx=0; idx<Size; idx++)
    { Out[BitRevIdx[idx]].re=Inp[idx].re*Window[idx]*WinInpScale;
      Out[BitRevIdx[idx]].im=Inp[idx].im*Window[idx]*WinInpScale; }
}
*/
// apply the window, copy and bit reverse - double precision
/*
void r2FFT::InpWinAndScr(dcmpx Inp[], dcmpx Out[])
{ int idx;
  if(Window==NULL)
    for(idx=0; idx<Size; idx++)
    { Out[BitRevIdx[idx]].re=Inp[idx].re*WinInpScale;
      Out[BitRevIdx[idx]].im=Inp[idx].im*WinInpScale; }
  else
    for(idx=0; idx<Size; idx++)
    { Out[BitRevIdx[idx]].re=Inp[idx].re*Window[idx]*WinInpScale;
      Out[BitRevIdx[idx]].im=Inp[idx].im*Window[idx]*WinInpScale; }
}
*/
// aply the window, copy and bit reverse - real input
// pack the two real inputs into the same (complex) buffer
// After you execute CoreProc(Out[]) you can SeparTwoReals()
// to get the FFTs results for both inputs.
/*
void r2FFT::InpWinAndScr(float Inp0[], float Inp1[], fcmpx Out[])
{ int idx,ridx;
  if(Window==NULL)
    for(idx=0; idx<Size; idx++)
    { ridx=BitRevIdx[idx];
      Out[ridx].re=Inp0[idx]*WinInpScale;
      Out[ridx].im=Inp1[idx]*WinInpScale; }
  else
    for(idx=0; idx<Size; idx++)
    { ridx=BitRevIdx[idx];
      Out[ridx].re=Inp0[idx]*Window[idx]*WinInpScale;
      Out[ridx].im=Inp1[idx]*Window[idx]*WinInpScale; }
}
*/
// aply the window, copy and bit reverse - real input, double precision
/*
void r2FFT::InpWinAndScr(double Inp0[], double Inp1[], dcmpx Out[])
{ int idx,ridx;
  if(Window==NULL)
    for(idx=0; idx<Size; idx++)
    { ridx=BitRevIdx[idx];
      Out[ridx].re=Inp0[idx]*WinInpScale;
      Out[ridx].im=Inp1[idx]*WinInpScale; }
  else
    for(idx=0; idx<Size; idx++)
    { ridx=BitRevIdx[idx];
      Out[ridx].re=Inp0[idx]*Window[idx]*WinInpScale;
      Out[ridx].im=Inp1[idx]*Window[idx]*WinInpScale; }
}
*/
/*
void r2FFT::OutWin(fcmpx Buff[], float Out0[], float Out1[])
{ int idx;
  if(Window==NULL)
    for(idx=0; idx<Size; idx++)
    { Out0[idx]=  Buff[idx].re *WinOutScale;
      Out1[idx]=(-Buff[idx].im)*WinOutScale; }
  else
    for(idx=0; idx<Size; idx++)
    { Out0[idx]=  Buff[idx].re *Window[idx]*WinOutScale;
      Out1[idx]=(-Buff[idx].im)*Window[idx]*WinOutScale; }
}

void r2FFT::OutWin(dcmpx Buff[], double Out0[], double Out1[])
{ int idx;
  if(Window==NULL)
    for(idx=0; idx<Size; idx++)
    { Out0[idx]=  Buff[idx].re *WinOutScale;
      Out1[idx]=(-Buff[idx].im)*WinOutScale; }
  else
    for(idx=0; idx<Size; idx++)
    { Out0[idx]=  Buff[idx].re *Window[idx]*WinOutScale;
      Out1[idx]=(-Buff[idx].im)*Window[idx]*WinOutScale; }
}
*/
// ..........................................................................

// radix-2 FFT: the first and the second pass are by hand
// looks like there is no gain by separating the second pass
// and even the first pass is in question ?
void r2FFT::CoreProc(fcmpx x[])
{ int Groups,GroupHalfSize,Group,Bf,TwidIdx;
  int HalfSize=Size/2;
  for(Bf=0; Bf<Size; Bf+=2) FFT2(x[Bf],x[Bf+1]); // first pass
  // for(Bf=0; Bf<Size; Bf+=4) FFT4(x[Bf],x[Bf+1],x[Bf+2],x[Bf+3]); // second
  for(Groups=HalfSize/2,GroupHalfSize=2; Groups; Groups>>=1, GroupHalfSize<<=1)
    for(Group=0,Bf=0; Group<Groups; Group++,Bf+=GroupHalfSize)
      for(TwidIdx=0; TwidIdx<HalfSize; TwidIdx+=Groups,Bf++)
      { FFTbf(x[Bf],x[Bf+GroupHalfSize],Twiddle[TwidIdx]);
	/* printf("%2d %2d %2d\n",Bf,Bf+GroupHalfSize,TwidIdx); */ }
}

// radix-2 FFT with double precision
void r2FFT::CoreProc(dcmpx x[])
{ int Groups,GroupHalfSize,Group,Bf,TwidIdx;
  int HalfSize=Size/2;
  for(Bf=0; Bf<Size; Bf+=2) FFT2(x[Bf],x[Bf+1]); // first pass
  // for(Bf=0; Bf<Size; Bf+=4) FFT4(x[Bf],x[Bf+1],x[Bf+2],x[Bf+3]); // second
  for(Groups=HalfSize/2,GroupHalfSize=2; Groups; Groups>>=1, GroupHalfSize<<=1)
    for(Group=0,Bf=0; Group<Groups; Group++,Bf+=GroupHalfSize)
      for(TwidIdx=0; TwidIdx<HalfSize; TwidIdx+=Groups,Bf++)
      { FFTbf(x[Bf],x[Bf+GroupHalfSize],Twiddle[TwidIdx]);
	/* printf("%2d %2d %2d\n",Bf,Bf+GroupHalfSize,TwidIdx); */ }
}
// ..........................................................................

// separate the result of "two reals at one time" processing
void r2FFT::SeparTwoReals(fcmpx Buff[], fcmpx Out0[], fcmpx Out1[])
{ int idx,HalfSize=Size/2;
//  for(idx=0; idx<Size; idx++)
//    printf("%2d %9.5f %9.5f\n",idx,Buff[idx].re,Buff[idx].im);
  Out0[0].re=Buff[0].re; Out1[0].re=Buff[0].im;
  for(idx=1; idx<HalfSize; idx++)
  { Out0[idx].re=  Buff[idx].re +Buff[Size-idx].re;
    Out0[idx].im=  Buff[idx].im -Buff[Size-idx].im;
    Out1[idx].re=  Buff[idx].im +Buff[Size-idx].im;
    Out1[idx].im=(-Buff[idx].re)+Buff[Size-idx].re;
  } Out0[0].im=Buff[HalfSize].re; Out1[0].im=Buff[HalfSize].im;
//  for(idx=0; idx<HalfSize; idx++)
//    printf("%2d  %9.5f %9.5f  %9.5f %9.5f\n",
//      idx,Out0[idx].re,Out0[idx].im,Out1[idx].re,Out1[idx].im);
}

// separate with double precision
void r2FFT::SeparTwoReals(dcmpx Buff[], dcmpx Out0[], dcmpx Out1[])
{ int idx,HalfSize=Size/2;
//  for(idx=0; idx<Size; idx++)
//    printf("%2d %9.5f %9.5f\n",idx,Buff[idx].re,Buff[idx].im);
  Out0[0].re=Buff[0].re; Out1[0].re=Buff[0].im;
  for(idx=1; idx<HalfSize; idx++)
  { Out0[idx].re=  Buff[idx].re +Buff[Size-idx].re;
    Out0[idx].im=  Buff[idx].im -Buff[Size-idx].im;
    Out1[idx].re=  Buff[idx].im +Buff[Size-idx].im;
    Out1[idx].im=(-Buff[idx].re)+Buff[Size-idx].re;
  } Out0[0].im=Buff[HalfSize].re; Out1[0].im=Buff[HalfSize].im;
//  for(idx=0; idx<HalfSize; idx++)
//    printf("%2d  %9.5f %9.5f  %9.5f %9.5f\n",
//      idx,Out0[idx].re,Out0[idx].im,Out1[idx].re,Out1[idx].im);
}

// the oposite of SeparTwoReals()
// but we NEGATE the .im part for Inverse FFT
// as a "by-product" we multiply the transform by 2
void r2FFT::JoinTwoReals(fcmpx Inp0[], fcmpx Inp1[], fcmpx Buff[])
{ int idx,HalfSize=Size/2;
//  for(idx=0; idx<HalfSize; idx++)
//    printf("%2d  %9.5f %9.5f  %9.5f %9.5f\n",
//      idx,Inp0[idx].re,Inp0[idx].im,Inp1[idx].re,Inp1[idx].im);
  Buff[0].re=2*Inp0[0].re; Buff[0].im=(-2*Inp1[0].re);
  for(idx=1; idx<HalfSize; idx++)
  { Buff[idx].re     =  Inp0[idx].re -Inp1[idx].im;
    Buff[idx].im     =(-Inp0[idx].im)-Inp1[idx].re;
    Buff[Size-idx].re=  Inp0[idx].re +Inp1[idx].im;
    Buff[Size-idx].im=  Inp0[idx].im -Inp1[idx].re;
  } Buff[HalfSize].re=2*Inp0[0].im; Buff[HalfSize].im=(-2*Inp1[0].im);
//  for(idx=0; idx<Size; idx++)
//    printf("%2d %9.5f %9.5f\n",idx,Buff[idx].re,Buff[idx].im);
}

// the oposite of SeparTwoReals() with double precision
void r2FFT::JoinTwoReals(dcmpx Inp0[], dcmpx Inp1[], dcmpx Buff[])
{ int idx,HalfSize=Size/2;
//  for(idx=0; idx<HalfSize; idx++)
//    printf("%2d  %9.5f %9.5f  %9.5f %9.5f\n",
//      idx,Inp0[idx].re,Inp0[idx].im,Inp1[idx].re,Inp1[idx].im);
  Buff[0].re=2*Inp0[0].re; Buff[0].im=(-2*Inp1[0].re);
  for(idx=1; idx<HalfSize; idx++)
  { Buff[idx].re     =  Inp0[idx].re -Inp1[idx].im;
    Buff[idx].im     =(-Inp0[idx].im)-Inp1[idx].re;
    Buff[Size-idx].re=  Inp0[idx].re +Inp1[idx].im;
    Buff[Size-idx].im=  Inp0[idx].im -Inp1[idx].re;
  } Buff[HalfSize].re=2*Inp0[0].im; Buff[HalfSize].im=(-2*Inp1[0].im);
//  for(idx=0; idx<Size; idx++)
//    printf("%2d %9.5f %9.5f\n",idx,Buff[idx].re,Buff[idx].im);
}

// ----------------------------------------------------------------------------
// Sliding window FFT for spectral analysis
// input: real-valued time-domain signal,
// output: complex-valued Fourier Transform
//
// We use a little trick here to process two real-valued FFT
// in one go using the complex FFT engine.
// This cuts the CPU but makes the input->output delay longer.

SlideWinFFT::SlideWinFFT()
{ SlideBuff=NULL; FFTbuff=NULL; Window=NULL; ExternWindow=1; }

SlideWinFFT::~SlideWinFFT()
{ free(SlideBuff); free(FFTbuff); if(!ExternWindow) free(Window); }

void SlideWinFFT::Free(void)
{ free(SlideBuff); SlideBuff=NULL;
  free(FFTbuff); FFTbuff=NULL;
  if(!ExternWindow) free(Window);
  Window=NULL; ExternWindow=1;
  Output.Free(); }

int SlideWinFFT::Preset(int size, int step, float *window)
{ int err,i;

  Size=size; SizeMask=Size-1;
  err=FFT.Preset(Size); if(err) goto Error;

  if(!ExternWindow) { free(Window); ExternWindow=1; }
  Window=window;

  err=ReallocArray(&FFTbuff,Size); if(err) goto Error;

  err=ReallocArray(&SlideBuff,Size); if(err) goto Error;
  for(i=0; i<Size; i++) SlideBuff[i]=0.0;
  SlidePtr=0; Slide=0; Dist=step; Left=Dist;

  return 0;

  Error: Free(); return -1;
}

int SlideWinFFT::SetWindow(double (*NewWindow)(double phase), float Scale)
{ int idx,err;
  if(NewWindow==NULL)
  { if(!ExternWindow) free(Window); Window=NULL; ExternWindow=1; return 0; }
  if(ExternWindow) { Window=NULL; ExternWindow=0; }
  err=ReallocArray(&Window,Size); if(err) return -1;
  for(idx=0; idx<Size; idx++)
    Window[idx]=Scale*(*NewWindow)(2*M_PI*(idx-Size/2+0.5)/Size);
  return 0;
}

int SlideWinFFT::Preset(int size, int step,
      double (*NewWindow)(double phase), float Scale)
{ int err;
  err=Preset(size,step,(float *)NULL); if(err) return -1;
  err=SetWindow(NewWindow,Scale);
  if(err) { Free(); return -1; }
  return 0; }

int SlideWinFFT::SetWindow(float *window)
{ if(!ExternWindow) { free(Window); ExternWindow=1; }
  Window=window; return 0; }

int SlideWinFFT::Process(float_buff *Input)
{ int err,len,i,t; int InpLen; float *Inp;
  Inp=Input->Data; InpLen=Input->Len; Output.Len=0;
  while(InpLen)
  { for(i=len=imin(InpLen,Left); i; i--)
    { SlideBuff[SlidePtr++]=(*Inp++); SlidePtr&=SizeMask; }
    InpLen-=len; Left-=len;
    if(Left==0)
    { Slide^=1; Left=Dist;
      if(Slide)
      { for(t=0,i=SlidePtr; i<Size; t++,i++)
	  FFTbuff[t].re=Window[t]*SlideBuff[i];
	for(i=0; t<Size; t++,i++)
	  FFTbuff[t].re=Window[t]*SlideBuff[i]; }
      else
      { for(t=0,i=SlidePtr; i<Size; t++,i++)
	  FFTbuff[t].im=Window[t]*SlideBuff[i];
	for(i=0; t<Size; t++,i++)
	  FFTbuff[t].im=Window[t]*SlideBuff[i];
	FFT.Scramble(FFTbuff); FFT.CoreProc(FFTbuff);
	len=Output.Len;
	err=Output.EnsureSpace(len+Size); if(err) return -1;
	FFT.SeparTwoReals(FFTbuff,Output.Data+len,Output.Data+len+Size/2);
	Output.Len+=Size;
      }
    }
  }
  return 0;
}

// ----------------------------------------------------------------------------
// Overlapping IFFT to convert sliced spectra into time-domain output



// ----------------------------------------------------------------------------
// Sliding window FFT for spectral processing
// input: real-valued signal
// in the middle you are given a chance to process
// the complex-valued Fourier Transform (SpectraProc() routine).
// output: real-valued signal
// If you don't touch the spectra in SpectralProc()
// the output will be an exact copy (only delayed) of the input.

SlideWinFFTproc::SlideWinFFTproc()
{ SlideBuff=NULL; OvlapBuff=NULL;
  FFTbuff=NULL; Spectr[0]=NULL; Spectr[1]=NULL;
  Window=NULL; ExternWindow=1; }

SlideWinFFTproc::~SlideWinFFTproc()
{ free(SlideBuff); free(OvlapBuff);
  free(FFTbuff); free(Spectr[0]); free(Spectr[1]);
  if(!ExternWindow) free(Window); }

void SlideWinFFTproc::Free(void)
{ int i;
  free(SlideBuff); SlideBuff=NULL;
  free(OvlapBuff); OvlapBuff=NULL;
  free(FFTbuff); FFTbuff=NULL;
  for(i=0; i<2; i++) { free(Spectr[0]); Spectr[0]=NULL; }
  if(!ExternWindow) free(Window);
  Window=NULL; ExternWindow=1;
  Output.Free(); }

int SlideWinFFTproc::Preset(int size, int step,
      void (*proc)(fcmpx *Spectra, int Len), float *window)
{ int err,i;

  Size=size; SizeMask=Size-1;
  err=FFT.Preset(Size); if(err) goto Error;

  if(!ExternWindow) { free(Window); ExternWindow=1; }
  Window=window;

  ReallocArray(&FFTbuff,Size); if(err) goto Error;

  for(i=0; i<2; i++)
  { err=ReallocArray(&Spectr[i],Size/2); if(err) goto Error; }

  err=ReallocArray(&SlideBuff,Size); if(err) goto Error;
  for(i=0; i<Size; i++) SlideBuff[i]=0.0;
  SlidePtr=0; Slide=0; Dist=step; Left=Dist;

  err=ReallocArray(&OvlapBuff,Size); if(err) goto Error;
  for(i=0; i<Size; i++) OvlapBuff[i]=0.0; OvlapPtr=0;

  SpectraProc=proc;

  return 0;

  Error: Free(); return -1;
}

int SlideWinFFTproc::Preset(int size, int step,
      void (*proc)(fcmpx *Spectra, int Len),
      double (*NewWindow)(double phase), float Scale)
{ int err;
  err=Preset(size,step,proc,(float *)NULL); if(err) return -1;
  err=SetWindow(NewWindow,Scale);
  if(err) { Free(); return -1; }
  return 0; }

int SlideWinFFTproc::SetWindow(float *window)
{ if(!ExternWindow) { free(Window); ExternWindow=1; }
  Window=window; return 0; }

int SlideWinFFTproc::SetWindow(double (*NewWindow)(double phase), float Scale)
{ int idx,err;
  if(NewWindow==NULL)
  { if(!ExternWindow) free(Window); Window=NULL; ExternWindow=1; return 0; }
  if(ExternWindow) { Window=NULL; ExternWindow=0; }
  err=ReallocArray(&Window,Size); if(err) return -1;
  if(Scale==0.0) Scale=sqrt(0.5/Size);
  for(idx=0; idx<Size; idx++)
    Window[idx]=Scale*(*NewWindow)(2*M_PI*(idx-Size/2+0.5)/Size);
  return 0;
}

int SlideWinFFTproc::Process(float_buff *Input)
{ int err,len,i,t; int InpLen; float *Inp,*Out;
  Inp=Input->Data; InpLen=Input->Len; Output.Len=0;
  while(InpLen)
  { for(i=len=imin(InpLen,Left); i; i--)
    { SlideBuff[SlidePtr++]=(*Inp++); SlidePtr&=SizeMask; }
    InpLen-=len; Left-=len;
    if(Left==0)
    { Slide^=1; Left=Dist;
      if(Slide)
      { for(t=0,i=SlidePtr; i<Size; t++,i++)
	  FFTbuff[t].re=Window[t]*SlideBuff[i];
	for(i=0; t<Size; t++,i++)
	  FFTbuff[t].re=Window[t]*SlideBuff[i]; }
      else
      { for(t=0,i=SlidePtr; i<Size; t++,i++)
	  FFTbuff[t].im=Window[t]*SlideBuff[i];
	for(i=0; t<Size; t++,i++)
	  FFTbuff[t].im=Window[t]*SlideBuff[i];

	FFT.Scramble(FFTbuff); FFT.CoreProc(FFTbuff);
	FFT.SeparTwoReals(FFTbuff,Spectr[0],Spectr[1]);

	for(i=0; i<2; i++) (*SpectraProc)(Spectr[i],Size);

	FFT.JoinTwoReals(Spectr[0],Spectr[1],FFTbuff);
	FFT.Scramble(FFTbuff); FFT.CoreProc(FFTbuff);

	err=Output.EnsureSpace(Output.Len+2*Dist); if(err) return -1;
	Out=Output.Data+Output.Len;

	for(t=0,i=OvlapPtr; i<Size; t++,i++)
	  OvlapBuff[i]+=Window[t]*FFTbuff[t].re;
	for(i=0; t<Size; t++,i++)
	  OvlapBuff[i]+=Window[t]*FFTbuff[t].re;
	for(i=0; i<Dist; i++)
	{ (*Out++)=OvlapBuff[OvlapPtr];
		   OvlapBuff[OvlapPtr++]=0.0; OvlapPtr&=SizeMask; }

	for(t=0,i=OvlapPtr; i<Size; t++,i++)
	  OvlapBuff[i]-=Window[t]*FFTbuff[t].im;
	for(i=0; t<Size; t++,i++)
	  OvlapBuff[i]-=Window[t]*FFTbuff[t].im;
	for(i=0; i<Dist; i++)
	{ (*Out++)=OvlapBuff[OvlapPtr];
		   OvlapBuff[OvlapPtr++]=0.0; OvlapPtr&=SizeMask; }

	Output.Len+=2*Dist;
      }
    }
  }
  return 0;
}



// ----------------------------------------------------------------------------
// Walsh (Hadamard ?) transform.

void WalshTrans(double *Data, int Len)  // Len must be 2^N
{ int step, ptr, ptr2; double bit1, bit2;
  for(step=1; step<Len; step*=2)
  { for(ptr=0; ptr<Len; ptr+=2*step)
    { for(ptr2=ptr; (ptr2-ptr)<step; ptr2+=1)
      { bit1=Data[ptr2];  bit2=Data[ptr2+step];
//	Data[ptr2]=(bit1+bit2); Data[ptr2+step]=(bit1-bit2);
	Data[ptr2]=(bit1+bit2); Data[ptr2+step]=(bit2-bit1);
      }
    }
  }
}

void WalshTrans(float *Data, int Len)  // Len must be 2^N
{ int step, ptr, ptr2; float bit1, bit2;
  for(step=1; step<Len; step*=2)
  { for(ptr=0; ptr<Len; ptr+=2*step)
    { for(ptr2=ptr; (ptr2-ptr)<step; ptr2+=1)
      { bit1=Data[ptr2];  bit2=Data[ptr2+step];
//	Data[ptr2]=(bit1+bit2); Data[ptr2+step]=(bit1-bit2);
	Data[ptr2]=(bit1+bit2); Data[ptr2+step]=(bit2-bit1);
      }
    }
  }
}

void WalshInvTrans(double *Data, int Len)  // Len must be 2^N
{ int step, ptr, ptr2; double bit1, bit2;
  for(step=Len/2; step; step/=2)
  { for(ptr=0; ptr<Len; ptr+=2*step)
    { for(ptr2=ptr; (ptr2-ptr)<step; ptr2+=1)
      { bit1=Data[ptr2];  bit2=Data[ptr2+step];
//	Data[ptr2]=(bit1+bit2); Data[ptr2+step]=(bit1-bit2);
	Data[ptr2]=(bit1-bit2); Data[ptr2+step]=(bit1+bit2);
      }
    }
  }
}

void WalshInvTrans(float *Data, int Len)  // Len must be 2^N
{ int step, ptr, ptr2; float bit1, bit2;
  for(step=Len/2; step; step/=2)
  { for(ptr=0; ptr<Len; ptr+=2*step)
    { for(ptr2=ptr; (ptr2-ptr)<step; ptr2+=1)
      { bit1=Data[ptr2];  bit2=Data[ptr2+step];
//	Data[ptr2]=(bit1+bit2); Data[ptr2+step]=(bit1-bit2);
	Data[ptr2]=(bit1-bit2); Data[ptr2+step]=(bit1+bit2);
      }
    }
  }
}

// ----------------------------------------------------------------------------


