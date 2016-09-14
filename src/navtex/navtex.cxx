// ---------------------------------------------------------------------
//
//  navtex.cxx
//
// Copyright (C) 2011-2016
//      Remi Chateauneu, F4ECW
//      Rik van Riel, AB1KW, <riel@surriel.com>
//
// This file is part of fldigi.  Adapted from code contained in JNX
// source code distribution.
//  JNX Copyright (C) Paul Lutus
// http://www.arachnoid.com/JNX/index.html
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
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Sync using multicorrelator, instead of null crossings
//      Rik van Riel, AB1KW, <riel@surriel.com>
//
// Null crossings are somewhat noisy, and the code to keep the navtex
// decoder in sync with the incoming signal using null crossings was
// rather fragile.
//
// Use a multicorrelator instead, which relies on the averaged magnitude
// of the signal accumulator to sync the decoder with the incoming signal.
//
// When debugging the code, the multicorrelator mostly corrects the
// modem forward in time, which can be explained by the fact that a
// bit takes 110.25 samples, while the code uses 110. When the NAVTEX
// transmitter is running at exactly 100 baud, one can expect to see
// the decoder get adjusted 25 times a second, to make up for the
// difference between 11000 and 11025.
//
// When multiple signals are on the air simultaneously, the null crossing
// code would often lose track of the signal. The multicorrelator seems
// to be more stable in this situation, though of course when both signals
// are close in strength things do not get decoded right.
//
// The signal sampling spread of 1/6 of the width of a bit was set through
// trial and error. A larger spread increases the signal difference between
// early, prompt, and late samples, but reduces the accumulator value seen
// by the demodulator. A smaller spread increases the accumulator value seen,
// but makes it harder to lock on in noisy conditions.
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// low pass mark & space individually
//      Rik van Riel, AB1KW, <riel@surriel.com>
//
// Putting individual low pass filters on mark and space seems to
// result in an improved ability to overcome pulse noise, and decode
// weaker navtex signals.
//
// I have not found any signal where the performance of the codec
// got worse with this change.
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Correct display metric
//      Rik van Riel, AB1KW, <riel@surriel.com>
//
// The NAVTEX display_metric() function was buggy, in that decayavg
// returns the decayed value, but does not store it. It always put
// the current value in the metric, and kept avg_ratio at 0.0.
//
// This resulted in a somewhat chaotic, and not very useful metric
// display. Copy over the S/N calculation from the RTTY code, because
// that code seems to work well.

// Also print the S/N in Status2, like the RTTY code and other modes
// do.

// Copying over the RTTY S/N code wholesale might still not be
// enough, since the NAVTEX wave form appears to be somewhat
// different from RTTY.  However, at least we have something
// now, and the metric used for squelch seems to work again.
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Correct display metric
//      Rik van Riel, AB1KW, <riel@surriel.com>
//
// Widen afc filter for 'jump 90 Hz' code
//
// When the NAVTEX code spots a power imbalance of more than a factor
// 5 between mark and space, it will shift the frequency by 90 Hz.
// This is reported to help with some signals.
//
// However, it breaks with some other signals, which have a different
// spectral distribution between mark and space, with a spectrum looking
// something like this:
//
//                                        *
//                                        *
//                                        *
//                                       **
//     ******                           ***
//    ********                         ******
//   **********                       ********
//  ********************************************
// **********************************************
//
// In this spectrum, mark & space have a similar amount of energy,
// but that is only apparent when the comparison between them is
// done on a wider sample than 10 Hz.
//
// Sampling 30 Hz instead seems to result in a more stable AFC.
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// use exact bit length
//      Rik van Riel, AB1KW, <riel@surriel.com>
//
// With a baud rate of 100 and a sample rate of 11025, the number
// of bits per sample is 110.25.  Approximating this with 110 bits
// per sample results in the decoder continuously chasing after the
// signal, and losing it more easily during transient noise or
// interference events.
//
// Simply changing the variable type from int to double makes life
// a little easier on the bit tracking code.
//
// The accumulator does not seem to care that it gets an extra sample
// every 4 bit periods.
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// improvements to the multi correlator
//      Rik van Riel, AB1KW, <riel@surriel.com>
//
// While the multi correlator for bit sync was a nice improvement over
// the null crossing tracking, it did lose sync too easily in the presence
// of transient noise or interference, and was full of magic adjustments.
//
// Replace the magic adjustments with a calculation, which makes the multi
// correlator able to ride out transient noise or interference, and then
// make a larger adjustment all at once (if needed).
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// use same mark/space detector as RTTY modem
//      Rik van Riel, AB1KW, <riel@surriel.com>
//
// Switch the NAVTEX modem over to the same mark/spac decoder, with W7AY's
// automatic threshold correction algorithm, like the RTTY modem uses.
//
// The noise subtraction is a little different than in the RTTY modem;
// the algorithm used in W7AY's code seems to work a little better with
// the noise present at 518 kHz, when compared to the algorithm used in
// the RTTY modem.
//
// I have compared this detector to a correlation detector; the latter
// appears to be a little more sensitive, which includes higher
// sensitivity to noise. With a 250 Hz filter on the radio, the
// correlation detector might be a little bit better, while with the
// filter on the radio opened up to 4kHz wide, this detector appears
// to be more robust.
//
// On signals with a large mark/space power imbalance, or where the power
// distribution in one of the two throws off the automatic frequency
// correction, this decoder is able to handle signals that neither of
// the alternatives tested does.
// ---------------------------------------------------------------------

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
#include "coordinate.h"
#include "misc.h"
#include "status.h"
#include "strutil.h"
#include "kmlserver.h"
#include "record_loader.h"
#include "fftfilt.h"

#include "FL/fl_ask.H"

/// This models a line of the file defining Navtex stations.
class NavtexRecord
{
	std::string       m_country ;
	std::string       m_country_code ;
	double            m_frequency ;
	char              m_origin ;
	std::string       m_callsign ;
	std::string       m_name ;
	CoordinateT::Pair m_coordinates ;

	std::string       m_locator ;

	/// Reads a CSV file.
	static const char m_delim = ';';
public:
	NavtexRecord()
	: m_frequency(0.0)
	, m_origin('?')
	, m_name( _("Unknown station") ) {}

	char origin(void) const { return m_origin; };
	const CoordinateT::Pair & coordinates() const { return m_coordinates; }
	double frequency(void) const { return m_frequency; };
	const std::string & country() const { return m_country; }
	const std::string & name() const { return m_name; }
	const std::string & callsign() const { return m_callsign; }

	/// Example: Azores;AZR;490.0;J;CTH;Horta;38 32 N;28 38 W;II;PP
	friend std::istream & operator>>( std::istream &  istrm, NavtexRecord & rec )
	{
		std::string input_str ;
		if( ! std::getline( istrm, input_str ) ) return istrm ;
		std::stringstream str_strm( input_str );

		if( read_until_delim( m_delim, str_strm, rec.m_country                 )
		&&  read_until_delim( m_delim, str_strm  /* Country code */            )
		&&  read_until_delim( m_delim, str_strm, rec.m_frequency               )
		&&  read_until_delim( m_delim, str_strm, rec.m_origin                  )
		&&  read_until_delim( m_delim, str_strm, rec.m_callsign                )
		&&  read_until_delim( m_delim, str_strm, rec.m_name                    )
		&&  read_until_delim( m_delim, str_strm, rec.m_coordinates.latitude()  )
		&&  read_until_delim( m_delim, str_strm, rec.m_coordinates.longitude() )
		&&  read_until_delim( m_delim, str_strm  /* Zone */                    )
		&&  read_until_delim( m_delim, str_strm  /* Language */                )

		&& ( rec.m_coordinates.latitude().is_lon() == false  )
		&& ( rec.m_coordinates.longitude().is_lon() == true  )
		)
		{
			return istrm ;
		}

		istrm.setstate(std::ios::eofbit);
		return istrm ;
	}
};

/// Navtex catalog of stations is used when logging to ADIF file: It gives the station name, callsign etc...
class NavtexCatalog : public RecordLoader< NavtexCatalog >
{
	// TODO: Consider a multimap<char, NavtexRecord>
	typedef std::deque< NavtexRecord > CatalogType ;
	CatalogType m_catalog ;

	/// Frequency more or less 1 %: 485-494 kHz, 512-523 kHz etc...
	static bool freq_close( double freqA, double freqB )
	{
		static const double freq_ratio = 1.01 ;
		return ( freqA < freqB * freq_ratio ) || ( freqA * freq_ratio > freqB );
	}

	/// Tells if this is a reasonable Navtex frequency.
	static bool freq_acceptable( double freq )
	{
		return  freq_close( freq, 490.0 )
		||  freq_close( freq, 518.0 )
		||  freq_close( freq, 4209.5 );
	}

	void Clear() {
		m_catalog.clear();
	}

	bool ReadRecord( std::istream & istrm ) {
		NavtexRecord tmp ;
		istrm >> tmp ;
		if( istrm || istrm.eof() ) {
			m_catalog.push_back( tmp );
			return true ;
		}
		return false ;
	}

	/// Minimal edit distance (Levenshtein) between the pattern and any token of the string.
	static double DistToStationName( const std::string & msg, const std::string & pattern ) {
		std::stringstream strm( msg );
		/// Any big number is OK, if bigger than any string length.
		double currDist = 1.7976931348623157e+308; // DBL_MAX ;
		typedef std::istream_iterator<std::string> StrmIterStr ;
		for( StrmIterStr itStrm( strm ); itStrm != StrmIterStr(); ++itStrm ) {
			const std::string tmp = *itStrm ;
			currDist = std::min( currDist, (double)levenshtein( tmp, pattern ) );
		}
		return currDist ;
	}

public:
	std::string base_filename() const
	{
		return "NAVTEX_Stations.csv";
	}

	const char * Description() const
	{
		return _("Navtex stations");
	}

	/// Usual frequencies are 490, 518 or 4209 kiloHertz.
	const NavtexRecord * FindStation(
		long long freq_ll,
		char origin,
		const std::string & maidenhead,
		const std::string & msg)
	{
		if( maidenhead.empty() ) return NULL;

		if( m_catalog.empty() ) {
			int nbRecs = LoadAndRegister();

			static bool error_signaled = false ;

			if( nbRecs <= 0 ) {
				LOG_WARN("Error reading Navtex stations file");
				if(error_signaled == false) {
					fl_alert("Cannot read Navtex file %s", storage_filename().first.c_str() );
					error_signaled = true ;
					return NULL;
				}
			}
			error_signaled = false ;
		}

		const CoordinateT::Pair coo( maidenhead );

		/// Possible Navtex stations stored by closer first.
		typedef std::multimap< double, CatalogType::const_iterator > SolutionType ;

		SolutionType solDistKm;

		double freq = freq_ll / 1000.0 ; // As kiloHertz in the data file.

		bool okFreq = freq_acceptable( freq );

		//LOG_INFO("Operator Maidenhead=%s lon=%lf lat=%lf okFreq=%d Origin=%c",
		//  maidenhead.c_str(), coo.longitude().angle(), coo.latitude().angle(), okFreq, origin );

		for( CatalogType::const_iterator it = m_catalog.begin(), en = m_catalog.end(); it != en ; ++it )
		{
			/// The origin letters must be identical.
			if( origin != it->origin() ) continue ;

			/// The two frequencies must be close more or less 10%.
			bool freqClose = freq_close( freq, it->frequency() );
			if( okFreq && ! freqClose ) continue ;

			/// Solutions are stored smallest distance first.
			double dist = coo.distance( it->coordinates() );
			solDistKm.insert( SolutionType::value_type( dist, it ) );
		}

		/// No station found.
		if( solDistKm.empty() ) return NULL;

		/// Only one station, no ambiguity.
		SolutionType::iterator begSolKm = solDistKm.begin();
		if( solDistKm.size() == 1 ) return & ( *begSolKm->second );

		SolutionType solStrDist ;
		// Maybe some station names appear but not others. This can be for example "Maltaradio", "Cullercoat", "Limnos" etc...
		for( SolutionType::iterator itSolKm = begSolKm, endSolKm = solDistKm.end(); itSolKm != endSolKm; ++itSolKm ) {
			std::stringstream strm ;
			strm << itSolKm->second->coordinates();
			// LOG_INFO("Name=%s Dist=%lf %s", itSolKm->second->name().c_str(), itSolKm->first, strm.str().c_str() );
			// The message is in uppercase anyway, so no need to convert.
			double str_dist = DistToStationName( msg, uppercase( itSolKm->second->name() ) );

			solStrDist.insert( SolutionType::value_type( str_dist, itSolKm->second ) );
		}

		// There are at least two elements, so we can do this.
		SolutionType::iterator begSolStr = solStrDist.begin();
		SolutionType::iterator nxtSolStr = begSolStr;
		++nxtSolStr ;

		// The first message only contains a string very similar to a radio station.
		if( (begSolStr->first < 2) && ( nxtSolStr->first > 2 ) ) {
			//LOG_INFO("Levenshtein beg=%lf beg_name=%s next=%lf next_name=%s",
			//  begSolStr->first, begSolStr->second->name().c_str(),
			//  nxtSolStr->first, nxtSolStr->second->name().c_str() );
			return & (*begSolStr->second) ;
		}

		// There are at least two elements, and more than one station name, or none of them,
		// is contained in the message.

		// Just returns the closest element.
		return & ( *begSolKm->second );

		// Now we could search for a coordinate in the message, and we will keep the station which is the closest
		// to this coordinate. We wish to do that anyway in order to map things in KML.
		// Possible formats are -(This is experimental):
		// 67-04.0N 032-25.7E
		// 47-29'30N 003-16'00W
		// 6930.1N 01729.9E
		// 48-21'45N 004-31'45W
		// 58-37N 003-32W
		// 314408N 341742E
		// 42-42N 005-10E
		// 54-02.3N 004-45.8E
		// 55-20.76N 014-45.27E
		// 55-31.1 N 012-44.7 E
		// 5330.4N 01051.5W
		// 43 45.0 N - 015 44.8 E
		// 34-33.7N 012-28.7E
		// 51 10.55 N - 001 51.02 E
		// 51.21.67N 002.13.29E
		// 73 NORTH 14 EAST
		// 58-01.20N 005-27.08W
		// 50.56N 007.00,5W
		// 5630,1N- 00501,6E
		// LAT. 41.06N - LONG 012.57E
		// 42 40 01 N - 018 05 10 E
		// 40 25 31N - 18 15 30E
		// 40-32.2N 000-33.5E
		// 58-01.2 NORTH 005-27.1 WEST
		// 39-07,7N 026-39,2E
		//
		// ESTIMATED LIMIT OF ALL KNOWN ICE:
		// 4649N 5411W TO 4530N 5400W TO
		// 4400N 4900W TO 4545N 4530W TO
		// 4715N 4530W TO 5000N 4715W TO
		// 5530N 5115W TO 5700N 5545W.
		//


	}
}; // NavtexCatalog

static const unsigned char code_to_ltrs[128] = {
	//0 1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
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
	//0 1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
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

	int bytes_to_code(int *pos) {
		int code = 0;
		int i;

		for (i = 0; i < 7; i++)
			code |= ((pos[i] > 0) << i);
		return code;
	}

	int bytes_to_char(int *pos, int shift) {
		int code = bytes_to_code(pos);
		return code_to_char(code, shift);
	}

	// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive
	/// Counting set bits, Brian Kernighan's way
	static bool check_bits(int v) {
		int bc = 0;
		while (v != 0) {
			bc++;
			v &= v - 1;
		}
		//printf("check_bits %d %d %c\n", bc, (int)code_to_ltrs[v], code_to_ltrs[v] );
		return bc == 4;
	}

	// Is there a valid character in the next 7 ints?
	bool valid_char_at(int *pos) {
		int count = 0;
		int i;

		for (i = 0; i < 7; i++)
			if (pos[i] > 0)
				count++;

		return (count == 4);
	}

};

/// This is temporary, to manipulate a multi-line string.
static const char * new_line = "\n";

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
	/// Remove non-Ascii chars, replace new-line by special character etc....
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
							  newStr.append(new_line);
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

		long long currFreq = wf->rfcarrier();

		if( ! progdefaults.NVTX_AdifLog && ! progdefaults.NVTX_KmlLog ) {
			return ;
		}

		const NavtexRecord * ptrNavRec = NavtexCatalog::InstCatalog().FindStation(currFreq, m_origin, progdefaults.myLocator, *this );
		if( ptrNavRec != NULL ) {
			LOG_INFO("Locator=%s Origin=%c freq=%d name=%s lon=%lf lat=%lf",
				progdefaults.myLocator.c_str(),
				m_origin,
				static_cast<int>(currFreq),
				ptrNavRec->name().c_str(),
				ptrNavRec->coordinates().longitude().angle(),
				ptrNavRec->coordinates().latitude().angle() );
		} else {
			LOG_INFO("Locator=%s Origin=%c freq=%d Navtex station not found",
				progdefaults.myLocator.c_str(),
				m_origin,
				static_cast<int>(currFreq) );
		}

		if( progdefaults.NVTX_AdifLog ) {
			/// For updating the logbook with received messages.
			QsoHelper qso(MODE_NAVTEX);

			if( ptrNavRec ) {
				qso.Push(QTH, ptrNavRec->country() );
				qso.Push(CALL, ptrNavRec->callsign() );
				qso.Push(COUNTRY, ptrNavRec->country() );
				qso.Push(GRIDSQUARE, ptrNavRec->coordinates().locator() );
				qso.Push(NAME, ptrNavRec->name() );
				/// If the header is clean, the message type is removed from the string.
				// In this context, this field cannot be used.
				qso.Push(XCHG1, msg_type() );
				qso.Push(SRX, strformat( "%d", m_number ) );
			} else {
				qso.Push(NAME, std::string("Station_") + m_origin );
			}

			// Sequence of Chars and line-breaks, ASCII CR (code 13) + ASCII LF (code 10)
			qso.Push(NOTES, strreplace( *this, new_line, ADIF_EOL ) );
		}

		// Adds a placemark to the navtex KML file.
		if( progdefaults.NVTX_KmlLog ) {
			if( ptrNavRec ) {
				KmlServer::CustomDataT custData ;
				custData.Push( "Callsign", ptrNavRec->callsign() );
				custData.Push( "Country", ptrNavRec->country() );
				custData.Push( "Locator", ptrNavRec->coordinates().locator() );
				custData.Push( "Message number", m_number );
				custData.Push( "Frequency", currFreq );

				custData.Push( "Mode", mode_info[MODE_NAVTEX].adif_name );
				custData.Push( "Message", *this );

				KmlServer::GetInstance()->Broadcast(
					"Navtex",
					0,
					ptrNavRec->coordinates(),
					0,
					ptrNavRec->name(),
					"navtex_station",
					substr( 0, 20 ) + "...",
					custData );
			}

			// TODO: Parse the message to extract coordinates.
		}
	} // display
}; // ccir_message

static const int deviation_f = 85;

static const double dflt_center_freq = 1000.0 ;

class navtex ;

/// Implements PIMPL idiom.
class navtex_implementation {

	enum State {
		NOSIGNAL, SYNC_SETUP, SYNC, READ_DATA
	};

	static const char * state_to_str( State s ) {
		switch( s ) {
			case NOSIGNAL  : return "NOSIGNAL";
			case SYNC_SETUP: return "SYNC_SETUP";
			case SYNC   : return "SYNC";
			case READ_DATA : return "READ_DATA";
			default     : return "Unknown" ;
		}
	}

	bool                            m_only_sitor_b ;
	int                             m_message_counter ;

	static const size_t             m_tx_block_len = 1024 ;
	/// Between -1 and 1.
	double                          m_tx_buf[m_tx_block_len];
	size_t                          m_tx_counter ;

	navtex                        * m_ptr_navtex ;

	pthread_mutex_t                 m_mutex_tx ;
	typedef std::list<std::string>  TxMsgQueueT ;
	TxMsgQueueT                     m_tx_msg_queue ;

	double                          m_metric ;

	CCIR476             m_ccir476;
	typedef std::list<int> sync_chrs_type ;
	ccir_message                    m_curr_msg ;

	double               m_message_time ;
	double               m_early_accumulator ;
	double               m_prompt_accumulator ;
	double               m_late_accumulator ;
	double               m_mark_f, m_space_f;
	double               m_audio_average ;
	double               m_audio_average_tc;
	double               m_audio_minimum ;
	double               m_time_sec;

	double               m_baud_rate ;
	int                 m_sample_rate ;
	int			m_averaged_mark_state;
	int                 m_bit_duration ;
	fftfilt					*m_mark_lowpass;
	fftfilt					*m_space_lowpass;
	double					m_mark_phase;
	double					m_space_phase;
	double                  m_bit_sample_count, m_half_bit_sample_count;
	State                 m_state;
	int                 m_sample_count;
	double                  m_next_early_event;
	double                  m_next_prompt_event;
	double                  m_next_late_event;
	double                  m_average_early_signal;
	double                  m_average_prompt_signal;
	double                  m_average_late_signal;
	std::vector<int>	m_bit_values;
	int			m_bit_cursor;
	bool                   m_shift ;
	bool                   m_pulse_edge_event;
	int                 m_error_count;
	bool                   m_alpha_phase ;
	bool                   m_header_found ;
	char snrmsg[80];
	// filter method related
	double               m_center_frequency_f ;

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
		m_early_accumulator = 0;
		m_prompt_accumulator = 0;
		m_late_accumulator = 0;
		m_audio_average = 0;
		m_audio_minimum = 0.15;
		m_sample_rate = the_sample_rate;
		m_bit_duration = 0;
		m_shift = false;
		m_alpha_phase = false;
		m_header_found = false;
		m_center_frequency_f = dflt_center_freq;
		m_audio_average_tc = 1000.0 / m_sample_rate;
		// this value must never be zero and bigger than 10.
		m_baud_rate = 100;
		double m_bit_duration_seconds = 1.0 / m_baud_rate;
		m_bit_sample_count = m_sample_rate * m_bit_duration_seconds;
		m_half_bit_sample_count = m_bit_sample_count / 2;
		// A narrower spread between signals allows the modem to
		// center on the pulses better, but a wider spread makes
		// more robust under noisy conditions. 1/5 seems to work.
		m_next_early_event = 0;
		m_next_prompt_event = m_bit_sample_count / 5;
		m_next_late_event = m_bit_sample_count * 2 / 5;
		m_average_early_signal = 0;
		m_average_prompt_signal = 0;
		m_average_late_signal = 0;
		m_error_count = 0;
		m_sample_count = 0;
		// keep 1 second worth of bit values for decoding
		m_bit_values.resize(m_baud_rate);
		m_bit_cursor = 0;

		m_mark_lowpass = 0;
		m_space_lowpass = 0;

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
		m_mark_phase = 0;
		m_space_phase = 0;
	}

	void configure_filters() {
		const int filtlen = 512;
		if (m_mark_lowpass) delete m_mark_lowpass;
		m_mark_lowpass = new fftfilt(m_baud_rate/m_sample_rate, filtlen);
		m_mark_lowpass->rtty_filter(m_baud_rate/m_sample_rate);

		if (m_space_lowpass) delete m_space_lowpass;
		m_space_lowpass = new fftfilt(m_baud_rate/m_sample_rate, filtlen);
		m_space_lowpass->rtty_filter(m_baud_rate/m_sample_rate);
	}

	void set_state(State s) {
		if (s != m_state) {
			m_state = s;
		set_label_from_state();
		}
	}

	/// The parameter is appended at the message end.
	void flush_message(const std::string & extra_info)
	{
		if( m_header_found )
		{
			m_header_found = false;
			display_message( m_curr_msg, m_curr_msg + extra_info );
		}
		else
		{
			display_message( m_curr_msg, "[Lost header]:" + m_curr_msg + extra_info );
		}
		m_curr_msg.reset_msg();
		m_message_time = m_time_sec;
	}

	/// Checks that we have no waited too long, and if so, flushes the message with a specific terminator.
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
				display_message( msg_cut.second, msg_cut.second + ":[Lost trailer]" );
			}
			else
			{
				/// Maybe only non-significant chars.
				if( ! msg_cut.second.empty() )
				{
					display_message( msg_cut.second, "[Lost header]:" + msg_cut.second + ":[Lost trailer]" );
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

	// The rep character is transmitted 5 characters (35 bits) ahead of
	// the alpha character.
	int fec_offset(int offset) {
		return offset - 35;
	}

	// Flip the sign of the smallest (least certain) bit in a character;
	// hopefully this will result in the right valid character.
	void flip_smallest_bit(int *pos) {
		int minimum = INT_MAX;
		int smallest_bit = -1;
		int i;

		for (i = 0; i < 7; i++) {
			if (abs(pos[i]) < minimum) {
				minimum = abs(pos[i]);
				smallest_bit = i;
			}
		}

		pos[smallest_bit] = -pos[smallest_bit];
	}

	// Try to find a position in the bit stream with:
	// - the largest number of valid characters, and
	// - with rep (duplicate) characters in the right locations
	// This way the code can sync up with an incoming signal after
	// the initial alpha/rep synchronisation
	//
	// http://www.arachnoid.com/JNX/index.html
	// "NAUTICAL" becomes:
	// rep alpha rep alpha N alpha A alpha U N T A I U C T A I L C blank A blank L
	int find_alpha_characters(void) {
		int best_offset = 0;
		int best_score = 0;
		int offset, i;

		// With 7 bits per character, and interleaved rep & alpha
		// characters, the first alpha character with a corresponding
		// rep in the stream can be in any of 14 locations
		for (offset = 35; offset < (35 + 14); offset++) {
			int score = 0;
			int reps = 0;
			int limit = m_bit_values.size() - 7;

			// Search for the largest sequence of valid characters
			for (i = offset; i < limit; i += 7) {
				if (m_ccir476.valid_char_at(&m_bit_values[i])) {
					int ri = fec_offset(i);
					int code = m_ccir476.bytes_to_code(&m_bit_values[i]);
					int rep = m_ccir476.bytes_to_code(&m_bit_values[ri]);

					// This character is valid
					score++;

					// Does it match its rep?
					if (code == rep) {
						// This offset is wrong, rep
						// and alpha are spaced odd
						if (code == code_alpha ||
						    code == code_rep) {
							score = 0;
							break;
						}
						reps++;
					} else if (code == code_alpha) {
						// Is there a matching rep to
						// this alpha?
						int ri = i - 7;
						int rep = m_ccir476.bytes_to_code(&m_bit_values[ri]);
						if (rep == code_rep) {
							reps++;
						}
					}
				}
			}

			// the most valid characters, with at least 3 FEC reps
			if (reps > 3 && score + reps > best_score) {
				best_score = score + reps;
				best_offset = offset;
			}
		}

		// m_bit_values fits 14 characters; if there are at least
		// 9 good ones, tell the caller where they start
		if (best_score > 8)
			return best_offset;
		else
			return -1;
	}

	// Turns accumulator values (estimates of whether a bit is 1 or 0)
	// into navtex messages
	void handle_bit_value(int accumulator) {
		int buffersize = m_bit_values.size();
		int i, offset = 0;

		// Store the received value in the bit stream
		for (i = 0; i < buffersize - 1; i++) {
			m_bit_values[i] = m_bit_values[i+1];
		}
		m_bit_values[buffersize - 1] = accumulator;
		if (m_bit_cursor > 0)
			m_bit_cursor--;

		// Find the most likely location where the message starts
		if (m_state == SYNC) {
			offset = find_alpha_characters();
			if (offset >= 0) {
				set_state(READ_DATA);
				m_bit_cursor = offset;
				m_alpha_phase = true;
			} else
				set_state(SYNC_SETUP);
		}

		// Process 7-bit characters as they come in,
		// skipping rep (duplicate) characters
		if (m_state == READ_DATA) {
			if (m_bit_cursor < buffersize - 7) {
				if (m_alpha_phase) {
					int ret = process_bytes(m_bit_cursor);
					m_error_count -= ret;
					if (m_error_count > 5)
						set_state(SYNC_SETUP);
					if (m_error_count < 0)
						m_error_count = 0;
				}
				m_alpha_phase = !m_alpha_phase;
				m_bit_cursor += 7;
			}
		}
	}

	// Turn a series of 7 bit confidence values into a character
	//
	// 1 on successful decode of the alpha character
	// 0 on unmodified FEC replacement
	// -1 on soft failure (FEC calculation)
	// -2 on hard failure
	int process_bytes(int m_bit_cursor) {
		int code = m_ccir476.bytes_to_code(&m_bit_values[m_bit_cursor]);
		int success = 0;

		if (m_ccir476.check_bits(code)) {
			LOG_DEBUG("valid code : %x (%c)", code, m_ccir476.code_to_char(code, m_shift));
			success = 1;
			goto decode;
		}

		if (fec_offset(m_bit_cursor) < 0)
			return -1;

		// The alpha (primary) character received was not correct.
		// Try the rep (duplicate) copy of the character, and some
		// permutations to see if the correct character can be found.
		{
			int i, calc, avg[7];
			// Rep is 5 characters before alpha.
			int reppos = fec_offset(m_bit_cursor);
			int rep = m_ccir476.bytes_to_code(&m_bit_values[reppos]);
			if (CCIR476::check_bits(rep)) {
				// Current code is probably code_alpha.
				// Skip decoding to avoid switching phase.
				if (rep == code_rep)
					return 0;
				LOG_DEBUG("FEC replacement: %x -> %x (%c)", code, rep, m_ccir476.code_to_char(rep, m_shift));
				code = rep;
				goto decode;
			}

			// Neither alpha or rep are valid. Check whether
			// the average of the two is a valid character.
			for (i = 0; i < 7; i++) {
				int a = m_bit_values[m_bit_cursor + i];
				int r = m_bit_values[rep + i];
				avg[i] = a + r;
			}

			calc = m_ccir476.bytes_to_code(avg);
			if (CCIR476::check_bits(calc)) {
				LOG_DEBUG("FEC calculation: %x & %x -> %x (%c)", code, rep, calc, m_ccir476.code_to_char(calc, m_shift));
				code = calc;
				success = -1;
				goto decode;
			}

			// Flip the lowest confidence bit in alpha.
			flip_smallest_bit(&m_bit_values[m_bit_cursor]);
			calc = m_ccir476.bytes_to_code(&m_bit_values[m_bit_cursor]);
			if (CCIR476::check_bits(calc)) {
				LOG_DEBUG("FEC calculation: %x & %x -> %x (%c)", code, rep, calc, m_ccir476.code_to_char(calc, m_shift));
				code = calc;
				success = -1;
				goto decode;
			}

			// Flip the lowest confidence bit in rep.
			flip_smallest_bit(&m_bit_values[reppos]);
			calc = m_ccir476.bytes_to_code(&m_bit_values[reppos]);
			if (CCIR476::check_bits(calc)) {
				LOG_DEBUG("FEC calculation: %x & %x -> %x (%c)", code, rep, calc, m_ccir476.code_to_char(calc, m_shift));
				code = calc;
				success = -1;
				goto decode;
			}

			// Try flipping the bit with the lowest confidence
			// in the average of alpha & rep.
			flip_smallest_bit(avg);
			calc = m_ccir476.bytes_to_code(avg);
			if (CCIR476::check_bits(calc)) {
				LOG_DEBUG("FEC calculation: %x & %x -> %x (%c)", code, rep, calc, m_ccir476.code_to_char(calc, m_shift));
				code = calc;
				success = -1;
				goto decode;
			}

			LOG_DEBUG("decode fail %x, %x", code, rep);
			return -2;
		}

	decode:
		process_char(code);
		return success;
	}

	bool process_char(int chr) {
		static int last_char = 0;
		switch (chr) {
			case code_rep:
				// This code should run in alpha phase, but
				// it just received two rep characters. Fix
				// the rep/alpha phase, so FEC works again.
				if (last_char == code_rep) {
					LOG_DEBUG("fixing rep/alpha sync");
					m_alpha_phase = false;
				}
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

		last_char = chr;
		return true;
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
		static double sigpwr = 0.0 ;
		static double noisepwr = 0.0;
		double delta = m_baud_rate/8.0;
		double np = wf->powerDensity(m_center_frequency_f, delta) * 3000 / delta;
		double sp =
			wf->powerDensity(m_mark_f, delta) +
				wf->powerDensity(m_space_f, delta) + 1e-10;
		double snr;

		sigpwr = decayavg ( sigpwr, sp, sp > sigpwr ? 2 : 8);
		noisepwr = decayavg ( noisepwr, np, 16 );
		snr = 10*log10(sigpwr / noisepwr);

		snprintf(snrmsg, sizeof(snrmsg), "s/n %3.0f dB", snr);
		put_Status2(snrmsg);
		m_metric = CLAMP((3000 / delta) * (sigpwr/noisepwr), 0.0, 100.0);
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
			{ -deviation_f - 10, -deviation_f + 5 },
			{  deviation_f - 5,  deviation_f + 10 } };
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
				double pwr_left = wf->powerDensity ( max_carrier - deviation_f, 30 );
				double pwr_right = wf->powerDensity( max_carrier + deviation_f, 30 );
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
			case SYNC:
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

	// The signal is sampled at three points: early, prompt, and late.
	// The prompt event is where the signal is decoded, while early and
	// late are only used to adjust the time of the sampling to match
	// the incoming signal.
	//
	// The early event happens 1/5 bit period before the prompt event,
	// and the late event 1/5 bit period later. If the incoming signal
	// peaks early, it means the decoder is late. That is, if the early
	// signal is "too large", decoding should to happen earlier.
	//
	// Attempt to center the signal so the accumulator is at its
	// maximum deviation at the prompt event. If the bit is decoded
	// too early or too late, the code is more sensitive to noise,
	// and less likely to decode the signal correctly.
	void process_multicorrelator() {
		// Adjust the sampling period once every 8 bit periods.
		if (m_sample_count % (int)(m_bit_sample_count * 8))
			return;

		// Calculate the slope between early and late signals
		// to align the logic sampling with the received signal
		double slope = m_average_late_signal - m_average_early_signal;

		if (m_average_prompt_signal < m_average_early_signal &&
		    m_average_prompt_signal < m_average_late_signal)
			// At a signal minimum. Get out quickly.
			slope /= 2;
		else if (m_average_prompt_signal > m_average_late_signal &&
			 m_average_prompt_signal > m_average_late_signal)
			// Limit the adjustment, to ride out noise
			slope /= 128;
		else
			slope /= 32;

		if (slope) {
			m_next_early_event += slope;
			m_next_prompt_event += slope;
			m_next_late_event += slope;
			LOG_DEBUG("adjusting by %1.2f, early %1.1f, prompt %1.1f, late %1.1f", slope, m_average_early_signal, m_average_prompt_signal, m_average_late_signal);
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
			int n_out;
			cmplx z, zmark, zspace, *zp_mark, *zp_space;

			short v = static_cast<short>(32767 * data[i]);

			m_time_sec = m_sample_count / m_sample_rate ;

			double dv = v;
			z = cmplx(dv, dv);

			zmark = mixer(m_mark_phase, m_mark_f, z);
			m_mark_lowpass->run(zmark, &zp_mark);

			zspace = mixer(m_space_phase, m_space_f, z);
			n_out = m_space_lowpass->run(zspace, &zp_space);

			if (n_out)
				process_fft_output(zp_mark, zp_space, n_out);
		}
	}

private:
	cmplx mixer(double &phase, double f, cmplx in)
	{
		cmplx z = cmplx( cos(phase), sin(phase)) * in;

		phase -= TWOPI * f / m_sample_rate;
		if (phase < -TWOPI) phase += TWOPI;

		return z;
	}

	// noise average decays fast down, slow up
	double noise_decay(double avg, double value) {
		int divisor;
		if (value < avg)
			divisor = m_bit_sample_count / 4;
		else
			divisor = m_bit_sample_count * 48;
		return decayavg(avg, value, divisor);
	}

	// envelope average decays fast up, slow down
	double envelope_decay(double avg, double value) {
		int divisor;
		if (value > avg)
			divisor = m_bit_sample_count / 4;
		else
			divisor = m_bit_sample_count * 16;
		return decayavg(avg, value, divisor);
	}

	void process_fft_output(cmplx *zp_mark, cmplx *zp_space, int samples) {
		// envelope & noise levels for mark & space, respectively
		static double mark_env = 0, space_env = 0;
		static double mark_noise = 0, space_noise = 0;

		for (int i = 0; i < samples; i++) {
			double mark_abs = abs(zp_mark[i]);
			double space_abs = abs(zp_space[i]);

			process_multicorrelator();

			m_audio_average += (std::max(mark_abs, space_abs) - m_audio_average) * m_audio_average_tc;

			m_audio_average = std::max(.1, m_audio_average);

			// determine noise floor & envelope for mark & space
			mark_env = envelope_decay(mark_env, mark_abs);
			mark_noise = noise_decay(mark_noise, mark_abs);

			space_env = envelope_decay(space_env, space_abs);
			space_noise = noise_decay(space_noise, space_abs);

			double noise_floor = (space_noise + mark_noise) / 2;

			// clip mark & space to envelope & floor
			mark_abs = min(mark_abs, mark_env);
			mark_abs = max(mark_abs, noise_floor);

			space_abs = min(space_abs, space_env);
			space_abs = max(space_abs, noise_floor);

			// mark-space discriminator with automatic threshold
			// correction, see:
			// http://www.w7ay.net/site/Technical/ATC/
			double logic_level =
				(mark_abs - noise_floor) * (mark_env - noise_floor) -
				(space_abs - noise_floor) * (space_env - noise_floor) -
				0.5 * ( (mark_env - noise_floor) * (mark_env - noise_floor) -
					 (space_env - noise_floor) * (space_env - noise_floor));

			// the accumulator hits max when mark_state flips sign
			bool mark_state = (logic_level > 0);
			m_early_accumulator += (mark_state) ? 1 : -1;
			m_prompt_accumulator += (mark_state) ? 1 : -1;
			m_late_accumulator += (mark_state) ? 1 : -1;

			// An average of the magnitude of the accumulator
			// is taken at the sample point, as well as a quarter
			// bit before and after. This allows the code to see
			// the best time to sample the signal without relying
			// on (noisy) null crossings.
			if (m_sample_count >= m_next_early_event) {
				m_average_early_signal = decayavg(
						m_average_early_signal,
						fabs(m_early_accumulator), 64);
				m_next_early_event += m_bit_sample_count;
				m_early_accumulator = 0;
			}

			if (m_sample_count >= m_next_late_event) {
				m_average_late_signal = decayavg(
						m_average_late_signal,
						fabs(m_late_accumulator), 64);
				m_next_late_event += m_bit_sample_count;
				m_late_accumulator = 0;
			}

			// the end of a signal pulse
			// the accumulator should be at maximum deviation
			m_pulse_edge_event = m_sample_count >= m_next_prompt_event;
			if (m_pulse_edge_event) {
				m_average_prompt_signal = decayavg(
						m_average_prompt_signal,
						fabs(m_prompt_accumulator), 64);
				m_next_prompt_event += m_bit_sample_count;
				m_averaged_mark_state = m_prompt_accumulator;
				if (m_ptr_navtex->get_reverse())
					m_averaged_mark_state = -m_averaged_mark_state;
				m_prompt_accumulator = 0;
			}

			if (m_audio_average < m_audio_minimum) {
				set_state(NOSIGNAL);
			} else if (m_state == NOSIGNAL) {
				set_state(SYNC_SETUP);
			}

			switch (m_state) {
				case NOSIGNAL: break;
				case SYNC_SETUP:
					m_error_count = 0;
					m_shift = false;
					set_state(SYNC);
					break;
				case SYNC:
				case READ_DATA:
					if (m_pulse_edge_event)
						handle_bit_value(m_averaged_mark_state);
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
			try
			{
				ccir_msg.display(alt_string);
				put_received_message( alt_string );
			} catch( const std::exception & exc ) {
				LOG_WARN("Caught %s", exc.what() );
			}
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

		LOG_DEBUG("Delay=%f", max_seconds );
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

	/// Note path std::string can contain null characters. TODO: Beware of the extra copy constructor.
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

	/// Input value must be between -1 and 1
	void add_sample( double sam )
	{
		m_tx_buf[ m_tx_counter++ ] = sam ;

		if( m_tx_counter == m_tx_block_len ) {
			tx_flush();
		}
	}

// REMI : Note change to send_sine
	void send_sine( double seconds, double freq )
	{
		static double phase = 0;
		int nb_samples = seconds * m_ptr_navtex->get_samplerate();
		double max_level = 0.9;//0.99 ; // Between -1.0 and 1.0
		double ratio = 2.0 * M_PI * (double)freq / (double)m_ptr_navtex->get_samplerate() ;
		for (int i = 0; i < nb_samples ; ++i )
		{
			add_sample( max_level * sin( phase += ratio));//i * ratio ) );
			if (phase > 2.0 * M_PI) phase -= 2.0*M_PI;
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
/// For testing purpose, this file can be compiled and run separately as a command-line program.
int main(int n, const char ** v )
{
	printf("%s\n", v[1] );
	FILE * f = fl_fopen( v[1], "r" );
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
	modem::cap |= CAP_AFC | CAP_REV;
	navtex::mode = md;
	modem::samplerate = 11025;
	modem::bandwidth = 2 * deviation_f ;
	modem::reverse = false ;
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

