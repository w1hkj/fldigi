// ----------------------------------------------------------------------------
//      re.cxx
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

#include <config.h>

#include <regex.h>
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
	  suboff(re.suboff), substr(re.substr)
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
	suboff = rhs.suboff;
	substr = rhs.substr;
	if (!error)
		regfree(&preg);
	compile();

	return *this;
}

re_t& re_t::operator=(const char* pattern_)
{
	pattern = pattern_;
	if (!error)
		regfree(&preg);
	compile();

	return *this;
}

void re_t::compile(void)
{
	error = regcomp(&preg, pattern.c_str(), cflags);
	if (!error && !(cflags & REG_NOSUB) && preg.re_nsub > 0)
		suboff.resize(preg.re_nsub + 1);
}

bool re_t::match(const char* str, int eflags_)
{
	if (error)
		return false;

	eflags = eflags_;
	bool nosub = cflags & REG_NOSUB;
	bool found = !regexec(&preg, str, (nosub ? 0 : preg.re_nsub+1),
			      (nosub ? NULL : &suboff[0]), eflags_);
	substr.clear();
	if (found && !nosub) {
		for (vector<regmatch_t>::iterator i = suboff.begin(); i != suboff.end(); i++)
			if (i->rm_so != -1)
				substr.push_back(string(str + i->rm_so, i->rm_eo));
	}

	return found;
}

const char* re_t::submatch(size_t n)
{
	return n < substr.size() ? substr[n].c_str() : 0;
}
