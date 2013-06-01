// ----------------------------------------------------------------------------
// strutil.h
//
// Copyright (C) 2009
//		Stelios Bounanos, M0GLD
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

#ifndef STRUTIL_H_
#define STRUTIL_H_

#include <ostream>
#include <iterator>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>

namespace join_ {
	template <typename T> struct empty {
		bool operator()(const T& v) const { return false; };
	};
	template <> struct empty<const char*> {
		bool operator()(const char* v) const { return !v || *v == '\0'; };
	};
	template <> struct empty<char*> {
		bool operator()(char* v) const { return !v || *v == '\0'; };
	};
	template <> struct empty<const wchar_t*> {
		bool operator()(const wchar_t* v) const { return !v || *v == L'\0'; };
	};
	template <> struct empty<wchar_t*> {
		bool operator()(wchar_t* v) const { return !v || *v == L'\0'; };
	};
	template <typename C> struct empty<std::basic_string<C> > {
		bool operator()(const std::basic_string<C>& v) const { return v.empty(); };
	};

	template <typename T, typename CharT = char, typename TraitsT = std::char_traits<CharT> >
	class ostream_iterator
		: public std::iterator<std::output_iterator_tag, void, void, void, void>
	{
	    public:
		typedef std::basic_ostream<CharT, TraitsT> ostream_type;

		ostream_iterator(ostream_type& s, const CharT* sep = 0, bool ie = false)
			: stream(&s), join_string(sep), print_sep(false), ignore_empty(ie) { }

		ostream_iterator& operator=(const T& value)
		{
			if (!ignore_empty || !is_empty(value)) {
				if (print_sep)
					*stream << join_string;
				*stream << value;
				print_sep = true;
			}

			return *this;
		}

		ostream_iterator& operator*(void) { return *this; }
		ostream_iterator& operator++(void) { return *this; }
		ostream_iterator& operator++(int) { return *this; }

	    private:
		ostream_type* stream;
		const CharT* join_string;
		bool print_sep, ignore_empty;
		empty<T> is_empty;
	};
};

template <typename T, typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>&
join(std::basic_ostream<CharT, TraitsT>& stream,
     const T* begin, const T* end, const char* sep, bool ignore_empty = false)
{
	std::copy(begin, end, join_::ostream_iterator<T, CharT, TraitsT>(stream, sep, ignore_empty));
	return stream;
}
template <typename T, typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>&
join(std::basic_ostream<CharT, TraitsT>& stream,
     const T* ptr, size_t len, const char* sep, bool ignore_empty = false)
{
	join<T, CharT, TraitsT>(stream, ptr, ptr + len, sep, ignore_empty);
	return stream;
}

template <typename T>
std::string join(const T* begin, const T* end, const char* sep, bool ignore_empty = false)
{
	std::ostringstream stream;
	join<T>(stream, begin, end, sep, ignore_empty);
	return stream.str();
}
template <typename T>
std::string join(const T* ptr, size_t len, const char* sep, bool ignore_empty = false)
{
	return join<T>(ptr, ptr + len, sep, ignore_empty);
}

template <typename CharT>
std::basic_string<CharT> join(const std::basic_string<CharT>* begin, const std::basic_string<CharT>* end,
			      const char* sep, bool ignore_empty = false)
{
	std::basic_ostringstream<CharT, std::char_traits<CharT> > stream;
	join<std::basic_string<CharT> >(stream, begin, end, sep, ignore_empty);
	return stream.str();
}
template <typename CharT>
std::basic_string<CharT>  join(const std::basic_string<CharT>* begin, size_t len,
		 const char* sep, bool ignore_empty = false)
{
	return join<CharT>(begin, begin + len, sep, ignore_empty);
}

/// Splits a string based on a char delimiter.
void strsplit( std::vector< std::string > & tokens, const std::string &str, char delim);

// Fills a string with snpritnf format string.
std::string strformat( const char * fmt, ... );

// Eliminates spaces and tabs at the beginning and the end.
void strtrim( std::string & str );

// First letter of each word in uppercase, the rest in lowercase.
void strcapitalize( std::string & str );

// Returns the replacement of all occurences of a given string by another.
std::string strreplace( const std::string & inp, const std::string & from, const std::string & to );

/// Edit distance: Returns an integer which is the distance between the two strings.
size_t levenshtein(const std::string & source, const std::string & target);

// Conversion to uppercase.
std::string uppercase( const std::string & str );

/// Surrounds a string with double-quotes and escapes control chars.
void string_escape( std::ostream & ostrm, const std::string & str );

/// Reads a string surrounded by double-quotes.
void string_unescape( std::istream & istrm, std::string & str );

/// Joins strings with a "," separator only if they are not empty.
std::string strjoin( const std::string & str1, const std::string & str2 );

/// Joins strings with a "," separator only if they are not empty.
std::string strjoin( const std::string & str1, const std::string & str2, const std::string & str3 );


// ----------------------------------------------------------------------------

#endif // STRUTIL_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
