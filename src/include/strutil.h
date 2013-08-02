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

#include <vector>
#include <climits>

std::vector<std::string> split(const char* re_str, const char* str, unsigned max_split = UINT_MAX);

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

// ----------------------------------------------------------------------------

/// This is a read-only replacement for std::stringstream.
struct imemstream : public std::streambuf, public std::istream {
	/// Faster than stringstream because no copy.
	imemstream(char* s, std::size_t n) : std::istream( this )
	{
		setg(s, s, s + n);
	}
	/// Faster than stringstream because no copy.
	imemstream(const std::string & r) : std::istream( this )
	{
		char * s = const_cast< char * >( r.c_str() );
		setg(s, s, s + r.size());
	}
};
// ----------------------------------------------------------------------------

/// Tells if type is a char[]. Used for SFINAE.
template< class T >
struct DtTyp {
	/// In the general case, data types are not char arrays.
	struct Any {};
};

/// Matches if the type is a char[].
template< size_t N >
struct DtTyp< char[N] > {
	struct Array {};
	static const size_t Size = N ;
};

/// Reads all chars until after the delimiter.
bool read_until_delim( char delim, std::istream & istrm );

/// Reads a char followed by the delimiter.
bool read_until_delim( char delim, std::istream & istrm, char & ref, const char dflt );

/// Reads a double up to the given delimiter.
inline bool read_until_delim( char delim, std::istream & istrm, double & ref )
{
	istrm >> ref ;
	if( ! istrm ) return false ;

	char tmp = istrm.get();
	if( istrm.eof() ) {
		/// Resets to good to mean that it worked fine.
		istrm.clear();
		return true ;
	}
	return tmp == delim ;
}

/// Reads a string up to the given delimiter.
inline bool read_until_delim( char delim, std::istream & istrm, std::string & ref )
{
	return std::getline( istrm, ref, delim );
}

/// For reading from a string with tokens separated by a char. Used to load CSV files.
template< typename Tp >
bool read_until_delim( char delim, std::istream & istrm, Tp & ref, typename DtTyp< Tp >::Any = typename DtTyp< Tp >::Any() )
{
	std::string parsed_str ;
	if( ! std::getline( istrm, parsed_str, delim ) ) {
		return false ;
	}
	imemstream sstrm( parsed_str );
	sstrm >> ref ;
	return sstrm ;
}

/// Same, with a default value if there is nothing to read.
template< typename Tp >
bool read_until_delim( char delim, std::istream & istrm, Tp & ref, const Tp dflt, typename DtTyp< Tp >::Any = typename DtTyp< Tp >::Any() )
{
	std::string parsed_str ;
	if( ! std::getline( istrm, parsed_str, delim ) ) {
		return false ;
	}
	if( parsed_str.empty() ) {
		ref = dflt ;
		return true;
	}
	imemstream sstrm( parsed_str );
	sstrm >> ref ;
	return sstrm ;
}

/// For reading from a string with tokens separated by a char to a fixed-size array.
template< typename Tp >
bool read_until_delim( char delim, std::istream & istrm, Tp & ref, typename DtTyp< Tp >::Array = typename DtTyp< Tp >::Array() )
{
	istrm.getline( ref, DtTyp< Tp >::Size, delim );
	// Should we return an error if buffer is too small?
	return istrm ;
}

/// Same, with a default value if there is nothing to read. Fixed-size array.
template< typename Tp >
bool read_until_delim( char delim, std::istream & istrm, Tp & ref, const Tp dflt, typename DtTyp< Tp >::Array = typename DtTyp< Tp >::Array() )
{
	istrm.getline( ref, DtTyp< Tp >::Size, delim );
	// If nothing to read, copy the default value.
	if( ref[0] == '\0' ) {
		strncpy( ref, dflt, DtTyp< Tp >::Size - 1 );
	}
	// Should we return an error if buffer is too small?
	return istrm;
}

// ----------------------------------------------------------------------------

#endif // STRUTIL_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
