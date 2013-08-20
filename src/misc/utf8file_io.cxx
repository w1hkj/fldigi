// ----------------------------------------------------------------------------
//      utf8file_io.cxx
//
// Copyright (C) 2012
//              Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <FL/Fl.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/fl_ask.H>

#include "utf8file_io.h"
#include "icons.h"

#define linelen 1024

//----------------------------------------------------------------------
// filter that produces, from an input stream fed by reading from fp,
// a UTF-8-encoded output stream written in buffer.
// Input can be UTF-8. If it is not, it is decoded with CP1252.
// Output is UTF-8.
// *input_was_changed is set to true if the input was not strict UTF-8 
// so output differs from input.
//----------------------------------------------------------------------

#include <FL/fl_utf8.h>

static int utf8_read_(
			char *buffer, int buflen,
			char *line, int sline, char *endline,
			FILE *fp,
			bool *input_was_changed )
{
	char *p, *q, multibyte[5];
	int l, lp, lq, r;
	unsigned u;
	p = line;
	q = buffer;
	while (q < buffer + buflen) {
		if (p >= endline) {
			r = fread(line, 1, sline, fp);
			endline = line + r; 
			if (r == 0) return q - buffer;
			p = line;
		}
		l = fl_utf8len1(*p);
		if (p + l > endline) {
			memmove(line, p, endline - p);
			endline -= (p - line);
			r = fread(endline, 1, sline - (endline - line), fp);
			endline += r;
			p = line;
			if (endline - line < l) break;
		}
		while ( l > 0) {
			u = fl_utf8decode(p, p+l, &lp);
			lq = fl_utf8encode(u, multibyte);
			if (lp != l || lq != l) *input_was_changed = true;
			if (q + lq > buffer + buflen) {
				memmove(line, p, endline - p);
				endline -= (p - line);
				return q - buffer;
			}
			memcpy(q, multibyte, lq);
			q += lq; 
			p += lp;
			l -= lp;
		}
	}
	memmove(line, p, endline - p);
	endline -= (p - line);
	return q - buffer;
}

static const char file_encoding_warning_message[] = 
"Input file was not UTF-8 encoded.\n"
"Text has been converted to UTF-8.";

//----------------------------------------------------------------------
// Read text from a file.
// utf8_input_filter accepts UTF-8 or CP1252 as input encoding.
// Output is always UTF-8 encoded std::string
//----------------------------------------------------------------------

int UTF8_readfile(const char *file, std::string &output)
{
	FILE *fp;
	if (!(fp = fl_fopen(file, "r")))
		return 1;
	char buffer[2 * linelen + 1], line[linelen];
	char *endline = line;
	int l;
	bool input_file_was_transcoded = false;

	while (true) {
		l = utf8_read_(
			buffer, linelen * 2 + 1,
			line, linelen, endline,
			fp,
			&input_file_was_transcoded);
		if (l == 0) break;
		buffer[l] = 0;
		output.append(buffer);
	}
	int e = ferror(fp) ? 2 : 0;
	fclose(fp);

	if ( (!e) && input_file_was_transcoded)
		fl_alert2("%s", file_encoding_warning_message);

	return e;
}

//----------------------------------------------------------------------
// Write text std::string to file.
// Unicode safe.
//----------------------------------------------------------------------

int UTF8_writefile( const char *file, std::string &text )
{
	FILE *fp;
	if (!(fp = fl_fopen(file, "w")))
		return 1;

	int buflen = text.length();
	int r = fwrite( text.c_str(), 1, buflen, fp );
	int e = (r != buflen) ? 1 : ferror(fp) ? 2 : 0;
	fclose(fp);
	return e;
}


