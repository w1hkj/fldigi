/*
 *	dsp.cc  --  various DSP algorithms
 *
 *	based on mt63 code by Pawel Jalocha
 *	Copyright (C) 1999-2004 Pawel Jalocha, SP9VRC
 *	Copyright (c) 2007-2011 Dave Freese, W1HKJ
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

// Please note, that you should not rely on the correctness
// of these routines. They generally work well, but you may find
// differences in respect to the mathematical formulas: signs flipped,
// orders swapped, etc.

#include <config.h>

#include <stdio.h> // only when we do some control printf's
#include <stdlib.h>
#include <math.h>
#include "dsp.h"

// ----------------------------------------------------------------------------

double dspPower(double *X, int Len)
{
	double Sum;
	for(Sum = 0.0; Len; Len--,X++)
		Sum += (*X)*(*X);
	return Sum;
}

double dspPower(double *I, double *Q, int Len)
{
	double Sum;
	for(Sum = 0.0; Len; Len--,I++,Q++)
		Sum += (*I)*(*I) + (*Q)*(*Q);
	return Sum;
}

double dspPower(dspCmpx *X, int Len)
{
	double Sum;
	for(Sum = 0.0; Len; Len--,X++)
		Sum += (X->re)*(X->re) + (X->im)*(X->im);
	return Sum;
}

// ----------------------------------------------------------------------------
// dspAverage, extremes, fitting

double dspAverage(double *Data, int Len)
{
	double Sum; int i;
	for(Sum = 0.0,i = 0; i < Len; i++) Sum += Data[i];
	return Sum/Len;
}

int dspCountInRange(double *Data, int Len, double Low, double Upp)
{
	int count, i;
	double D;
	for(count = i = 0; i<Len; i++) {
		D = Data[i];
		count += ((Low<=D)&&(D<=Upp));
	}
	return count;
}

double dspFindMaxdspPower(dspCmpx *Data, int Len)
{
	double Max, Pwr;
	int i;
	Max = dspPower(Data[0]);
	for(i = 1; i < Len; i++) {
		Pwr = dspPower(Data[i]);
		if (Pwr > Max) Max = Pwr;
	}
	return Max;
}

double dspFindMaxdspPower(dspCmpx *Data, int Len, int &MaxPos)
{
	double Max,	Pwr;
	int i, pos;
	Max = dspPower(Data[0]);
	pos = 0;
	for (i = 1; i < Len; i++) {
		Pwr = dspPower(Data[i]);
		if (Pwr > Max) {
			Max = Pwr;
			pos = i;
		}
	}
	MaxPos = pos;
	return Max;
}

double dspFitPoly1(double *Data, int Len, double &A, double &B)
{
	double Sum;
	int i;
	A = (Data[Len-1] - Data[0])/(Len - 1);
	for (Sum = 0.0,i = 0; i < Len; i++)
		Sum += Data[i] - A*i;
	B = Sum/Len;
	for (Sum = 0.0, i = 0; i < Len; i++)
		Sum += dspPower(Data[i] - (A*i + B));
	return sqrt(Sum/Len);
}

double dspFitPoly2(double *Data, int Len, double &A, double &B, double &C)
{
	double Sum;
	int i;
	A = ((Data[Len - 1] - Data[Len - 2]) - (Data[1] - Data[0]))/(Len - 2)/2;
	B = (Data[Len - 1] - A*(Len - 1)*(Len - 1) - Data[0])/(Len - 1);
	for (Sum = 0.0, i = 0; i < Len; i++)
		Sum += Data[i] - (A*i*i + B*i);
	C = Sum/Len;
	for (Sum = 0.0, i = 0; i < Len; i++)
		Sum += dspPower(Data[i] - (A*i*i + B*i + C));
	return sqrt(Sum/Len);
}

void dspFitPoly2(double Data[3], double &A, double &B, double &C)
{
	C = Data[0];
	A = (Data[0]- 2*Data[1] + Data[2])/2;
	B = (Data[1] - Data[0]) - A;
}

// ----------------------------------------------------------------------------
// various window shapes (for the FFT and FIR filters)
// these functions are supposed to be called with the argument "dspPhase"
// between -PI and +PI. Most (or even all) will return zero for input
// euqal -PI or +PI.

double WindowHamming(double dspPhase)
{
	return cos(dspPhase/2);
} // not exactly ...

double dspWindowHanning(double dspPhase)
{
	return (1.0 + cos(dspPhase))/2;
}

double WindowBlackman2(double dspPhase) // from Freq 5.1 FFT analyzer
{
	return 0.42 + 0.5*cos(dspPhase) + 0.08*cos(2*dspPhase);
}

double dspWindowBlackman3(double dspPhase) // from the Motorola BBS
{
	return 0.35875 + 0.48829*cos(dspPhase) + 0.14128*cos(2*dspPhase) + 0.01168*cos(3*dspPhase);
}

// ----------------------------------------------------------------------------
// FIR shape calculation for a flat response from FreqLow to FreqUpp

void dspWinFirI(
		double LowOmega, double UppOmega,
		double *Shape, int Len, double (*Window)(double), double shift)
{
	int i;
	double time, dspPhase, shape;
// printf("dspWinFirI: %5.3f %5.3f %d\n",LowOmega,UppOmega,Len);
	for (i = 0; i < Len; i++) {
		time = i + (1.0 - shift) - (double)Len/2;
		dspPhase = 2*M_PI*time/Len;
		if (time == 0)
			shape = UppOmega - LowOmega;
		else
			shape = (sin(UppOmega*time) - sin(LowOmega*time))/time;
// printf("%2d %5.1f %5.2f %7.4f %7.4f\n",i,time,dspPhase,shape,(*Window)(dspPhase));
	Shape[i] = shape*(*Window)(dspPhase)/M_PI; }
}

void WinFirQ(
		double LowOmega, double UppOmega,
		double *Shape, int Len, double (*Window)(double), double shift)
{
	int i;
	double time, dspPhase, shape;
// printf("WinFirQ: %5.3f %5.3f %d\n",LowOmega,UppOmega,Len);
	for (i = 0; i < Len; i++) {
		time = i + (1.0 - shift) - (double)Len/2;
		dspPhase = 2*M_PI*time/Len;
		if (time == 0)
			shape=0.0;
		else
			shape = (-cos(UppOmega*time) + cos(LowOmega*time))/time;
// printf("%2d %5.1f %5.2f %7.4f %7.4f\n",i,time,dspPhase,shape,(*Window)(dspPhase));
		Shape[i] = (-shape)*(*Window)(dspPhase)/M_PI;
	}
} // we put minus for the Q-part because the FIR shapes must be placed
  // in reverse order for simpler indexing

// ----------------------------------------------------------------------------
// convert 16-bit signed or 8-bit unsigned into doubles

void dspConvS16todouble(dspS16 *dspS16, double *dble, int Len, double Gain)
{
	for (; Len; Len--)
		(*dble++) = (*dspS16++)*Gain;
}

int dspConvS16todouble(short int *dspS16, double_buff *dble, int Len, double Gain)
{
	int err = dble->EnsureSpace(Len);
	if (err) return -1;
	dspConvS16todouble(dspS16, dble->Data, Len, Gain);
	dble->Len = Len;
	return 0;
}

void dspConvdoubleTodspS16(double *dble, dspS16 *dspS16, int Len, double Gain)
{
	double out;
	for (; Len; Len--) {
		out = (*dble++)*Gain;
		if (out > 32767.0)
			out = 32767.0;
		else if (out < (-32767.0))
			out = (-32767.0);
		(*dspS16++) = (short int)floor(out+0.5);
	}
} // we could count the over/underflows ?

void dspConvU8todouble(unsigned char *U8, double *dble, int Len, double Gain)
{
	for (; Len; Len--)
		(*dble++) = ((int)(*U8++) - 128)*Gain;
}

int  dspConvU8todouble(unsigned char *U8, double_buff *dble, int Len, double Gain)
{
	int err = dble->EnsureSpace(Len);
	if (err) return -1;
	dspConvU8todouble(U8, dble->Data, Len, Gain);
	dble->Len = Len;
	return 0;
}

// ----------------------------------------------------------------------------
// other converts

void dspConvCmpxTodspPower(dspCmpx *Inp, int Len, double *Out)
{
	for (; Len; Len--)
		(*Out++) = dspPower(*Inp++);
}

int dspConvCmpxTodspPower(dspCmpx_buff *Input, double_buff *Output)
{
	int err = Output->EnsureSpace(Input->Len);
	if (err) return err;
	dspConvCmpxTodspPower(Input->Data, Input->Len, Output->Data);
	Output->Len = Input->Len;
	return 0;
}

void dspConvCmpxTodspAmpl(dspCmpx *Inp, int Len, double *Out)
{
	for (; Len; Len--)
		(*Out++) = sqrt(dspPower(*Inp++));
}

int dspConvCmpxTodspAmpl(dspCmpx_buff *Input, double_buff *Output)
{
	int err = Output->EnsureSpace(Input->Len);
	if(err) return err;
	dspConvCmpxTodspAmpl(Input->Data, Input->Len, Output->Data);
	Output->Len = Input->Len;
	return 0;
}

void dspConvCmpxTodspPhase(dspCmpx *Inp, int Len, double *Out)
{
	for (; Len; Len--)
		(*Out++) = dspPhase(*Inp++);
}

int dspConvCmpxTodspPhase(dspCmpx_buff *Input, double_buff *Output)
{
	int err = Output->EnsureSpace(Input->Len);
	if(err) return err;
	dspConvCmpxTodspPhase(Input->Data, Input->Len, Output->Data);
	Output->Len = Input->Len;
	return 0;
}

// ----------------------------------------------------------------------------
// Pulse noise limiter

dspPulseLimiter::dspPulseLimiter()
{
	Tap = NULL;
}

dspPulseLimiter::~dspPulseLimiter()
{
	free(Tap);
}

void dspPulseLimiter::Free(void)
{
	free(Tap);
	Tap = NULL;
}

int dspPulseLimiter::Preset(int TapLen, double LimitThres)
{
	Len = TapLen;
	Thres = LimitThres*LimitThres;
	if (dspRedspAllocArray(&Tap, Len)) return -1;
	dspClearArray(Tap, Len);
	Ptr = 0;
	PwrSum = 0.0;
	return 0;
}

int dspPulseLimiter::Process(double *Inp, int InpLen, double *Out)
{
	int i, o;
	double Lim;
	for (i = 0; i < InpLen; i++) {
		PwrSum -= Tap[Ptr]*Tap[Ptr];
		Tap[Ptr++] = Inp[i];
		PwrSum += Inp[i]*Inp[i];
		if (Ptr >= Len) Ptr -= Len;
		o = Ptr + (Len/2);
		if (o >= Len) o -= Len;
		Lim = Thres*PwrSum/Len;
		if (Tap[o]*Tap[o] <= Lim)
			Out[i] = Tap[o];
		else {
			if (Tap[o] > 0.0)
				Out[i] = sqrt(Lim);
			else
				Out[i] = (-sqrt(Lim));
		}
	}
	for (PwrSum = 0.0, i = 0; i < Len; i++)
		PwrSum += Tap[i]*Tap[i];
	dspRMS = sqrt(PwrSum/Len);
	return 0;
}

int dspPulseLimiter::Process(double_buff *Input)
{
	int err = Output.EnsureSpace(Input->Len);
	if (err) return -1;
	Process(Input->Data, Input->Len, Output.Data);
	Output.Len = Input->Len;
	return 0;
}

// ----------------------------------------------------------------------------
// Signal level monitor

dspLevelMonitor::dspLevelMonitor()
{ }

dspLevelMonitor::~dspLevelMonitor()
{ }

int dspLevelMonitor::Preset(double Integ, double Range)
{
	dspLowPass2Coeff(Integ, W1, W2, W5);
	MaxSqr = Range*Range;
	PwrMid = 0.0;
	PwrOut = 0.0;
	dspRMS = 0.0;
	OutOfRangeMid = 0.0;
	OutOfRange = 0.0;
	return 0;
}

int dspLevelMonitor::Process(double *Inp, int Len)
{
	int i, Out;
	double Sqr, Sum;
	if (Len <= 0) return 0;
	for (Sum = 0.0, Out = 0,i = 0; i < Len; i++) {
		Sum += Sqr = Inp[i]*Inp[i];
		Out += (Sqr > MaxSqr);
	}
	dspLowPass2(Sum/Len, PwrMid, PwrOut, W1, W2, W5);
	dspLowPass2((double)Out/Len, OutOfRangeMid, OutOfRange, W1, W2, W5);
	if (OutOfRange < 0.0)
		OutOfRange = 0.0;
	if (PwrOut <= 0.0)
		dspRMS = 0.0;
	else
		dspRMS = sqrt(PwrOut);
	return 0;
}

int dspLevelMonitor::Process(double_buff *Input)
{ return Process(Input->Data,Input->Len); }

// ----------------------------------------------------------------------------
// Automatic Gain/Level Control for the Mixer

dspMixerAutoLevel::dspMixerAutoLevel()
{
	MinMS = 0.01;
	MaxMS = 0.05;
	IntegLen = 8000;
	PeakHold = 4000;
	MinHold = 800;
	MinLevel = 0;
	MaxLevel = 100;
	AdjStep = 1;
	Level = 75;
	Hold = (-IntegLen);
	AvedspRMS = 0.0;
}

int dspMixerAutoLevel::Process(double *Inp, int InpLen)
{
	double MS = dspPower(Inp, InpLen) / IntegLen;
	double W = 1.0 - ((double)InpLen) / IntegLen;

	AvedspRMS = AvedspRMS*W + MS;
	Hold += InpLen;
	if (Hold < MinHold) return 0;

	if(AvedspRMS>MaxMS) {
		Level -= AdjStep;
		if (Level < MinLevel)
			Level = MinLevel;
		Hold=0;
		return 1;
	}

	if (Hold < PeakHold) return 0;

	if (AvedspRMS < MinMS) {
		Level += AdjStep;
		if (Level > MaxLevel)
			Level = MaxLevel;
		Hold = 0;
		return 1;
	}

	return 0;
}

// ----------------------------------------------------------------------------

void dspLowPass2(dspCmpx *Inp, dspCmpx *Mid, dspCmpx *Out, double W1, double W2, double W5)
{
	double Sum, Diff;
//  printf("\n[dspLowPass2] %6.3f %6.3f %6.3f",Inp->re,Mid->re,Out->re);
	Sum = Mid->re + Out->re;
	Diff = Mid->re - Out->re;
	Mid->re += W2*Inp->re - W1*Sum;
	Out->re += W5*Diff;
//  printf(" => %6.3f %6.3f\n",Mid->re,Out->re);
	Sum = Mid->im + Out->im;
	Diff = Mid->im - Out->im;
	Mid->im += W2*Inp->im - W1*Sum;
	Out->im += W5*Diff;
}

// ----------------------------------------------------------------------------
// periodic low pass

dspPeriodLowPass2::dspPeriodLowPass2()
{
	TapMid = NULL;
	TapOut = NULL;
}

dspPeriodLowPass2::~dspPeriodLowPass2()
{
	free(TapMid);
	free(TapOut);
	Output.Free();
}

void dspPeriodLowPass2::Free(void)
{
	free(TapMid);
	TapMid = NULL;
	free(TapOut);
	TapOut = NULL;
}

int dspPeriodLowPass2::Preset(int Period, double IntegLen)
{
	int i;
	Len = Period;
	if (dspRedspAllocArray(&TapMid, Len)) goto Error;
	if (dspRedspAllocArray(&TapOut, Len)) goto Error;
	for (i = 0; i < Len; i++) {
		TapMid[i] = 0.0;
		TapOut[i] = 0.0;
	}
	TapPtr = 0;
	dspLowPass2Coeff(IntegLen, W1, W2, W5);
	return 0;
Error:
	Free();
	return -1;
}

int dspPeriodLowPass2::Process(double Inp, double &Out)
{
	dspLowPass2(Inp, TapMid[TapPtr], TapOut[TapPtr], W1, W2, W5);
	Out = TapOut[TapPtr++];
	if(TapPtr >= Len)
		TapPtr = 0;
	return 0;
}

int dspPeriodLowPass2::Process(double *Inp, int InpLen, double *Out)
{
	int i, batch;
	for (i = 0; i < InpLen; ) {
		for (batch = dspIntmin(InpLen-i, Len - TapPtr), i += batch; batch; batch--) {
			dspLowPass2(*Inp++, TapMid[TapPtr], TapOut[TapPtr], W1, W2, W5);
			(*Out++) = TapOut[TapPtr++];
		}
		if (TapPtr >= Len)
			TapPtr = 0;
	}
	return 0;
}

int dspPeriodLowPass2::Process(double_buff *Input)
{
	int err = Output.EnsureSpace(Input->Len);
	if (err) return -1;
	Process(Input->Data, Input->Len, Output.Data);
	Output.Len = Input->Len;
	return 0;
}

// ----------------------------------------------------------------------------
// Low pass "moving box" FIR filter
// very unpure spectral response but complexity very low
// and independent on the integration time

dspBoxFilter::dspBoxFilter()
{
	Tap = NULL;
}

dspBoxFilter::~dspBoxFilter()
{
	free(Tap);
}

void dspBoxFilter::Free(void)
{
	free(Tap);
	Tap = NULL;
	Output.Free();
}

int dspBoxFilter::Preset(int BoxLen)
{
	int i;
	if (dspRedspAllocArray(&Tap, BoxLen)) return -1;
	for (i = 0; i < BoxLen; i++)
		Tap[i] = 0;
	Len = BoxLen;
	TapPtr = 0;
	Sum = 0;
	return 0;
}

int dspBoxFilter::Process(double *Inp, int InpLen, double *Out)
{
	int i, batch;
	for (i = 0; i < InpLen; ) {
		for (batch = dspIntmin(InpLen-i, Len - TapPtr), i += batch; batch; batch--) {
			Sum -= Tap[TapPtr];
			Out[i] = (Sum += Tap[TapPtr++] = Inp[i]);
		}
		if (TapPtr >= Len)
			TapPtr = 0;
	}
	for (Sum = 0, i = 0; i < Len; i++)
		Sum += Tap[i];
	return InpLen;
}

void dspBoxFilter::Recalibrate()
{
	int i;
	for (Sum = 0, i = 0; i < Len; i++)
		Sum += Tap[i];
}

int dspBoxFilter::Process(double_buff *Input)
{
	int err = Output.EnsureSpace(Input->Len);
	if (err) return err;
	Process(Input->Data, Input->Len, Output.Data);
	Output.Len = Input->Len;
	return 0;
}

dspCmpxBoxFilter::dspCmpxBoxFilter()
{
	Tap = NULL;
}

dspCmpxBoxFilter::~dspCmpxBoxFilter()
{
	free(Tap);
}

void dspCmpxBoxFilter::Free(void)
{
	free(Tap);
	Tap = NULL;
	Output.Free();
}

int dspCmpxBoxFilter::Preset(int BoxLen)
{
	int i;
	if (dspRedspAllocArray(&Tap, BoxLen)) return -1;
	for (i = 0; i < BoxLen; i++)
		Tap[i].re = Tap[i].im = 0.0;
	Len = BoxLen;
	TapPtr = 0;
	Sum.re = 0.0;
	Sum.im = 0.0;
	return 0;
}

int dspCmpxBoxFilter::Process(dspCmpx *Inp, int InpLen, dspCmpx *Out)
{
	int i, batch;
	for (i = 0; i < InpLen; ) {
		for (batch = dspIntmin(InpLen-i, Len - TapPtr), i += batch; batch; batch--) {
			Sum.re -= Tap[TapPtr].re;
			Sum.im -= Tap[TapPtr].im;
			Tap[TapPtr] = Inp[i];
			Sum.re += Inp[i].re;
			Sum.im += Inp[i].im;
			Out[i].re = Sum.re;
			Out[i].im = Sum.im;
		}
		if (TapPtr >= Len)
			TapPtr = 0;
	}
	for (Sum.re = Sum.im = 0.0, i = 0; i < Len; i++) {
		Sum.re += Tap[i].re;
		Sum.im += Tap[i].im;
	}
	return InpLen;
}

void dspCmpxBoxFilter::Recalibrate()
{
	int i;
	for (Sum.re = Sum.im = 0.0, i = 0; i < Len; i++) {
		Sum.re += Tap[i].re;
		Sum.im += Tap[i].im;
	}
}

int dspCmpxBoxFilter::Process(dspCmpx_buff *Input)
{
	int err = Output.EnsureSpace(Input->Len);
	if(err) return err;
	Process(Input->Data, Input->Len, Output.Data);
	Output.Len = Input->Len;
	return 0;
}

// ----------------------------------------------------------------------------
// FIR filter with a given shape

dspFirFilter::dspFirFilter()
{
	Tap = NULL;
	ExternShape = 1;
}

dspFirFilter::~dspFirFilter()
{
	free(Tap);
	if (!ExternShape)
		free(Shape);
}

void dspFirFilter::Free(void)
{
	free(Tap);
	Tap = NULL;
	if (!ExternShape)
		free(Shape);
	Shape = NULL;
	Output.Free();
}

int dspFirFilter::Preset(int FilterLen, double *FilterShape)
{
	int i;
	if (dspRedspAllocArray(&Tap, FilterLen)) return -1;
	for (i = 0; i < FilterLen; i++)
		Tap[i] = 0;
	Len = FilterLen;
	TapPtr = 0;
	if (!ExternShape)
		free(Shape);
	Shape = FilterShape;
	return 0;
}

int dspFirFilter::ComputeShape(
		double LowOmega, double UppOmega,
		double (*Window)(double))
{
	if (ExternShape) {
		Shape = NULL;
		ExternShape = 0;
	}
	if (dspRedspAllocArray(&Shape, Len)) return -1;
	dspWinFirI(LowOmega, UppOmega, Shape, Len, Window);
		return 0;
}

int dspFirFilter::Process(double *Inp, int InpLen, double *Out)
{
	int i, s, t;
	double Sum;
	if(InpLen<Len) {
		for (i = 0; i < InpLen; i++) {
			for (Sum = 0.0, t = 0, s = i; s < Len; s++)
				Sum += Tap[s]*Shape[t++];
			for (s -= Len; t < Len; s++)
				Sum += Inp[s]*Shape[t++];
			Out[i] = Sum;
		}
		memmove(Tap, Tap + InpLen, (Len - InpLen)*sizeof(double));
		memcpy(Tap + (Len - InpLen), Inp, InpLen*sizeof(double));
	} else {
		for (i = 0; i < Len; i++) {
			for (Sum = 0.0, t = 0, s = i; s < Len; s++)
				Sum += Tap[s]*Shape[t++];
			for (s -= Len; t < Len; s++)
				Sum += Inp[s]*Shape[t++];
			Out[i] = Sum;
		}
		for (; i < InpLen; i++) {
			for (Sum = 0.0, t = 0, s = i - Len; t < Len; s++)
				Sum += Inp[s]*Shape[t++];
			Out[i] = Sum;
		}
		memcpy(Tap, Inp + (InpLen - Len), Len*sizeof(double));
	}
	return InpLen;
}

int dspFirFilter::Process(double_buff *Input)
{
	int err = Output.EnsureSpace(Input->Len);
	if(err) return err;
	Process(Input->Data, Input->Len, Output.Data);
	Output.Len = Input->Len;
	return 0;
}

// ----------------------------------------------------------------------------
// a pair of FIR filters. for quadrature split & decimate
// the decimation rate must be an integer

dspQuadrSplit::dspQuadrSplit()
{
	ExternShape = 1;
}

dspQuadrSplit::~dspQuadrSplit()
{
	if (!ExternShape) {
		free(ShapeI);
		free(ShapeQ);
	}
}

void dspQuadrSplit::Free(void)
{
	Tap.Free();
	if (!ExternShape) {
		free(ShapeI);
		free(ShapeQ);
	}
	ShapeI = NULL;
	ShapeQ = NULL;
	Output.Free();
}

int dspQuadrSplit::Preset(
				int FilterLen,
				double *FilterShape_I, double *FilterShape_Q,
				int DecimateRate)
{
	Len = FilterLen;
	if (!ExternShape) {
		free(ShapeI);
		free(ShapeQ);
	}
	ShapeI = FilterShape_I;
	ShapeQ = FilterShape_Q;
	ExternShape = 1;
	Tap.EnsureSpace(Len);
	Tap.Len = Len;
	dspClearArray(Tap.Data, Tap.Len);
	Rate = DecimateRate;
	return 0;
}

int dspQuadrSplit::ComputeShape(double LowOmega,double UppOmega,
			 double (*Window)(double))
{
	if (ExternShape) {
		ShapeI = NULL;
		ShapeQ = NULL;
		ExternShape = 0;
	}
	if (dspRedspAllocArray(&ShapeI, Len)) return -1;
	if (dspRedspAllocArray(&ShapeQ, Len)) return -1;
	dspWinFirI(LowOmega, UppOmega, ShapeI, Len, Window);
	WinFirQ(LowOmega, UppOmega, ShapeQ, Len, Window);
	return 0;
}

int dspQuadrSplit::Process(double_buff *Input)
{
	int err, i, s, t, o, l;
	double SumI, SumQ;
	double *Inp;
	dspCmpx *Out;
	int InpLen;

	InpLen = Input->Len;
	err = Tap.EnsureSpace(Tap.Len + InpLen);
	if (err) return err;
	dspCopyArray(Tap.Data+Tap.Len, Input->Data, InpLen);
// printf("dspQuadrSplit: InpLen=%d, Tap.Len=%d",InpLen,Tap.Len);
	Tap.Len += InpLen;
	Inp = Tap.Data;
// printf(" -> %d",Tap.Len);
	err = Output.EnsureSpace( InpLen / Rate + 2);
	if (err) return err;
	Out = Output.Data;
	for (l = Tap.Len-Len,o = 0, i = 0; i < l; i += Rate) {
		for (SumI = SumQ = 0.0, s = i,t = 0; t < Len; t++,s++) {
			SumI += Inp[s] * ShapeI[t];
			SumQ += Inp[s] * ShapeQ[t];
		}
		Out[o].re=SumI;
		Out[o++].im=SumQ;
	}
	Tap.Len -= i;
	dspMoveArray(Tap.Data,Tap.Data+i,Tap.Len);
	Output.Len = o;
// printf(" => Tap.Len=%d\n",Tap.Len);
	return 0;
}

// ----------------------------------------------------------------------------
// reverse of dspQuadrSplit: interpolates and combines the I/Q
// back into 'real' signal.

dspQuadrComb::dspQuadrComb()
{
	Tap = NULL;
	ExternShape = 1;
}

dspQuadrComb::~dspQuadrComb()
{
	free(Tap);
	if (!ExternShape) {
		free(ShapeI);
		free(ShapeQ);
	}
}

void dspQuadrComb::Free(void)
{
	free(Tap);
	Tap = NULL;
	if (!ExternShape) {
		free(ShapeI);
		free(ShapeQ);
	}
	ShapeI = NULL;
	ShapeQ = NULL;
	Output.Free();
}

int dspQuadrComb::Preset(
			int FilterLen,
			double *FilterShape_I, double *FilterShape_Q,
			int DecimateRate)
{
	int i;
	Len = FilterLen;
	if (dspRedspAllocArray(&Tap, Len)) return -1;
	if (!ExternShape) {
		free(ShapeI);
		free(ShapeQ);
	}
	ShapeI = FilterShape_I;
	ShapeQ = FilterShape_Q;
	ExternShape = 1;
	for (i = 0; i < FilterLen; i++)
		Tap[i] = 0.0;
	TapPtr = 0;
	Rate = DecimateRate;
	return 0;
}

int dspQuadrComb::ComputeShape(
			double LowOmega,double UppOmega,
			double (*Window)(double))
{
	if (ExternShape) {
		ShapeI = NULL;
		ShapeQ = NULL;
		ExternShape = 0;
	}
	if (dspRedspAllocArray(&ShapeI, Len)) return -1;
	if (dspRedspAllocArray(&ShapeQ, Len)) return -1;

	dspWinFirI(LowOmega, UppOmega, ShapeI, Len, Window);
	WinFirQ(LowOmega, UppOmega, ShapeQ, Len, Window);
	return 0;
}

int dspQuadrComb::Process(dspCmpx_buff *Input)
{
	int err, i, o, r, t, len;
	dspCmpx *Inp;
	double *Out;
	int InpLen;
	double I, Q;
	InpLen = Input->Len;
	err = Output.EnsureSpace(InpLen*Rate);
	if (err) return err;
	Inp = Input->Data;
	Out = Output.Data;
	Output.Len = InpLen*Rate;
	for(o=0,i=0; i<InpLen; i++) {
		I = Inp[i].re;
		Q = Inp[i].im;
		for (r = 0,t = TapPtr; t < Len; t++,r++)
			Tap[t] += I*ShapeI[r] + Q*ShapeQ[r];
		for (t = 0; t < TapPtr; t++, r++)
			Tap[t] += I*ShapeI[r] + Q*ShapeQ[r];
		len = Len - TapPtr;
		if (len < Rate) {
			for (r = 0; r < len; r++) {
				Out[o++] = Tap[TapPtr];
				Tap[TapPtr++] = 0.0;
			}
			TapPtr = 0;
			for ( ; r<Rate; r++) {
				Out[o++] = Tap[TapPtr];
				Tap[TapPtr++] = 0.0;
			}
		} else {
			for (r = 0; r < Rate; r++) {
				Out[o++] = Tap[TapPtr];
				Tap[TapPtr++] = 0.0;
			}
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------
// complex mix with an oscilator (carrier)

dspCmpxMixer::dspCmpxMixer()
{
	dspPhase = 0.0;
	Omega = 0.0;
}

void dspCmpxMixer::Free(void)
{
	Output.Free();
}

int dspCmpxMixer::Preset(double CarrierOmega)
{
	Omega = CarrierOmega;
	return 0;
}

int dspCmpxMixer::Process(dspCmpx *Inp, int InpLen, dspCmpx *Out)
{
	int i;
	double I, Q;
	for (i = 0; i < InpLen; i++) {
		I = cos(dspPhase);
		Q = sin(dspPhase);
		Out[i].re = I*Inp[i].re + Q*Inp[i].im;
		Out[i].im = I*Inp[i].im - Q*Inp[i].re;
		dspPhase += Omega;
		if (dspPhase >= 2*M_PI)
			dspPhase -= 2*M_PI;
		}
	return InpLen;
}

int dspCmpxMixer::ProcessFast(dspCmpx *Inp, int InpLen, dspCmpx *Out)
{
	int i;
	double dI, dQ, I, Q, nI, nQ, N;
	dI = cos(Omega);
	dQ = sin(Omega);
	I = cos(dspPhase);
	Q = sin(dspPhase);
	for (i = 0; i < InpLen; i++) {
		Out[i].re = I*Inp[i].re + Q*Inp[i].im;
		Out[i].im = I*Inp[i].im - Q*Inp[i].re;
		nI = I*dI - Q*dQ;
		nQ = Q*dI + I*dQ;
		I = nI;
		Q = nQ;
	}
	dspPhase += InpLen*Omega;
	N = floor(dspPhase/(2*M_PI));
	dspPhase -= N*2*M_PI;
	return InpLen;
}

int dspCmpxMixer::Process(dspCmpx_buff *Input)
{
	int err = Output.EnsureSpace(Input->Len);
	if (err) return err;
	Process(Input->Data, Input->Len, Output.Data);
	Output.Len = Input->Len;
	return 0;
}

int dspCmpxMixer::ProcessFast(dspCmpx_buff *Input)
{
	int err = Output.EnsureSpace(Input->Len);
	if(err) return err;
	ProcessFast(Input->Data, Input->Len, Output.Data);
	Output.Len = Input->Len;
	return 0;
}

// ----------------------------------------------------------------------------
// FM demodulator (dspPhase rotation speed meter)

dspFMdemod::dspFMdemod()
{
	PrevdspPhase = 0.0;
	RefOmega = 0.0;
}

int dspFMdemod::Preset(double CenterOmega)
{
	RefOmega = CenterOmega;
	return 0;
}

int dspFMdemod::Process(double *InpI, double *InpQ, int InpLen, double *Out)
{
	int i;
	double dspPhase, dspPhaseDiff;
	for (i = 0; i < InpLen; i++) {
		if ((InpI[i] == 0.0) && (InpQ[i] == 0.0))
			dspPhase = 0;
		else
			dspPhase = atan2(InpQ[i], InpI[i]);
		dspPhaseDiff = dspPhase - PrevdspPhase - RefOmega;
		if (dspPhaseDiff >= M_PI)
			dspPhaseDiff -= 2*M_PI;
		else if (dspPhaseDiff < (-M_PI))
			dspPhaseDiff += 2*M_PI;
		Out[i] = dspPhaseDiff;
		PrevdspPhase = dspPhase;
	}
	return InpLen;
}

int dspFMdemod::Process(dspCmpx *Inp, int InpLen, double *Out)
{
	int i;
	double dspPhase, dspPhaseDiff;
	for (i = 0; i < InpLen; i++) {
		if ((Inp[i].re == 0.0) && (Inp[i].im == 0.0))
			dspPhase = PrevdspPhase;
		else
			dspPhase = atan2(Inp[i].im, Inp[i].re);
		dspPhaseDiff = dspPhase - PrevdspPhase - RefOmega;
		if (dspPhaseDiff >= M_PI)
			dspPhaseDiff -= 2*M_PI;
		else if (dspPhaseDiff < (-M_PI))
			dspPhaseDiff += 2*M_PI;
		Out[i] = dspPhaseDiff;
		PrevdspPhase = dspPhase;
	}
	return InpLen;
}

int dspFMdemod::Process(dspCmpx_buff *Input)
{
	int err = Output.EnsureSpace(Input->Len);
	if(err) return err;
	Process(Input->Data, Input->Len, Output.Data);
	Output.Len = Input->Len;
	return 0;
}

// ----------------------------------------------------------------------------
// Rate converter - real input/output, linear interpolation
// expect large error when high frequency components are present
// thus the best place to convert rates is after a low pass filter
// of a demodulator.

// note: in fldigi these rate converters are not used.
//       libsamplerate is the preferred solution

dspRateConvLin::dspRateConvLin()
{
	PrevSample = 0;
	OutdspPhase = 0;
	OutStep = 1.0;
}

void dspRateConvLin::SetOutVsInp(double OutVsInp)
{
	OutStep = 1.0 / OutVsInp;
}

void dspRateConvLin::SetInpVsOut(double InpVsOut)
{
	OutStep = InpVsOut;
}

int dspRateConvLin::Process(double_buff *Input)
{
	int err, i, o;
	double *Inp, *Out;
	int InpLen;
	Inp = Input->Data;
	InpLen = Input->Len;
	err = Output.EnsureSpace((int)ceil(InpLen/OutStep) + 2);
	if (err) return err;
	Out = Output.Data;
	for (o = 0; OutdspPhase < 0; ) {
		Out[o++] = Inp[0]*(1.0 + OutdspPhase) - OutdspPhase*PrevSample;
		OutdspPhase += OutStep;
	}
	for (i = 0; i < (InpLen-1); ) {
		if (OutdspPhase >= 1.0) {
			OutdspPhase -= 1.0;
			i++;
		} else {
			Out[o++] = Inp[i]*(1.0 - OutdspPhase) + Inp[i+1]*OutdspPhase;
			OutdspPhase += OutStep;
		}
	}
	Output.Len = o;
	PrevSample = Inp[i];
	OutdspPhase -= 1.0;
	return 0;
}

// ----------------------------------------------------------------------------
// Rate converter - real input/output, quadratic interpolation
// similar limits like for RateConv1

dspRateConvQuadr::dspRateConvQuadr()
{
	int i;
	for (i = 0; i < 4; i++)
		Tap[i] = 0;
	OutStep = 1.0;
	OutdspPhase = 0;
	TapPtr = 0;
}

void dspRateConvQuadr::SetOutVsInp(double OutVsInp)
{
	OutStep = 1.0 / OutVsInp;
}

void dspRateConvQuadr::SetInpVsOut(double InpVsOut)
{
	OutStep = InpVsOut;
}

int dspRateConvQuadr::Process(
		double *Inp, int InpLen,
		double *Out, int MaxOutLen, int *OutLen)
{
	int i, o, t;
	double Ref0, Ref1, Diff0, Diff1;
	for (o = i = 0; (i < InpLen) && (o < MaxOutLen); ) {
		if (OutdspPhase >= 1.0) {
			Tap[TapPtr] = (*Inp++);
			i++;
			TapPtr = (TapPtr + 1) & 3;
			OutdspPhase -= 1.0;
		} else {
			t = TapPtr;
			Diff0 = (Tap[t^2] - Tap[t]) / 2;
			Ref1 = Tap[t^2];
			t = (t + 1) & 3;
			Diff1 = (Tap[t^2] - Tap[t]) / 2;
			Ref0 = Tap[t];
			(*Out++) = Ref0 * (1.0 - OutdspPhase) + Ref1*OutdspPhase // linear piece
					  -(Diff1-Diff0)*OutdspPhase*(1.0-OutdspPhase)/2; // quadr. piece
			o++;
			OutdspPhase += OutStep;
		}
	}
	(*OutLen) = o;
	return i;
}

int dspRateConvQuadr::Process(double_buff *Input)
{
	int err, i, o, t;
	double Ref0, Ref1, Diff0, Diff1;
	double *Inp,*Out;
	int InpLen;
	Inp = Input->Data;
	InpLen = Input->Len;
	err = Output.EnsureSpace((int)ceil(InpLen / OutStep) + 2);
	if (err) return err;
	Out = Output.Data;
	for (o = i = 0; i < InpLen; ) {
		if (OutdspPhase >= 1.0) {
			Tap[TapPtr] = (*Inp++);
			i++;
			TapPtr = (TapPtr + 1) & 3;
			OutdspPhase -= 1.0;
		} else {
			t = TapPtr;
			Diff0 = (Tap[t^2] - Tap[t]) / 2;
			Ref1 = Tap[t^2];
			t = (t + 1) & 3;
			Diff1 = (Tap[t^2] - Tap[t]) / 2;
			Ref0 = Tap[t];
			(*Out++) = Ref0 * (1.0 - OutdspPhase) + Ref1*OutdspPhase // linear piece
					   -(Diff1 - Diff0)*OutdspPhase*(1.0 - OutdspPhase)/2; // quadr. piece
			o++;
			OutdspPhase += OutStep;
		}
	}
	Output.Len = o;
	return 0;
}

// ----------------------------------------------------------------------------
// Rate converter, real input/output,
// bandwidth-limited interpolation, several shifted FIR filters

dspRateConvBL::dspRateConvBL()
{
	Tap = NULL;
	Shape = NULL;
	ExternShape = 1;
}

dspRateConvBL::~dspRateConvBL()
{
	Free();
}

void dspRateConvBL::Free(void)
{
	int s;
	free(Tap);
	Tap = NULL;
	if (ExternShape) return;
	if (Shape) {
		for (s = 0; s < ShapeNum; s++)
			free(Shape[s]);
		free(Shape);
		Shape = NULL;
	}
}

int dspRateConvBL::Preset(int FilterLen, double **FilterShape, int FilterShapeNum)
{
	int i;
	Free();
	Len = FilterLen;
	if (dspRedspAllocArray(&Tap, Len)) return -1;
	TapSize = Len;
	for (i = 0; i < Len; i++)
		Tap[i] = 0.0;
	Shape = FilterShape;
	ShapeNum = FilterShapeNum;
	ExternShape = 1;
	OutStep = 1.0;
	OutdspPhase = 0.0;
	return 0;
}

int dspRateConvBL::ComputeShape(double LowOmega, double UppOmega, double (*Window)(double))
{
	int idx;
	if (ExternShape) {
		if (dspAllocArray(&Shape, ShapeNum)) return -1;
		for (idx = 0; idx < ShapeNum; idx++) {
			if (dspAllocArray(&Shape[idx], Len)) return -1;
		}
		ExternShape = 0;
	}
	for (idx = 0; idx < ShapeNum; idx++)
		dspWinFirI(LowOmega, UppOmega, Shape[idx], Len, Window, (double)idx/ShapeNum);
	return 0;
}

void dspRateConvBL::SetOutVsInp(double OutVsInp)
{
	OutStep = 1.0 / OutVsInp;
}

void dspRateConvBL::SetInpVsOut(double InpVsOut)
{
	OutStep = InpVsOut;
}

int dspRateConvBL::Process(double_buff *Input)
{
	int i, o, idx, t, err;
	double *shape;
	double Sum;
	double *Inp, *Out;
	int InpLen;
	Inp = Input->Data;
	InpLen = Input->Len;
	err = Output.EnsureSpace((int)ceil(InpLen/OutStep)+2);
	if (err) return err;
	Out = Output.Data;
	if ((Len + InpLen) > TapSize) {
		Tap = (double*)realloc(Tap, (Len+InpLen)*sizeof(double));
		if (Tap == NULL) return -1;
		TapSize = Len + InpLen;
	}
	memcpy(Tap + Len, Inp, InpLen*sizeof(double));
	for(o=i=0; i<InpLen; ) {
		if (OutdspPhase >= 1.0) {
			OutdspPhase -= 1.0;
			i++;
		} else {
			idx = (int)floor(OutdspPhase*ShapeNum);
			shape = Shape[idx];
			for (Sum = 0.0, t = 0; t < Len; t++)
				Sum += Tap[i + t]*shape[t];
			Out[o++] = Sum;
			OutdspPhase += OutStep;
		}
	}
	Output.Len = o;
	memmove(Tap, Tap + InpLen, Len*sizeof(double));
	return 0;
}

int dspRateConvBL::ProcessLinI(double_buff *Input)
{
	int i, o, idx, t, err;
	double *Inp, *Out;
	int InpLen;
	double Sum0, Sum1;
	double *shape;
	double d;
	Inp = Input->Data;
	InpLen = Input->Len;
	err = Output.EnsureSpace((int)ceil(InpLen/OutStep)+2);
	if (err) return err;
	Out = Output.Data;
	if ((Len + InpLen) > TapSize) {
		Tap = (double*)realloc(Tap, (Len + InpLen)*sizeof(double));
		if (Tap == NULL) return -1;
		TapSize = Len + InpLen;
	}
	memcpy(Tap + Len, Inp, InpLen*sizeof(double));
	for (o = i = 0; i < InpLen; ) {
		if (OutdspPhase >= 1.0) {
			OutdspPhase -= 1.0;
			i++;
		} else {
			idx = (int)floor(OutdspPhase*ShapeNum);
			d = OutdspPhase*ShapeNum - idx;
			shape = Shape[idx];
			for (Sum0 = 0.0, t = 0; t < Len; t++)
				Sum0 += Tap[i+t]*shape[t];
			idx += 1;
			if (idx >= ShapeNum) {
				idx = 0;
				i++;
			}
			shape = Shape[idx];
			for (Sum1 = 0.0, t = 0; t < Len; t++)
				Sum1 += Tap[i + t]*shape[t];
			if (idx == 0) i--;
			Out[o++] = (1.0 - d)*Sum0 + d*Sum1;
			OutdspPhase += OutStep;
		}
	}
	Output.Len = o;
	memmove(Tap, Tap + InpLen, Len*sizeof(double));
	return 0;
}

// ----------------------------------------------------------------------------
// Sliding window (for FFT input)

dspCmpxSlideWindow::dspCmpxSlideWindow()
{
	Buff = NULL;
	Window = NULL;
	ExternWindow = 1;
}

dspCmpxSlideWindow::~dspCmpxSlideWindow()
{
	free(Buff);
	if (!ExternWindow)
		free(Window);
}

void dspCmpxSlideWindow::Free(void)
{
	free(Buff);
	Buff = NULL;
	if (!ExternWindow)
		free(Window);
	Window = NULL;
}

int dspCmpxSlideWindow::Preset(int WindowLen, int SlideDist, double *WindowShape)
{
	int i;
	if (SlideDist > WindowLen) return -1;
	Len = WindowLen;
	Dist = SlideDist;
	if (dspRedspAllocArray(&Buff, Len)) return -1;
	for (i = 0; i < Len; i++)
		Buff[i].re = Buff[i].im = 0.0;
	Ptr = 0;
	if (!ExternWindow)
		free(Window);
	Window = WindowShape;
	ExternWindow = 1;
	return 0;
}

int dspCmpxSlideWindow::SetWindow(double (*NewWindow)(double dspPhase), double Scale)
{
	int idx;
	if (NewWindow == NULL) {
		if (!ExternWindow)
			free(Window);
		Window = NULL;
		ExternWindow = 1;
		return 0;
	}
	if (ExternWindow) {
		Window = NULL;
		ExternWindow = 0;
	}
	if (dspRedspAllocArray(&Window, Len)) return -1;
	for (idx = 0; idx < Len; idx++)
		Window[idx] = (*NewWindow)(2*M_PI*(idx - Len/2 + 0.5)/Len)*Scale;
	return 0;
}

int dspCmpxSlideWindow::Process(dspCmpx_buff *Input)
{
	dspCmpx *Inp = Input->Data;
	int InpLen = Input->Len;
	int i, len, err;
	Output.Len = 0;
	while (InpLen > 0) {
		len = dspIntmin(Len - Ptr, InpLen);
		memcpy(Buff + Ptr, Inp, len*sizeof(dspCmpx));
		Ptr += len;
		Inp += len;
		InpLen -= len;
		if (Ptr >= Len) {
			len = Output.Len;
			err = Output.EnsureSpace(len + Len); if(err) return err;
			if (Window == NULL)
				memcpy(Output.Data, Buff, Len*sizeof(dspCmpx));
			else for (i = 0; i < Len; i++) {
				Output.Data[len + i].re = Buff[i].re*Window[i];
				Output.Data[len + i].im = Buff[i].im*Window[i];
			}
			Output.Len += Len;
			memmove(Buff, Buff + Dist, (Len - Dist)*sizeof(dspCmpx));
			Ptr -= Dist;
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------
// Overlaping window (for IFFT output)

dspCmpxOverlapWindow::dspCmpxOverlapWindow()
{
	Buff = NULL;
	Window = NULL;
	ExternWindow = 1;
}

dspCmpxOverlapWindow::~dspCmpxOverlapWindow()
{
	free(Buff);
	if (!ExternWindow)
		free(Window);
}

void dspCmpxOverlapWindow::Free(void)
{
	free(Buff);
	Buff=NULL;
	if (!ExternWindow)
		free(Window);
	Window = NULL;
}

int dspCmpxOverlapWindow::Preset(int WindowLen, int SlideDist, double *WindowShape)
{
	int i;
	if (SlideDist > WindowLen) return -1;
	Len = WindowLen;
	Dist = SlideDist;
	if (dspRedspAllocArray(&Buff, Len)) return -1;
	for (i = 0; i < Len; i++)
		Buff[i].re = Buff[i].im = 0.0;
	if (!ExternWindow)
		free(Window);
	Window = WindowShape;
	ExternWindow = 1;
	return 0;
}

int dspCmpxOverlapWindow::SetWindow(double (*NewWindow)(double dspPhase), double Scale)
{
	int idx;
	if (NewWindow == NULL) {
		if (!ExternWindow)
			free(Window);
		Window = NULL;
		ExternWindow = 1;
		return 0;
	}
	if (ExternWindow) {
		Window = NULL;
		ExternWindow = 0;
	}
	if (dspRedspAllocArray(&Window, Len)) return -1;
	for (idx = 0; idx < Len; idx++)
		Window[idx] = (*NewWindow)(2*M_PI*(idx - Len/2 + 0.5)/Len)*Scale;
	return 0;
}

int dspCmpxOverlapWindow::Process(dspCmpx_buff *Input)
{
	int i, err;
	Output.Len = 0;
	for (i = 0; i < Input->Len; i += Len) {
		err = Output.EnsureSpace(Output.Len + Dist);
		if (err) return err;
		Process(Input->Data + i, Output.Data + Output.Len);
		Output.Len += Dist;
	}
	return 0;
}

int dspCmpxOverlapWindow::Process(dspCmpx *Input)
{
	int err;
	err = Output.EnsureSpace(Dist);
	if (err) return err;
	Process(Input, Output.Data);
	Output.Len = Dist;
	return 0;
}

void dspCmpxOverlapWindow::Process(dspCmpx *Inp, dspCmpx *Out)
{
	int i;
	if(Window == NULL) {
		for (i = 0; i < Dist; i++) {
			Out[i].re = Buff[i].re + Inp[i].re;
			Out[i].im = Buff[i].im + Inp[i].im;
		}
		for ( ; i < Len - Dist; i++) {
			Buff[i - Dist].re = Buff[i].re + Inp[i].re;
			Buff[i - Dist].im = Buff[i].im + Inp[i].im;
		}
		for ( ; i < Len; i++) {
			Buff[i - Dist].re = Inp[i].re;
			Buff[i - Dist].im = Inp[i].im;
		}
	} else {
		for (i = 0; i < Dist; i++) {
			Out[i].re = Buff[i].re + Inp[i].re*Window[i];
			Out[i].im = Buff[i].im + Inp[i].im*Window[i];
		}
		for ( ; i < Len - Dist; i++) {
			Buff[i - Dist].re = Buff[i].re + Inp[i].re*Window[i];
			Buff[i - Dist].im = Buff[i].im + Inp[i].im*Window[i];
		}
		for ( ; i < Len; i++) {
			Buff[i - Dist].re = Inp[i].re*Window[i];
			Buff[i - Dist].im = Inp[i].im*Window[i]; }
	}
}

int dspCmpxOverlapWindow::ProcessSilence(int Slides)
{
	int err, slide;
	err = Output.EnsureSpace(Slides*Dist);
	if (err) return err;
	Output.Len = 0;
	for (slide = 0; slide < Slides; slide++) {
		memcpy(Output.Data + Output.Len, Buff, Dist*sizeof(dspCmpx));
		memcpy(Buff, Buff + Dist, (Len - Dist)*sizeof(dspCmpx));
		Output.Len += Dist;
	}
	return 0;
}

// ----------------------------------------------------------------------------
// FFT dspPhase corrector

dspFFT_TimeShift::dspFFT_TimeShift()
{
	FreqTable = NULL;
}

dspFFT_TimeShift::~dspFFT_TimeShift()
{
	free(FreqTable);
}

void dspFFT_TimeShift::Free(void)
{
	free(FreqTable);
	FreqTable = NULL;
}

int dspFFT_TimeShift::Preset(int FFTlen, int Backwards)
{
	int i;
	double p;
	dspPhase = 0;
	Len = FFTlen;
	LenMask = FFTlen - 1;
	if ((LenMask^Len) != (2*Len - 1)) return -1;
	if (dspRedspAllocArray(&FreqTable, Len)) return -1;
	for (i = 0; i < Len; i++) {
		p = (2*M_PI*i)/Len;
		if (Backwards) p = (-p);
		FreqTable[i].re = cos(p);
		FreqTable[i].im = sin(p);
	}
	return 0;
}

int dspFFT_TimeShift::Process(dspCmpx *Data, int Time)
{
	double nI, nQ;
	int i, p;
	dspPhase = (dspPhase + Time) & LenMask;
	for (p = i = 0; i < Len; i++) {
		nI = Data[i].re*FreqTable[i].re - Data[i].im*FreqTable[i].im;
		nQ = Data[i].re*FreqTable[i].im + Data[i].im*FreqTable[i].re;
		Data[i].re = nI;
		Data[i].im = nQ;
		p = (p + dspPhase) & LenMask;
	}
	return 0;
}

// ----------------------------------------------------------------------------
// bit synchronizer, the bit rate is the input rate divided by four

dspDiffBitSync4::dspDiffBitSync4(int IntegBits)
{
	int i;
	IntegLen = IntegBits;
	InpTapLen = 4*IntegLen + 8;
	InpTap = (double*)malloc(InpTapLen*sizeof(double));
	for (i = 0; i < InpTapLen; i++)
		InpTap[i] = 0;
	InpTapPtr = 0;
	for (i = 0; i < 4; i++) {
		DiffInteg[i] = DiffInteg0[i] = 0.0;
	}
	DiffTapPtr = 0;
	BitPtr = 0;
	SyncdspPhase = 0.0;
	SyncDrift = SyncDrift0 = 0;
	SyncConfid = 0.0;
	dspLowPass2Coeff((double)IntegLen*2, W1, W2, W5);
}

dspDiffBitSync4::~dspDiffBitSync4()
{
	free(InpTap);
}

void dspDiffBitSync4::Free() { free(InpTap); InpTap=NULL; }

int dspDiffBitSync4::Process(
		double *Inp, int InpLen,
		double *BitOut, double *IbitOut,
		int MaxOutLen, int *OutLen)
{
	int i, o, t, step;
	double diff;
	double Sum, SumI, SumQ, dspPhase;
	for (step = 0,o = i = 0; (i < InpLen) && (o < MaxOutLen); i++) {
		diff = (-InpTap[InpTapPtr++]);
		if (InpTapPtr >= InpTapLen)
			InpTapPtr = 0;
		diff += (InpTap[InpTapPtr] = (*Inp++));
		DiffTapPtr = (DiffTapPtr + 1) & 3;
		dspLowPass2(diff*diff, DiffInteg0[DiffTapPtr], DiffInteg[DiffTapPtr], W1, W2, W5);
		if (DiffTapPtr == BitPtr) {
			for (Sum = 0, t = 0; t < 4; t++)
				Sum += DiffInteg[t];
			t = DiffTapPtr;
			SumI = DiffInteg[t] - DiffInteg[t^2];
			t = (t + 1) & 3;
			SumQ = DiffInteg[t] - DiffInteg[t^2];
			if ((Sum == 0.0) || ((SyncConfid = (SumI*SumI + SumQ*SumQ)/(Sum*Sum)) == 0.0)) {
				(*BitOut++) = 0;
				(*IbitOut++) = 0;
				o++;
				continue;
			}
			dspPhase = atan2(-SumQ, -SumI)*(4/(2*M_PI));
			dspLowPass2(dspPhase - SyncdspPhase, SyncDrift0, SyncDrift, W1, W2, W5);
			SyncdspPhase = dspPhase;
			if (dspPhase > 0.52) {
				step = 1;
				SyncdspPhase -= 1.0;
			} else if (dspPhase < (-0.52)) {
				step = (-1);
				SyncdspPhase += 1.0;
			} else
				step = 0;
			double Samp[5], bit, ibit, dx;
			int p;
			p = InpTapPtr - 4*IntegLen - 2;
			if (p < 0)
				p += InpTapLen;
			for (t = 0; t < 5; t++) {
				Samp[t] = InpTap[p++];
				if (p >= InpTapLen)
					p = 0;
			}
			dx = dspPhase-0.5;
	  // bit=Samp[2]+dx*(Samp[2]-Samp[1]); // linear interpolation
			bit = Samp[2]*(1.0 + dx) - Samp[1]*dx // or quadratic
				+ ((Samp[3] - Samp[1]) - (Samp[2] - Samp[0]))/2*dx*(1.0 + dx)/2;
			ibit = Samp[4] + dx*(Samp[4] - Samp[3]); //linear interpolation is enough
			(*BitOut++) = bit;
			(*IbitOut++) = ibit;
			o++;
		} else if (DiffTapPtr == (BitPtr^2)) {
			BitPtr = (BitPtr + step) & 3;
			step = 0;
		}
	}
	(*OutLen) = o;
	return i;
}

double dspDiffBitSync4::GetSyncConfid()
{
	return 4*SyncConfid;
}

double dspDiffBitSync4::GetSyncDriftRate()
{
	return SyncDrift/4;
}

// ----------------------------------------------------------------------------
// bit slicer, SNR/Tune meter

dspBitSlicer::dspBitSlicer(int IntegBits)
{
	int i;
	TapLen = IntegLen = IntegBits;
	Tap = (double *)malloc(TapLen*sizeof(double));
	for (i = 0; i < TapLen; i++)
		Tap[i] = 0;
	TapPtr = 0;
	for (i = 0; i < 2; i++) {
		Sum[i] = Sum0[i] = 0.0;
		SumSq[i] = SumSq0[i] = 0.0;
		TimeAsym = TimeAsym0 = 0.0;
		dspAmplAsym = dspAmplAsym0 = 0.0;
		Noise[i] = 0;
	}
	dspLowPass2Coeff((double)IntegLen*2, W1, W2, W5);
	PrevBit = PrevIBit = 0.0;
	OptimThres = 0.0;
}

dspBitSlicer::~dspBitSlicer()
{
	free(Tap);
}

int dspBitSlicer::Process(double *Bits, double *IBits, int InpLen, double *OutBits)
{
	int i, l;
	double Bit, soft;
	for (i = 0; i < InpLen; i++) {
		Bit = Bits[i];
		l = Bit > 0;
		dspLowPass2(Bit, Sum0[l], Sum[l], W1, W2, W5);
		dspLowPass2(Bit*Bit, SumSq0[l], SumSq[l], W1, W2, W5);
		Noise[l] = sqrt(SumSq[l] - Sum[l]*Sum[l]);
		if (Noise[0] + Noise[1] <= 0)
			OptimThres = 0;
		else
			OptimThres = (Sum[0]*Noise[1] + Sum[1]*Noise[0]) / (Noise[0] + Noise[1]);
		soft = Tap[TapPtr] - OptimThres; // we could do a better soft-decision
		if (Bit*PrevBit < 0) {
			dspLowPass2(PrevIBit, dspAmplAsym0, dspAmplAsym, W1, W2, W5);
			if (Bit > 0)
				PrevIBit = (-PrevIBit);
			dspLowPass2(PrevIBit, TimeAsym0, TimeAsym, W1, W2, W5);
		}
		(*OutBits++) = soft;
		PrevBit = Bit;
		PrevIBit = IBits[i];
		Tap[TapPtr] = Bit;
		TapPtr++;
		if (TapPtr >= TapLen)
		TapPtr = 0;
	}
	return InpLen;
}

double dspBitSlicer::GetSigToNoise()
{ return Noise[1]>0 ? (Sum[1]-OptimThres)/Noise[1] : 0.0; }

double dspBitSlicer::GetdspAmplAsym()
{ double Sweep=Sum[1]-Sum[0]; return Sweep>0 ? 2*dspAmplAsym/Sweep : 0.0; }

double dspBitSlicer::GetTimeAsym()
{ double Sweep=Sum[1]-Sum[0]; return Sweep>0 ? 2*TimeAsym/Sweep : 0.0; }

// ----------------------------------------------------------------------------
// The decoder for the HDLC frames,
// makes no AX.25 CRC check, only the length in bytes against MinLen and MaxLen
// however it does not pass frames with non-complete bytes.

dspHDLCdecoder::dspHDLCdecoder(
		int minlen, int maxlen, int diff, int invert,
		int chan, int (*handler)(int, char *, int))
{
	MinLen = minlen;
	MaxLen = maxlen;
	RxDiff = diff;
	RxInvert = invert;
	ChanId = chan;
	FrameHandler = handler;
	Buff = (char *)malloc(MaxLen);
	Len = (-1);
	PrevLev = 0;
	ShiftReg = 0;
	BitCount = 0;
	Count1s = 0;
	AllFrameCount = 0;
	BadFrameCount = 0;
}

dspHDLCdecoder::~dspHDLCdecoder()
{
	free(Buff);
}

int dspHDLCdecoder::Process(double *Inp, int InpLen)
{
	int i, lev, bit, Flag;

	for (i = 0; i < InpLen; i++) {
		lev = Inp[i] > 0;
		bit = (lev^(PrevLev & RxDiff))^RxInvert;
		PrevLev = lev;
		ShiftReg = (ShiftReg >> 1) | (bit << 7);
		BitCount += 1;
		Flag = 0;
		if (bit)
			Count1s += 1;
		else {
			if (Count1s >= 7)
				Len = (-1);
			else if (Count1s == 6)
				Flag = 1;
			else if (Count1s == 5) {
				ShiftReg <<= 1;
				BitCount -= 1;
			}
			Count1s = 0;
		}
		if (Flag) {
			if ((Len >= MinLen) && (BitCount == 8))
				(*FrameHandler)(ChanId, Buff, Len);
			Len = 0;
			BitCount = 0;
		} else if (Len >= 0) {
			if (BitCount == 8) {
				if (Len < MaxLen)
					Buff[Len++] = (char)ShiftReg;
				else
					Len = (-1);
				BitCount = 0;
			}
		}
	}
	return InpLen;
}

// ----------------------------------------------------------------------------
// AX.25 CRC, adress decoding, etc.

short unsigned int dspAX25CRCtable[256] = {
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

short unsigned int dspAX25CRC(char *Data, int Len)
{
	int i, idx;
	short unsigned int CRC;
	for (CRC = 0xFFFF, i = 0; i < Len; i++) {
		idx = (unsigned char)CRC^(unsigned char)Data[i];
		CRC = (CRC>>8)^dspAX25CRCtable[idx];
	}
	CRC ^= 0xFFFF;
	return CRC;
}

// ----------------------------------------------------------------------------
// radix-2 FFT

// constructor
dsp_r2FFT::dsp_r2FFT()
{
	BitRevIdx = NULL;
	Twiddle = NULL; /* Window=NULL; */
}

// destructor: free twiddles, bit-reverse lookup and window tables
dsp_r2FFT::~dsp_r2FFT()
{
	free(BitRevIdx);
	free(Twiddle); /* free(Window); */
}

void dsp_r2FFT::Free(void)
{
	free(BitRevIdx);
	BitRevIdx = NULL;
	free(Twiddle);
	Twiddle = NULL;
}

// ..........................................................................

// a radix-2 FFT bufferfly
inline void dsp_r2FFT::FFTbf(dspCmpx &x0, dspCmpx &x1, dspCmpx &W)
{
	dspCmpx x1W;
	x1W.re = x1.re*W.re + x1.im*W.im;	// x1W.re=x1.re*W.re-x1.im*W.im;
	x1W.im = (-x1.re*W.im) + x1.im*W.re; // x1W.im=x1.re*W.im+x1.im*W.re;
	x1.re = x0.re - x1W.re;
	x1.im = x0.im - x1W.im;
	x0.re = x0.re + x1W.re;
	x0.im = x0.im + x1W.im;
}

// 2-point FFT
inline void dsp_r2FFT::FFT2(dspCmpx &x0, dspCmpx &x1)
{
	dspCmpx x1W;
	x1W.re = x1.re;
	x1W.im = x1.im;
	x1.re = x0.re - x1.re;
	x1.im = x0.im - x1.im;
	x0.re += x1W.re;
	x0.im += x1W.im;
}

// 4-point FFT
// beware: these depend on the convention for the twiddle factors !
inline void dsp_r2FFT::FFT4(dspCmpx &x0, dspCmpx &x1, dspCmpx &x2, dspCmpx &x3)
{
	dspCmpx x1W;
	x1W.re = x2.re;
	x1W.im = x2.im;
	x2.re = x0.re - x1W.re;
	x2.im = x0.im - x1W.im;
	x0.re = x0.re + x1W.re;
	x0.im = x0.im + x1W.im;
	x1W.re = x3.im;
	x1W.im = (-x3.re);
	x3.re = x1.re - x1W.re;
	x3.im = x1.im - x1W.im;
	x1.re = x1.re + x1W.re;
	x1.im = x1.im + x1W.im;
}

// ..........................................................................

// bit reverse (in place) the dspSequence (before the actuall FFT)
void dsp_r2FFT::Scramble(dspCmpx x[])
{
	int idx, ridx;
	dspCmpx tmp;
	for (idx = 0; idx < Size; idx++)
		if ((ridx = BitRevIdx[idx]) > idx) {
			tmp = x[idx];
			x[idx] = x[ridx];
			x[ridx] = tmp;
/* printf("%d <=> %d\n",idx,ridx); */
	}
}

// Preset for given processing size
int dsp_r2FFT::Preset(int size)
{
	int err, idx, ridx, mask, rmask;
	double dspPhase;

	if (!dspPowerOf2(size)) goto Error;
	Size = size;
	err = dspRedspAllocArray(&BitRevIdx, Size);
	if (err) goto Error;
	err = dspRedspAllocArray(&Twiddle, Size);
	if (err) goto Error;
//printf("size, %d\n\n", size);
//printf("idx,dspPhase,Twiddle.re,Twiddle.im\n");
	for (idx = 0; idx < Size; idx++) {
		dspPhase = (2*M_PI*idx)/Size;
		Twiddle[idx].re = cos(dspPhase);
		Twiddle[idx].im = sin(dspPhase);
//printf("%2d,%6.4f,%6.4f,%6.4f\n", idx,dspPhase,Twiddle[idx].re,Twiddle[idx].im); 
	}
//printf("\n\nidx,BitRevIdx\n");
	for (ridx = 0, idx = 0; idx < Size; idx++) {
		for (ridx = 0, mask = Size/2, rmask = 1; mask; mask >>= 1, rmask <<= 1) {
			if (idx & mask)
				ridx |= rmask;
		}
		BitRevIdx[idx] = ridx;
//printf("%d,%d\n",idx,ridx);
	}
//  free(Window); Window=NULL; WinInpScale=1.0/Size; WinOutScale=0.5;
	return 0;

Error:
	Free();
	return -1;
}

// ..........................................................................

// radix-2 FFT: the first and the second pass are by hand
// looks like there is no gain by separating the second pass
// and even the first pass is in question ?
void dsp_r2FFT::CoreProc(dspCmpx x[])
{
	int Groups, GroupHalfSize, Group, Bf, TwidIdx;
	int HalfSize = Size/2;
	for (Bf = 0; Bf < Size; Bf += 2)
		FFT2(x[Bf], x[Bf+1]); // first pass
  // for(Bf=0; Bf<Size; Bf+=4) FFT4(x[Bf],x[Bf+1],x[Bf+2],x[Bf+3]); // second
	for (Groups = HalfSize/2, GroupHalfSize = 2; Groups; Groups >>= 1, GroupHalfSize <<= 1)
		for (Group = 0, Bf = 0; Group < Groups; Group++, Bf += GroupHalfSize)
			for (TwidIdx = 0; TwidIdx < HalfSize; TwidIdx += Groups, Bf++) {
				FFTbf(x[Bf], x[Bf + GroupHalfSize], Twiddle[TwidIdx]);
/* printf("%2d %2d %2d\n",Bf,Bf+GroupHalfSize,TwidIdx); */
		}
}

// ..........................................................................

// separate the result of "two reals at one time" processing
void dsp_r2FFT::SeparTwoReals(dspCmpx Buff[], dspCmpx Out0[], dspCmpx Out1[])
{
	int idx, HalfSize = Size/2;
//  for(idx=0; idx<Size; idx++)
//	printf("%2d %9.5f %9.5f\n",idx,Buff[idx].re,Buff[idx].im);
	Out0[0].re = Buff[0].re;
	Out1[0].re = Buff[0].im;
	for (idx = 1; idx < HalfSize; idx++) {
		Out0[idx].re = Buff[idx].re + Buff[Size-idx].re;
		Out0[idx].im = Buff[idx].im - Buff[Size-idx].im;
		Out1[idx].re = Buff[idx].im + Buff[Size-idx].im;
		Out1[idx].im = (-Buff[idx].re) + Buff[Size-idx].re;
	}
	Out0[0].im = Buff[HalfSize].re;
	Out1[0].im = Buff[HalfSize].im;
//  for(idx=0; idx<HalfSize; idx++)
//	printf("%2d  %9.5f %9.5f  %9.5f %9.5f\n",
//	  idx,Out0[idx].re,Out0[idx].im,Out1[idx].re,Out1[idx].im);
}

// the oposite of SeparTwoReals()
// but we NEGATE the .im part for Inverse FFT
// as a "by-product" we multiply the transform by 2
void dsp_r2FFT::JoinTwoReals(dspCmpx Inp0[], dspCmpx Inp1[], dspCmpx Buff[])
{
	int idx, HalfSize = Size/2;
//  for(idx=0; idx<HalfSize; idx++)
//	printf("%2d  %9.5f %9.5f  %9.5f %9.5f\n",
//	  idx,Inp0[idx].re,Inp0[idx].im,Inp1[idx].re,Inp1[idx].im);
	Buff[0].re = 2*Inp0[0].re;
	Buff[0].im = (-2*Inp1[0].re);
	for (idx = 1; idx < HalfSize; idx++) {
		Buff[idx].re = Inp0[idx].re - Inp1[idx].im;
		Buff[idx].im = (-Inp0[idx].im) - Inp1[idx].re;
		Buff[Size-idx].re = Inp0[idx].re + Inp1[idx].im;
		Buff[Size-idx].im = Inp0[idx].im - Inp1[idx].re;
	}
	Buff[HalfSize].re = 2*Inp0[0].im;
	Buff[HalfSize].im = (-2*Inp1[0].im);
//  for(idx=0; idx<Size; idx++)
//	printf("%2d %9.5f %9.5f\n",idx,Buff[idx].re,Buff[idx].im);
}

// ----------------------------------------------------------------------------
// Sliding window FFT for spectral analysis
// input: real-valued time-domain signal,
// output: complex-valued Fourier Transform
//
// We use a little trick here to process two real-valued FFT
// in one go using the complex FFT engine.
// This cuts the CPU but makes the input->output dspDelay longer.

dspSlideWinFFT::dspSlideWinFFT()
{
	SlideBuff = NULL;
	FFTbuff = NULL;
	Window = NULL;
	ExternWindow = 1;
}

dspSlideWinFFT::~dspSlideWinFFT()
{
	free(SlideBuff);
	free(FFTbuff);
	if (!ExternWindow)
	 free(Window);
}

void dspSlideWinFFT::Free(void)
{
	free(SlideBuff);
	SlideBuff = NULL;
	free(FFTbuff);
	FFTbuff = NULL;
	if (!ExternWindow)
		free(Window);
	Window = NULL;
	ExternWindow = 1;
	Output.Free();
}

int dspSlideWinFFT::Preset(int size, int step, double *window)
{
	int err,i;

	Size = size;
	SizeMask = Size - 1;
	err = FFT.Preset(Size);
	if (err) goto Error;

	if (!ExternWindow) {
		free(Window);
		ExternWindow = 1;
	}
	Window = window;

	err = dspRedspAllocArray(&FFTbuff, Size);
	if (err) goto Error;

	err = dspRedspAllocArray(&SlideBuff, Size);
	if (err) goto Error;
	for (i = 0; i < Size; i++)
		SlideBuff[i] = 0.0;
	SlidePtr = 0;
	Slide = 0;
	Dist = step;
	Left = Dist;

	return 0;

Error:
	Free();
	return -1;
}

int dspSlideWinFFT::SetWindow(double (*NewWindow)(double dspPhase), double Scale)
{
	int idx, err;
	if (NewWindow == NULL) {
		if (!ExternWindow)
			free(Window);
		Window = NULL;
		ExternWindow = 1;
		return 0;
	}
	if (ExternWindow) {
		Window = NULL;
		ExternWindow = 0;
	}
	err = dspRedspAllocArray(&Window, Size);
	if (err) return -1;
	for (idx = 0; idx < Size; idx++)
		Window[idx] = Scale*(*NewWindow)(2*M_PI*(idx - Size/2 + 0.5)/Size);
	return 0;
}

int dspSlideWinFFT::Preset(
		int size, int step,
		double (*NewWindow)(double dspPhase), double Scale)
{
	int err;
	err = Preset(size, step, (double *)NULL);
	if (err) return -1;
	err = SetWindow(NewWindow, Scale);
	if (err) {
		Free();
		return -1;
	}
	return 0;
}

int dspSlideWinFFT::SetWindow(double *window)
{
	if (!ExternWindow) {
		free(Window);
		ExternWindow = 1;
	}
	Window = window;
	return 0;
}

int dspSlideWinFFT::Process(double_buff *Input)
{
	int err, len, i, t;
	int InpLen;
	double *Inp;
	Inp = Input->Data;
	InpLen = Input->Len;
	Output.Len = 0;
	while (InpLen) {
		for (i = len = dspIntmin(InpLen, Left); i; i--) {
			SlideBuff[SlidePtr++] = (*Inp++);
			SlidePtr &= SizeMask;
		}
		InpLen -= len;
		Left -= len;
		if(Left==0) {
			Slide ^= 1;
			Left = Dist;
			if (Slide) {
				for (t = 0, i = SlidePtr; i < Size; t++,i++)
					FFTbuff[t].re = Window[t]*SlideBuff[i];
				for (i = 0; t < Size; t++, i++)
					FFTbuff[t].re = Window[t]*SlideBuff[i];
			} else {
				for (t = 0, i = SlidePtr; i < Size; t++,i++)
					FFTbuff[t].im = Window[t]*SlideBuff[i];
				for (i = 0; t < Size; t++,i++)
					FFTbuff[t].im = Window[t]*SlideBuff[i];
				FFT.Scramble(FFTbuff);
				FFT.CoreProc(FFTbuff);
				len = Output.Len;
				err = Output.EnsureSpace(len + Size);
				if (err) return -1;
				FFT.SeparTwoReals(FFTbuff, Output.Data + len, Output.Data + len + Size/2);
				Output.Len += Size;
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
// the output will be an exact copy (only dspDelayed) of the input.

dspSlideWinFFTproc::dspSlideWinFFTproc()
{
	SlideBuff = NULL;
	OvlapBuff = NULL;
	FFTbuff = NULL;
	Spectr[0] = NULL;
	Spectr[1] = NULL;
	Window = NULL;
	ExternWindow = 1;
}

dspSlideWinFFTproc::~dspSlideWinFFTproc()
{
	free(SlideBuff);
	free(OvlapBuff);
	free(FFTbuff);
	free(Spectr[0]);
	free(Spectr[1]);
	if (!ExternWindow)
		free(Window);
}

void dspSlideWinFFTproc::Free(void)
{
	int i;
	free(SlideBuff);
	SlideBuff=NULL;
	free(OvlapBuff);
	OvlapBuff=NULL;
	free(FFTbuff);
	FFTbuff = NULL;
	for (i = 0; i < 2; i++) {
		free(Spectr[0]);
		Spectr[0] = NULL;
	}
	if (!ExternWindow)
		free(Window);
	Window = NULL;
	ExternWindow = 1;
	Output.Free();
}

int dspSlideWinFFTproc::Preset(
		int size, int step,
		void (*proc)(dspCmpx *Spectra, int Len), double *window)
{
	int err, i;

	Size = size;
	SizeMask = Size - 1;
	err = FFT.Preset(Size);
	if (err) goto Error;

	if (!ExternWindow) {
		free(Window);
		ExternWindow = 1;
	}
	Window = window;

	dspRedspAllocArray(&FFTbuff, Size);
	if (err) goto Error;

	for (i = 0; i < 2; i++) {
		err = dspRedspAllocArray(&Spectr[i], Size/2);
		if (err) goto Error;
	}

	err = dspRedspAllocArray(&SlideBuff, Size);
	if (err) goto Error;
	for (i = 0; i < Size; i++)
		SlideBuff[i] = 0.0;
	SlidePtr = 0;
	Slide = 0;
	Dist = step;
	Left = Dist;

	err = dspRedspAllocArray(&OvlapBuff, Size);
	if (err) goto Error;
	for (i = 0; i < Size; i++)
		OvlapBuff[i] = 0.0;
	OvlapPtr = 0;

	SpectraProc = proc;

	return 0;

Error:
	Free();
	return -1;
}

int dspSlideWinFFTproc::Preset(int size, int step,
		void (*proc)(dspCmpx *Spectra, int Len),
		double (*NewWindow)(double dspPhase), double Scale)
{
	int err;
	err = Preset(size, step, proc, (double *)NULL);
	if (err) return -1;

	err = SetWindow(NewWindow, Scale);
	if (err) {
		Free();
		return -1;
	}
	return 0;
}

int dspSlideWinFFTproc::SetWindow(double *window)
{
	if (!ExternWindow) {
		free(Window);
		ExternWindow = 1;
	}
	Window = window;
	return 0;
}

int dspSlideWinFFTproc::SetWindow(double (*NewWindow)(double dspPhase), double Scale)
{
	int idx, err;
	if (NewWindow == NULL) {
		if (!ExternWindow)
			free(Window);
		Window = NULL;
		ExternWindow = 1;
		return 0;
	}
	if (ExternWindow) {
		Window = NULL;
		ExternWindow = 0;
	}

	err = dspRedspAllocArray(&Window, Size);
	if (err) return -1;

	if (Scale == 0.0)
		Scale = sqrt(0.5/Size);
	for (idx = 0; idx < Size; idx++)
		Window[idx] = Scale*(*NewWindow)(2*M_PI*(idx - Size/2 + 0.5)/Size);
	return 0;
}

int dspSlideWinFFTproc::Process(double_buff *Input)
{
	int err, len, i, t;
	int InpLen;
	double *Inp, *Out;
	Inp = Input->Data;
	InpLen = Input->Len;
	Output.Len = 0;
	while (InpLen) {
		for (i = len = dspIntmin(InpLen, Left); i; i--) {
			SlideBuff[SlidePtr++] = (*Inp++);
			SlidePtr &= SizeMask;
		}
		InpLen -= len;
		Left -= len;
		if (Left == 0) {
			Slide ^= 1;
			Left = Dist;
			if (Slide) {
				for (t = 0, i = SlidePtr; i < Size; t++,i++)
					FFTbuff[t].re = Window[t]*SlideBuff[i];
				for (i = 0; t < Size; t++,i++)
					FFTbuff[t].re = Window[t]*SlideBuff[i];
			} else {
				for (t = 0, i = SlidePtr; i < Size; t++,i++)
					FFTbuff[t].im = Window[t]*SlideBuff[i];
				for (i = 0; t < Size; t++,i++)
					FFTbuff[t].im = Window[t]*SlideBuff[i];

				FFT.Scramble(FFTbuff);
				FFT.CoreProc(FFTbuff);
				FFT.SeparTwoReals(FFTbuff, Spectr[0], Spectr[1]);

				for (i = 0; i < 2; i++)
					(*SpectraProc)(Spectr[i], Size);

				FFT.JoinTwoReals(Spectr[0], Spectr[1], FFTbuff);
				FFT.Scramble(FFTbuff);
				FFT.CoreProc(FFTbuff);

				err = Output.EnsureSpace(Output.Len + 2*Dist);
				if (err) return -1;
				Out = Output.Data + Output.Len;

				for (t = 0, i = OvlapPtr; i < Size; t++,i++)
					OvlapBuff[i] += Window[t]*FFTbuff[t].re;

				for (i = 0; t < Size; t++, i++)
					OvlapBuff[i] += Window[t]*FFTbuff[t].re;

				for (i = 0; i < Dist; i++) {
					(*Out++) = OvlapBuff[OvlapPtr];
					OvlapBuff[OvlapPtr++] = 0.0;
					OvlapPtr &= SizeMask;
				}

				for (t = 0, i = OvlapPtr; i < Size; t++,i++)
					OvlapBuff[i] -= Window[t]*FFTbuff[t].im;

				for (i = 0; t < Size; t++,i++)
					OvlapBuff[i] -= Window[t]*FFTbuff[t].im;

				for (i = 0; i < Dist; i++) {
					(*Out++) = OvlapBuff[OvlapPtr];
					OvlapBuff[OvlapPtr++] = 0.0;
					OvlapPtr &= SizeMask;
				}

				Output.Len += 2*Dist;
			}
		}
  }
  return 0;
}



// ----------------------------------------------------------------------------
// Walsh (Hadamard ?) transform.

void dspWalshTrans(double *Data, int Len)  // Len must be 2^N
{
	int step, ptr, ptr2;
	double bit1, bit2;
	for (step = 1; step < Len; step *= 2) {
		for (ptr = 0; ptr < Len; ptr += 2*step) {
			for (ptr2 = ptr; (ptr2 - ptr) < step; ptr2 += 1) {
				bit1 = Data[ptr2];
				bit2 = Data[ptr2 + step];
//	Data[ptr2]=(bit1+bit2); Data[ptr2+step]=(bit1-bit2);
				Data[ptr2] = (bit1 + bit2);
				Data[ptr2 + step] = (bit2 - bit1);
			}
		}
	}
}

void dspWalshInvTrans(double *Data, int Len)  // Len must be 2^N
{
	int step, ptr, ptr2;
	double bit1, bit2;
	for (step = Len/2; step; step /= 2) {
		for (ptr = 0; ptr < Len; ptr += 2*step) {
			for (ptr2 = ptr; (ptr2 - ptr) < step; ptr2 += 1) {
				bit1 = Data[ptr2];
				bit2 = Data[ptr2 + step];
//	Data[ptr2]=(bit1+bit2); Data[ptr2+step]=(bit1-bit2);
				Data[ptr2] = (bit1 - bit2);
				Data[ptr2 + step] = (bit1 + bit2);
			}
		}
	}
}

// ----------------------------------------------------------------------------


