// ----------------------------------------------------------------------------
//      re.h
//
// Copyright (C) 2008-2009
//              Stelios Bounanos, M0GLD
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

#ifndef RE_H_
#define RE_H_

#if HAVE_REGEX_H
#  include <regex.h>
#else
#  include "compat/regex.h"
#endif
#include <string>
#include <vector>

class re_t
{
public:
	re_t(const char* pattern_ = "", int cflags_ = 0);
	re_t(const re_t& re);
	~re_t();
	re_t& operator=(const re_t& rhs);
	void recompile(const char* pattern_);
	operator bool(void) const { return !error; }
	bool operator==(const re_t& o) const { return o.cflags == cflags && o.pattern == pattern; }

	bool match(const char* str, int eflags_ = 0);
	const std::string& submatch(size_t n) const;
	void suboff(size_t n, int* start, int* end) const;
	const std::vector<regmatch_t>& suboff(void) const { return suboffsets; }

	size_t nsub(void) const { return suboffsets.size(); }
	const std::string& re(void) const { return pattern; }
	int cf(void) const { return cflags; }

	size_t hash(void) const;
protected:
	void compile(void);

	std::string pattern;
	int cflags, eflags;
	regex_t preg;
	std::vector<regmatch_t> suboffsets;
	std::vector<std::string> substrings;
	bool error;
	bool need_substr;
};

class fre_t : public re_t
{
public:
	fre_t(const char* pattern_, int cflags_ = 0);
	bool match(const char* str, int eflags_ = 0);
};

#endif // RE_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
