// ----------------------------------------------------------------------------
// strutil.cxx
//
// Copyright (C) 2009-2012
//		Stelios Bounanos, M0GLD
//		Remi Chateauneu, F4ECW
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

#include <stdio.h>
#include <stdarg.h>

#include <vector>
#include <string>
#include <cstring>
#include <climits>
#include <stdexcept>

#include "re.h"
#include "strutil.h"

using namespace std;

vector<string> split(const char* re_str, const char* str, unsigned max_split)
{
	vector<string> v;
	size_t n = strlen(re_str);
	string s; s.reserve(n + 2); s.append(1, '(').append(re_str, n).append(1, ')');
	fre_t re(s.c_str(), REG_EXTENDED);

	bool ignore_trailing_empty = false;
	if (max_split == 0) {
		max_split = UINT_MAX;
		ignore_trailing_empty = true;
	}

	s = str;
	const vector<regmatch_t>& sub = re.suboff();
	while (re.match(s.c_str())) {
		if (unlikely(sub.empty() || ((max_split != UINT_MAX) && --max_split == 0)))
			break;
		else {
			s[sub[0].rm_so] = '\0';
			v.push_back(s.c_str());
			s.erase(0, sub[0].rm_eo);
		}
	}

	if (!(ignore_trailing_empty && s.empty()))
		v.push_back(s);
	return v;
}

/// Builds a string out of a printf-style formatted vararg list.
string strformat( const char * fmt, ... )
{
	static const int sz_buf = 512 ;
	char buf_usual[sz_buf];
 
 	va_list ap;
	va_start(ap, fmt);
	int res = vsnprintf( buf_usual, sz_buf, fmt, ap);
	va_end(ap);
 	if( res < 0 ) throw runtime_error(__FUNCTION__);
	if( res < sz_buf ) return buf_usual ;

	string str( res, ' ' );
	va_start(ap, fmt);
	res = vsnprintf( &str[0], res + 1, fmt, ap);
	va_end(ap);
	if( res < 0 ) throw runtime_error(__FUNCTION__);
	return str ;
}

