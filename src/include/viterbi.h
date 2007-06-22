// ----------------------------------------------------------------------------
// viterbi.h  --  Viterbi decoder
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  These filters were adapted from code contained
// in the gmfsk source code distribution.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------


#ifndef VITERBI_H
#define VITERBI_H

#define PATHMEM 64

class viterbi  {
private:
	int _traceback;
	int _chunksize;
	int nstates;
	int *output;
	int *metrics[PATHMEM];
	int *history[PATHMEM];
	int sequence[PATHMEM];
	int mettab[2][256];
	unsigned int ptr;
	int traceback(int *metric);
public:
	viterbi(int k, int poly1, int poly2);
	~viterbi();
	void reset();
	int settraceback(int trace);
	int setchunksize(int chunk);
	int decode(unsigned char *sym, int *metric);
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
