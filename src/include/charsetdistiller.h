// ----------------------------------------------------------------------------
// charsetdistiller.h  --  input charset cleaning and conversion
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

#ifndef CHARSETDISTILLER_H
#define CHARSETDISTILLER_H

#include <string>
#include "tiniconv.h"

class CharsetDistiller
{
   public:
      CharsetDistiller(const int charset_in);
      int set_input_encoding(const int charset_in);
      void rx(const unsigned char c);
      void rx(const unsigned char *c);
      void flush();
      void reset(void);
      void clear(void);
      int data_length(void);
      int num_chars(void);
      const std::string &data(void);

   private:
      void process_buffer(void);
      void shift_first_out();

      unsigned char buf[6];	// input buffer
      unsigned char *bufptr;	// points to the next unused byte in the buffer
      tiniconv_ctx_s ctx;	// libtiniconv conversion state for input encoding -> UTF-8
      tiniconv_ctx_s ctx1252;	// libtiniconv conversion state for CP1252 -> UTF-8
      std::string outdata;	// valid data
      int nutf8;		// number of UTF-8 characters in the output buffer
};

#endif
