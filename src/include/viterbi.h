// ----------------------------------------------------------------------------
// viterbi.h  --  Viterbi decoder
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  These filters were adapted from code contained
// in the gmfsk source code distribution.
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


#ifndef VITERBI_H
#define VITERBI_H

#define PATHMEM 64

class viterbi  {
protected:
	int mettab[2][256];
	viterbi( int poly1, int poly2, unsigned int * output, int outsize );
public:
	virtual ~viterbi() ;

	/// CPU cost of this virtual is negligible compared to the rest.
	virtual int decode(const unsigned char * sym, int * __restrict__ metric = NULL) = 0;

	template<	int k,
			int _chunksize = 8,
			int _traceback = PATHMEM - 1 >
	class impl ;

};

/// Real implementation of viterbi interface.
template<	int k,
		int _chunksize,
		int _traceback >
class viterbi::impl : public viterbi {
	// Find the state with the best metric
	// We are sure that nstates is even and greater or equal than 2
	int best_metric(int p) const {
		const int *  __restrict__ metrics_p = metrics[p];
		int bestmetric0 = metrics_p[0];
		int beststate0 = 0;
		int bestmetric1 = metrics_p[1];
		int beststate1 = 1;

		/// Loop is unrolled with two tests at a time.
		for (int i = 2; i < nstates; i+=2) {
			int metrics_p_i_0 = metrics_p[i];
			if (metrics_p_i_0 > bestmetric0) {
				bestmetric0 = metrics_p_i_0;
				beststate0 = i;
			}
			int metrics_p_i_1 = metrics_p[i+1];
			if (metrics_p_i_1 > bestmetric1) {
				bestmetric1 = metrics_p_i_1;
				beststate1 = i+1;
			}
		}
		return bestmetric0 > bestmetric1 ? beststate0 : beststate1 ;
	}

	// n is always even because k is always bigger than 1,
	// because the number of states must be at least equal to 2.
	static const int nstates = 1 << ( k - 1 );
	static const int outsize = 1 << k ;
	/// Stores values between zero and three.
	unsigned int output[outsize];
	int metrics[PATHMEM][nstates];
	int history[PATHMEM][nstates];
	int sequence[PATHMEM];
	unsigned int ptr;

	int traceback(int * __restrict__ metric)
	{
		unsigned int p = ptr ? ptr - 1 : PATHMEM - 1 ;

		// Trace back 'traceback' steps, starting from the best state
		sequence[p] = best_metric(p);

		int delta = p - _traceback ;
		unsigned int limit = delta > 0 ? delta : 0 ;
		for( ; p > limit ; --p )
			sequence[p-1] = history[p][sequence[p]];

		if( delta < 0 ) {
			sequence[PATHMEM-1] = history[0][sequence[0]];
			limit = PATHMEM + delta ;
			for( p = PATHMEM-1 ; p > limit ; --p )
				sequence[p-1] = history[p][sequence[p]];
		}

		if (metric)
			*metric = metrics[p][sequence[p]];

		// Decode 'chunksize' bits
		unsigned int c = 0;
		for (int i = 0; i < _chunksize; i++) {
			// low bit of state is the previous input bit
			c = (c << 1) | (sequence[p] & 1);
			p = ( p == PATHMEM - 1 ) ? 0 : p + 1 ;
		}

		if (metric)
			*metric = metrics[p][sequence[p]] - *metric;

		return c;
	}

public:
	impl(int poly1, int poly2 ) : viterbi( poly1, poly2, output, outsize )
	{
		/// Will be eliminated at compile-time if OK.
		if (_traceback > PATHMEM - 1) abort();
		if (_chunksize > _traceback) abort();

		memset( sequence, 0, sizeof(int) * PATHMEM );
		memset( metrics, 0, nstates * sizeof(int) * PATHMEM );
		memset( history, 0, nstates * sizeof(int) * PATHMEM );
		ptr = 0;
	}

	~impl() {}

	int decode(const unsigned char *sym, int * __restrict__ metric = NULL)
	{
		unsigned int currptr = ptr;
		unsigned int prevptr = currptr ? currptr - 1 : PATHMEM - 1 ;

		const int sym0 = sym[0], sym1 = sym[1];
		const int * __restrict__ mettab0 = viterbi::mettab[0];
		const int * __restrict__ mettab1 = viterbi::mettab[1] ;
		const int met[4] = {
			mettab0[sym1] + mettab0[sym0],
			mettab0[sym1] + mettab1[sym0],
			mettab1[sym1] + mettab0[sym0],
			mettab1[sym1] + mettab1[sym0] } ;

		const int * __restrict__ metrics_prevptr = metrics[prevptr];
		int * __restrict__ metrics_currptr = metrics[currptr];
		int * __restrict__ history_currptr = history[currptr];
		// n and nstates are always even.
		for (int n = 0; n < nstates; n+=2) {
			/// p0 and p1 do not change if n = n+1, because n is even.
			int p0 = n >> 1;
			/// Equal to (n + nstates)>> 1 because n and nstates are even.
			int p1 = p0 + ( nstates >> 1 );

			int metrics_p0 = metrics_prevptr[p0];
			int metrics_p1 = metrics_prevptr[p1];

			int m0 = metrics_p0 + met[output[n]];
			int m1 = metrics_p1 + met[output[n + nstates]];

			bool m0_gt_1 = m0 > m1 ;
			metrics_currptr[n] = m0_gt_1 ? m0 : m1 ;
			history_currptr[n] = m0_gt_1 ? p0 : p1 ;

			m0 = metrics_p0 + met[output[n + 1]];
			m1 = metrics_p1 + met[output[n + 1 + nstates]];

			m0_gt_1 = m0 > m1 ;
			metrics_currptr[n + 1] = m0_gt_1 ? m0 : m1 ;
			history_currptr[n + 1] = m0_gt_1 ? p0 : p1 ;
		}

		ptr = ( ptr == PATHMEM - 1 ) ? 0 : ptr + 1 ;

		if ((ptr % _chunksize) == 0)
			return traceback(metric);

		if (metrics[currptr][0] > INT_MAX / 2) {
			for (int i = 0; i < PATHMEM; i++)
				for (int j = 0; j < nstates; j++)
					metrics[i][j] -= INT_MAX / 2;
		}
		/// This test or the previous, but not both.
		else if (metrics[currptr][0] < INT_MIN / 2) {
			for (int i = 0; i < PATHMEM; i++)
				for (int j = 0; j < nstates; j++)
					metrics[i][j] += INT_MIN / 2;
		}

		return -1;
	}
};


class encoder {
private:
	int *output;
	unsigned int shreg;
	unsigned int shregmask;
public:
	encoder(int k, int poly1, int poly2);
	~encoder();
	int encode(int bit);
};

#endif
