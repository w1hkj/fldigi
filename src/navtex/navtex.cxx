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

#include "config.h"

#include "configuration.h"
#include "fl_digi.h"
#include "debug.h"
#include "gettext.h"
#include "navtex.h"
#include "logbook.h"

static const double PI = 3.1415926 ;

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

	// allow parameter change while running
	void reconfigure(double cf) {
		m_center_freq = cf;
		// only used for peaking and shelving filters
		m_gain_abs = pow(10, m_gainDB / 40);
		double omega = 2 * PI * cf / m_sample_rate;
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
	bool m_shift;
public:
	CCIR476() {
		m_shift = false;
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

	int char_to_code(int ch, bool ex_shift) {
		ch = toupper(ch);
		// default: return -ch
		// avoid unnececessary shifts
		if (ex_shift && m_figs_to_code[ch] != '\0') {
			return m_figs_to_code[ch];
		}
		if (!ex_shift && m_ltrs_to_code[ch] != '\0') {
			return m_ltrs_to_code[ch];
		}
		if (m_figs_to_code[ch] != '\0') {
			m_shift = true;
			return m_figs_to_code[ch];
		}
		if (m_ltrs_to_code[ch] != '\0') {
			m_shift = false;
			return m_ltrs_to_code[ch];
		}
		return -ch;
	}

	int code_to_char(int code, bool shift) {
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

	char m_subject ;
	const char * msg_type(void) const
	{
		switch(m_subject) {
			case 'A' : return "Navigational warning";
			case 'B' : return "Meteorological warning";
			case 'C' : return "Ice report";
			case 'D' : return "Search & rescue information, pirate warnings";
			case 'E' : return "Meteorological forecast";
			case 'F' : return "Pilot service message";
			case 'G' : return "AIS message";
			case 'H' : return "LORAN message";
			case 'I' : return "Not used";
			case 'J' : return "SATNAV messages";
			case 'K' : return "Other electronic navaid messages";
			case 'L' : return "Navigational warnings";
			case 'T' : return "Test transmissions (UK only)";
			case 'V' : return "Notice to fishermen (U.S. only)";
			case 'W' : return "Environmental (U.S. only)";
			case 'X' : return "Special services - allocation by IMO NAVTEX Panel";
			case 'Y' : return "Special services - allocation by IMO NAVTEX Panel";
			case 'Z' : return "No message on hand";
			default  : return "";
		}
	}
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
				default  : chrSeen = true;
						   if( wasDelim ) {
							   newStr.push_back(';');
							   wasDelim = false ;
							   wasSpace = false ;
						   }
						   if( wasSpace && chrSeen ) {
							   newStr.push_back(' ');
							   wasSpace = false ;
						   }
						   newStr.push_back( *it );
			}
		}
		swap( newStr );
	}
public:
	ccir_message() {}

	ccir_message( const std::string & s )
	: std::string(s)
	, m_subject(' ') {
		cleanup();
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

				ccir_message msg_cut( substr( 0, size() - header_len ) );
				m_subject = comp[6];
				// Remove the beginning useless chars.
				std::string tp_str = msg_type() ;
				if( ! tp_str.empty() ) tp_str += ":";
				tp_str += substr( size() - header_len + 5 );
				swap( tp_str );
				return detect_result( true, msg_cut );
			}
		}
		return detect_result( false, ccir_message() ); ;
	}

	bool detect_end() {
		// static const char * stop_valid = "\r\nNNNN\r\n";
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

	void display( const std::string & type ) {
		cleanup();
		if( progdefaults.NVTX_AdifLog ) {
			/// For updating the logbook with received messages.
			cQsoRec qso_rec ;

			qso_rec.putField(CALL, "Navtex");
			qso_rec.putField(NAME, "Navtex");
			qso_rec.setDateTime(true);
			qso_rec.setDateTime(false);
			qso_rec.setFrequency( wf->rfcarrier() );

			qso_rec.putField(MODE, mode_info[MODE_NAVTEX].adif_name );

			// m_qso_rec.putField(QTH, inpQth_log->value());
			// m_qso_rec.putField(STATE, inpState_log->value());
			// m_qso_rec.putField(VE_PROV, inpVE_Prov_log->value());
			// m_qso_rec.putField(COUNTRY, inpCountry_log->value());
			// m_qso_rec.putField(GRIDSQUARE, inpLoc_log->value());
			// m_qso_rec.putField(QSLRDATE, inpQSLrcvddate_log->value());
			// m_qso_rec.putField(QSLSDATE, inpQSLsentdate_log->value());
			// m_qso_rec.putField(RST_RCVD, inpRstR_log->value ());
			// m_qso_rec.putField(RST_SENT, inpRstS_log->value ());
			// m_qso_rec.putField(SRX, inpSerNoIn_log->value());
			// m_qso_rec.putField(STX, inpSerNoOut_log->value());
			// m_qso_rec.putField(XCHG1, inpXchgIn_log->value());
			// m_qso_rec.putField(MYXCHG, inpMyXchg_log->value());
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

static const double deviation_f = 90.0;

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

	CCIR476				m_ccir476;
	typedef std::list<int> sync_chrs_type ;
	sync_chrs_type		 m_sync_chrs;
	ccir_message		   m_curr_msg ;

	int					m_c1, m_c2, m_c3;
	static const int	   m_zero_crossings_divisor = 4;
	std::vector<int>	   m_zero_crossings ;
	long				   m_zero_crossing_count;
	double				 m_message_time ;
	double				 m_sample_interval;
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
	bool				   m_valid_message ;
	// filter method related
	double				 m_center_frequency_f ;

public:
	navtex_implementation(int the_sample_rate) {
		m_state = NOSIGNAL;
		m_message_time = 0.0 ;
		m_signal_accumulator = 0;
		m_audio_average = 0;
		m_audio_minimum = 256;
		m_baud_rate = 100;
		m_sample_rate = the_sample_rate;
		m_bit_duration = 0;
		m_next_event_count = 0;
		m_shift = false;
		m_alpha_phase = false;
		m_valid_message = false;
		m_center_frequency_f = 1000.0;
		m_audio_average_tc = 1000.0 / m_sample_rate;
		// this value must never be zero and bigger than 10.
		m_baud_rate = 100;
		double m_bit_duration_seconds = 1.0 / m_baud_rate;
		m_bit_sample_count = (int) (m_sample_rate * m_bit_duration_seconds + 0.5);
		m_half_bit_sample_count = m_bit_sample_count / 2;
		m_pulse_edge_event = false;
		m_error_count = 0;
		m_valid_count = 0;
		m_sample_interval = 1.0 / m_sample_rate;
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
private:

	void set_filter_values() {
		// carefully manage the parameters WRT the center frequency
		m_center_frequency_f = 1000 ;
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

	void process_messages(int c) {
		m_curr_msg.push_back((char) c);
		ccir_message::detect_result msg_cut = m_curr_msg.detect_header();
		if ( msg_cut.first ) {
			/// Maybe the message was already valid.
			if( m_valid_message )
			{
				display_message("Lost trailer", msg_cut.second );
			}
			else
			{
				/// Maybe only non-significant chars.
				if( ! msg_cut.second.empty() )
				{
					LOG_INFO("msg_cut=(%s)", msg_cut.second.c_str() );
					display_message("Lost header and trailer", msg_cut.second );
				}
			}
			m_valid_message = true;
			m_message_time = m_time_sec;

		} else { // valid message state
			// if stop mark detected or 10-min time limit exceeded
			if ( m_curr_msg.detect_end() || (m_time_sec - m_message_time > 600)) {
				if( m_valid_message)
				{
					m_valid_message = false;
					display_message("Message OK", m_curr_msg);
				}
				else
				{
					display_message("Lost header", m_curr_msg);
				}
				m_curr_msg.clear();
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
			/// TODO: Add a real beep.
			for( const char * p = "[BEEP]"; *p; ++p ) {
			put_rx_char(*p);
		}
		} else if (c != -1 && c != '\r' && c != code_alpha && c != code_rep) {
			put_rx_char(c);
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
		for( int i =0; i < nb_samples; ++i ) {
			short v = static_cast<short>(32767 * data[i]);

			// printf("%d\n",(int)v);
			m_time_sec = m_sample_count * m_sample_interval;
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
								LOG_INFO("restarting sync");
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
	}

	/// This updates the window label according to the state.
	void set_label_from_state(void) const
	{
		put_status( state_to_str(m_state) );
	}

private:
	/// Each received file has its name pushed in this queue.
	syncobj m_sync_rx ;
	std::queue< std::string > m_received_messages ;

	void display_message( const std::string & type, ccir_message & ccir_msg ) {
		ccir_msg.display( type );
		put_received_message( type + ":" + ccir_msg );
	}

	/// Called by the engine each time a file is saved.
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
	navtex::mode = md;
	modem::samplerate = 11025;
    modem::bandwidth = 2 * deviation_f ;
	m_impl = new navtex_implementation( modem::samplerate );
	cap &= ~CAP_TX;
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
}

int  navtex::tx_process()
{
	return -1;
}

/// This returns the next received message.
std::string navtex::get_message(int max_seconds)
{
	return m_impl->get_received_message(max_seconds);
}

