// ----------------------------------------------------------------------------
// outputencoder.cxx  --  output charset conversion
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

#include <cstring>
#include <iostream>
#include <string>
#include <tiniconv.h>
#include <outputencoder.h>

#include "config.h"
#include "debug.h"

using namespace std;

/*
 OutputEncoder accepts UTF-8 strings at input, converts them to the
 selected encoding and outputs them one character at a time.
*/


/*
 Constructor. Look up tiniconv.h for the list of possible values of
 charset_in.
*/
OutputEncoder::OutputEncoder(const int charset_out, unsigned int buffer_size)
{
	this->buffer_size = buffer_size;
	buffer = new unsigned char[buffer_size];
	encoding_ptr = buffer;
	pop_ptr = buffer;
	set_output_encoding(charset_out);
}


/*
 Destructor.
*/
OutputEncoder::~OutputEncoder(void)
{
	delete[] buffer;
}


/*
 Set output encoding. Look up tiniconv.h for the list of possible values of
 charset_in.
*/
void OutputEncoder::set_output_encoding(const int charset_out)
{
	tiniconv_init(TINICONV_CHARSET_UTF_8, charset_out,	TINICONV_OPTION_IGNORE_OUT_ILSEQ, &ctx);
}


/*
 Push input data into the encoder.
*/
void OutputEncoder::push(string s)
{
	int available = buffer_size - (encoding_ptr - buffer);
	int consumed_in;
	int consumed_out;

	int status = tiniconv_convert(&ctx,
		(unsigned char*)s.data(), s.length(), &consumed_in,
		encoding_ptr, available, &consumed_out);
	if (status != TINICONV_CONVERT_OK) {
		LOG_ERROR("Error %s",
			status == TINICONV_CONVERT_IN_TOO_SMALL  ? "input too small" :
			status == TINICONV_CONVERT_OUT_TOO_SMALL ? "output too small" :
			status == TINICONV_CONVERT_IN_ILSEQ ? "input illegal sequence" :
			status == TINICONV_CONVERT_OUT_ILSEQ ? "output illegal sequence" :
			"unknown error");
		return;
	}

	encoding_ptr += consumed_out;

	if (consumed_in < (int)s.length())
	{
		// All input data was not consumed, possibly because the
		// output buffer was too small. Try to vacuum the buffer,
		// i.e., remove the data that was already pop()ed.
		memmove(buffer, pop_ptr, buffer + buffer_size - pop_ptr);
		encoding_ptr -= (pop_ptr - buffer);
		pop_ptr = buffer;

		// Now try again; fingers crossed. We don't check for
		// success anymore, because there is nothing that we can do
		// if the buffer is still too small.
		int available = buffer_size - (encoding_ptr - buffer);
		tiniconv_convert(&ctx,
			(unsigned char*)s.data()+consumed_in, s.length()-consumed_in, &consumed_in,
			encoding_ptr, available, &consumed_out);

		encoding_ptr += consumed_out;
	}
}


/*
 Pop a single character of the encoded data.
 Returns -1 in case there is no data available.
*/
const unsigned int OutputEncoder::pop(void)
{
	if (pop_ptr == encoding_ptr)
		return(-1);

	unsigned int c = *pop_ptr++;

	// Note that by only advancing pop_ptr, we leave stale data at the
	// beginning of the buffer, so sooner or later it will clutter up.
	// If there is no data left to send, both encoding_ptr and pop_ptr
	// can be safely reset to the beginning of the buffer; we handle
	// this trivial case here. More thorough vacuuming will be performed
	// in push() if the need arises.
	if (pop_ptr == encoding_ptr)
		pop_ptr = encoding_ptr = buffer;

	return(c);
}
