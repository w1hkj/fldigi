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
#include <climits>

std::vector<std::string> split(const char* re_str, const char* str, unsigned max_split = UINT_MAX);

// Fills a string with snprintf format string.
std::string strformat( const char * fmt, ... );

// Eliminates spaces and tabs at the beginning and the end.
void strtrim( std::string & str );

// First letter of each word in ucasestr, the rest in lowercase.
void strcapitalize( std::string & str );

// Returns the replacement of all occurences of a given string by another.
std::string strreplace( const std::string & inp, const std::string & from, const std::string & to );

/// Edit distance: Returns an integer which is the distance between the two strings.
size_t levenshtein(const std::string & source, const std::string & target);

// Conversion to ucasestr.
std::string ucasestr( std::string str );
std::string ucasestr( const char *str);

// find independent of case
size_t ufind(std::string s1, std::string s2, size_t idx = 0);

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
	std::getline( istrm, ref, delim );
	if ( (istrm.rdstate() & std::istream::goodbit) == 0 )
		return true ;
	else
		return false ;
}

/// For reading from a string with tokens separated by a char. Used to load CSV files.
template< typename Tp >
bool read_until_delim( char delim, std::istream & istrm, Tp & ref, typename DtTyp< Tp >::Any = typename DtTyp< Tp >::Any() )
{
	std::string parsed_str ;
	std::getline( istrm, parsed_str, delim );
	if( ! ((istrm.rdstate() & std::istream::goodbit) == 0)) {
		return false ;
	}
	imemstream sstrm( parsed_str );
	sstrm >> ref ;
	return true ;
}

/// Same, with a default value if there is nothing to read.
template< typename Tp >
bool read_until_delim( char delim, std::istream & istrm, Tp & ref, const Tp dflt, typename DtTyp< Tp >::Any = typename DtTyp< Tp >::Any() )
{
	std::string parsed_str ;
	std::getline( istrm, parsed_str, delim ) ;
	if( ! ((istrm.rdstate() & std::istream::goodbit) == 0) ) {
		return false ;
	}
	if( parsed_str.empty() ) {
		ref = dflt ;
		return true;
	}
	imemstream sstrm( parsed_str );
	sstrm >> ref ;
	return true ;
}

/// For reading from a string with tokens separated by a char to a fixed-size array.
template< typename Tp >
bool read_until_delim( char delim, std::istream & istrm, Tp & ref, typename DtTyp< Tp >::Array = typename DtTyp< Tp >::Array() )
{
	istrm.getline( ref, DtTyp< Tp >::Size, delim );
	// Should we return an error if buffer is too small?
	return( (istrm.rdstate() & std::istream::goodbit) == 0 );
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
	return ((istrm.rdstate() & std::istream::goodbit) == 0 );
}

// ----------------------------------------------------------------------------

#endif // STRUTIL_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
