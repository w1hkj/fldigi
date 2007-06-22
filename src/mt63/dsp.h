/*
 *    dsp.h  --  various DSP algorithms
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

#include <stdlib.h>
#include <string.h>
#include <math.h>

// ----------------------------------------------------------------------------
// float/double/other-complex type

template <class type> struct Cmpx { type re,im; } ;

typedef Cmpx<float> fcmpx;
typedef Cmpx<double> dcmpx;

// Some complex operators
// at least with the BC++ they carry some overhead because
// a function is always called instead of making the code inline.
template <class type>
 inline void operator +=(Cmpx<type> &Dst, Cmpx<type> &Src)
{ Dst.re+=Src.re; Dst.im+=Src.im; }

template <class type>
 inline void operator -=(Cmpx<type> &Dst, Cmpx<type> &Src)
{ Dst.re-=Src.re; Dst.im-=Src.im; }

template <class type, class num>
 inline void operator *=(Cmpx<type> &Dst, num Src)
{ Dst.re*=Src; Dst.im*=Src; }

template <class type, class num>
 inline void operator /=(Cmpx<type> &Dst, num Src)
{ Dst.re/=Src; Dst.im/=Src; }

// scalar product of two vectors
template <class typeA, class typeB>
 inline double ScalProd(Cmpx<typeA> &A, Cmpx<typeB> &B)
{ return A.re*B.re+A.im*B.im; }

template <class typeA, class typeB>
 inline double ScalProd(typeA Ia, typeA Qa, Cmpx<typeB> &B)
{ return Ia*B.re+Qa*B.im; }

// complex multiply
template <class typeDst, class typeA, class typeB>
 inline void CmpxMultAxB(Cmpx<typeDst> &Dst, Cmpx<typeA> &A, Cmpx<typeB> &B)
{ Dst.re=A.re*B.re-A.im*B.im;
  Dst.im=A.re*B.im+A.im*B.re; }

template <class typeDst, class typeA, class typeB>
 inline void CmpxMultAxB(typeDst &DstI, typeDst &DstQ, Cmpx<typeA> &A, Cmpx<typeB> &B)
{ DstI=A.re*B.re-A.im*B.im;
  DstQ=A.re*B.im+A.im*B.re; }

// complex multiply, second argument with a "star" (B.im is negated)
template <class typeDst, class typeA, class typeB>
 inline void CmpxMultAxBs(Cmpx<typeDst> &Dst, Cmpx<typeA> &A, Cmpx<typeB> &B)
{ Dst.re=A.re*B.re+A.im*B.im;
  Dst.im=A.im*B.re-A.re*B.im; }

// ----------------------------------------------------------------------------
// signed 16-bit format (standard 16-bit audio)

typedef short s16;

// ----------------------------------------------------------------------------

template <class type> inline int ReallocArray(type **Array, int Size)
{ (*Array)=(type *)realloc(*Array,Size*sizeof(type));
  return (*Array)==NULL; }

template <class type> inline int AllocArray(type **Array, int Size)
{ (*Array)=(type *)malloc(Size*sizeof(type));
  return (*Array)==NULL; }

template <class type> inline void ClearArray(type *Array, int Size)
{ memset(Array,0,Size*sizeof(type)); }

template <class type> inline void CopyArray(type *Dst, type *Src, int Size)
{ memcpy(Dst,Src,Size*sizeof(type)); }

template <class type> inline void MoveArray(type *Dst, type *Src, int Size)
{ memmove(Dst,Src,Size*sizeof(type)); }

template <class type> int AllocArray2D(type ***Array, int Size1, int Size2)
{ int i;
  (*Array)=(type **)malloc(Size1*(sizeof(type *)));
  if((*Array)==NULL) return 1;
  for(i=0; i<Size1; i++) (*Array)[i]=NULL;
  for(i=0; i<Size1; i++)
  { (*Array)[i]=(type *)malloc(Size2*sizeof(type));
    if((*Array)[i]==NULL) goto Error; }
  return 0;
Error:
  for(i=0; i<Size1; i++) free((*Array)[i]); free(*Array); return 1;
}

template <class type> void FreeArray2D(type **Array, int Size1)
{ int i; for(i=0; i<Size1; i++) free(Array[i]); free(Array); }

template <class type> void ClearArray2D(type **Array, int Size1, int Size2)
{ int i; for(i=0; i<Size1; i++) memset(Array[i],0,Size2*sizeof(type)); }

// ----------------------------------------------------------------------------
// processing buffers:

template <class type> class Seq
{ public:
   Seq(); ~Seq();
   int EnsureSpace(int ReqSpace); // make sure that there is enough space
   void Free(void); // free space to save RAM when buffer is not in use
   int Space;   // that much is allocated in *Data
   int Len;	// that much is filled up
   type *Data;  // contains Len elements
} ;

template <class type> Seq<type>::Seq() { Data=NULL; Len=Space=0; }
template <class type> Seq<type>::~Seq() { free(Data); }
template <class type> int Seq<type>::EnsureSpace(int ReqSpace)
{ if(ReqSpace<=Space) return 0;
  Data=(type *)realloc(Data,ReqSpace*sizeof(type));
  if(Data==NULL) { Space=Len=0; return -1; }
  Space=ReqSpace; return 0; }
template <class type> void Seq<type>::Free(void)
{ free(Data); Data=NULL; Space=Len=0; }

typedef Seq<float> float_buff;
typedef Seq<double> double_buff;
typedef Seq<fcmpx> fcmpx_buff;
typedef Seq<dcmpx> dcmpx_buff;
// typedef Seq<short> int16_buff; <- this doesn't work - why ?!
typedef Seq<s16>   s16_buff;
typedef Seq<char>  char_buff;

// ----------------------------------------------------------------------------
// First-In First-Out pipes

template <class type> class FIFO
{ public:
   FIFO(); ~FIFO();
   int Preset(int Max);
   void Free(void);
   void Clear(void);
   int Inp(type Elem);
   int Out(type &Elem);
   int InpReady(void);
   int OutReady(void);
  private:
   type *Buff;
   int Size;
   int Rd,Wr;
} ;

template <class type> FIFO<type>::FIFO() { Buff=NULL; }
template <class type> FIFO<type>::~FIFO() { free(Buff); }

template <class type> void FIFO<type>::Free(void) { free(Buff); Buff=NULL; }

template <class type> int FIFO<type>::Preset(int Max)
{ Size=Max+1;
  if(ReallocArray(&Buff,Size)) return -1;
  Rd=0; Wr=0; return 0; }

template <class type> void FIFO<type>::Clear(void) { Rd=Wr; }

template <class type> int FIFO<type>::Inp(type Elem)
{ int w=Wr;
  Buff[w]=Elem; w+=1; if(w>=Size) w=0;
  if(w==Rd) return -1;
  Wr=w; return 0; }

template <class type> int FIFO<type>::Out(type &Elem)
{ if(Rd==Wr) return -1;
  Elem=Buff[Rd]; Rd+=1; if(Rd>=Size) Rd=0; return 0; }

template <class type> int FIFO<type>::OutReady(void)
{ return (Wr>=Rd) ? Wr-Rd : Wr-Rd+Size; }

template <class type> int FIFO<type>::InpReady(void)
{ return (Rd>Wr) ? Rd-Wr-1 : Rd-Wr+Size-1; }

typedef FIFO<char> char_fifo;

// ----------------------------------------------------------------------------
// power of single and complex values and sequences of these

inline double Power(float X) { return X*X; }
inline double Power(double X) { return X*X; }
inline double Power(float I, float Q) { return I*I + Q*Q; }
inline double Power(double I, double Q) { return I*I + Q*Q; }
inline double Power(fcmpx X) { return X.re*X.re+X.im*X.im; }
inline double Power(dcmpx X) { return X.re*X.re+X.im*X.im; }

/*
template <class type>
 double Power(type *X, int Len)
{ double Sum;
  for(Sum=0.0; Len; Len--,X++)
    Sum+=(*X)*(*X);
  return Sum; }

template <class type>
 double Power(type *I, type *Q, int Len)
{ double Sum;
  for(Sum=0.0; Len; Len--,I++,Q++)
    Sum+=(*I)*(*I)+(*Q)*(*Q);
  return Sum; }

template <class type>
 double Power(Cmpx<type> *X, int Len)
{ double Sum;
  for(Sum=0.0; Len; Len--,X++)
    Sum+=(X->re)*(X->re)+(X->im)*(X->im);
  return Sum; }
*/

double Power(float *X, int Len);
double Power(double *X, int Len);
double Power(float *I, double *Q, int Len);
double Power(double *I, float *Q, int Len);
double Power(fcmpx *X, int Len);
double Power(dcmpx *X, int Len);

inline double Power(float_buff *buff) { return Power(buff->Data,buff->Len); }
inline double Power(fcmpx_buff *buff) { return Power(buff->Data,buff->Len); }
inline double Power(dcmpx_buff *buff) { return Power(buff->Data,buff->Len); }

// Amplitude calculations

inline double Ampl(float I, float Q) { return sqrt(I*I+Q*Q); }
inline double Ampl(double I, double Q) { return sqrt(I*I+Q*Q); }
inline double Ampl(fcmpx X) { return sqrt(X.re*X.re+X.im*X.im); }
inline double Ampl(dcmpx X) { return sqrt(X.re*X.re+X.im*X.im); }

// Phase calculation (output = <-PI..PI) )

inline double Phase(float I, float Q) { return atan2(Q,I); }
inline double Phase(double I, double Q) { return atan2(Q,I); }
inline double Phase(fcmpx X) { return atan2(X.im,X.re); }
inline double Phase(dcmpx X) { return atan2(X.im,X.re); }

// Phase normalization

inline double PhaseNorm(double Phase)
{ if(Phase>=M_PI) return Phase-2*M_PI;
  if(Phase<(-M_PI)) return Phase+2*M_PI;
  return Phase; }

// ----------------------------------------------------------------------------
// min./max. of integers

inline int imin(int i1, int i2)
{ return i1<i2 ? i1 : i2; }

inline int imax(int i1, int i2)
{ return i1>i2 ? i1 : i2; }

inline int imin(int i1, int i2, int i3)
{ return i1<i2 ? (i1<i3 ? i1 : i3) : (i2<i3 ? i2 : i3); }

inline int imax(int i1, int i2, int i3)
{ return i1>i2 ? (i1>i3 ? i1 : i3) : (i2>i3 ? i2 : i3); }

// ----------------------------------------------------------------------------
// Extreme search, average, fitting

double Average(float *Data, int Len);
double Average(double *Data, int Len);

int CountInRange(float *Data, int Len, float Low, float Upp);
int CountInRange(double *Data, int Len, double Low, double Upp);

inline int CountInRange(float_buff *Input, float Low, float Upp)
{ return CountInRange(Input->Data,Input->Len,Low,Upp); }

inline double RMS(float *Data, int Len) { return sqrt(Power(Data,Len)/Len); }
inline double RMS(double *Data, int Len) { return sqrt(Power(Data,Len)/Len); }
inline double RMS(fcmpx *Data, int Len) { return sqrt(Power(Data,Len)/Len); }
inline double RMS(dcmpx *Data, int Len) { return sqrt(Power(Data,Len)/Len); }
inline double RMS(float_buff *Input) { return RMS(Input->Data,Input->Len); }
inline double RMS(fcmpx_buff *Input) { return RMS(Input->Data,Input->Len); }
inline double RMS(dcmpx_buff *Input) { return RMS(Input->Data,Input->Len); }

template <class type> type FindMin(type *Data, int Len)
{ type Min; int i;
  Min=Data[0];
  for(i=1; i<Len; i++)
    if(Data[i]<Min) Min=Data[i];
  return Min; }

template <class type> type FindMin(type *Data, int Len, int &MinPos)
{ type Min; int i,pos;
  Min=Data[0]; pos=0;
  for(i=1; i<Len; i++)
    if(Data[i]<Min) { Min=Data[i]; pos=i; }
  MinPos=pos; return Min; }

template <class type> type FindMax(type *Data, int Len)
{ type Max; int i;
  Max=Data[0];
  for(i=1; i<Len; i++)
    if(Data[i]>Max) Max=Data[i];
  return Max; }

template <class type> type FindMax(type *Data, int Len, int &MaxPos)
{ type Max; int i,pos;
  Max=Data[0]; pos=0;
  for(i=1; i<Len; i++)
    if(Data[i]>Max) { Max=Data[i]; pos=i; }
  MaxPos=pos; return Max; }

double FindMaxPower(fcmpx *Data, int Len);
double FindMaxPower(fcmpx *Data, int Len, int &MaxPos);

double FitPoly1(float *Data, int Len, double &A, double &B);
double FitPoly1(double *Data, int Len, double &A, double &B);
double FitPoly2(float *Data, int Len, double &A, double &B, double &C);
double FitPoly2(double *Data, int Len, double &A, double &B, double &C);

void FitPoly2(float Data[3], double &A, double &B, double &C);
void FitPoly2(double Data[3], double &A, double &B, double &C);

// ----------------------------------------------------------------------------
// "selective" average fit

template <class type>
 int SelFitAver(type *Data, int Len, float SelThres, int Loops,
		double &Aver, double &RMS, int &Sel)
{ int i,loop,Incl,prev; double Sum,ErrSum,Lev,dLev,Diff,Thres;
  for(ErrSum=Sum=0.0,i=0; i<Len; i++)
  { Sum+=Data[i]; ErrSum+=Power(Data[i]); }
  Lev=Sum/Len; ErrSum/=Len; ErrSum-=Lev*Lev;
  // printf("Len=%d, Lev=%+7.4f, ErrSum=%7.4f, RMS=%7.4f\n",
  //	 Len,Lev,ErrSum,sqrt(ErrSum));
  for(Incl=0,prev=Len,loop=0; loop<Loops; loop++)
  { Thres=SelThres*SelThres*ErrSum;
    for(ErrSum=Sum=0.0,Incl=0,i=0; i<Len; i++)
    { Diff=Power(Data[i]-Lev);
      if(Diff<=Thres) { Sum+=Data[i]; ErrSum+=Diff; Incl+=1; }
      // else printf(" %d",i);
    } Sum/=Incl; dLev=Sum-Lev; ErrSum/=Incl;
      ErrSum-=dLev*dLev; Lev+=dLev; ErrSum=fabs(ErrSum);
      // printf("\nLoop #%d, Lev=%+7.4f, dLev=%+7.4f, ErrSum=%7.4f, RMS=%7.4f, Incl=%d\n",
      //	     loop,Lev,dLev,ErrSum,sqrt(ErrSum),Incl);
    // if(Incl==prev) { loop++; break; }
    // prev=Incl;
  } Aver=Lev; RMS=sqrt(ErrSum); Sel=Incl;
  return loop;
}

template <class type>
 int SelFitAver(Cmpx<type> *Data, int Len, float SelThres, int Loops,
		Cmpx<double> &Aver, double &RMS, int &Sel)
{ int i,loop,Incl,prev; dcmpx Sum,Lev,dLev; double ErrSum,Diff,Thres;
  for(ErrSum=0.0,Sum.re=Sum.im=0.0,i=0; i<Len; i++)
  { Sum.re+=Data[i].re; Sum.im+=Data[i].im; ErrSum+=Power(Data[i]); }
  Lev.re=Sum.re/Len; Lev.im=Sum.im/Len; ErrSum/=Len; ErrSum-=Power(Lev);
  // printf("Len=%d, Lev=[%+7.4f,%+7.4f], ErrSum=%7.4f, RMS=%7.4f\n",
  //	 Len,Lev.re,Lev.im,ErrSum,sqrt(ErrSum));
  for(Incl=0,prev=Len,loop=0; loop<Loops; loop++)
  { Thres=0.5*SelThres*SelThres*ErrSum;
    for(ErrSum=0.0,Sum.re=Sum.im=0.0,Incl=0,i=0; i<Len; i++)
    { Diff=Power(Data[i].re-Lev.re,Data[i].im-Lev.im);
      if(Diff<=Thres) { Sum.re+=Data[i].re; Sum.im+=Data[i].im; ErrSum+=Diff; Incl+=1; }
      // else printf(" %d",i);
    } Sum.re/=Incl; Sum.im/=Incl;
      dLev.re=Sum.re-Lev.re; dLev.im=Sum.im-Lev.im;
      ErrSum/=Incl; ErrSum-=Power(dLev); ErrSum=fabs(ErrSum);
      Lev.re+=dLev.re; Lev.im+=dLev.im;
      // printf("\nLoop #%d, Lev=[%+6.3f,%+6.3f], dLev=[%+6.3f,%+6.3f], ErrSum=%7.4f, RMS=%7.4f, Incl=%d\n",
      //	     loop, Lev.re,Lev.im, dLev.re,dLev.im, ErrSum,sqrt(ErrSum), Incl);
    // if(Incl==prev) { loop++; break; }
    // prev=Incl;
  } Aver=Lev; RMS=sqrt(ErrSum); Sel=Incl;
  return loop;
}

// ----------------------------------------------------------------------------
// white noise generator

template <class type>
 void WhiteNoise(type &X)
{ double Rand,Power,Phase;
  Rand=((double)rand()+1.0)/((double)RAND_MAX+1.0); Power=sqrt(-2*log(Rand));
  Rand=(double)rand()/(double)RAND_MAX;             Phase=2*M_PI*Rand;
  X=Power*cos(Phase); }

template <class type>
 void CmpxWhiteNoise(Cmpx<type> &X)
{ double Rand,Power,Phase;
  Rand=((double)rand()+1.0)/((double)RAND_MAX+1.0); Power=sqrt(-log(Rand));
  Rand=(double)rand()/(double)RAND_MAX;             Phase=2*M_PI*Rand;
  X.re=Power*cos(Phase); X.im=Power*sin(Phase); }

/*
void WhiteNoise(float *Data, double Rms);
void WhiteNoise(double *Data, double Rms);
void WhiteNoise(Cmpx<float> *Data, double Rms);
void WhiteNoise(Cmpx<double> *Data, double Rms);
void WhiteNoise(float *Data, int Len, double Rms);
void WhiteNoise(double *Data, int Len, double Rms);
void WhiteNoise(Cmpx<float> *Data, int Len, double Rms);
void WhiteNoise(Cmpx<double> *Data, int Len, double Rms);
*/
// ----------------------------------------------------------------------------
// various window shapes (for the FFT and FIR filters)
// these functions are supposed to be called with the argument "phase"
// between -PI and +PI. Most (or even all) will return zero for input
// euqal -PI or +PI.

double WindowHamming(double phase);
double WindowHanning(double phase);
double WindowBlackman2(double phase);
double WindowBlackman3(double phase);

// ----------------------------------------------------------------------------
// FIR shape calculation for a flat response from FreqLow to FreqUpp

void WinFirI(float LowOmega, float UppOmega,
       float *Shape, int Len, double (*Window)(double), float shift=0.0);
void WinFirQ(float LowOmega, float UppOmega,
       float *Shape, int Len, double (*Window)(double), float shift=0.0);

// ----------------------------------------------------------------------------
// convert 16-bit signed or 8-bit unsigned into floats

void ConvS16toFloat(s16 *S16, float *Float, int Len, float Gain=1.0/32768.0);
int  ConvS16toFloat(s16 *S16, float_buff *Float, int Len, float Gain=1.0/32768.0);

void ConvFloatToS16(float *Float, s16 *S16, int Len, float Gain=32768.0);
inline int  ConvFloatToS16(float_buff *Float, s16_buff *S16, float Gain=32768.0)
{ int err=S16->EnsureSpace(Float->Len); if(err) return -1;
  ConvFloatToS16(Float->Data,S16->Data,Float->Len,Gain);
  S16->Len=Float->Len; return 0; }

void ConvU8toFloat(unsigned char *U8, float *Float, int Len, float Gain=1.0/128.0);
int  ConvU8toFloat(unsigned char *U8, float_buff *Float, int Len, float Gain=1.0/128.0);

// ----------------------------------------------------------------------------
// other converts

void ConvCmpxToPower(fcmpx *Inp, int InpLen, float *Out);
int ConvCmpxToPower(fcmpx_buff *Input, float_buff *Output);

void ConvCmpxToAmpl(fcmpx *Inp, int InpLen, float *Out);
int ConvCmpxToAmpl(fcmpx_buff *Input, float_buff *Output);

void ConvCmpxToPhase(fcmpx *Inp, int InpLen, float *Out);
int ConvCmpxToPhase(fcmpx_buff *Input, float_buff *Output);

// ----------------------------------------------------------------------------
// Pulse noise limiter

class PulseLimiter
{ public:
   PulseLimiter(); ~PulseLimiter();
   void Free(void);
   int Preset(int TapLen, float Limit=4.0);
   int Process(float *Inp, int InpLen, float *Out);
   int Process(float_buff *Input);
   float_buff Output;
   float RMS;
  private:
   int Len;
   float Thres;
   float *Tap;
   int Ptr;
   double PwrSum;
} ;

// ----------------------------------------------------------------------------
// Signal level monitor

class LevelMonitor
{ public:
   LevelMonitor(); ~LevelMonitor();
   int Preset(float Integ, float Range=0.75);
   int Process(float *Inp, int Len);
   int Process(float_buff *Input);
   double RMS;
   double OutOfRange;
  private:
   double PwrMid,PwrOut;
   double OutOfRangeMid;
   float MaxSqr;
   float W1,W2,W5;
} ;

// ----------------------------------------------------------------------------
// Automatic Gain/Level Control for the Mixer

class MixerAutoLevel
{ public:
   MixerAutoLevel(); // ~MixerAutoLevel();
   int Process(float *Inp, int InpLen);
   int Process(float_buff *Inp) { return Process(Inp->Data, Inp->Len); }
  public:
   int IntegLen; // mean power integration time [samples]
   float MinMS;  // minimum acceptable average power
   float MaxMS;  // maximum acceptable average power
   int PeakHold; // level holding time after a peak [samples]
   int MinHold;  // minimal time between changing the mixer level [samples]
   int AdjStep;  // mixer level adjusting step
   int MinLevel; // mimimum allowed mixer level
   int MaxLevel; // maximum allowed mixer level
   double AverMS; // average power of the input signal
   int Hold;     // time counter for holding levels
   int Level;    // actual mixer level
} ;

// ----------------------------------------------------------------------------
// Two-element IIR low pass filter

struct LowPass2elem   { double Mid,Out; } ;

struct LowPass2weight { double W1,W2,W5; } ;

// first calculate the coefficiants W1,W2 and W5 for given integration time
template <class typeLen, class typeW>
 inline void LowPass2Coeff(typeLen IntegLen, typeW &W1, typeW &W2, typeW &W5)
{ W1=1.0/IntegLen; W2=2.0/IntegLen; W5=5.0/IntegLen; }

template <class typeLen>
 inline void LowPass2Coeff(typeLen IntegLen, LowPass2weight &Weight)
{ Weight.W1=1.0/IntegLen; Weight.W2=2.0/IntegLen; Weight.W5=5.0/IntegLen; }

// then you can process samples
template <class typeInp, class typeOut, class typeW>
 inline void LowPass2(typeInp Inp, typeOut &Mid, typeOut &Out,
		typeW W1, typeW W2, typeW W5)
{ double Sum, Diff;
  Sum=Mid+Out; Diff=Mid-Out; Mid+=W2*Inp-W1*Sum; Out+=W5*Diff; }

template <class typeInp, class typeW>
 inline void LowPass2(typeInp Inp, LowPass2elem &Elem,
		typeW W1, typeW W2, typeW W5)
{ double Sum, Diff;
  Sum=Elem.Mid+Elem.Out; Diff=Elem.Mid-Elem.Out; Elem.Mid+=W2*Inp-W1*Sum; Elem.Out+=W5*Diff; }

template <class typeInp>
 inline void LowPass2(typeInp Inp, LowPass2elem &Elem, LowPass2weight &Weight)
{ double Sum, Diff;
  Sum=Elem.Mid+Elem.Out;
  Diff=Elem.Mid-Elem.Out;
  Elem.Mid+=Weight.W2*Inp-Weight.W1*Sum;
  Elem.Out+=Weight.W5*Diff; }

/*
inline void LowPass2(float Inp, double &Mid, double &Out,
		float W1, float W2, float W5)
{ double Sum, Diff;
  Sum=Mid+Out; Diff=Mid-Out; Mid+=W2*Inp-W1*Sum; Out+=W5*Diff; }
*/
/*
inline void LowPass2(double Inp, double *Mid, double *Out,
		float W1, float W2, float W5)
{ double Sum, Diff;
  Sum=(*Mid)+(*Out); Diff=(*Mid)-(*Out);
  (*Mid)+=W2*Inp-W1*Sum; (*Out)+=W5*Diff; }

inline void LowPass2(float Inp, double *Mid, double *Out,
		float W1, float W2, float W5)
{ double Sum, Diff;
  Sum=(*Mid)+(*Out); Diff=(*Mid)-(*Out);
  (*Mid)+=W2*Inp-W1*Sum; (*Out)+=W5*Diff; }
*/
void LowPass2(dcmpx *Inp, dcmpx *Mid, dcmpx *Out,
		float W1, float W2, float W5);

void LowPass2(fcmpx *Inp, dcmpx *Mid, dcmpx *Out,
		float W1, float W2, float W5);

// ----------------------------------------------------------------------------
// periodic low pass

class PeriodLowPass2
{ public:
   PeriodLowPass2();
   ~PeriodLowPass2();
   void Free(void);
   int Preset(int Period, float IntegLen);
   int Process(float Inp, float &Out);
   int Process(float *Inp, int InpLen, float *Out);
   int Process(float_buff *Input);
   float_buff Output;
  private:
   int Len; double *TapMid,*TapOut; int TapPtr;
   float W1,W2,W5;
} ;

// ----------------------------------------------------------------------------
// a simple delay

template <class type>
 class Delay
{ public:
   Delay(); ~Delay();
   void Free(void); int Preset(int len);
   void Process(type *Inp, int InpLen, type *Out);
   int Process(Seq<type> *Input);
   Seq<type> Output;
  private:
   int Len; type *Tap; int TapPtr;
} ;

template <class type>
 Delay<type>::Delay() { Tap=NULL; }

template <class type>
 Delay<type>::~Delay() { free(Tap); }

template <class type>
 void Delay<type>::Free(void) { free(Tap); Tap=NULL; }

template <class type>
 int Delay<type>::Preset(int DelayLen)
{ Len=DelayLen; if(ReallocArray(&Tap,Len)) return -1;
  ClearArray(Tap,Len); TapPtr=0; return 0; }

template <class type>
 void Delay<type>::Process(type *Inp, int InpLen, type *Out)
{ int i,batch;
  for(i=0; i<InpLen; )
  { for(batch=imin(InpLen-i,Len-TapPtr), i+=batch; batch; batch--)
    { (*Out++)=Tap[TapPtr]; Tap[TapPtr++]=(*Inp++); }
    if(TapPtr>=Len) TapPtr=0; }
}

template <class type>
 int Delay<type>::Process(Seq<type> *Input)
{ int err=Output.EnsureSpace(Input->Len); if(err) return -1;
  Process(Input->Data,Input->Len,Output.Data);
  Output.Len=Input->Len; return 0; }

// ----------------------------------------------------------------------------
// DelayLine, like delay but more flexible
// The idea is that we hold addressable history of at least MaxDelay
// samples.
// After each input batch is processed, the InpPtr points to the first sample
// of this batch and we can address samples backwards upto MaxDelay.
// For more optimal performace we allocate more RAM than just for MaxDelay.
// Infact the allocated size (MaxSize) should be at least
// MaxDelay plus the largest expected input length.

template <class type>
 class DelayLine
{ public:
   DelayLine(); ~DelayLine();
   void Free(void);
   int Preset(int MaxDelay, int MaxSize=0);
   int Process(type *Inp, int Len);
   int Process(Seq<type> *Input);
   type *Line; // line storage
   int Delay;	// how many (at least) backward samples are stored
   int LineSize; // allocated size
   int DataLen; // length of the valid data
   type *InpPtr; // first sample for the most recent processed batch
   int InpLen;	 // number of samples for the most recent input
} ;

template <class type>
 DelayLine<type>::DelayLine() { Line=NULL; }

template <class type>
 DelayLine<type>::~DelayLine() { free(Line); }

template <class type>
 void DelayLine<type>::Free(void) { free(Line); Line=NULL; }

template <class type>
 int DelayLine<type>::Preset(int MaxDelay, int MaxSize)
{ LineSize=MaxSize; if(LineSize<(2*MaxDelay)) LineSize=2*MaxDelay;
  DataLen=MaxDelay; Delay=MaxDelay;
  if(ReallocArray(&Line,LineSize)) return -1;
  ClearArray(Line,LineSize);
  InpPtr=Line+DataLen; InpLen=0; return 0; }

template <class type>
 int DelayLine<type>::Process(type *Inp, int Len)
{ if((DataLen+Len)>LineSize)
  { MoveArray(Line,Line+DataLen-Delay,Delay); DataLen=Delay; }
  if((DataLen+Len)>LineSize) return -1;
  CopyArray(Line+DataLen,Inp,Len);
  InpPtr=Line+DataLen; InpLen=Len; DataLen+=Len;
  return 0; }

template <class type>
 int DelayLine<type>::Process(Seq<type> *Input)
{ return Process(Input->Data,Input->Len); }

// ----------------------------------------------------------------------------
// Low pass "moving box" FIR filter
// very unpure spectral response but CPU complexity very low
// and independent on the integration time

class BoxFilter
{ public:
   BoxFilter(); ~BoxFilter();
   void Free(void);
   int Preset(int BoxLen);
   int Process(float Inp, float &Out);
   int Process(float *Inp, int InpLen, float *Out);
   int Process(float_buff *Input);
   void Recalibrate();
   float_buff Output;
  private:
   int Len; float *Tap; int TapPtr; double Sum;
} ;

class CmpxBoxFilter
{ public:
   CmpxBoxFilter(); ~CmpxBoxFilter();
   void Free(void);
   int Preset(int BoxLen);
   int Process(fcmpx *Inp, int InpLen, fcmpx *Out);
   void Recalibrate();
   int Process(fcmpx_buff *Input);
   fcmpx_buff Output;
  private:
   int Len; fcmpx *Tap; int TapPtr; dcmpx Sum;
} ;

// ----------------------------------------------------------------------------
// FIR filter with a given shape

class FirFilter
{ public:
   FirFilter(); ~FirFilter();
   void Free(void);
   int Preset(int FilterLen, float *FilterShape=(float*)NULL);
   int Process(float *Inp, int InpLen, float *Out);
   int Process(float_buff *Input);
  //   Response(float Freq, float *Resp);
   int ComputeShape(float LowOmega, float UppOmega, double (*Window)(double));
  //   UseExternShape(float *shape);
   float_buff Output;
  private:
   int Len;		// Tap/Shape length
   float *Shape;	// Response shape
   int ExternShape;	// that we are using an externally provided shape
   float *Tap; int TapPtr;
} ;

// ----------------------------------------------------------------------------
// a pair of FIR filters. quadrature split, decimate
// the decimation rate must be integer

class QuadrSplit
{ public:
   QuadrSplit(); ~QuadrSplit();
   void Free(void);
   int Preset(int FilterLen,
	      float *FilterShape_I, float *FilterShape_Q,
	      int DecimateRate);
   int ComputeShape(float LowOmega, float UppOmega, double (*Window)(double));
//   int Process(float *Inp, int InpLen,
//	       float *OutI, float *OutQ, int MaxOutLen, int *OutLen);
//   int Process(float *Inp, int InpLen,
//	       fcmpx *Out, int MaxOutLen, int *OutLen);
   int Process(float_buff *Input);
   fcmpx_buff Output;
  private:
   int Len;
   float_buff Tap;
   float *ShapeI, *ShapeQ; int ExternShape;
   int Rate;
} ;

// ----------------------------------------------------------------------------
// reverse of QuadrSplit: interpolates and combines the I/Q
// back into 'real' signal.

class QuadrComb
{ public:
   QuadrComb(); ~QuadrComb();
   void Free(void);
   int Preset(int FilterLen,
	      float *FilterShape_I, float *FilterShape_Q,
	      int DecimateRate);
   int ComputeShape(float LowOmega, float UppOmega, double (*Window)(double));
   int Process(fcmpx_buff *Input);
   float_buff Output;
  private:
   int Len; double *Tap; int TapPtr;
   float *ShapeI, *ShapeQ; int ExternShape;
   int Rate;
} ;

// ----------------------------------------------------------------------------
// complex mix with an oscilator (carrier)
// here we could avoid computing sine/cos at every sample

class CmpxMixer
{ public:
   CmpxMixer(); // ~CmpxMixer();
   void Free(void);
   int      Preset(double CarrierOmega);
//   int     Process(float *InpI, float *InpQ, int InpLen,
//		   float *OutI, float *OutQ);
   int ProcessFast(float *InpI, float *InpQ, int InpLen,
		   float *OutI, float *OutQ);
   int     Process(fcmpx *Inp, int InpLen, fcmpx *Out);
   int ProcessFast(fcmpx *Inp, int InpLen, fcmpx *Out);
   int     Process(fcmpx_buff *Input);
   int ProcessFast(fcmpx_buff *Input);
   fcmpx_buff Output;
  public:
   double Phase,Omega;
} ;

// ----------------------------------------------------------------------------
// FM demodulator (phase rotation speed-meter)

class FMdemod
{ public:
   FMdemod(); // ~FMdemod();
   int Preset(double CenterOmega);
   int Process(float *InpI, float *InpQ, int InpLen, float *Out);
   int Process(fcmpx *Inp, int InpLen, float *Out);
   int Process(fcmpx_buff *Input);
   float_buff Output;
  private:
   float PrevPhase;
  public:
   float RefOmega;
} ;

// ----------------------------------------------------------------------------
// Rate converter - real input/output, linear interpolation
// expect large error when high frequency components are present
// thus the best place to convert rates is after a low pass filter
// of a demodulator.

class RateConvLin
{ public:
   RateConvLin(); // ~RateConvLin();
   void SetOutVsInp(float OutVsInp);
   void SetInpVsOut(float InpVsOut);
   int Process(float_buff *InpBuff);
   float_buff Output;
  private:
   float OutStep, OutPhase;
   float PrevSample;
} ;

// ----------------------------------------------------------------------------
// Rate converter - real input/output, quadratic interpolation
// similar limits like for RateConv1

class RateConvQuadr
{ public:
   RateConvQuadr(); // ~RateConvQuadr();
   void SetOutVsInp(float OutVsInp);
   void SetInpVsOut(float InpVsOut);
   int Process(float *Inp, int InpLen,
	       float *Out, int MaxOutLen, int *OutLen);
   int Process(float_buff *InpBuff);
   float_buff Output;
  private:
   float OutStep, OutPhase;
   float Tap[4]; int TapPtr;
} ;

// ----------------------------------------------------------------------------
// Rate converter, real input/output,
// bandwidth-limited interpolation, several shifted FIR filters

class RateConvBL
{ public:
   RateConvBL(); ~RateConvBL();
   void Free(void);
   int Preset(int FilterLen, float *FilterShape[], int FilterShapeNum);
   int ComputeShape(float LowOmega, float UppOmega, double (*Window)(double));
   void SetOutVsInp(float OutVsInp);
   void SetInpVsOut(float InpVsOut);
   int Process(float_buff *Input);
   int ProcessLinI(float_buff *Input);
   float_buff Output;
  private:
   float OutStep, OutPhase;
   int Len;
   float *Tap; int TapSize;
   float **Shape; int ShapeNum; int ExternShape;
} ;

// ----------------------------------------------------------------------------
// Sliding window (for FFT input)

class CmpxSlideWindow
{ public:
   CmpxSlideWindow(); ~CmpxSlideWindow();
   void Free(void);
   int Preset(int WindowLen, int SlideDist, float *WindowShape=(float*)NULL);
   int SetWindow(double (*NewWindow)(double phase), double Scale=1.0);
   int Process(fcmpx_buff *Input);
   fcmpx_buff Output;
  private:
   int Len;	 // Window length
   fcmpx *Buff; // storage
   int Dist;	 // distance between slides
   int Ptr;     // data pointer in Buff
   float *Window; // window shape
   int ExternWindow;
} ;

// ----------------------------------------------------------------------------
// Overlapping window (for IFFT output)

class CmpxOverlapWindow
{ public:
   CmpxOverlapWindow(); ~CmpxOverlapWindow();
   void Free(void);
   int Preset(int WindowLen, int SlideDist, float *WindowShape=(float*)NULL);
   int SetWindow(double (*NewWindow)(double phase), double Scale=1.0);
   void Process(fcmpx *Inp, fcmpx *Out);
   int ProcessSilence(int Slides=1);
   int Process(fcmpx_buff *Input);
   int Process(fcmpx *Input);
//   int Process(fcmpx_buff *Input);
   fcmpx_buff Output;
  private:
   int Len;	 // Window length
   fcmpx *Buff; // storage
   int Dist;	 // distance between slides
   float *Window; // window shape
   int ExternWindow;
} ;

// ----------------------------------------------------------------------------
// FFT phase corrector

class FFT_TimeShift
{ public:
   FFT_TimeShift();
   ~FFT_TimeShift();
   void Free(void);
   int Preset(int FFTlen, int Backwards=0);
   int Process(fcmpx *Data, int Time);
  private:
   int Len;	// FFT length
   int LenMask; // length-1 for pointer wrapping
   fcmpx *FreqTable; // sin/cos table
   int Phase;
} ;

// ----------------------------------------------------------------------------
// bit synchronizer, the bit rate is the input rate divided by four

class DiffBitSync4
{ public:
   DiffBitSync4(int IntegBits); ~DiffBitSync4();
   void Free(void);
   int Process(float *Inp, int InpLen,
	 float *BitOut, float *IbitOut,
	 int MaxOutLen, int *OutLen);
   float GetSyncDriftRate();    // get aver. sync. drift
   float GetSyncConfid();
  private:                      // eg. 0.01 means 1 bit drift per 100 bits
   float *InpTap; int InpTapLen, InpTapPtr; // buffer tap, length and pointer
   int IntegLen;                // integrate timing over that many bits
   float W1,W2,W5;              // weights for the two-stage IIR lopass filter
   double DiffInteg0[4], DiffInteg[4]; // signal diff. integrators
   int DiffTapPtr;              // integrator and bit-sampling pointer
   int BitPtr; float SyncPhase; // sync. pointer/phase
   double SyncDrift0,SyncDrift; // low pass filter for the sync. drift rate
   float SyncConfid;
} ;

// ----------------------------------------------------------------------------
// bit slicer, SNR/Tune meter

class BitSlicer
{ public:
   BitSlicer(int IntegBits);
   ~BitSlicer();
   int Process(float *Bits, float *IBits, int InpLen, float *OutBits);
   float GetSigToNoise(); float GetAmplAsym(); float GetTimeAsym();
  private:
   int IntegLen,TapLen; float W1,W2,W5;
   double Sum0[2],   Sum[2];
   double SumSq0[2], SumSq[2];
   double TimeAsym0, TimeAsym;
   double AmplAsym0, AmplAsym;
   double Noise[2]; float OptimThres;
   float *Tap; int TapPtr;
   float PrevBit, PrevIBit;
} ;

// ----------------------------------------------------------------------------
// The decoder for the HDLC frames,
// makes no AX.25 CRC check, only the length in bytes against MinLen and MaxLen
// however it does not pass frames with non-complete bytes.

class HDLCdecoder
{ public:
   HDLCdecoder(int minlen, int maxlen, int diff, int invert,
               int chan, int (*handler)(int, char *, int));
   ~HDLCdecoder();
   int Process(float *Inp, int InpLen);
  public:
   int AllFrameCount;
   int BadFrameCount;
  private:
   int MinLen,MaxLen;
   int RxDiff,RxInvert;
   int ChanId;
   int (*FrameHandler)(int ChanId, char *Frame, int Len);
   char *Buff; int Len,PrevLev;
   unsigned int ShiftReg; int BitCount,Count1s;
} ;

// ----------------------------------------------------------------------------
// AX.25 CRC

short unsigned int AX25CRC(char *Data, int Len);

// ----------------------------------------------------------------------------
// check if the given number (an integer) is a power of 2
template <class type> int PowerOf2(type I)
{ int c; if(I<=0) return 0;
  for(c=0; I!=0; I>>=1) c+=I&1;
  return c==1; }

// ----------------------------------------------------------------------------

class r2FFT // radix-2 FFT
{ public:   // size must a power of 2: 2,4,8,16,32,64,128,256,...
   r2FFT();
   ~r2FFT();
   void Free(void);
  // preset tables for given processing size
   int Preset(int size);
  // scramble/unscramble input
   void Scramble(fcmpx x[]);
   void Scramble(dcmpx x[]);
  // apply input window
//   void InpWinAndScr(fcmpx Inp[], fcmpx Out[]);
//   void InpWinAndScr(dcmpx Inp[], dcmpx Out[]);
//   void InpWinAndScr(float Inp0[], float Inp1[], fcmpx Out[]);
//   void InpWinAndScr(double Inp0[], double Inp1[], dcmpx Out[]);
  // apply output window (with overlap ?)
//   void OutWin(fcmpx Buff[], float Out0[], float Out1[]);
//   void OutWin(dcmpx Buff[], double Out0[], double Out1[]);
//   void OutWin(fcmpx Buff[], fcmpx Out0[]);
//   void OutWin(dcmpx Buff[], dcmpx Out0[]);
  // separate the result of a two real channels FFT
   void SeparTwoReals(fcmpx Buff[], fcmpx Out0[], fcmpx Out1[]);
   void SeparTwoReals(dcmpx Buff[], dcmpx Out0[], dcmpx Out1[]);
  // join spectra of two real channels
   void JoinTwoReals(fcmpx Inp0[], fcmpx Inp1[], fcmpx Buff[]);
   void JoinTwoReals(dcmpx Inp0[], dcmpx Inp1[], dcmpx Buff[]);
  // core process: the classic tripple loop of butterflies
   void CoreProc(fcmpx x[]);
   void CoreProc(dcmpx x[]);
  // complex FFT process in place, includes unscrambling
   inline void ProcInPlace(fcmpx x[]) { Scramble(x); CoreProc(x); }
   inline void ProcInPlace(dcmpx x[]) { Scramble(x); CoreProc(x); }
  // define the FFT window and input/output scales (NULL => rectangular window)
//   int SetWindow(double (*NewWindow)(double phase),
//	       double InpScale, double OutScale);
  // forward FFT with window
//   inline void WinForwProc(fcmpx Inp[], fcmpx Out[])
//				{ InpWinAndScr(Inp,Out); CoreProc(Out); }
//   inline void WinForwProc(dcmpx Inp[], dcmpx Out[])
//				{ InpWinAndScr(Inp,Out); CoreProc(Out); }
  // two real channels forward FFT with window
//   inline void RealWinForwProc(float Inp0[], float Inp1[],
//		   fcmpx Buff[], fcmpx Out0[], fcmpx Out1[])
//		       { InpWinAndScr(Inp0,Inp1,Buff); CoreProc(Buff);
//			 SeparTwoReals(Buff,Out0,Out1); }
//   inline void RealWinForwProc(double Inp0[], double Inp1[],
//		   dcmpx Buff[], dcmpx Out0[], dcmpx Out1[])
//		       { InpWinAndScr(Inp0,Inp1,Buff); CoreProc(Buff);
//			 SeparTwoReals(Buff,Out0,Out1); }
  // two real channels inverse FFT with window
//   inline void RealWinInvProc(fcmpx Inp0[], fcmpx Inp1[],
//		   fcmpx Buff[], float Out0[], float Out1[])
//	    { JoinTwoReals(Inp0,Inp1,Buff); Scramble(Buff); CoreProc(Buff);
//	      OutWin(Buff,Out0,Out1); }
//   inline void RealWinInvProc(dcmpx Inp0[], dcmpx Inp1[],
//		   dcmpx Buff[], double Out0[], double Out1[])
//	    { JoinTwoReals(Inp0,Inp1,Buff); Scramble(Buff); CoreProc(Buff);
//	      OutWin(Buff,Out0,Out1); }
  public:
   int Size;	        // FFT size
   int *BitRevIdx;	// Bit-reverse indexing table for data (un)scrambling
   dcmpx *Twiddle;	// Twiddle factors (sine/cos values)
  private:
//   double *Window;	// window shape (NULL => rectangular window
//   double WinInpScale, WinOutScale; // window scales on input/output
  private:
  // classic radix-2 butterflies
   inline void FFTbf(fcmpx &x0, fcmpx &x1, dcmpx &W);
   inline void FFTbf(dcmpx &x0, dcmpx &x1, dcmpx &W);
  // special 2-elem. FFT for the first pass
   inline void FFT2(fcmpx &x0, fcmpx &x1);
   inline void FFT2(dcmpx &x0, dcmpx &x1);
  // special 2-elem. FFT for the second pass
   inline void FFT4(fcmpx &x0, fcmpx &x1, fcmpx &x2, fcmpx &x3);
   inline void FFT4(dcmpx &x0, dcmpx &x1, dcmpx &x2, dcmpx &x3);
} ;

// ---------------------------------------------------------------------------
// Sliding window FFT for spectral analysis (e.g. SETI)
// input: real-valued time-domain signal,
// output: complex-valued Fourier Transform
//
// We use a little trick here to process two real-valued FFT
// in one go using the complex FFT engine.
// This cuts the CPU but makes the input->output delay longer.

class SlideWinFFT
{ public:
   SlideWinFFT(); ~SlideWinFFT();
   void Free();
   int Preset(int size, int step, float *window);
   int Preset(int size, int step,
	      double (*NewWindow)(double phase), float Scale=1.0);
   int SetWindow(float *window);
   int SetWindow(double (*NewWindow)(double phase), float Scale=1.0);
   int Process(float_buff *Input);
   r2FFT FFT;		// FFT engine
   fcmpx_buff Output;	// output buffer
   int Size; int SizeMask; // FFT size, size mask for pointer wrapping
   int Dist; int Left;	// distance between slides, samples left before the next slide
   int Slide;		// even/odd slide
  private:
   float *SlideBuff; int SlidePtr; // sliding window buffer, pointer
   float *Window; int ExternWindow; // window shape
   fcmpx *FFTbuff;		    // FFT buffer
} ;

// ---------------------------------------------------------------------------
// Overlapping window Inverse FFT to convert spectra into time-domain signal

class OvlapWinIFFT
{ public:
   OvlapWinIFFT(); ~OvlapWinIFFT();
   void Free(void);
   int Preset(int size, int step, float *window);
   int Preset(int size, int step,
	      double (*NewWindow)(double phase), float Scale=1.0);
   int SetWindow(float *window);
   int SetWindow(double (*NewWindow)(double phase), float Scale=1.0);
   int Process(fcmpx *Input);
   r2FFT FFT;		// FFT engine
   float_buff Output;	// output buffer
   int Size; int SizeMask; // FFT size, size mask for pointer wrapping
   int Dist;		// distance between slides
   int Slide;
  private:
   fcmpx *Spectr[2];
   fcmpx *FFTbuff;		    // FFT buffer
   float *Window; int ExternWindow; // window shape
   float *OvlapBuff; int OvlapPtr;
} ;

// ---------------------------------------------------------------------------
// Sliding window FFT for spectral processing (e.g. de-noising)
// input: real-valued signal
// in the middle you are given a chance to process
// the complex-valued Fourier Transform (SpectraProc() routine).
// output: real-valued signal
// If you don't touch the spectra in SpectralProc()
// the output will be an exact copy (only delayed) of the input.

class SlideWinFFTproc
{ public:
   SlideWinFFTproc(); ~SlideWinFFTproc();
   void Free(void);
   int Preset(int size, int step, void (*proc)(fcmpx *Spectra, int Len),
	      float *window);
   int Preset(int size, int step, void (*proc)(fcmpx *Spectra, int Len),
	      double (*NewWindow)(double phase), float Scale=0.0);
   int SetWindow(float *window);
   int SetWindow(double (*NewWindow)(double phase), float Scale=0.0);
   int Process(float_buff *Input);
   r2FFT FFT;
   float_buff Output;
   int Size; int SizeMask;
   int Dist; int Left;
   int Slide;
  private:
   float *SlideBuff; int SlidePtr;
   float *Window; int ExternWindow;
   fcmpx *FFTbuff;
   fcmpx *Spectr[2];
   void (*SpectraProc)(fcmpx *Spectra, int Len);
   float *OvlapBuff; int OvlapPtr;
} ;

// ---------------------------------------------------------------------------
// Walsh (Hadamard ?) transform.

void WalshTrans(double *Data, int Len);
void WalshTrans(float *Data, int Len);

void WalshInvTrans(double *Data, int Len);
void WalshInvTrans(float *Data, int Len);

// ---------------------------------------------------------------------------



