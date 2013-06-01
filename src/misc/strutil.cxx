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
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <limits>
#include <iostream>

#include "strutil.h"

using namespace std;


/// Splits a string based on a char delimiter.
void strsplit( std::vector< std::string > & tokens, const std::string &str, char delim)
{
	for( const char * prev = str.c_str(), * next ; ; prev = next + 1 )
	{
		next = strchr( prev, delim );
		if( next == NULL )
		{
			tokens.push_back( std::string( prev ) );
			break;
		}
		tokens.push_back( std::string( prev, next ) );
	}
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

/// Removes leading spaces and tabs.
static std::string & strtriml(std::string &str) {
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return str;
}

/// Removes trailing spaces and tabs.
static std::string & strtrimr(std::string &str) {
        str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), str.end());
        return str;
}

/// Removes leading trailing spaces and tabs.
void strtrim(std::string &str) {
        strtriml(strtrimr(str));
}

void strcapitalize(std::string &str) {
	bool isStart = true ;
	for( size_t i = 0; i < str.size(); ++i ) {
		const char tmpC = str[i];
		if( isalpha( tmpC ) ) {
			if( isStart ) {
				str[ i ] = toupper( tmpC );
				isStart = false ;
			} else {
				str[ i ] = tolower( tmpC );
			}
		} else {
			isStart = true ;
		}
	}
}

std::string strreplace( const std::string & inp, const std::string & from, const std::string & to )
{
	size_t from_sz=from.size();
	std::string tmp ;

	for( size_t old_curr = 0 ; ; ) {
		size_t new_curr = inp.find( from, old_curr );
		if( new_curr == std::string::npos ) {
			tmp.append( inp, old_curr, std::string::npos );
			break ;
		}
		else
		{
			tmp.append( inp, old_curr, new_curr - old_curr );
			tmp.append( to );
			old_curr = new_curr + from_sz ;
		}
	}
	return tmp ;
}

/// Edit distance. Not the fastest implementation.
size_t levenshtein(const string & source, const string & target) {
	const size_t n = source.size();
	const size_t m = target.size();
	if (n == 0) return m;
	if (m == 0) return n;
	
	typedef std::vector< std::vector<size_t> > Tmatrix; 
	
	Tmatrix matrix(n+1);
	
	for (size_t i = 0; i <= n; i++) {
		matrix[i].resize(m+1);
	}
	
	for (size_t i = 0; i <= n; i++) {
		matrix[i][0]=i;
	}
	
	for (size_t j = 0; j <= m; j++) {
		matrix[0][j]=j;
	}
	
	for (size_t i = 1; i <= n; i++) {
		char s_i = source[i-1];
		
		for (size_t j = 1; j <= m; j++) {	
			char t_j = target[j-1];
			
			size_t cost = (s_i == t_j) ? 0 : 1 ;
			
			size_t above = matrix[i-1][j];
			size_t left = matrix[i][j-1];
			size_t diag = matrix[i-1][j-1];
			size_t cell = std::min( above + 1, std::min(left + 1, diag + cost));
			
			// Step 6A: Cover transposition, in addition to deletion,
			// insertion and substitution. This step is taken from:
			// Berghel, Hal ; Roach, David : "An Extension of Ukkonen's 
			// Enhanced Dynamic Programming ASM Algorithm"
			// (http://www.acm.org/~hlb/publications/asm/asm.html)
			
			if (i>2 && j>2) {
				size_t trans=matrix[i-2][j-2]+1;
				if (source[i-2]!=t_j) trans++;
				if (s_i!=target[j-2]) trans++;
				if (cell>trans) cell=trans;
			}
			
			matrix[i][j]=cell;
		}
	}
	
	return matrix[n][m];
}

/// Converts a string to uppercase.
string uppercase( const string & str )
{
	string resu ;
	for( size_t i = 0 ; i < str.size(); ++i )
	{
		resu += static_cast<char>( toupper( str[i] ) );
	}
	return resu ;
}

/// Surrounds a string with double-quotes and escapes control chars.
void string_escape( std::ostream & ostrm, const std::string & str )
{
	ostrm << '"';
        for( const char * it = str.c_str(), * beg = it, * ptr = NULL ; ; ++it )
        {
                char ch = *it ;
                switch( ch )
                {
			/// TODO: Printf the hexadecimal value for a non-printable char.
                        case '"'    : ptr = "\""; break;
                        case '\n'   : ptr = "\\n"; break;
                        case '\r'   : ptr = "\\r"; break;
                        case '\b'   : ptr = "\\b"; break;
                        case '\t'   : ptr = "\\t"; break;
                        case '\0'  : break ;
                        default    : continue ;
                }
                if( it != beg ) {
                        ostrm.write( beg, it - beg );
                }

                if( ch == '\0' ) break ;
                ostrm << ptr ;
                beg = it + 1 ;
        }
	ostrm << '"';
}

/// Reads a string surrounded by double-quotes.
void string_unescape( std::istream & istrm, std::string & str )
{
	if( ! str.empty() ) str.clear();
	bool is_quoted = false ;
	for(;;) {
		/// Starts to read only after the double-quote.
		char ch = istrm.get();
		switch(ch) {
			case ' '  : /// The string is not started yet.
			case '\t' : continue ;
			case '\r' : /// String ends before having started.
			case '\n' :
			case  -1  : return ;
			default   : is_quoted = false ;
				    break ;
			case '"'  : is_quoted = true;
				    break ;
		}
		break ;
	}

	/// Any chars can be between the quotes if it is escaped.
	bool backslashed = false ;
	for(;;) {
		char ch = istrm.get();
		if( backslashed ) {
			backslashed = false ;
			switch(ch) {
				case -1  : return;
				case '"' : str += '\"'; continue;
				case 'r' : str += '\r'; continue;
				case 'n' : str += '\n'; continue;
				case 'b' : str += '\b'; continue;
				case 't' : str += '\t'; continue;
				/// This should not happen, but we accept that all chars can be escaped.
				default  : str += ch; continue ;
			}
		} else {
			switch(ch) {
				case -1  :
				case '\r':
				case '\n':
				case '\0':
				case '"' : return ;
				case '\\': backslashed = true ; continue ;
				case ' ' : /// Spaces and tabs end the string if it was not quoted.
				case '\t': if( ! is_quoted ) return ;
				default  : str += ch ; continue ;
			}
		}
	}
}

/// Joins strings with a "," separator only if they are not empty.
std::string strjoin( const std::string & str1, const std::string & str2 )
{
	std::string trim1(str1);
	strtrim(trim1);
	std::string trim2(str2);
	strtrim(trim2);

	// This could be faster by detecting the beginning and the end
	// of the spaces in the string.
	if( trim2.empty() ) return trim1;
	if( trim1.empty() ) return trim2;
	return trim1 + "," + trim2 ;
}

/// Joins strings with a "," separator only if they are not empty.
std::string strjoin( const std::string & str1, const std::string & str2, const std::string & str3 )
{
	return strjoin( str1, strjoin( str2, str3 ) );
}



// ----------------------------------------------------------------------------

