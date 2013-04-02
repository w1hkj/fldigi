/*
 *    dsp.h  --  various DSP algorithms
 *
 *    based on mt63 code by Pawel Jalocha
 *    Copyright (C) 1999-2004 Pawel Jalocha, SP9VRC
 *    Copyright (c) 2007-2008 Dave Freese, W1HKJ
 *
 *    This file is part of fldigi.
 *
 *    Fldigi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Fldigi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <cstdlib>
#include <cstring>
#include <cmath>

// ----------------------------------------------------------------------------
// double/other-complex type

template <class type> struct Cdspcmpx { type re,im; } ;

typedef Cdspcmpx<double> dspCmpx;

// Some complex operators
template <class type>
 inline void operator +=(Cdspcmpx<type> &Dst, Cdspcmpx<type> &Src)
{ Dst.re+=Src.re; Dst.im+=Src.im; }

template <class type>
 inline void operator -=(Cdspcmpx<type> &Dst, Cdspcmpx<type> &Src)
{ Dst.re-=Src.re; Dst.im-=Src.im; }

template <class type, class num>
 inline void operator *=(Cdspcmpx<type> &Dst, num Src)
{ Dst.re*=Src; Dst.im*=Src; }

template <class type, class num>
 inline void operator /=(Cdspcmpx<type> &Dst, num Src)
{ Dst.re/=Src; Dst.im/=Src; }

// scalar product of two vectors
template <class typeA, class typeB>
 inline double dspScalProd(Cdspcmpx<typeA> &A, Cdspcmpx<typeB> &B)
{ return A.re*B.re+A.im*B.im; }

template <class typeA, class typeB>
 inline double dspScalProd(typeA Ia, typeA Qa, Cdspcmpx<typeB> &B)
{ return Ia*B.re+Qa*B.im; }

// complex multiply
template <class typeDst, class typeA, class typeB>
 inline void CdspcmpxMultAxB(Cdspcmpx<typeDst> &Dst, Cdspcmpx<typeA> &A, Cdspcmpx<typeB> &B)
{ Dst.re=A.re*B.re-A.im*B.im;
  Dst.im=A.re*B.im+A.im*B.re; }

template <class typeDst, class typeA, class typeB>
 inline void CdspcmpxMultAxB(typeDst &DstI, typeDst &DstQ, Cdspcmpx<typeA> &A, Cdspcmpx<typeB> &B)
{ DstI=A.re*B.re-A.im*B.im;
  DstQ=A.re*B.im+A.im*B.re; }

// complex multiply, second argument with a "star" (B.im is negated)
template <class typeDst, class typeA, class typeB>
 inline void CdspcmpxMultAxBs(Cdspcmpx<typeDst> &Dst, Cdspcmpx<typeA> &A, Cdspcmpx<typeB> &B)
{ Dst.re=A.re*B.re+A.im*B.im;
  Dst.im=A.im*B.re-A.re*B.im; }

// ----------------------------------------------------------------------------
// signed 16-bit format (standard 16-bit audio)

typedef short dspS16;

// ----------------------------------------------------------------------------

template <class type> inline int dspRedspAllocArray(type **Array, int Size)
{ (*Array)=(type *)realloc(*Array,Size*sizeof(type));
  return (*Array)==NULL; }

template <class type> inline int dspAllocArray(type **Array, int Size)
{ (*Array)=(type *)malloc(Size*sizeof(type));
  return (*Array)==NULL; }

template <class type> inline void dspClearArray(type *Array, int Size)
{ memset(Array,0,Size*sizeof(type)); }

template <class type> inline void dspCopyArray(type *Dst, type *Src, int Size)
{ memcpy(Dst,Src,Size*sizeof(type)); }

template <class type> inline void dspMoveArray(type *Dst, type *Src, int Size)
{ memmove(Dst,Src,Size*sizeof(type)); }

template <class type> int dspAllocArray2D(type ***Array, int Size1, int Size2)
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

template <class type> void dspFreeArray2D(type **Array, int Size1)
{ int i; for(i=0; i<Size1; i++) free(Array[i]); free(Array); }

template <class type> void dspClearArray2D(type **Array, int Size1, int Size2)
{ int i; for(i=0; i<Size1; i++) memset(Array[i],0,Size2*sizeof(type)); }

// ----------------------------------------------------------------------------
// processing buffers:

template <class type> class dspSeq
{ public:
	dspSeq();
	~dspSeq();
	int EnsureSpace(int ReqSpace); // make sure that there is enough space
	void Free(void); // free space to save RAM when buffer is not in use
	int Space;   // that much is allocated in *Data
	int Len;	// that much is filled up
	type *Data;  // contains Len elements
} ;

template <class type> dspSeq<type>::dspSeq() { 
	Data = NULL; 
	Len = Space = 0; 
}

template <class type> dspSeq<type>::~dspSeq() { 
	free(Data);
}

template <class type> int dspSeq<type>::EnsureSpace(int ReqSpace)
{ 
	if (ReqSpace <= Space) 
		return 0;
	Data = (type *)realloc(Data, ReqSpace * sizeof(type));
	if (Data == NULL) { 
		Space = Len = 0; 
		return -1; 
	}
	Space = ReqSpace; 
	return 0; 
}

template <class type> void dspSeq<type>::Free(void)
{ 
	free(Data); 
	Data = NULL;
	Space = Len = 0;
}

typedef dspSeq<float> float_buff;
typedef dspSeq<double> double_buff;
typedef dspSeq<dspCmpx> dspCmpx_buff;
typedef dspSeq<dspCmpx> dspCmpx_buff;
// typedef dspSeq<short> int16_buff; <- this doesn't work - why ?!
typedef dspSeq<dspS16>   dspS16_buff;
typedef dspSeq<char>  char_buff;

// ----------------------------------------------------------------------------
// First-In First-Out pipes

template <class type> class dspFIFO
{ public:
   dspFIFO(); ~dspFIFO();
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

template <class type> dspFIFO<type>::dspFIFO() { Buff=NULL; }
template <class type> dspFIFO<type>::~dspFIFO() { free(Buff); }

template <class type> void dspFIFO<type>::Free(void) { free(Buff); Buff=NULL; }

template <class type> int dspFIFO<type>::Preset(int Max)
{ Size=Max+1;
  if(dspRedspAllocArray(&Buff,Size)) return -1;
  Rd=0; Wr=0; return 0; }

template <class type> void dspFIFO<type>::Clear(void) { Rd=Wr; }

template <class type> int dspFIFO<type>::Inp(type Elem)
{ int w=Wr;
  Buff[w]=Elem; w+=1; if(w>=Size) w=0;
  if(w==Rd) return -1;
  Wr=w; return 0; }

template <class type> int dspFIFO<type>::Out(type &Elem)
{ if(Rd==Wr) return -1;
  Elem=Buff[Rd]; Rd+=1; if(Rd>=Size) Rd=0; return 0; }

template <class type> int dspFIFO<type>::OutReady(void)
{ return (Wr>=Rd) ? Wr-Rd : Wr-Rd+Size; }

template <class type> int dspFIFO<type>::InpReady(void)
{ return (Rd>Wr) ? Rd-Wr-1 : Rd-Wr+Size-1; }

typedef dspFIFO<char> char_dspFIFO;

// ----------------------------------------------------------------------------
// dspPower of single and complex values and dspSequences of these

inline double dspPower(double X) { return X*X; }
inline double dspPower(double I, double Q) { return I*I + Q*Q; }
inline double dspPower(dspCmpx X) { return X.re*X.re+X.im*X.im; }

double dspPower(double *X, int Len);
double dspPower(double *I, double *Q, int Len);
double dspPower(dspCmpx *X, int Len);

inline double dspPower(double_buff *buff) { return dspPower(buff->Data,buff->Len); }
inline double dspPower(dspCmpx_buff *buff) { return dspPower(buff->Data,buff->Len); }

// dspAmplitude calculations

inline double dspAmpl(double I, double Q) { return sqrt(I*I+Q*Q); }
inline double dspAmpl(dspCmpx X) { return sqrt(X.re*X.re+X.im*X.im); }

// dspPhase calculation (output = <-PI..PI) )

inline double dspPhase(double I, double Q) { return atan2(Q,I); }
inline double dspPhase(dspCmpx X) { return atan2(X.im,X.re); }

// dspPhase normalization

inline double dspPhaseNorm(double dspPhase)
{ if(dspPhase>=M_PI) return dspPhase-2*M_PI;
  if(dspPhase<(-M_PI)) return dspPhase+2*M_PI;
  return dspPhase; }

// ----------------------------------------------------------------------------
// min./max. of integers

inline int dspIntmin(int i1, int i2)
{ return i1<i2 ? i1 : i2; }

inline int dspIntmax(int i1, int i2)
{ return i1>i2 ? i1 : i2; }

inline int dspIntmin(int i1, int i2, int i3)
{ return i1<i2 ? (i1<i3 ? i1 : i3) : (i2<i3 ? i2 : i3); }

inline int dspIntmax(int i1, int i2, int i3)
{ return i1>i2 ? (i1>i3 ? i1 : i3) : (i2>i3 ? i2 : i3); }

// ----------------------------------------------------------------------------
// Extreme search, dspAverage, fitting

double dspAverage(double *Data, int Len);

int dspCountInRange(double *Data, int Len, double Low, double Upp);

inline int dspCountInRange(double_buff *Input, double Low, double Upp)
{ return dspCountInRange(Input->Data,Input->Len,Low,Upp); }

inline double dspRMS(double *Data, int Len) { return sqrt(dspPower(Data,Len)/Len); }
inline double dspRMS(dspCmpx *Data, int Len) { return sqrt(dspPower(Data,Len)/Len); }
inline double dspRMS(double_buff *Input) { return dspRMS(Input->Data,Input->Len); }
inline double dspRMS(dspCmpx_buff *Input) { return dspRMS(Input->Data,Input->Len); }

template <class type> type dspFindMin(type *Data, int Len)
{ type Min; int i;
  Min=Data[0];
  for(i=1; i<Len; i++)
    if(Data[i]<Min) Min=Data[i];
  return Min; }

template <class type> type dspFindMin(type *Data, int Len, int &MinPos)
{ type Min; int i,pos;
  Min=Data[0]; pos=0;
  for(i=1; i<Len; i++)
    if(Data[i]<Min) { Min=Data[i]; pos=i; }
  MinPos=pos; return Min; }

template <class type> type dspFindMax(type *Data, int Len)
{ type Max; int i;
  Max=Data[0];
  for(i=1; i<Len; i++)
    if(Data[i]>Max) Max=Data[i];
  return Max; }

template <class type> type dspFindMax(type *Data, int Len, int &MaxPos)
{ type Max; int i,pos;
  Max=Data[0]; pos=0;
  for(i=1; i<Len; i++)
    if(Data[i]>Max) { Max=Data[i]; pos=i; }
  MaxPos=pos; return Max; }

double dspFindMaxdspPower(dspCmpx *Data, int Len);
double dspFindMaxdspPower(dspCmpx *Data, int Len, int &MaxPos);

double dspFitPoly1(double *Data, int Len, double &A, double &B);
double dspFitPoly2(double *Data, int Len, double &A, double &B, double &C);

void dspFitPoly2(double Data[3], double &A, double &B, double &C);

// ----------------------------------------------------------------------------
// "selective" dspAverage fit

template <class type>
 int dspSelFitAver(type *Data, int Len, double SelThres, int Loops,
		double &Aver, double &dspRMS, int &Sel)
{
	int i, loop, Incl;//, prev;
	double Sum, ErrSum, Lev, dLev, Diff, Thres;
	for (ErrSum = Sum = 0.0, i = 0; i < Len; i++) {
		Sum += Data[i];
		ErrSum += dspPower(Data[i]);
	}
	Lev = Sum/Len;
	ErrSum /= Len;
	ErrSum -= Lev*Lev;
// printf("Len=%d, Lev=%+7.4f, ErrSum=%7.4f, dspRMS=%7.4f\n",
//	 Len,Lev,ErrSum,sqrt(ErrSum));
//	prev = Len;
	Incl = 0;
	for (loop = 0; loop < Loops; loop++) {
		Thres = SelThres * SelThres * ErrSum;
		for (ErrSum = Sum = 0.0, Incl = 0, i = 0; i < Len; i++) {
			Diff = dspPower(Data[i]-Lev);
			if (Diff <= Thres) {
				Sum += Data[i];
				ErrSum += Diff;
				Incl += 1;
			}
			// else printf(" %d",i);
		}
		Sum /= Incl;
		dLev = Sum - Lev;
		ErrSum /= Incl;
		ErrSum -= dLev * dLev;
		Lev += dLev;
		ErrSum = fabs(ErrSum);
		// printf("\nLoop #%d, Lev=%+7.4f, dLev=%+7.4f, ErrSum=%7.4f, dspRMS=%7.4f, Incl=%d\n",
		//	     loop,Lev,dLev,ErrSum,sqrt(ErrSum),Incl);
		// if(Incl==prev) { loop++; break; }
		// prev=Incl;
	}
	Aver = Lev;
	dspRMS = sqrt(ErrSum);
	Sel=Incl;
	return loop;
}

template <class type>
int dspSelFitAver(Cdspcmpx<type> *Data, int Len, double SelThres, int Loops,
		Cdspcmpx<double> &Aver, double &dspRMS, int &Sel)
{
	int i, loop, Incl;//, prev;
	dspCmpx Sum, Lev, dLev;
	double ErrSum, Diff, Thres;
	for (ErrSum = 0.0, Sum.re = Sum.im = 0.0, i = 0; i < Len; i++) {
		Sum.re += Data[i].re;
		Sum.im += Data[i].im;
		ErrSum += dspPower(Data[i]);
	}
	Lev.re = Sum.re / Len;
	Lev.im = Sum.im / Len;
	ErrSum /= Len;
	ErrSum -= dspPower(Lev);
	// printf("Len=%d, Lev=[%+7.4f,%+7.4f], ErrSum=%7.4f, dspRMS=%7.4f\n",
	//	 Len,Lev.re,Lev.im,ErrSum,sqrt(ErrSum));
	Incl = 0;
//	prev = Len;
	for (loop = 0; loop < Loops; loop++) {
		Thres = 0.5 * SelThres * SelThres * ErrSum;
		for (ErrSum = 0.0, Sum.re = Sum.im = 0.0, Incl = 0, i = 0; i < Len; i++) {
			Diff = dspPower(Data[i].re - Lev.re, Data[i].im - Lev.im);
			if (Diff <= Thres) {
				Sum.re += Data[i].re;
				Sum.im += Data[i].im;
				ErrSum += Diff;
				Incl += 1;
			}
			// else printf(" %d",i);
		}
		Sum.re /= Incl;
		Sum.im /= Incl;
		dLev.re = Sum.re - Lev.re;
		dLev.im = Sum.im - Lev.im;
		ErrSum /= Incl;
		ErrSum -= dspPower(dLev);
		ErrSum = fabs(ErrSum);
		Lev.re += dLev.re;
		Lev.im += dLev.im;
		// printf("\nLoop #%d, Lev=[%+6.3f,%+6.3f], dLev=[%+6.3f,%+6.3f], ErrSum=%7.4f, dspRMS=%7.4f, Incl=%d\n",
		//	     loop, Lev.re,Lev.im, dLev.re,dLev.im, ErrSum,sqrt(ErrSum), Incl);
		// if(Incl==prev) { loop++; break; }
		// prev=Incl;
	}
	Aver = Lev;
	dspRMS = sqrt(ErrSum);
	Sel = Incl;
	return loop;
}

// ----------------------------------------------------------------------------
// white noise generator

template <class type>
 void dspWhiteNoise(type &X)
{ double Rand,dspPower,dspPhase;
  Rand=((double)rand()+1.0)/((double)RAND_MAX+1.0); dspPower=sqrt(-2*log(Rand));
  Rand=(double)rand()/(double)RAND_MAX;             dspPhase=2*M_PI*Rand;
  X=dspPower*cos(dspPhase); }

template <class type>
 void CdspcmpxdspWhiteNoise(Cdspcmpx<type> &X)
{ double Rand,dspPower,dspPhase;
  Rand=((double)rand()+1.0)/((double)RAND_MAX+1.0); dspPower=sqrt(-log(Rand));
  Rand=(double)rand()/(double)RAND_MAX;             dspPhase=2*M_PI*Rand;
  X.re=dspPower*cos(dspPhase); X.im=dspPower*sin(dspPhase); }

// ----------------------------------------------------------------------------
// various window shapes (for the FFT and FIR filters)
// these functions are supposed to be called with the argument "dspPhase"
// between -PI and +PI. Most (or even all) will return zero for input
// euqal -PI or +PI.

double dspWindowHanning(double dspPhase);
double WindowBlackman2(double dspPhase); // from Freq 5.1 FFT analyzer
double dspWindowBlackman3(double dspPhase); // from the Motorola BBS

// ----------------------------------------------------------------------------
// FIR shape calculation for a flat response from FreqLow to FreqUpp

void dspWinFirI(double LowOmega, double UppOmega,
       double *Shape, int Len, double (*Window)(double), double shift=0.0);
void WinFirQ(double LowOmega, double UppOmega,
       double *Shape, int Len, double (*Window)(double), double shift=0.0);

// ----------------------------------------------------------------------------
// convert 16-bit signed or 8-bit unsigned into doubles

void dspConvS16todouble(dspS16 *dspS16, double *dbl, int Len, double Gain=1.0/32768.0);
int  dspConvS16todouble(dspS16 *dspS16, double_buff *dbl, int Len, double Gain=1.0/32768.0);

void dspConvdoubleTodspS16(double *dbl, dspS16 *dspS16, int Len, double Gain=32768.0);
inline int  dspConvdoubleTodspS16(double_buff *dbl, dspS16_buff *dspS16, double Gain=32768.0)
{ int err=dspS16->EnsureSpace(dbl->Len); if(err) return -1;
  dspConvdoubleTodspS16(dbl->Data,dspS16->Data, dbl->Len,Gain);
  dspS16->Len=dbl->Len; return 0; }

void dspConvU8todouble(unsigned char *U8, double *dbl, int Len, double Gain=1.0/128.0);
int  dspConvU8todouble(unsigned char *U8, double_buff *dbl, int Len, double Gain=1.0/128.0);

// ----------------------------------------------------------------------------
// other converts

void dspConvCmpxTodspPower(dspCmpx *Inp, int InpLen, double *Out);
int dspConvCmpxTodspPower(dspCmpx_buff *Input, double_buff *Output);

void dspConvCmpxTodspAmpl(dspCmpx *Inp, int InpLen, double *Out);
int dspConvCmpxTodspAmpl(dspCmpx_buff *Input, double_buff *Output);

void dspConvCmpxTodspPhase(dspCmpx *Inp, int InpLen, double *Out);
int dspConvCmpxTodspPhase(dspCmpx_buff *Input, double_buff *Output);

// ----------------------------------------------------------------------------
// Pulse noise limiter

class dspPulseLimiter
{ public:
   dspPulseLimiter(); ~dspPulseLimiter();
   void Free(void);
   int Preset(int TapLen, double Limit=4.0);
   int Process(double *Inp, int InpLen, double *Out);
   int Process(double_buff *Input);
   double_buff Output;
   double dspRMS;
  private:
   int Len;
   double Thres;
   double *Tap;
   int Ptr;
   double PwrSum;
} ;

// ----------------------------------------------------------------------------
// Signal level monitor

class dspLevelMonitor
{ public:
   dspLevelMonitor(); ~dspLevelMonitor();
   int Preset(double Integ, double Range=0.75);
   int Process(double *Inp, int Len);
   int Process(double_buff *Input);
   double dspRMS;
   double OutOfRange;
  private:
   double PwrMid,PwrOut;
   double OutOfRangeMid;
   double MaxSqr;
   double W1,W2,W5;
} ;

// ----------------------------------------------------------------------------
// Automatic Gain/Level Control for the Mixer

class dspMixerAutoLevel
{ public:
   dspMixerAutoLevel(); // ~dspMixerAutoLevel();
   int Process(double *Inp, int InpLen);
   int Process(double_buff *Inp) { return Process(Inp->Data, Inp->Len); }
  public:
   int IntegLen; // mean dspPower integration time [samples]
   double MinMS;  // minimum acceptable dspAverage dspPower
   double MaxMS;  // maximum acceptable dspAverage dspPower
   int PeakHold; // level holding time after a peak [samples]
   int MinHold;  // minimal time between changing the mixer level [samples]
   int AdjStep;  // mixer level adjusting step
   int MinLevel; // mimimum allowed mixer level
   int MaxLevel; // maximum allowed mixer level
   double AvedspRMS; // dspAverage dspPower of the input signal
   int Hold;     // time counter for holding levels
   int Level;    // actual mixer level
} ;

// ----------------------------------------------------------------------------
// Two-element IIR low pass filter

struct dspLowPass2elem   { double Mid,Out; } ;

struct dspLowPass2weight { double W1,W2,W5; } ;

// first calculate the coefficiants W1,W2 and W5 for given integration time
template <class typeLen, class typeW>
 inline void dspLowPass2Coeff(typeLen IntegLen, typeW &W1, typeW &W2, typeW &W5)
{ W1=1.0/IntegLen; W2=2.0/IntegLen; W5=5.0/IntegLen; }

template <class typeLen>
 inline void dspLowPass2Coeff(typeLen IntegLen, dspLowPass2weight &Weight)
{ Weight.W1=1.0/IntegLen; Weight.W2=2.0/IntegLen; Weight.W5=5.0/IntegLen; }

// then you can process samples
template <class typeInp, class typeOut, class typeW>
 inline void dspLowPass2(typeInp Inp, typeOut &Mid, typeOut &Out,
		typeW W1, typeW W2, typeW W5)
{ double Sum, Diff;
  Sum=Mid+Out; Diff=Mid-Out; Mid+=W2*Inp-W1*Sum; Out+=W5*Diff; }

template <class typeInp, class typeW>
 inline void dspLowPass2(typeInp Inp, dspLowPass2elem &Elem,
		typeW W1, typeW W2, typeW W5)
{ double Sum, Diff;
  Sum=Elem.Mid+Elem.Out; Diff=Elem.Mid-Elem.Out; Elem.Mid+=W2*Inp-W1*Sum; Elem.Out+=W5*Diff; }

template <class typeInp>
 inline void dspLowPass2(typeInp Inp, dspLowPass2elem &Elem, dspLowPass2weight &Weight)
{ double Sum, Diff;
  Sum=Elem.Mid+Elem.Out;
  Diff=Elem.Mid-Elem.Out;
  Elem.Mid+=Weight.W2*Inp-Weight.W1*Sum;
  Elem.Out+=Weight.W5*Diff; }

void dspLowPass2(dspCmpx *Inp, dspCmpx *Mid, dspCmpx *Out,
		double W1, double W2, double W5);

// ----------------------------------------------------------------------------
// periodic low pass

class dspPeriodLowPass2
{ public:
   dspPeriodLowPass2();
   ~dspPeriodLowPass2();
   void Free(void);
   int Preset(int Period, double IntegLen);
   int Process(double Inp, double &Out);
   int Process(double *Inp, int InpLen, double *Out);
   int Process(double_buff *Input);
   double_buff Output;
  private:
   int Len; double *TapMid,*TapOut; int TapPtr;
   double W1,W2,W5;
} ;

// ----------------------------------------------------------------------------
// a simple dspDelay

template <class type>
 class dspDelay
{ public:
   dspDelay(); ~dspDelay();
   void Free(void); int Preset(int len);
   void Process(type *Inp, int InpLen, type *Out);
   int Process(dspSeq<type> *Input);
   dspSeq<type> Output;
  private:
   int Len; type *Tap; int TapPtr;
} ;

template <class type>
 dspDelay<type>::dspDelay() { Tap=NULL; }

template <class type>
 dspDelay<type>::~dspDelay() { free(Tap); }

template <class type>
 void dspDelay<type>::Free(void) { free(Tap); Tap=NULL; }

template <class type>
 int dspDelay<type>::Preset(int dspDelayLen)
{ Len=dspDelayLen; if(dspRedspAllocArray(&Tap,Len)) return -1;
  dspClearArray(Tap,Len); TapPtr=0; return 0; }

template <class type>
 void dspDelay<type>::Process(type *Inp, int InpLen, type *Out)
{ int i,batch;
  for(i=0; i<InpLen; )
  { for(batch=dspIntmin(InpLen-i,Len-TapPtr), i+=batch; batch; batch--)
    { (*Out++)=Tap[TapPtr]; Tap[TapPtr++]=(*Inp++); }
    if(TapPtr>=Len) TapPtr=0; }
}

template <class type>
 int dspDelay<type>::Process(dspSeq<type> *Input)
{ int err=Output.EnsureSpace(Input->Len); if(err) return -1;
  Process(Input->Data,Input->Len,Output.Data);
  Output.Len=Input->Len; return 0; }

// ----------------------------------------------------------------------------
// dspDelayLine, like dspDelay but more flexible
// The idea is that we hold addressable history of at least MaxdspDelay
// samples.
// After each input batch is processed, the InpPtr points to the first sample
// of this batch and we can address samples backwards upto MaxdspDelay.
// For more optimal performace we allocate more RAM than just for MaxdspDelay.
// Infact the allocated size (MaxSize) should be at least
// MaxdspDelay plus the largest expected input length.

template <class type>
 class dspDelayLine
{ public:
   dspDelayLine(); ~dspDelayLine();
   void Free(void);
   int Preset(int MaxdspDelay, int MaxSize=0);
   int Process(type *Inp, int Len);
   int Process(dspSeq<type> *Input);
   type *Line; // line storage
   int dspDelay;	// how many (at least) backward samples are stored
   int LineSize; // allocated size
   int DataLen; // length of the valid data
   type *InpPtr; // first sample for the most recent processed batch
   int InpLen;	 // number of samples for the most recent input
} ;

template <class type>
 dspDelayLine<type>::dspDelayLine() { Line=NULL; }

template <class type>
 dspDelayLine<type>::~dspDelayLine() { free(Line); }

template <class type>
 void dspDelayLine<type>::Free(void) { free(Line); Line=NULL; }

template <class type>
 int dspDelayLine<type>::Preset(int MaxdspDelay, int MaxSize)
{ LineSize=MaxSize; if(LineSize<(2*MaxdspDelay)) LineSize=2*MaxdspDelay;
  DataLen=MaxdspDelay; dspDelay=MaxdspDelay;
  if(dspRedspAllocArray(&Line,LineSize)) return -1;
  dspClearArray(Line,LineSize);
  InpPtr=Line+DataLen; InpLen=0; return 0; }

template <class type>
 int dspDelayLine<type>::Process(type *Inp, int Len)
{ if((DataLen+Len)>LineSize)
  { dspMoveArray(Line,Line+DataLen-dspDelay,dspDelay); DataLen=dspDelay; }
  if((DataLen+Len)>LineSize) return -1;
  dspCopyArray(Line+DataLen,Inp,Len);
  InpPtr=Line+DataLen; InpLen=Len; DataLen+=Len;
  return 0; }

template <class type>
 int dspDelayLine<type>::Process(dspSeq<type> *Input)
{ return Process(Input->Data,Input->Len); }

// ----------------------------------------------------------------------------
// Low pass "moving box" FIR filter
// very unpure spectral response but CPU complexity very low
// and independent on the integration time

class dspBoxFilter
{ public:
   dspBoxFilter(); ~dspBoxFilter();
   void Free(void);
   int Preset(int BoxLen);
   int Process(double Inp, double &Out);
   int Process(double *Inp, int InpLen, double *Out);
   int Process(double_buff *Input);
   void Recalibrate();
   double_buff Output;
  private:
   int Len; double *Tap; int TapPtr; double Sum;
} ;

class dspCmpxBoxFilter
{ public:
   dspCmpxBoxFilter(); ~dspCmpxBoxFilter();
   void Free(void);
   int Preset(int BoxLen);
   int Process(dspCmpx *Inp, int InpLen, dspCmpx *Out);
   void Recalibrate();
   int Process(dspCmpx_buff *Input);
   dspCmpx_buff Output;
  private:
   int Len; dspCmpx *Tap; int TapPtr; dspCmpx Sum;
} ;

// ----------------------------------------------------------------------------
// FIR filter with a given shape

class dspFirFilter
{ public:
   dspFirFilter(); ~dspFirFilter();
   void Free(void);
   int Preset(int FilterLen, double *FilterShape=(double*)NULL);
   int Process(double *Inp, int InpLen, double *Out);
   int Process(double_buff *Input);
  //   Response(double Freq, double *Resp);
   int ComputeShape(double LowOmega, double UppOmega, double (*Window)(double));
  //   UseExternShape(double *shape);
   double_buff Output;
  private:
   int Len;		// Tap/Shape length
   double *Shape;	// Response shape
   int ExternShape;	// that we are using an externally provided shape
   double *Tap; int TapPtr;
} ;

// ----------------------------------------------------------------------------
// a pair of FIR filters. quadrature split, decimate
// the decimation rate must be integer

class dspQuadrSplit
{ public:
   dspQuadrSplit(); ~dspQuadrSplit();
   void Free(void);
   int Preset(int FilterLen,
	      double *FilterShape_I, double *FilterShape_Q,
	      int DecimateRate);
   int ComputeShape(double LowOmega, double UppOmega, double (*Window)(double));
//   int Process(double *Inp, int InpLen,
//	       double *OutI, double *OutQ, int MaxOutLen, int *OutLen);
//   int Process(double *Inp, int InpLen,
//	       dspCmpx *Out, int MaxOutLen, int *OutLen);
   int Process(double_buff *Input);
   dspCmpx_buff Output;
  private:
   int Len;
   double_buff Tap;
   double *ShapeI, *ShapeQ; int ExternShape;
   int Rate;
} ;

// ----------------------------------------------------------------------------
// reverse of dspQuadrSplit: interpolates and combines the I/Q
// back into 'real' signal.

class dspQuadrComb
{ public:
   dspQuadrComb(); ~dspQuadrComb();
   void Free(void);
   int Preset(int FilterLen,
	      double *FilterShape_I, double *FilterShape_Q,
	      int DecimateRate);
   int ComputeShape(double LowOmega, double UppOmega, double (*Window)(double));
   int Process(dspCmpx_buff *Input);
   double_buff Output;
  private:
   int Len; double *Tap; int TapPtr;
   double *ShapeI, *ShapeQ; int ExternShape;
   int Rate;
} ;

// ----------------------------------------------------------------------------
// complex mix with an oscilator (carrier)
// here we could avoid computing sine/cos at every sample

class dspCmpxMixer
{ public:
   dspCmpxMixer(); // ~dspCmpxMixer();
   void Free(void);
   int      Preset(double CarrierOmega);
   int ProcessFast(double *InpI, double *InpQ, int InpLen,
		   double *OutI, double *OutQ);
   int     Process(dspCmpx *Inp, int InpLen, dspCmpx *Out);
   int ProcessFast(dspCmpx *Inp, int InpLen, dspCmpx *Out);
   int     Process(dspCmpx_buff *Input);
   int ProcessFast(dspCmpx_buff *Input);
   dspCmpx_buff Output;
  public:
   double dspPhase,Omega;
} ;

// ----------------------------------------------------------------------------
// FM demodulator (dspPhase rotation speed-meter)

class dspFMdemod
{ public:
   dspFMdemod(); // ~dspFMdemod();
   int Preset(double CenterOmega);
   int Process(double *InpI, double *InpQ, int InpLen, double *Out);
   int Process(dspCmpx *Inp, int InpLen, double *Out);
   int Process(dspCmpx_buff *Input);
   double_buff Output;
  private:
   double PrevdspPhase;
  public:
   double RefOmega;
} ;

// ----------------------------------------------------------------------------
// Rate converter - real input/output, linear interpolation
// expect large error when high frequency components are present
// thus the best place to convert rates is after a low pass filter
// of a demodulator.

class dspRateConvLin
{ public:
   dspRateConvLin(); // ~dspRateConvLin();
   void SetOutVsInp(double OutVsInp);
   void SetInpVsOut(double InpVsOut);
   int Process(double_buff *InpBuff);
   double_buff Output;
  private:
   double OutStep, OutdspPhase;
   double PrevSample;
} ;

// ----------------------------------------------------------------------------
// Rate converter - real input/output, quadratic interpolation
// similar limits like for RateConv1

class dspRateConvQuadr
{ public:
   dspRateConvQuadr(); // ~dspRateConvQuadr();
   void SetOutVsInp(double OutVsInp);
   void SetInpVsOut(double InpVsOut);
   int Process(double *Inp, int InpLen,
	       double *Out, int MaxOutLen, int *OutLen);
   int Process(double_buff *InpBuff);
   double_buff Output;
  private:
   double OutStep, OutdspPhase;
   double Tap[4]; int TapPtr;
} ;

// ----------------------------------------------------------------------------
// Rate converter, real input/output,
// bandwidth-limited interpolation, several shifted FIR filters

class dspRateConvBL
{ public:
   dspRateConvBL(); ~dspRateConvBL();
   void Free(void);
   int Preset(int FilterLen, double *FilterShape[], int FilterShapeNum);
   int ComputeShape(double LowOmega, double UppOmega, double (*Window)(double));
   void SetOutVsInp(double OutVsInp);
   void SetInpVsOut(double InpVsOut);
   int Process(double_buff *Input);
   int ProcessLinI(double_buff *Input);
   double_buff Output;
  private:
   double OutStep, OutdspPhase;
   int Len;
   double *Tap; int TapSize;
   double **Shape; int ShapeNum; int ExternShape;
} ;

// ----------------------------------------------------------------------------
// Sliding window (for FFT input)

class dspCmpxSlideWindow
{ public:
   dspCmpxSlideWindow(); ~dspCmpxSlideWindow();
   void Free(void);
   int Preset(int WindowLen, int SlideDist, double *WindowShape=(double*)NULL);
   int SetWindow(double (*NewWindow)(double dspPhase), double Scale=1.0);
   int Process(dspCmpx_buff *Input);
   dspCmpx_buff Output;
  private:
   int Len;	 // Window length
   dspCmpx *Buff; // storage
   int Dist;	 // distance between slides
   int Ptr;     // data pointer in Buff
   double *Window; // window shape
   int ExternWindow;
} ;

// ----------------------------------------------------------------------------
// Overlapping window (for IFFT output)

class dspCmpxOverlapWindow
{ public:
   dspCmpxOverlapWindow(); ~dspCmpxOverlapWindow();
   void Free(void);
   int Preset(int WindowLen, int SlideDist, double *WindowShape=(double*)NULL);
   int SetWindow(double (*NewWindow)(double dspPhase), double Scale=1.0);
   void Process(dspCmpx *Inp, dspCmpx *Out);
   int ProcessSilence(int Slides=1);
   int Process(dspCmpx_buff *Input);
   int Process(dspCmpx *Input);
//   int Process(dspCmpx_buff *Input);
   dspCmpx_buff Output;
  private:
   int Len;	 // Window length
   dspCmpx *Buff; // storage
   int Dist;	 // distance between slides
   double *Window; // window shape
   int ExternWindow;
} ;

// ----------------------------------------------------------------------------
// FFT dspPhase corrector

class dspFFT_TimeShift
{ public:
   dspFFT_TimeShift();
   ~dspFFT_TimeShift();
   void Free(void);
   int Preset(int FFTlen, int Backwards=0);
   int Process(dspCmpx *Data, int Time);
  private:
   int Len;	// FFT length
   int LenMask; // length-1 for pointer wrapping
   dspCmpx *FreqTable; // sin/cos table
   int dspPhase;
} ;

// ----------------------------------------------------------------------------
// bit synchronizer, the bit rate is the input rate divided by four

class dspDiffBitSync4
{ public:
   dspDiffBitSync4(int IntegBits); ~dspDiffBitSync4();
   void Free(void);
   int Process(double *Inp, int InpLen,
	 double *BitOut, double *IbitOut,
	 int MaxOutLen, int *OutLen);
   double GetSyncDriftRate();    // get aver. sync. drift
   double GetSyncConfid();
  private:                      // eg. 0.01 means 1 bit drift per 100 bits
   double *InpTap; int InpTapLen, InpTapPtr; // buffer tap, length and pointer
   int IntegLen;                // integrate tdspIntming over that many bits
   double W1,W2,W5;              // weights for the two-stage IIR lopass filter
   double DiffInteg0[4], DiffInteg[4]; // signal diff. integrators
   int DiffTapPtr;              // integrator and bit-sdspAmpling pointer
   int BitPtr; double SyncdspPhase; // sync. pointer/dspPhase
   double SyncDrift0,SyncDrift; // low pass filter for the sync. drift rate
   double SyncConfid;
} ;

// ----------------------------------------------------------------------------
// bit slicer, SNR/Tune meter

class dspBitSlicer
{ public:
   dspBitSlicer(int IntegBits);
   ~dspBitSlicer();
   int Process(double *Bits, double *IBits, int InpLen, double *OutBits);
   double GetSigToNoise(); double GetdspAmplAsym(); double GetTimeAsym();
  private:
   int IntegLen,TapLen; double W1,W2,W5;
   double Sum0[2],   Sum[2];
   double SumSq0[2], SumSq[2];
   double TimeAsym0, TimeAsym;
   double dspAmplAsym0, dspAmplAsym;
   double Noise[2]; double OptimThres;
   double *Tap; int TapPtr;
   double PrevBit, PrevIBit;
} ;

// ----------------------------------------------------------------------------
// The decoder for the HDLC frames,
// makes no AX.25 CRC check, only the length in bytes against MinLen and MaxLen
// however it does not pass frames with non-complete bytes.

class dspHDLCdecoder
{ public:
   dspHDLCdecoder(int minlen, int maxlen, int diff, int invert,
               int chan, int (*handler)(int, char *, int));
   ~dspHDLCdecoder();
   int Process(double *Inp, int InpLen);
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

short unsigned int dspAX25CRC(char *Data, int Len);

// ----------------------------------------------------------------------------
// check if the given number (an integer) is a dspPower of 2
template <class type> int dspPowerOf2(type I)
{ int c; if(I<=0) return 0;
  for(c=0; I!=0; I>>=1) c+=I&1;
  return c==1; }

// ----------------------------------------------------------------------------

class dsp_r2FFT // radix-2 FFT
{ public:   // size must a dspPower of 2: 2,4,8,16,32,64,128,256,...
   dsp_r2FFT();
   ~dsp_r2FFT();
   void Free(void);
  // preset tables for given processing size
   int Preset(int size);
  // scramble/unscramble input
   void Scramble(dspCmpx x[]);
  // apply input window
  // separate the result of a two real channels FFT
   void SeparTwoReals(dspCmpx Buff[], dspCmpx Out0[], dspCmpx Out1[]);
  // join spectra of two real channels
   void JoinTwoReals(dspCmpx Inp0[], dspCmpx Inp1[], dspCmpx Buff[]);
  // core process: the classic tripple loop of butterflies
   void CoreProc(dspCmpx x[]);
  // complex FFT process in place, includes unscrambling
   inline void ProcInPlace(dspCmpx x[]) { Scramble(x); CoreProc(x); }
  // define the FFT window and input/output scales (NULL => rectangular window)
  public:
   int Size;	        // FFT size
   int *BitRevIdx;	// Bit-reverse indexing table for data (un)scrambling
   dspCmpx *Twiddle;	// Twiddle factors (sine/cos values)
  private:
//   double *Window;	// window shape (NULL => rectangular window
//   double WinInpScale, WinOutScale; // window scales on input/output
  private:
  // classic radix-2 butterflies
   inline void FFTbf(dspCmpx &x0, dspCmpx &x1, dspCmpx &W);
  // special 2-elem. FFT for the first pass
   inline void FFT2(dspCmpx &x0, dspCmpx &x1);
  // special 2-elem. FFT for the second pass
   inline void FFT4(dspCmpx &x0, dspCmpx &x1, dspCmpx &x2, dspCmpx &x3);
} ;

// ---------------------------------------------------------------------------
// Sliding window FFT for spectral analysis (e.g. SETI)
// input: real-valued time-domain signal,
// output: complex-valued Fourier Transform
//
// We use a little trick here to process two real-valued FFT
// in one go using the complex FFT engine.
// This cuts the CPU but makes the input->output dspDelay longer.

class dspSlideWinFFT
{ public:
   dspSlideWinFFT(); ~dspSlideWinFFT();
   void Free();
   int Preset(int size, int step, double *window);
   int Preset(int size, int step,
	      double (*NewWindow)(double dspPhase), double Scale=1.0);
   int SetWindow(double *window);
   int SetWindow(double (*NewWindow)(double dspPhase), double Scale=1.0);
   int Process(double_buff *Input);
   dsp_r2FFT FFT;		// FFT engine
   dspCmpx_buff Output;	// output buffer
   int Size; int SizeMask; // FFT size, size mask for pointer wrapping
   int Dist; int Left;	// distance between slides, samples left before the next slide
   int Slide;		// even/odd slide
  private:
   double *SlideBuff; int SlidePtr; // sliding window buffer, pointer
   double *Window; int ExternWindow; // window shape
   dspCmpx *FFTbuff;		    // FFT buffer
} ;

// ---------------------------------------------------------------------------
// Overlapping window Inverse FFT to convert spectra into time-domain signal

class dspOvlapWinIFFT
{ public:
   dspOvlapWinIFFT(); ~dspOvlapWinIFFT();
   void Free(void);
   int Preset(int size, int step, double *window);
   int Preset(int size, int step,
	      double (*NewWindow)(double dspPhase), double Scale=1.0);
   int SetWindow(double *window);
   int SetWindow(double (*NewWindow)(double dspPhase), double Scale=1.0);
   int Process(dspCmpx *Input);
   dsp_r2FFT FFT;		// FFT engine
   double_buff Output;	// output buffer
   int Size; int SizeMask; // FFT size, size mask for pointer wrapping
   int Dist;		// distance between slides
   int Slide;
  private:
   dspCmpx *Spectr[2];
   dspCmpx *FFTbuff;		    // FFT buffer
   double *Window; int ExternWindow; // window shape
   double *OvlapBuff; int OvlapPtr;
} ;

// ---------------------------------------------------------------------------
// Sliding window FFT for spectral processing (e.g. de-noising)
// input: real-valued signal
// in the middle you are given a chance to process
// the complex-valued Fourier Transform (SpectraProc() routine).
// output: real-valued signal
// If you don't touch the spectra in SpectralProc()
// the output will be an exact copy (only dspDelayed) of the input.

class dspSlideWinFFTproc
{ public:
   dspSlideWinFFTproc(); ~dspSlideWinFFTproc();
   void Free(void);
   int Preset(int size, int step, void (*proc)(dspCmpx *Spectra, int Len),
	      double *window);
   int Preset(int size, int step, void (*proc)(dspCmpx *Spectra, int Len),
	      double (*NewWindow)(double dspPhase), double Scale=0.0);
   int SetWindow(double *window);
   int SetWindow(double (*NewWindow)(double dspPhase), double Scale=0.0);
   int Process(double_buff *Input);
   dsp_r2FFT FFT;
   double_buff Output;
   int Size; int SizeMask;
   int Dist; int Left;
   int Slide;
  private:
   double *SlideBuff; int SlidePtr;
   double *Window; int ExternWindow;
   dspCmpx *FFTbuff;
   dspCmpx *Spectr[2];
   void (*SpectraProc)(dspCmpx *Spectra, int Len);
   double *OvlapBuff; int OvlapPtr;
} ;

// ---------------------------------------------------------------------------
// Walsh (Hadamard ?) transform.

void dspWalshTrans(double *Data, int Len);
void dspWalshInvTrans(double *Data, int Len);

// ---------------------------------------------------------------------------



