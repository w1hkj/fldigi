// ----------------------------------------------------------------------------
//      re.h
//
// Copyright (C) 2008
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

#ifdef HAVE_REGEX_H
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
	re_t& operator=(const char* pattern_);
	operator bool(void) { return !error; }

	bool match(const char* str, int eflags_ = 0);
	const std::string& submatch(size_t n);
	void suboff(size_t n, int* start, int* end);
	size_t nsub(void) { return substrings.size(); }
protected:
	void compile(void);

	std::string pattern;
	int cflags, eflags;
	regex_t preg;
	std::vector<regmatch_t> suboffsets;
	std::vector<std::string> substrings;
	bool error;
};

class fre_t : private re_t
{
public:
	fre_t(const char* pattern_, int cflags_ = 0);
	operator bool(void) { return !error; }

	bool match(const char* str, int eflags_ = 0);
	const std::vector<regmatch_t>& suboff(void) { return suboffsets; }

private:
	fre_t(const fre_t& re);
	fre_t& operator=(const fre_t& rhs);
};

#include <sys/types.h>
#if (defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 3) || \
     defined(__FreeBSD__) || defined(__APPLE__) || defined(__MINGW32__)
#  define HAVE_REGEX_BACKREF 1
#else
#  define HAVE_REGEX_BACKREF 0
#endif

#endif // RE_H_
