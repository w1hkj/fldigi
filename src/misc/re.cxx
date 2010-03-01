// ----------------------------------------------------------------------------
//      re.cxx
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

#include <config.h>

#include <vector>
#include <string>

#include "re.h"

using namespace std;


re_t::re_t(const char* pattern_, int cflags_)
	: pattern(pattern_), cflags(cflags_), eflags(0), error(false)
{
	compile();
}

re_t::re_t(const re_t& re)
	: pattern(re.pattern), cflags(re.cflags), eflags(re.eflags),
	  suboffsets(re.suboffsets), substrings(re.substrings)
{
	compile();
}

re_t::~re_t()
{
	if (!error)
		regfree(&preg);
}

re_t& re_t::operator=(const re_t& rhs)
{
	if (&rhs == this)
		return *this;

	pattern = rhs.pattern;
	cflags = rhs.cflags;
	eflags = rhs.eflags;
	suboffsets = rhs.suboffsets;
	substrings = rhs.substrings;
	if (!error)
		regfree(&preg);
	compile();

	return *this;
}

void re_t::recompile(const char* pattern_)
{
	pattern = pattern_;
	if (!error)
		regfree(&preg);
	compile();
}

void re_t::compile(void)
{
	error = regcomp(&preg, pattern.c_str(), cflags);
	if (!error && !(cflags & REG_NOSUB) && preg.re_nsub > 0)
		suboffsets.resize(preg.re_nsub + 1);
}

bool re_t::match(const char* str, int eflags_)
{
	if (error)
		return false;

	eflags = eflags_;
	bool nosub = cflags & REG_NOSUB || preg.re_nsub == 0;
	bool found = !regexec(&preg, str, (nosub ? 0 : preg.re_nsub+1),
			      (nosub ? NULL : &suboffsets[0]), eflags_);
	substrings.clear();
	if (found && !nosub) {
		size_t n = suboffsets.size();
		substrings.resize(n);
		for (size_t i = 0; i < n; i++)
			if (suboffsets[i].rm_so != -1)
				substrings[i].assign(str + suboffsets[i].rm_so,
						     suboffsets[i].rm_eo - suboffsets[i].rm_so);
	}

	return found;
}

const string& re_t::submatch(size_t n) const
{
	return substrings[n];
}

void re_t::suboff(size_t n, int* start, int* end) const
{
	if (n < nsub()) {
		if (start) *start = suboffsets[n].rm_so;
		if (end) *end = suboffsets[n].rm_eo;
	}
	else {
		if (start) *start = -1;
		if (end) *end = -1;
	}
}

#include <tr1/functional>

size_t re_t::hash(void) const
{
	size_t h = tr1::hash<string>()(pattern);
	return h ^ (tr1::hash<int>()(cflags) + 0x9e3779b9 + (h << 6) + (h >> 2));
}

// ------------------------------------------------------------------------

fre_t::fre_t(const char* pattern_, int cflags_) : re_t(pattern_, cflags_) { }

bool fre_t::match(const char* str, int eflags_)
{
	if (error)
		return false;

	bool nosub = cflags & REG_NOSUB || preg.re_nsub == 0;
	return !regexec(&preg, str, (nosub ? 0 : preg.re_nsub+1),
			(nosub ? NULL : &suboffsets[0]), eflags_);
}
