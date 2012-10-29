// ----------------------------------------------------------------------------
// outputencoder.h  --  output charset conversion
//
// Copyright (C) 2012
//		Andrej Lajovic, S57LN
//
// This file is part of fldigi.
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

#ifndef OUTPUTENCODER_H
#define OUTPUTENCODER_H

#include <string>
#include <tiniconv.h>

class OutputEncoder
{
	public:
		OutputEncoder(const int charset_out, unsigned int buffer_size = 32);
		~OutputEncoder(void);
		void set_output_encoding(const int charset_out);
		void push(std::string s);
		const unsigned int pop(void);
		
	private:
		unsigned int buffer_size;
		unsigned char *buffer;
		unsigned char *encoding_ptr;
		unsigned char *pop_ptr;
		unsigned int data_length;
		tiniconv_ctx_s ctx;
};

#endif
