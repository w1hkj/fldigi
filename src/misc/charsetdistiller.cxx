// ----------------------------------------------------------------------------
// charsetdistiller.cxx  --  input charset cleaning and conversion
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

#include <config.h>

#include <cstring>
#include <string>

#include "debug.h"
#include "charsetdistiller.h"
#include "tiniconv.h"

using namespace std;

/*
    CharsetDistiller

 This class implements a charset "distiller" that receives input data one
 byte at a time and converts this data stream from a particular character
 set into UTF-8. Invalid input data is treated as if it was encoded in
 CP1252. Character set conversion is performed as soon as possible, i.e.,
 when enough input is received to constitute a valid character in the input
 character set, this character is immediatly converted into UTF-8 and made
 available at the output.
 */


/*
 The constructor. Look up tiniconv.h for the list of possible values of
 charset_in.
*/
CharsetDistiller::CharsetDistiller(const int charset_in)
{
   bufptr = buf;
   nutf8 = 0;
   tiniconv_init(charset_in, TINICONV_CHARSET_UTF_8, 0, &ctx);
   tiniconv_init(TINICONV_CHARSET_CP1252, TINICONV_CHARSET_UTF_8, TINICONV_OPTION_IGNORE_IN_ILSEQ, &ctx1252);
}


/*
 Change the input encoding. Look up tiniconv.h for the list of possible
 values of charset_in.

 Returns 0 if successful or -1 in case of error.
 */
int CharsetDistiller::set_input_encoding(const int charset_in)
{
   flush();
   return tiniconv_init(charset_in, TINICONV_CHARSET_UTF_8, 0, &ctx);
}


/*
 Receive a single byte of input data and make an immediate conversion
 attempt.
 */
void CharsetDistiller::rx(const unsigned char c)
{
   *bufptr++ = c;
   process_buffer();
}


/*
 Receive a zero-terminated string of input data.
 
 This is a convenience method: it merely feeds the string into the distiller
 one byte at a time.
 */
void CharsetDistiller::rx(const unsigned char *c)
{
   const unsigned char *ptr;
   for (ptr = c; *ptr != 0; ptr++)
      rx(*ptr);
}


/*
 Examine the input buffer and decide on the possible actions (construct an
 UTF-8 character, interpret the bytes as invalid input etc.)
 */
void CharsetDistiller::process_buffer(void)
{
   bool again = true;
   
   while (again)
   {
      if (bufptr == buf)
      {
         // the buffer is empty
         return;
      }
      
      int convert_status;
      int consumed_in;
      int consumed_out;
      unsigned char outbuf[6];

      convert_status = tiniconv_convert(&ctx, buf, (bufptr - buf), &consumed_in, outbuf, sizeof(outbuf), &consumed_out);
      
      if (consumed_out)
      {
         // Append the converted data to the output string.
         outdata.append(reinterpret_cast<char *>(outbuf), consumed_out);
         
         // Count the number of converted UTF-8 characters (by counting the
         // number of bytes that are not continuation bytes).
         for (unsigned char *iptr = outbuf; iptr < outbuf + consumed_out; iptr++)
         {
            if ((*iptr & 0xc0) != 0x80)
               nutf8++;
         }
         
         // If not all input was consumed, move the remaining data to the
         // beginning of the buffer
         if (bufptr - buf > consumed_in)
         {
            memmove(buf, buf + consumed_in, bufptr - buf - consumed_in);
            bufptr -= consumed_in;
         }
         else
            bufptr = buf;
      }
      
      again = false;
      
      if (convert_status == TINICONV_CONVERT_OK)
      {
         // Successful conversion, nothing else to do.
         return;
      }
      else if (convert_status == TINICONV_CONVERT_IN_TOO_SMALL)
      {
         // Partial data left in the input buffer. We can't proceed with the
         // conversion until we get more input.
         return;
      }
      else if (convert_status == TINICONV_CONVERT_IN_ILSEQ)
      {
         // Invalid sequence in input; spit out the offending byte and try again.
         shift_first_out();
         again = true;
      }
      else if (convert_status == TINICONV_CONVERT_OUT_TOO_SMALL)
      {
         // More characters were available than could be converted in one
         // go. Have another round.
         again = true;
      }
      // The following two cases should never happen.
      else if (convert_status == TINICONV_CONVERT_OUT_ILSEQ)
      {
         LOG_ERROR("Character not representable in UTF-8? Is this possible?");
         bufptr = buf;
         return;
      }
      else
      {
         LOG_ERROR("Unknown tiniconv return value %d.", convert_status);
         bufptr = buf;
         return;
      }
   }
}


/*
 Convert the first byte of the input buffer; treat it as if it was encoded
 in CP1252
 */
void CharsetDistiller::shift_first_out(void)
{
   int consumed_in;
   int consumed_out;
   unsigned char outbuf[6];

   tiniconv_convert(&ctx1252, buf, 1, &consumed_in, outbuf, sizeof(outbuf), &consumed_out);

   outdata.append(reinterpret_cast<char *>(outbuf), consumed_out);
   nutf8++;
   
   memmove(buf, buf+1, (bufptr - buf - 1));
   bufptr--;
}


/*
 Flush input. Recode the input data left in the buffer in whatever way
 necessary to make the buffer empty.
 */
void CharsetDistiller::flush(void)
{
   while (bufptr > buf)
      shift_first_out();
}


/*
 Reset input buffer. All data still waiting in the input buffer is lost.
 Data already converted and waiting at the output is not affected.
*/
void CharsetDistiller::reset(void)
{
	bufptr = buf;
}


/*
 Clear the output buffer.
 */
void CharsetDistiller::clear(void)
{
   outdata.clear();
   nutf8 = 0;
}


/*
 Return the number of bytes available in the output buffer.
 */
int CharsetDistiller::data_length(void)
{
   return outdata.length();
}


/*
 Return the number of UTF-8 characters in the output buffer.
 */
int CharsetDistiller::num_chars(void)
{
   return nutf8;
}


/*
 Return a reference to the output buffer.
 */
const string &CharsetDistiller::data(void)
{
   return outdata;
}
