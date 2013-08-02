//
//	navtex.cxx
//
// Copyright (C) 2011
//		Remi Chateauneu, F4ECW
//
// This file is part of fldigi.  Adapted from code contained in JNX source code 
// distribution.
//  JNX Copyright (C) Paul Lutus
// http://www.arachnoid.com/BiQuadDesigner/index.html
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <assert.h>

#include <list>
#include <vector>
#include <string>
#include <queue>
#include <deque>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>

#include "config.h"
#include "configuration.h"
#include "fl_digi.h"
#include "debug.h"
#include "gettext.h"
#include "navtex.h"
#include "logbook.h"
#include "locator.h"
#include "misc.h"
#include "status.h"

class CoordinateT
{
	double m_angle ;
	bool   m_is_lon ;
public:
	CoordinateT()
	: m_angle(0.0), m_is_lon(true) {};

	CoordinateT( double angle, bool is_lon ) : m_angle(angle), m_is_lon(is_lon) {};

	CoordinateT( char direction, int degree, int minute, int second )
	{
		// std::cout << "ctor d=" << direction << " " << degree << " " << minute << " " << second << "\n";
		if( ( degree < 0 ) || ( degree > 180.0 ) )
			throw std::runtime_error("Invalid degree");

		if( ( minute < 0 ) || ( minute >= 60.0 ) )
			throw std::runtime_error("Invalid minute");

		if( ( second < 0 ) || ( second >= 60.0 ) )
			throw std::runtime_error("Invalid second");

		m_angle = (double)degree + (double)minute / 60.0 + (double)second / 3600.0 ;
		switch( direction )
		{
			case 'E':
			case 'e':
				m_angle = -m_angle ;
			case 'W':
			case 'w':
				if( ( degree < -180.0 ) || ( degree > 180.0 ) )
					throw std::runtime_error("Invalid longitude degree");
				m_is_lon = true ;
				break ;
			case 'S':
			case 's':
				m_angle = -m_angle ;
			case 'N':
			case 'n':
				if( ( degree < -90.0 ) || ( degree > 90.0 ) )
					throw std::runtime_error("Invalid latitude degree");
				m_is_lon = false ;
				break ;
			default:
				throw std::runtime_error("Invalid direction");
		}
		// std::cout << "Angle=" << m_angle << "\n" ;
	}

	double angle(void) const { return m_angle ; }
	bool is_lon(void) const { return m_is_lon; }

	// 57 06 N : It should work with other formats.
	// Specific for reading from the file of naxtex stations.
	friend std::istream & operator>>( std::istream & istrm, CoordinateT & ref )
	{
		int degree, minute ;
		if( ! istrm ) return istrm ;

		istrm >> degree ;
		if( ! istrm ) return istrm ;

		istrm >> minute ;
		if( ! istrm ) return istrm ;
		
		char direction ;
		istrm >> direction ;
		if( ! istrm ) return istrm ;

		ref = CoordinateT( direction, degree, minute, 0 );
		// std::cout << "::" << ref << ":" << degree << " " << minute << " " << direction << "\n" ;
		return istrm ;
	}

	class Pair ;
}; // CoordinateT

	// Latitude , longitude.
class CoordinateT::Pair
{
	CoordinateT m_lon, m_lat ;
public:
	Pair() {}

	Pair( const CoordinateT & coo1, const CoordinateT & coo2 )
	: m_lon( coo1.is_lon() ? coo1 : coo2 )
	, m_lat( coo2.is_lon() ? coo1 : coo2 )
	{
		if( ! ( coo1.is_lon() ^ coo2.is_lon() ) )
		{
			throw std::runtime_error("Internal inconsistency");
		}
	}

	CoordinateT longitude() const { return m_lon ; }

	CoordinateT latitude() const { return m_lat ; }

	CoordinateT & longitude() { return m_lon ; }

	CoordinateT & latitude() { return m_lat ; }

	double distance( const Pair & a ) const
	{
		double dist, azimuth ;
		int res = qrb(
			longitude().angle(), latitude().angle(),
			a.longitude().angle(), a.latitude().angle(),
			&dist, &azimuth );
		if( res != RIG_OK) {
			throw std::runtime_error("Bad qrb result");
		}
		return dist ;
	}
}; // CoordinateT::Pair

static const char delim = ';';

template< typename Type >
bool read_until_delim( std::istream & istrm, Type & ref )
{
	std::string parsed_str ;
	if( ! std::getline( istrm, parsed_str, delim ) ) return false ;
	std::stringstream sstrm( parsed_str );
	sstrm >> ref ;
	return sstrm ;
}

static bool read_until_delim( std::istream & istrm, std::string & ref )
{
	return std::getline( istrm, ref, delim );
}

class NavtexRecord
{
	std::string       m_country ;
	std::string       m_country_code ;
	double            m_frequency ;
	char              m_origin ;
	std::string       m_callsign ;
	std::string       m_name ;
	CoordinateT::Pair m_coordinates ;
	std::string       m_zone ;
	std::string       m_language ;

	std::string       m_locator ;

public:
	NavtexRecord()
	: m_frequency(0.0)
	, m_origin('?')
	, m_name( _("Unknown station") ) {}

	char origin(void) const { return m_origin; };
	const CoordinateT::Pair & coordinates() const { return m_coordinates; }
	double frequency(void) const { return m_frequency; };
	const std::string & country_code() const { return m_country_code; }
	const std::string & country() const { return m_country; }
	const std::string & name() const { return m_name; }
	const std::string & callsign() const { return m_callsign; }
	const std::string & locator() const { return m_locator; }

	friend std::istream & operator>>( std::istream &  istrm, NavtexRecord & rec )
	{
		std::string input_str ;
		if( ! std::getline( istrm, input_str ) ) return istrm ;
		std::stringstream str_strm( input_str );
		
		if( read_until_delim( str_strm, rec.m_country                 )
		&&  read_until_delim( str_strm, rec.m_country_code            )
		&&  read_until_delim( str_strm, rec.m_frequency               )
		&&  read_until_delim( str_strm, rec.m_origin                  )
		&&  read_until_delim( str_strm, rec.m_callsign                )
		&&  read_until_delim( str_strm, rec.m_name                    )
		&&  read_until_delim( str_strm, rec.m_coordinates.latitude()  )
		&&  read_until_delim( str_strm, rec.m_coordinates.longitude() )
		&&  read_until_delim( str_strm, rec.m_zone                    )
		&&  read_until_delim( str_strm, rec.m_language                )

		&& ( rec.m_coordinates.latitude().is_lon() == false  )
		&& ( rec.m_coordinates.longitude().is_lon() == true  )
		) 
		{
			char buf[64];
			int ret = longlat2locator(
					rec.m_coordinates.longitude().angle(),
					rec.m_coordinates.latitude().angle(),
					buf,
					3 );

			if( ret == RIG_OK ) {
				rec.m_locator = buf ;
				return istrm ;
			}
		}

		istrm.setstate(std::ios::eofbit);
		return istrm ;
	}
};

class NavtexCatalog
{
	// TODO: Consider a multimap<char, NavtexRecord>
	typedef std::deque< NavtexRecord > CatalogType ;
	CatalogType m_catalog ;

	static bool freq_close( double freqA, double freqB )
	{
		static const double freq_ratio = 1.1 ;
		return ( freqA < freqB * freq_ratio ) || ( freqA * freq_ratio > freqB );
	}

	static bool freq_acceptable( double freq )
	{
		return	freq_close( freq, 518.0 )
		||	freq_close( freq, 518.0 )
		||	freq_close( freq, 4209.5 );
	}

	typedef std::multimap< double, CatalogType::const_iterator > SolutionType ;

	static const NavtexRecord & dflt_solution(void) {
		static const NavtexRecord dfltNavtex ;
		return dfltNavtex ;
	}

	NavtexCatalog( const std::string & filnam )
	{
		LOG_INFO("Opening:%s", filnam.c_str());
		std::ifstream ifs( filnam.c_str() );
		if( !ifs )
		{
			std::string msg = "Cannot open:" + filnam ;
			LOG_ERROR("%s",msg.c_str() );
			throw std::runtime_error(msg);
		}
		for( ; ; )
		{
			NavtexRecord tmp ;
			ifs >> tmp ;
			if( !ifs) break ;
			m_catalog.push_back( tmp );
		}
		ifs.close();
	}

	static const NavtexCatalog & inst() {
		static const NavtexCatalog *ptr = NULL;
		if( ptr == NULL ) ptr = new NavtexCatalog(progdefaults.NVTX_Catalog);
		return *ptr ;
	}

public:
	/// Usual frequencies are 490, 518 or 4209 kiloHertz.
	static const NavtexRecord & Find(
		long long freq_ll,
		char origin,
		const CoordinateT::Pair & coo )
	{
		SolutionType sol;

		double freq = freq_ll / 1000.0 ; // As kiloHertz in the data file.

		bool okFreq = freq_acceptable( freq );

		for( CatalogType::const_iterator it = inst().m_catalog.begin(), en = inst().m_catalog.end(); it != en ; ++it )
		{
			if( origin != it->origin() ) continue ;

			bool freqClose = freq_close( freq, it->frequency() );
			if( okFreq && ! freqClose ) continue ;

			double dist = coo.distance( it->coordinates() );
			sol.insert( SolutionType::value_type( dist, it ) );
		}
		
		return sol.empty() ? dflt_solution() : *( sol.begin()->second );
	}

	static const NavtexRecord & Find(
		long long freq_ll,
		char origin,
		const std::string & maidenhead )
	{
		double lon, lat ;
		int res = locator2longlat( &lon, &lat, maidenhead.c_str() );
		if( res != RIG_OK ) {
			throw std::runtime_error("Cannot decode:" + maidenhead );
		};
		CoordinateT::Pair coo(
				CoordinateT( lon, true ),
				CoordinateT( lat, false ) );
		return Find( freq_ll, origin, coo );
	}
}; // NavtexCatalog

class BiQuadraticFilter {
public:
	enum Type {
		BANDPASS, LOWPASS, HIGHPASS, NOTCH, PEAK, LOWSHELF, HIGHSHELF
	};
private:
	double m_a0, m_a1, m_a2, m_b0, m_b1, m_b2;
	double m_x1, m_x2, y, m_y1, m_y2;
	double m_gain_abs;
	Type   m_type;
	double m_center_freq;
	double m_sample_rate;
	double m_Q;
	double m_gainDB;
public:
	BiQuadraticFilter() {}

	BiQuadraticFilter(Type type, double center_freq, double sample_rate, double Q, double gainDB = 0.0) {
		configure(type, center_freq, sample_rate, Q, gainDB);
	}

	void configure(Type aType, double aCenter_freq, double aSample_rate, double aQ, double aGainDB = 0.0) {
		m_x1 = m_x2 = m_y1 = m_y2 = 0;
		aQ = (aQ == 0) ? 1e-9 : aQ;
		m_type = aType;
		m_sample_rate = aSample_rate;
		m_Q = aQ;
		m_gainDB = aGainDB;
		reconfigure(aCenter_freq);
	}
private:
	// allow parameter change while running
	void reconfigure(double cf) {
		m_center_freq = cf;
		// only used for peaking and shelving filters
		m_gain_abs = pow(10, m_gainDB / 40);
		double omega = 2 * M_PI * cf / m_sample_rate;
		double sn = sin(omega);
		double cs = cos(omega);
		double alpha = sn / (2 * m_Q);
		double beta = sqrt(m_gain_abs + m_gain_abs);
		switch (m_type) {
			case BANDPASS:
				m_b0 = alpha;
				m_b1 = 0;
				m_b2 = -alpha;
				m_a0 = 1 + alpha;
				m_a1 = -2 * cs;
				m_a2 = 1 - alpha;
				break;
			case LOWPASS:
				m_b0 = (1 - cs) / 2;
				m_b1 = 1 - cs;
				m_b2 = (1 - cs) / 2;
				m_a0 = 1 + alpha;
				m_a1 = -2 * cs;
				m_a2 = 1 - alpha;
				break;
			case HIGHPASS:
				m_b0 = (1 + cs) / 2;
				m_b1 = -(1 + cs);
				m_b2 = (1 + cs) / 2;
				m_a0 = 1 + alpha;
				m_a1 = -2 * cs;
				m_a2 = 1 - alpha;
				break;
			case NOTCH:
				m_b0 = 1;
				m_b1 = -2 * cs;
				m_b2 = 1;
				m_a0 = 1 + alpha;
				m_a1 = -2 * cs;
				m_a2 = 1 - alpha;
				break;
			case PEAK:
				m_b0 = 1 + (alpha * m_gain_abs);
				m_b1 = -2 * cs;
				m_b2 = 1 - (alpha * m_gain_abs);
				m_a0 = 1 + (alpha / m_gain_abs);
				m_a1 = -2 * cs;
				m_a2 = 1 - (alpha / m_gain_abs);
				break;
			case LOWSHELF:
				m_b0 = m_gain_abs * ((m_gain_abs + 1) - (m_gain_abs - 1) * cs + beta * sn);
				m_b1 = 2 * m_gain_abs * ((m_gain_abs - 1) - (m_gain_abs + 1) * cs);
				m_b2 = m_gain_abs * ((m_gain_abs + 1) - (m_gain_abs - 1) * cs - beta * sn);
				m_a0 = (m_gain_abs + 1) + (m_gain_abs - 1) * cs + beta * sn;
				m_a1 = -2 * ((m_gain_abs - 1) + (m_gain_abs + 1) * cs);
				m_a2 = (m_gain_abs + 1) + (m_gain_abs - 1) * cs - beta * sn;
				break;
			case HIGHSHELF:
				m_b0 = m_gain_abs * ((m_gain_abs + 1) + (m_gain_abs - 1) * cs + beta * sn);
				m_b1 = -2 * m_gain_abs * ((m_gain_abs - 1) + (m_gain_abs + 1) * cs);
				m_b2 = m_gain_abs * ((m_gain_abs + 1) + (m_gain_abs - 1) * cs - beta * sn);
				m_a0 = (m_gain_abs + 1) - (m_gain_abs - 1) * cs + beta * sn;
				m_a1 = 2 * ((m_gain_abs - 1) - (m_gain_abs + 1) * cs);
				m_a2 = (m_gain_abs + 1) - (m_gain_abs - 1) * cs - beta * sn;
				break;
		}
		// prescale filter constants
		m_b0 /= m_a0;
		m_b1 /= m_a0;
		m_b2 /= m_a0;
		m_a1 /= m_a0;
		m_a2 /= m_a0;
	}
public:
	// perform one filtering step
	double filter(double x) {
		y = m_b0 * x + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
		m_x2 = m_x1;
		m_x1 = x;
		m_y2 = m_y1;
		m_y1 = y;
		return (y);
	}
};

static const unsigned char code_to_ltrs[128] = {
	//0	1	2	3	4	5	6	7	8	9	a	b	c	d	e	f
	'_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', // 0
	'_', '_', '_', '_', '_', '_', '_', 'J', '_', '_', '_', 'F', '_', 'C', 'K', '_', // 1
	'_', '_', '_', '_', '_', '_', '_', 'W', '_', '_', '_', 'Y', '_', 'P', 'Q', '_', // 2
	'_', '_', '_', '_', '_', 'G', '_', '_', '_', 'M', 'X', '_', 'V', '_', '_', '_', // 3
	'_', '_', '_', '_', '_', '_', '_', 'A', '_', '_', '_', 'S', '_', 'I', 'U', '_', // 4
	'_', '_', '_', 'D', '_', 'R', 'E', '_', '_', 'N', '_', '_', ' ', '_', '_', '_', // 5
	'_', '_', '_', 'Z', '_', 'L', '_', '_', '_', 'H', '_', '_', '\n', '_', '_', '_', // 6
	'_', 'O', 'B', '_', 'T', '_', '_', '_', '\r', '_', '_', '_', '_', '_', '_', '_' // 7
};

static const unsigned char code_to_figs[128] = {
	//0	1	2	3	4	5	6	7	8	9	a	b	c	d	e	f
	'_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', // 0
	'_', '_', '_', '_', '_', '_', '_', '\'', '_', '_', '_', '!', '_', ':', '(', '_', // 1
	'_', '_', '_', '_', '_', '_', '_', '2', '_', '_', '_', '6', '_', '0', '1', '_', // 2
	'_', '_', '_', '_', '_', '&', '_', '_', '_', '.', '/', '_', ';', '_', '_', '_', // 3
	'_', '_', '_', '_', '_', '_', '_', '-', '_', '_', '_', '\07', '_', '8', '7', '_', // 4
	'_', '_', '_', '$', '_', '4', '3', '_', '_', ',', '_', '_', ' ', '_', '_', '_', // 5
	'_', '_', '_', '"', '_', ')', '_', '_', '_', '#', '_', '_', '\n', '_', '_', '_', // 6
	'_', '9', '?', '_', '5', '_', '_', '_', '\r', '_', '_', '_', '_', '_', '_', '_' // 7
};

static const int code_ltrs = 0x5a;
static const int code_figs = 0x36;
static const int code_alpha = 0x0f;
static const int code_beta = 0x33;
static const int code_char32 = 0x6a;
static const int code_rep = 0x66;
static const int char_bell = 0x07;

class CCIR476 {

	unsigned char m_ltrs_to_code[128];
	unsigned char m_figs_to_code[128];
	bool m_valid_codes[128];
public:
	CCIR476() {
		memset( m_ltrs_to_code, 0, 128 );
		memset( m_figs_to_code, 0, 128 );
		for( size_t i = 0; i < 128; ++i ) m_valid_codes[i] = false ;
		for (int code = 0; code < 128; code++) {
			// Valid codes have four bits set only. This leaves three bits for error detection.
			// TODO: If a code is invalid, we could take the closest value in terms of bits.
			if (check_bits(code)) {
				m_valid_codes[code] = true;
				unsigned char figv = code_to_figs[code];
				unsigned char ltrv = code_to_ltrs[code];
				if ( figv != '_') {
					m_figs_to_code[figv] = code;
				}
				if ( ltrv != '_') {
					m_ltrs_to_code[ltrv] = code;
				}
			}
		}
	}

	void char_to_code(std::string & str, int ch, bool & ex_shift) const {
		ch = toupper(ch);
		// avoid unnecessary shifts
		if (ex_shift && m_figs_to_code[ch] != '\0') {
			str.push_back(  m_figs_to_code[ch] );
		}
		else if (!ex_shift && m_ltrs_to_code[ch] != '\0') {
			str.push_back( m_ltrs_to_code[ch] );
		}
		else if (m_figs_to_code[ch] != '\0') {
			ex_shift = true;
			str.push_back( code_figs );
			str.push_back( m_figs_to_code[ch] );
		}
		else if (m_ltrs_to_code[ch] != '\0') {
			ex_shift = false;
			str.push_back( code_ltrs );
			str.push_back( m_ltrs_to_code[ch] );
		}
	}

	int code_to_char(int code, bool shift) const {
		const unsigned char * target = (shift) ? code_to_figs : code_to_ltrs;
		if (target[code] != '_') {
			return target[code];
		}
		// default: return negated code
		return -code;
	}

	// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive
	// Counting set bits, Brian Kernighan's way
	static bool check_bits(int v) {
		int bc = 0;
		while (v != 0) {
			bc++;
			v &= v - 1;
		}
		//printf("check_bits %d %d %c\n", bc, (int)code_to_ltrs[v], code_to_ltrs[v] );
		return bc == 4;
	}
};

// Coordinates samples:
// 52-08.5N 003-18.0E
// 51-03.93N 001-09.17E
// 50-40.2N 001-03.7W
class ccir_message : public std::string {
	static const size_t header_len = 10 ;
	static const size_t trunc_len = 5 ;

	// Header structure is:
	// ZCZCabcd message text NNNN
	// a  : Origin of the station.
	// b  : Message type.
	// cd : Message number from this station.
	char m_origin ;
	char m_subject ;
	int  m_number ;
public:
	const char * msg_type(void) const
	{
		switch(m_subject) {
			case 'A' : return _("Navigational warning");
			case 'B' : return _("Meteorological warning");
			case 'C' : return _("Ice report");
			case 'D' : return _("Search & rescue information, pirate warnings");
			case 'E' : return _("Meteorological forecast");
			case 'F' : return _("Pilot service message");
			case 'G' : return _("AIS message");
			case 'H' : return _("LORAN message");
			case 'I' : return _("Not used");
			case 'J' : return _("SATNAV messages");
			case 'K' : return _("Other electronic navaid messages");
			case 'L' : return _("Navigational warnings");
			case 'T' : return _("Test transmissions (UK only)");
			case 'V' : return _("Notice to fishermen (U.S. only)");
			case 'W' : return _("Environmental (U.S. only)");
			case 'X' : return _("Special services - allocation by IMO NAVTEX Panel");
			case 'Y' : return _("Special services - allocation by IMO NAVTEX Panel");
			case 'Z' : return _("No message on hand");
			default  : return _("Invalid navtex subject");
		}
	}
private:
	/// Remove non-Ascii chars, replace new-line by semi-colon etc....
	void cleanup() {
		/// It would be possible to do the change in place, because the new string
		/// it shorter than the current one, but at the expense of clarity.
		bool wasDelim = false, wasSpace = false, chrSeen = false ;
		std::string newStr ;
		for( iterator it = begin(); it != end(); ++it ) {
			switch( *it ) {
				case '\n':
				case '\r': wasDelim = true ;
					   break ;
				case ' ' :
				case '\t': wasSpace = true ;
					   break ;
				default  : if( chrSeen ) {
						   if( wasDelim ) {
							  newStr.push_back('~');
					   	   } else if( wasSpace ) {
							  newStr.push_back(' ');
					   	}
					   }
					   wasDelim = false ;
					   wasSpace = false ;
					   chrSeen = true ;
					   newStr.push_back( *it );
			}
		}
		swap( newStr );
	}

	void init_members() {
		m_origin = '?';
		m_subject = '?';
		m_number = 0 ;
	}
public:
	ccir_message() {
		init_members();
	}

	ccir_message( const std::string & s, char origin, char subject, int number )
	: std::string(s)
	, m_origin(origin)
	, m_subject(subject)
	, m_number(number) {
		cleanup();
	}

	void reset_msg() {
		clear();
		init_members();
	}

	typedef std::pair<bool, ccir_message> detect_result ;
	detect_result detect_header() {
		size_t qlen = size();

		if (qlen >= header_len) {
			const char * comp = & (*this)[ qlen - header_len ];
			if( 
				(comp[0] == 'Z') &&
				(comp[1] == 'C') &&
				(comp[2] == 'Z') &&
				(comp[3] == 'C') &&
				(comp[4] == ' ') &&
				isalnum(comp[5]) &&
				isalnum(comp[6]) &&
				isdigit(comp[7]) &&
				isdigit(comp[8]) &&
				// (comp[9] == '\r') ) 
				(strchr( "\n\r", comp[9] ) ) ) {

				/// This returns the garbage before the valid header.
				// Garbage because the trailer could not be read, but maybe header OK.
				ccir_message msg_cut(
					substr( 0, size() - header_len ),
				       	m_origin,
					m_subject,
					m_number );
				m_origin  = comp[5];
				m_subject = comp[6];
				m_number = ( comp[7] - '0' ) * 10 + ( comp[8] - '0' );
				// Remove the beginning useless chars.
				/// TODO: Read broken headers such as "ZCZC EA0?"
				clear();
				return detect_result( true, msg_cut );
			}
		}
		return detect_result( false, ccir_message() ); ;
	}

	bool detect_end() {
		// Should be "\r\nNNNN\r\n" theoretically, but tolerates shorter strings.
		static const size_t slen = 4 ;
		static const char stop_valid[slen + 1] = "NNNN";
		size_t qlen = size();
		if (qlen < slen) {
			return false;
		}
		std::string comp = substr(qlen - slen, slen);
		bool end_seen = comp == stop_valid;
		if( end_seen ) {
			erase( qlen - slen, slen );
			LOG_INFO("\n%s", c_str());
		}
		return end_seen ;
	}

	void display( const std::string & alt_string ) {
		std::string::operator=( alt_string );
		cleanup();
		if( progdefaults.NVTX_AdifLog ) {
			/// For updating the logbook with received messages.
			cQsoRec qso_rec ;

			long long currFreq = wf->rfcarrier();

			if( ! progdefaults.myLocator.empty() ) {
				const NavtexRecord & refStat = NavtexCatalog::Find(currFreq, m_origin, progdefaults.myLocator );
				LOG_INFO("name=%s lon=%lf lat=%lf",
					refStat.name().c_str(),
					refStat.coordinates().longitude().angle(),
					refStat.coordinates().latitude().angle() );

				qso_rec.putField(QTH, refStat.country().c_str() );
				qso_rec.putField(CALL, refStat.callsign().c_str() );
				qso_rec.putField(COUNTRY, refStat.country().c_str() );
				qso_rec.putField(GRIDSQUARE, refStat.locator().c_str() );
				qso_rec.putField(NAME, refStat.name().c_str() );
				/// If the header is clean, the message type is removed from the string.
				// In this context, this field cannot be used.
				qso_rec.putField(XCHG1, msg_type() );
				char numberBuf[32];
				sprintf( numberBuf, "%d", m_number );
				qso_rec.putField(SRX, numberBuf );
			} else {
				std::string tmpNam("Station_");
				tmpNam += m_origin ;
				qso_rec.putField(NAME, tmpNam.c_str() );
			}
			qso_rec.setDateTime(true);
			qso_rec.setDateTime(false);
			qso_rec.setFrequency( currFreq );

			qso_rec.putField(MODE, mode_info[MODE_NAVTEX].adif_name );

			// m_qso_rec.putField(QTH, inpQth_log->value());
			// m_qso_rec.putField(STATE, inpState_log->value());
			// m_qso_rec.putField(VE_PROV, inpVE_Prov_log->value());
			// m_qso_rec.putField(QSLRDATE, inpQSLrcvddate_log->value());
			// m_qso_rec.putField(QSLSDATE, inpQSLsentdate_log->value());
			// m_qso_rec.putField(RST_RCVD, inpRstR_log->value ());
			// m_qso_rec.putField(RST_SENT, inpRstS_log->value ());
			// m_qso_rec.putField(IOTA, inpIOTA_log->value());
			// m_qso_rec.putField(DXCC, inpDXCC_log->value());
			// m_qso_rec.putField(CONT, inpCONT_log->value());
			// m_qso_rec.putField(CQZ, inpCQZ_log->value());
			// m_qso_rec.putField(ITUZ, inpITUZ_log->value());
			// m_qso_rec.putField(TX_PWR, inpTX_pwr_log->value());

			qso_rec.putField(NOTES, c_str() );

			qsodb.qsoNewRec (&qso_rec);
			qsodb.isdirty(0);

			loadBrowser(true);

			adifFile.writeLog (logbook_filename.c_str(), &qsodb);

			LOG_INFO( _("Updating log book %s"), logbook_filename.c_str() );
		}
	} // display
}; // ccir_message

static const int deviation_f = 90;

static const double dflt_center_freq = 1000.0 ;

class navtex ;

class navtex_implementation {

	enum State {
		NOSIGNAL, SYNC_SETUP, SYNC1, SYNC2, READ_DATA
	};

	static const char * state_to_str( State s ) {
		switch( s ) {
			case NOSIGNAL  : return "NOSIGNAL";
			case SYNC_SETUP: return "SYNC_SETUP";
			case SYNC1	 : return "SYNC1";
			case SYNC2	 : return "SYNC2";
			case READ_DATA : return "READ_DATA";
			default		: return "Unknown" ;
		}
	}

	bool                            m_only_sitor_b ;
	int                             m_message_counter ;

	static const size_t             m_tx_block_len = 1024 ;
	// Between -1 and 1.
	double                          m_tx_buf[m_tx_block_len];
	size_t                          m_tx_counter ;

	navtex                        * m_ptr_navtex ;

	pthread_mutex_t                 m_mutex_tx ;
	typedef std::list<std::string>  TxMsgQueueT ;
	TxMsgQueueT                     m_tx_msg_queue ;

	double                          m_metric ;

	CCIR476				m_ccir476;
	typedef std::list<int> sync_chrs_type ;
	sync_chrs_type		 m_sync_chrs;
	ccir_message		   m_curr_msg ;

	int					m_c1, m_c2, m_c3;
	static const int	   m_zero_crossings_divisor = 4;
	std::vector<int>	   m_zero_crossings ;
	long				   m_zero_crossing_count;
	double				 m_message_time ;
	double				 m_signal_accumulator ;
	double				 m_mark_f, m_space_f;
	double				 m_audio_average ;
	double				 m_audio_average_tc;
	double				 m_audio_minimum ;
	double				 m_time_sec;

	double				 m_baud_rate ;
	double				 m_baud_error;
	int					m_sample_rate ;
	bool				   m_averaged_mark_state;
	int					m_bit_duration ;
	bool				   m_old_mark_state;
	BiQuadraticFilter	  m_biquad_mark;
	BiQuadraticFilter	  m_biquad_space;
	BiQuadraticFilter	  m_biquad_lowpass;
	int					m_bit_sample_count, m_half_bit_sample_count;
	State				  m_state;
	int					m_sample_count;
	int					m_next_event_count ;
	int					m_bit_count;
	int					m_code_bits;
	bool				   m_shift ;
	bool				   m_inverse ;
	bool				   m_pulse_edge_event;
	int					m_error_count;
	int					m_valid_count;
	double				 m_sync_delta;
	bool				   m_alpha_phase ;
	bool				   m_header_found ;
	// filter method related
	double				 m_center_frequency_f ;

	navtex_implementation( const navtex_implementation & );
	navtex_implementation();
	navtex_implementation & operator=( const navtex_implementation & );
public:
	navtex_implementation(int the_sample_rate, bool only_sitor_b, navtex * ptr_navtex ) {
		pthread_mutex_init( &m_mutex_tx, NULL );
		m_ptr_navtex = ptr_navtex ;
		m_only_sitor_b = only_sitor_b ;
		m_message_counter = 1 ;
		m_metric = 0.0 ;
		m_time_sec = 0.0 ;
		m_state = NOSIGNAL;
		m_message_time = 0.0 ;
		m_signal_accumulator = 0;
		m_audio_average = 0;
		m_audio_minimum = 256;
		m_sample_rate = the_sample_rate;
		m_bit_duration = 0;
		m_next_event_count = 0;
		m_shift = false;
		m_alpha_phase = false;
		m_header_found = false;
		m_center_frequency_f = dflt_center_freq;
		m_audio_average_tc = 1000.0 / m_sample_rate;
		// this value must never be zero and bigger than 10.
		m_baud_rate = 100;
		double m_bit_duration_seconds = 1.0 / m_baud_rate;
		m_bit_sample_count = (int) (m_sample_rate * m_bit_duration_seconds + 0.5);
		m_half_bit_sample_count = m_bit_sample_count / 2;
		m_pulse_edge_event = false;
		m_error_count = 0;
		m_valid_count = 0;
		m_inverse = false;
		m_sample_count = 0;
		m_next_event_count = 0;
		m_zero_crossing_count = 0;
		/// Maybe m_bit_sample_count is not a multiple of m_zero_crossings_divisor.
		m_zero_crossings.resize( ( m_bit_sample_count + m_zero_crossings_divisor - 1 ) / m_zero_crossings_divisor, 0 );
		m_sync_delta = 0;
		m_old_mark_state = false;
		m_averaged_mark_state = false ;

		set_filter_values();
		configure_filters();
	}
	~navtex_implementation() {
		pthread_mutex_destroy( &m_mutex_tx );
	}
private:

	void set_filter_values() {
		// carefully manage the parameters WRT the center frequency
		// Q must change with frequency
		// try to maintain a zero mixer output at the carrier frequency
		double qv = m_center_frequency_f + (4.0 * 1000 / m_center_frequency_f);
		m_mark_f = qv + deviation_f;
		m_space_f = qv - deviation_f;
	}

	void configure_filters() {
		const double mark_space_filter_q = 6 * m_center_frequency_f / 1000.0;
		m_biquad_mark.configure(BiQuadraticFilter::BANDPASS, m_mark_f, m_sample_rate, mark_space_filter_q);
		m_biquad_space.configure(BiQuadraticFilter::BANDPASS, m_space_f, m_sample_rate, mark_space_filter_q);
		static const double lowpass_filter_f = 140.0;
		static const double invsqr2 = 1.0 / sqrt(2);
		m_biquad_lowpass.configure(BiQuadraticFilter::LOWPASS, lowpass_filter_f, m_sample_rate, invsqr2);
	}

	void set_state(State s) {
		if (s != m_state) {
			m_state = s;
		set_label_from_state();
		}
	}

	void flush_message(const std::string & extra_info)
	{
		if( m_header_found )
		{
			m_header_found = false;
			display_message( m_curr_msg, m_curr_msg + extra_info );
		}
		else
		{
			display_message( m_curr_msg, "<Lost header>:" + m_curr_msg + extra_info );
		}
		m_curr_msg.reset_msg();
		m_message_time = m_time_sec;
	}

	void process_timeout() {
		/// No messaging in SitorB
		if ( m_only_sitor_b ) {
			return ;
		}
		bool timeOut = m_time_sec - m_message_time > 600 ;
		if ( ! timeOut ) return ;
		LOG_INFO("Timeout: time_sec=%lf, message_time=%lf", m_time_sec, m_message_time );

		// TODO: Headerless messages could be dropped if shorter than X chars.
		flush_message(":<TIMEOUT>");
	}

	void process_messages(int c) {
		m_curr_msg.push_back((char) c);

		/// No header nor trailer for plain SitorB.
		if ( m_only_sitor_b ) {
			m_header_found = true;
			m_message_time = m_time_sec;
			return;
		}

		ccir_message::detect_result msg_cut = m_curr_msg.detect_header();
		if ( msg_cut.first ) {
			/// Maybe the message was already valid.
			if( m_header_found )
			{
				display_message( msg_cut.second, msg_cut.second + ":<Lost trailer>" );
			}
			else
			{
				/// Maybe only non-significant chars.
				if( ! msg_cut.second.empty() )
				{
					display_message( msg_cut.second, "<Lost header>:" + msg_cut.second + ":<Lost trailer>" );
				}
			}
			m_header_found = true;
			m_message_time = m_time_sec;

		} else { // valid message state
			if ( m_curr_msg.detect_end() ) {
				flush_message("");
			}
		}
	}

	// two phases: alpha and rep
	// marked during sync by code_alpha and code_rep
	// then for data: rep phase character is sent first,
	// then, three chars later, same char is sent in alpha phase
	bool process_char(int code) {
		bool success = CCIR476::check_bits(code);
		int chr = -1;
		// force phasing with the two phasing characters
		if (code == code_rep) {
			m_alpha_phase = false;
		} else if (code == code_alpha) {
			m_alpha_phase = true;
		}
		if (!m_alpha_phase) {
			m_c1 = m_c2;
			m_c2 = m_c3;
			m_c3 = code;
		} else { // alpha channel
			bool strict = false ;
			if (strict) {
				if (success && m_c1 == code) {
					chr = code;
				}
			} else {
				if (success) {
					chr = code;
				} else if (CCIR476::check_bits(m_c1)) {
					chr = m_c1;
					LOG_INFO( _("FEC replacement: %x -> %x"), code, m_c1);
				}
			}
			if (chr == -1) {
				LOG_INFO(_("Fail all options: %x %x"), code, m_c1); 
			} else {
				switch (chr) {
					case code_rep:
						break;
					case code_alpha:
						break;
					case code_beta:
						break;
					case code_char32:
						break;
					case code_ltrs:
						m_shift = false;
						break;
					case code_figs:
						m_shift = true;
						break;
					default:
						chr = m_ccir476.code_to_char(chr, m_shift);
						if (chr < 0) {
							LOG_INFO(_("Missed this code: %x"), abs(chr));
						} else {
							filter_print(chr);
							process_messages(chr);
						}
						break;
				} // switch

			} // if test != -1
		} // alpha channel

		// alpha/rep phasing
		m_alpha_phase = !m_alpha_phase;
		return success;
	}

	void filter_print(int c) {
		if (c == char_bell) {
			/// TODO: It should be a beep, but French navtex displays a quote.
			put_rx_char('\'');
		} else if (c != -1 && c != '\r' && c != code_alpha && c != code_rep) {
			put_rx_char(c);
		}
	}

	void compute_metric(void)
	{
		static double avg_ratio = 0.0 ;
		static const double width_f = 10.0 ;
       		double numer_mark = wf->powerDensity(m_mark_f, width_f);
       		double numer_space = wf->powerDensity(m_space_f, width_f);
       		double numer_mid = wf->powerDensity(m_center_frequency_f, width_f);
       		double denom = wf->powerDensity(m_center_frequency_f, 2 * deviation_f) + 1e-10;

		double ratio = ( numer_space + numer_mark + numer_mid ) / denom ;

		/// The only power in this band should come from the signal.
       		m_metric = 100 * decayavg( avg_ratio, ratio, 20 );

		// LOG_INFO("m_metric=%lf",m_metric);
		m_ptr_navtex->display_metric(m_metric);
	}

	void process_afc() {
		if( progStatus.afconoff == false ) return ;
		static size_t cnt_upd = 0 ;
		static const size_t delay_upd = 50 ;
		++cnt_upd ;

		/// AFC from time to time.
		if( ( cnt_upd % delay_upd ) != 0 ) {
			return ;
		}
		static int cnt_read_data = 0 ;
		/// This centers the carrier where the activity is the strongest.
		static const int bw[][2] = {
			{ -deviation_f - 2, -deviation_f + 8 },
			{  deviation_f - 8,  deviation_f + 2 } };
       		double max_carrier = wf->powerDensityMaximum( 2, bw );

		/// Do not change the frequency too quickly if an image is received.
		double next_carr = 0.0 ;

		State lingering_state ;
		if( m_state == READ_DATA ) {
			/// Proportional to the number of lines between each AFC update.
			cnt_read_data = delay_upd / 20 ;
			lingering_state = READ_DATA ;
		} else {
			if( cnt_read_data ) {
				--cnt_read_data ;
				lingering_state = READ_DATA ;
			} else {
				lingering_state = m_state ;
				/// Maybe this is the phasing signal, so we recenter.
				double pwr_left = wf->powerDensity ( max_carrier - deviation_f, 10 );
				double pwr_right = wf->powerDensity( max_carrier + deviation_f, 10 );
				static const double ratio_left_right = 5.0 ;
				if( pwr_left > ratio_left_right * pwr_right ) {
					max_carrier -= deviation_f ;
				} else if ( ratio_left_right * pwr_left < pwr_right ) {
					max_carrier += deviation_f ;
				}
			}
		}
		switch( lingering_state ) {
			case NOSIGNAL:
			case SYNC_SETUP:
				next_carr = max_carrier ;
				break;
			case SYNC1:
			case SYNC2:
				next_carr = decayavg( m_center_frequency_f, max_carrier, 1 );
				break;
			case READ_DATA:
				// It will stay stable for a couple of calls.
				if( max_carrier < m_center_frequency_f )
					next_carr = std::max( max_carrier, m_center_frequency_f - 3.0 );
				else if( max_carrier > m_center_frequency_f )
					next_carr = std::min( max_carrier, m_center_frequency_f + 3.0 );
				else next_carr = max_carrier ;
				break;
			default:
				LOG_ERROR("Should not happen: lingering_state=%d", (int)lingering_state );
				break ;
		}

		LOG_DEBUG("m_center_frequency_f=%f max_carrier=%f next_carr=%f cnt_read_data=%d",
			(double)m_center_frequency_f, max_carrier, next_carr, cnt_read_data );
		double delta = fabs( m_center_frequency_f - next_carr );
		if( delta > 1.0 ) { // Hertz.
			m_ptr_navtex->set_freq(next_carr);
		}
	}

	/* A NAVTEX message is built on SITOR collective B-mode and consists of:
	* a phasing signal of at least ten seconds
	* the four characters "ZCZC" that identify the end of phasing
	* a single space
	* four characters B1, B2, B3 and B4:
		* B1 is an alpha character identifying the station,
	* B2 is an alpha character used to identify the subject of the message.
		* B3 and B4 are two-digit numerics identifying individual messages
	* a carriage return and a line feed
	* the information
	* the four characters "NNNN" to identify the end of information
	* a carriage return and two line feeds
	* either
		* 5 or more seconds of phasing signal and another message starting with "ZCZC" or
		* an end of emission idle signal alpha for at least 2 seconds.  */
public:
	void process_data(const double * data, int nb_samples) {
		process_afc();
		process_timeout();
		for( int i =0; i < nb_samples; ++i ) {
			short v = static_cast<short>(32767 * data[i]);

			m_time_sec = m_sample_count / m_sample_rate ;
			double dv = v;

			// separate mark and space by narrow filtering
			double mark_level = m_biquad_mark.filter(dv);
			double space_level = m_biquad_space.filter(dv);

			double mark_abs = fabs(mark_level);
			double space_abs = fabs(space_level);

			m_audio_average += (std::max(mark_abs, space_abs) - m_audio_average) * m_audio_average_tc;

			m_audio_average = std::max(.1, m_audio_average);

			// produce difference of absolutes of mark and space
			double diffabs = (mark_abs - space_abs);

			diffabs /= m_audio_average;

			// now low-pass the resulting difference
			double logic_level = m_biquad_lowpass.filter(diffabs);

			bool mark_state = (logic_level > 0);
			m_signal_accumulator += (mark_state) ? 1 : -1;
			m_bit_duration++;

			// adjust signal synchronization over time
			// by detecting zero crossings
			if (mark_state != m_old_mark_state) {
				// a valid bit duration must be longer than bit duration / 2
				if ((m_bit_duration % m_bit_sample_count) > m_half_bit_sample_count) {
					// create a relative index for this zero crossing
					assert( m_sample_count - m_next_event_count + m_bit_sample_count * 8 >= 0 );
					size_t index = size_t((m_sample_count - m_next_event_count + m_bit_sample_count * 8) % m_bit_sample_count);

					// TODO: This never happened so could be replaced by assert() for speed-up.
					// Size = m_bit_sample_count / m_zero_crossings_divisor
					if( index / m_zero_crossings_divisor >= m_zero_crossings.size() ) {
						LOG_ERROR("index=%d m_zero_crossings_divisor=%d m_zero_crossings.size()=%d\n",
								(int)index, m_zero_crossings_divisor, (int)m_zero_crossings.size() );
						LOG_ERROR("m_sample_count=%d m_next_event_count=%d m_bit_sample_count=%d\n",
						m_sample_count, m_next_event_count, m_bit_sample_count );
						exit(EXIT_FAILURE);
					}

					m_zero_crossings.at( index / m_zero_crossings_divisor )++;
				}
				m_bit_duration = 0;
			}
			m_old_mark_state = mark_state;
			if (m_sample_count % m_bit_sample_count == 0) {
				m_zero_crossing_count++;
				static const int zero_crossing_samples = 16;
				if (m_zero_crossing_count >= zero_crossing_samples) {
					int best = 0;
					int index = 0;
					// locate max zero crossing
					for (size_t i = 0; i < m_zero_crossings.size(); i++) {
						int q = m_zero_crossings[i];
						m_zero_crossings[i] = 0;
						if (q > best) {
							best = q;
							index = i;
						}
					}
					if (best > 0) { // if there is a basis for choosing
						// create a signed correction value
						index *= m_zero_crossings_divisor;
						index = ((index + m_half_bit_sample_count) % m_bit_sample_count) - m_half_bit_sample_count;
						// limit loop gain
						double dbl_idx = (double)index / 8.0 ;
						// m_sync_delta is a temporary value that is
						// used once, then reset to zero
						m_sync_delta = dbl_idx;
						// m_baud_error is persistent -- used by baud error label
						m_baud_error = dbl_idx;
					}
					m_zero_crossing_count = 0;
				}
			}

			// flag the center of signal pulses
			m_pulse_edge_event = m_sample_count >= m_next_event_count;
			if (m_pulse_edge_event) {
				m_averaged_mark_state = (m_signal_accumulator > 0) ^ m_inverse;
				m_signal_accumulator = 0;
				// set new timeout value, include zero crossing correction
				m_next_event_count = m_sample_count + m_bit_sample_count + (int) (m_sync_delta + 0.5);
				m_sync_delta = 0;
			}

			if (m_audio_average < m_audio_minimum) {
				set_state(NOSIGNAL);
			} else if (m_state == NOSIGNAL) {
				set_state(SYNC_SETUP);
			}

			switch (m_state) {
				case NOSIGNAL: break;
				case SYNC_SETUP:
					m_bit_count = -1;
					m_code_bits = 0;
					m_error_count = 0;
					m_valid_count = 0;
					m_shift = false;
					m_sync_chrs.clear();
					set_state(SYNC1);
					break;
				// scan indefinitely for valid bit pattern
				case SYNC1:
					if (m_pulse_edge_event) {
						m_code_bits = (m_code_bits >> 1) | ( m_averaged_mark_state ? 64 : 0);
						if (CCIR476::check_bits(m_code_bits)) {
							m_sync_chrs.push_back(m_code_bits);
							m_bit_count = 0;
							m_code_bits = 0;
							set_state(SYNC2);
						}
					}
					break;
				//  sample and validate bits in groups of 7
				case SYNC2:
					// find any bit alignment that produces a valid character
					// then test that synchronization in subsequent groups of 7 bits
					if (m_pulse_edge_event) {
						m_code_bits = (m_code_bits >> 1) | ( m_averaged_mark_state ? 64 : 0);
						m_bit_count++;
						if (m_bit_count == 7) {
							if (CCIR476::check_bits(m_code_bits)) {
								m_sync_chrs.push_back(m_code_bits);
								m_code_bits = 0;
								m_bit_count = 0;
								m_valid_count++;
								// successfully read 4 characters?
								if (m_valid_count == 4) {
									for( sync_chrs_type::const_iterator it = m_sync_chrs.begin(), en = m_sync_chrs.end(); it != en; ++it ) {
										process_char(*it);
									}
									set_state(READ_DATA);
								}
							} else { // failed subsequent bit test
								m_code_bits = 0;
								m_bit_count = 0;
								// LOG_INFO("restarting sync");
								set_state(SYNC_SETUP);
							}
						}
					}
					break;
				case READ_DATA:
					if (m_pulse_edge_event) {
						m_code_bits = (m_code_bits >> 1) | ( m_averaged_mark_state ? 64 : 0);
						m_bit_count++;
						if (m_bit_count == 7) {
							if (m_error_count > 0) {
								LOG_INFO(_("Error count: %d"), m_error_count);
							}
							if (process_char(m_code_bits)) {
								if (m_error_count > 0) {
									m_error_count--;
								}
							} else {
								m_error_count++;
								if (m_error_count > 2) {
									LOG_INFO(_("Returning to sync"));
									set_state(SYNC_SETUP);
								}
							}
							m_bit_count = 0;
							m_code_bits = 0;
						}
					}
					break;
			}

			m_sample_count++;
		}
		compute_metric();
	}

	/// This updates the window label according to the state.
	void set_label_from_state(void) const
	{
		put_status( state_to_str(m_state) );
	}

private:
	/// Each received message is pushed in this queue, so it can be read by XML/RPC.
	syncobj m_sync_rx ;
	std::queue< std::string > m_received_messages ;

	void display_message( ccir_message & ccir_msg, const std::string & alt_string ) {
		if( ccir_msg.size() >= (size_t)progdefaults.NVTX_MinSizLoggedMsg )
		{
			ccir_msg.display(alt_string);
			put_received_message( alt_string );
		}
		else
		{
			LOG_INFO("Do not log short message:%s", ccir_msg.c_str() );
		}
	}

	/// Called by the engine each time a message is saved.
	void put_received_message( const std::string &message )
	{
		guard_lock g( m_sync_rx.mtxp() );
		LOG_INFO("%s", message.c_str() );
		m_received_messages.push( message );
		m_sync_rx.signal();
	}

public:
	/// Returns a received message, by chronological order.
	std::string get_received_message( double max_seconds )
	{
		guard_lock g( m_sync_rx.mtxp() );

		LOG_INFO(_("delay=%f"), max_seconds );
		if( m_received_messages.empty() )
		{
			if( ! m_sync_rx.wait(max_seconds) ) return "Timeout";
		}
		std::string message = m_received_messages.front();
		m_received_messages.pop();
		return message ;
	}

	// http://www.arachnoid.com/JNX/index.html
	// "NAUTICAL" becomes:
	// rep alpha rep alpha N alpha A alpha U N T A I U C T A I L C blank A blank L
	std::string create_fec( const std::string & str ) const
	{
		std::string res ;
		const size_t sz = str.size();

		static const size_t offset = 2 ;
		for( size_t i = 0 ; i < offset ; ++i ) {
			res.push_back( code_rep );
			res.push_back( code_alpha );
		}

		for ( size_t i = 0; i < sz; ++i ) {
			res.push_back( str[i] );
			res.push_back( i >= offset ? str[ i - offset ] : code_alpha );
		}

		for( size_t i = 0 ; i < offset ; ++i ) {
			res.push_back( code_char32 );
			res.push_back( str[ sz - offset + i ] );
		}
		return res;
	}

	// Note path std::string can contain null characters.
	// TODO: Beware of the extra copy constructor.
	std::string encode( const std::string & str ) const
	{
		std::string res ;
		bool shift = false ;
		for ( size_t i = 0, sz = str.size(); i < sz; ++ i ) {
			m_ccir476.char_to_code(res, str[i], shift );
		}
		return res;
	}

	void tx_flush()
	{
		if( m_tx_counter != 0 ) {
			m_ptr_navtex->ModulateXmtr( m_tx_buf, m_tx_counter );
			m_tx_counter = 0 ;
		}
	}

	// Input value must be between -1 and 1
	void add_sample( double sam )
	{
		m_tx_buf[ m_tx_counter++ ] = sam ;

		if( m_tx_counter == m_tx_block_len ) {
			tx_flush();
		}
	}

	void send_sine( double seconds, double freq )
	{
		int nb_samples = seconds * m_ptr_navtex->get_samplerate();
		double max_level = 0.99 ; // Between -1.0 and 1.0
		double ratio = 2.0 * M_PI * (double)freq / (double)m_ptr_navtex->get_samplerate() ;
		for (int i = 0; i < nb_samples ; ++i )
		{
			add_sample( max_level * sin( i * ratio ) );
		}
	}

	void send_phasing( int seconds )
	{
		send_sine( seconds, m_center_frequency_f );
	}

	void send_bit( bool bit )
	{
		send_sine( 1.0 / (double)m_baud_rate, bit ? m_mark_f : m_space_f );
	}

	void send_string( const std::string & msg )
	{
		std::string encod = encode( msg );
		std::string sevenbits = create_fec( encod );

		for( size_t i = 0, sz = sevenbits.size(); i < sz; i++ )
		{
			char tmp_stat[64];
			sprintf( tmp_stat, "Transmission %d%%", (int)( 100.0  * ( i + 1.0 ) / sz ) );
			put_status( tmp_stat );

			char c = sevenbits[i];
			for( size_t j = 0; j < 7 ; ++j, c >>= 1 )
			{
				send_bit( c & 1 );
			}
		}
	}
	void send_message( const std::string & msg, bool is_first, bool is_last )
	{
		put_status( "Transmission" );
		m_tx_counter = 0 ;

		if( m_only_sitor_b )
		{
			send_string( msg );
		}
		else
		{
			put_status( "Phasing" );
			send_phasing( is_first ? 10.0 : 5.0 );
			char preamble[64];
			const char origin = 'Z' ; // Never seen this value.
			const char subject = 'I' ; // This code is not used.
			sprintf( preamble, "ZCZC %c%c%02d\r\n", origin, subject, m_message_counter );
			m_message_counter = ( m_message_counter + 1 ) % 100 ;

			/// The extra cr-nl before NNNN is not in the specification but clarify things.
			std::string full_msg = preamble + msg + "\r\nNNNN\r\n\n";
			send_string( full_msg );

			// 5 or more seconds of phasing signal and another message starting with "ZCZC" or
			// an end of emission idle signal alpha for at least 2 seconds.  */
			if( is_last ) {
				put_status( "Trailer" );
				send_phasing(2.0);
			}
		}
		tx_flush();
		put_status( "" );
	}

	void append_message_to_send( const std::string & msg )
	{
		guard_lock g( &m_mutex_tx );
		m_tx_msg_queue.push_back( msg );
	}

	void transmit_message_async( const std::string & msg )
	{
		LOG_INFO("%s", msg.c_str() );

		append_message_to_send( msg );

		bool is_first = true ;
		for(;;)
		{
			guard_lock g( &m_mutex_tx );

			TxMsgQueueT::iterator it = m_tx_msg_queue.begin(), en = m_tx_msg_queue.end();
			if( it == en ) break ;
			TxMsgQueueT::iterator it_next = it ;
			++it_next ;
			bool is_last = it_next == en ;
			send_message( *it, is_first, is_last );
			is_first = false ;
			m_tx_msg_queue.erase(it);
		}
	}

	void process_tx()
	{
		std::string msg ;

		for(;;)
		{
			int c = get_tx_char();
			if( c == GET_TX_CHAR_NODATA ) {
				break ;
			}
			msg.push_back( c );
		}

		for( size_t i = 0 ; i < msg.size(); ++ i)
		{
			put_echo_char( msg[i] );
		}

		transmit_message_async(msg);
	}

	void set_carrier( double freq )
	{
		m_center_frequency_f = freq;
		set_filter_values();
		configure_filters();
	}

}; // navtex_implementation

#ifdef NAVTEX_COMMAND_LINE
int main(int n, const char ** v )
{
	printf("%s\n", v[1] );
	FILE * f = fopen( v[1], "r" );
	fseek( f, 0, SEEK_END );
	long l = ftell( f );
	printf("l=%ld\n", l);
	char * buf = new char[l];
	fseek( f, 0, SEEK_SET );
	size_t lr = fread( buf, 1, l, f );
	if( lr - l ) {
		printf("Err reading\n");
		exit(EXIT_FAILURE);
	};

	navtex_implementation nv(11025) ;
	double * tmp = new double[l/2];
	const short * shrt = (const short *)buf;
	for( int i = 0; i < l/2; i++ )
		tmp[i] = ( (double)shrt[i] ) / 32767.0;
	nv.process_data( tmp, l / 2 );
	return 0 ;
}
#endif // NAVTEX_COMMAND_LINE

navtex::navtex (trx_mode md)
{
	modem::cap |= CAP_AFC ;
	navtex::mode = md;
	modem::samplerate = 11025;
	modem::bandwidth = 2 * deviation_f ;
	bool only_sitor_b = false ;
	switch( md )
	{
		case MODE_NAVTEX : only_sitor_b = false ;
				   break;
		case MODE_SITORB : only_sitor_b = true ;
				   break;
		default          : LOG_ERROR("Unknown mode");
	}
	m_impl = new navtex_implementation( modem::samplerate, only_sitor_b, this );
}

navtex::~navtex()
{
	if( m_impl )
	{
		delete m_impl ;
	}
}
void navtex::rx_init()
{
	put_MODEstatus(modem::mode);
}

void navtex::restart()
{
}

int  navtex::rx_process(const double *buf, int len)
{
	m_impl->process_data( buf, len );
	return 0;
}

void navtex::tx_init(SoundBase *sc)
{
	modem::scard = sc; // SoundBase
	videoText(); // In trx/modem.cxx
}

int  navtex::tx_process()
{
	m_impl->process_tx();

	return -1;
}

void navtex::set_freq( double freq )
{
	modem::set_freq( freq );
	m_impl->set_carrier( freq );
}

/// This returns the next received message.
std::string navtex::get_message(int max_seconds)
{
	return m_impl->get_received_message(max_seconds);
}

std::string navtex::send_message(const std::string &msg)
{
	m_impl->append_message_to_send(msg);
	start_tx(); // If this is not done.
	return "";
}

