// ----------------------------------------------------------------------------
// wefax.cxx  --  Weather Fax modem
//
// Copyright (C) 2010
//		Remi Chateauneu, F4ECW
//
// This file is part of fldigi.  Adapted from code contained in HAMFAX source code 
// distribution.
//  Hamfax Copyright (C) Christof Schmitt
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
// ----------------------------------------------------------------------------

#include <config.h>

#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <valarray>
#include <cmath>
#include <cstddef>
#include <queue>
#include <map>
#include <libgen.h>

#include "debug.h"
#include "gettext.h"
#include "wefax.h"
#include "modem.h"
#include "main.h"

#include "misc.h"
#include "timeops.h"
#include "fl_digi.h"
#include "configuration.h"
#include "status.h"
#include "filters.h"
#include "strutil.h"

#include "wefax-pic.h"

#include "ascii.h"

#include "qrunner.h"

using namespace std;

//=============================================================================
//
//=============================================================================

#define PUT_STATUS( A )                             \
{                                                   \
	std::stringstream strm_status ;              \
	strm_status << A ;                           \
	put_status( strm_status.str().c_str() );     \
	LOG_DEBUG("%s", strm_status.str().c_str() ); \
}

//=============================================================================
// Core of the code taken from Hamfax.
//=============================================================================

/// No reasonable FIR filter should have more coefficients than that.
#define MAX_FILT_SIZE 256

struct fir_coeffs
{
	const char * _name ;
	int          _size ;
	const double _coefs[MAX_FILT_SIZE];
};

// Narrow, middle and wide fir low pass filter from ACfax
static const fir_coeffs input_filters[] = {
	{ _("Narrow"), 17,
		{ -7,-18,-15, 11, 56,116,177,223,240,223,177,116, 56, 11,-15,-18, -7}
	},
	{ _("Middle"), 17,
		{  0,-18,-38,-39,  0, 83,191,284,320,284,191, 83,  0,-39,-38,-18,  0}
	},
	{ _("Wide"),   17,
		{  6, 20,  7,-42,-74,-12,159,353,440,353,159,-12,-74,-42,  7, 20,  6}
	},
	{ "Blackman", 17,
		{
			-2.7756e-15,
			2.9258e+00,
			1.3289e+01,
			3.4418e+01,
			6.8000e+01,
			1.1095e+02,
			1.5471e+02,
			1.8770e+02,
			2.0000e+02,
			1.8770e+02,
			1.5471e+02,
			1.1095e+02,
			6.8000e+01,
			3.4418e+01,
			1.3289e+01,
			2.9258e+00,
			-2.7756e-15
		}
	},
	{ "Hanning", 17,
		{
     			0.00000,
     			7.61205,
    			29.28932,
    			61.73166,
   			100.00000,
   			138.26834,
   			170.71068,
   			192.38795,
   			200.00000,
   			192.38795,
   			170.71068,
   			138.26834,
   			100.00000,
    			61.73166,
    			29.28932,
     			7.61205,
     			0.00000
		}
	},
	{ "Hamming", 17,
		{
			16.000,
			23.003,
			42.946,
			72.793,
			108.000,
			143.207,
			173.054,
			192.997,
			200.000,
			192.997,
			173.054,
			143.207,
			108.000,
			72.793,
			42.946,
			23.003,
			16.000
		}
	}
};
static const size_t nb_filters = sizeof(input_filters)/sizeof(input_filters[0]); ;

/// This contains all possible reception filters.
class fir_filter_pair_set : public std::vector< C_FIR_filter >
{
	/// This, because C_FIR_filter cannot be copied.
	fir_filter_pair_set(const fir_filter_pair_set &);
	fir_filter_pair_set & operator=(const fir_filter_pair_set &);
public:
	static const char ** filters_list(void)
	{
		/// There will be a small memory leak when leaving. No problem at all.
		static const char ** stt_filters = NULL ;
		if( stt_filters == NULL ) {
			stt_filters = new const char * [nb_filters + 1];
			for( size_t ix_filt = 0 ; ix_filt < nb_filters ; ++ix_filt ) {
				stt_filters[ix_filt] = input_filters[ix_filt]._name ;
			}
			stt_filters[nb_filters] = NULL ;
		}
		return stt_filters ;
	}

	fir_filter_pair_set()
	{
		/// Beware that C_FIR_filter cannot be copied with its content.
		resize( nb_filters );
		for( size_t ix_filt = 0 ; ix_filt < nb_filters ; ++ix_filt )
		{
			// Same filter for real and imaginary.
			const fir_coeffs * ptr_filt = input_filters + ix_filt ;
			// init() should take const double pointers.
			operator[]( ix_filt ).init( ptr_filt->_size, 1,
					const_cast< double * >( ptr_filt->_coefs ),
					const_cast< double * >( ptr_filt->_coefs ) );
		}
	}
}; // fir_filter_pair_set

/// Speed-up of trigonometric calculations.
template <class T> class lookup_table {
	T    * m_table;
	size_t m_table_size;
	size_t m_next;
	size_t m_increment;

	/// No default constructor because it would invalidate the buffer pointer.
	lookup_table();
	lookup_table(const lookup_table &);
	lookup_table & operator=(const lookup_table &);
public:
	lookup_table(const size_t N)
	: m_table_size(N), m_next(0), m_increment(0)
	{
		assert( N != 0 );

		/// If no more memory, an exception will be thrown.
		m_table=new T[m_table_size];
	}

	~lookup_table(void)
	{
		delete[] m_table;
	}

	T& operator[](size_t i)
	{
		return m_table[i];
	}

	void set_increment(size_t i)
	{
		m_increment=i;
	}

	T next_value(void)
	{
		m_next+=m_increment;
		if(m_next>=m_table_size) {
			m_next%=m_table_size;
		}
		return m_table[m_next];
	}

	size_t size(void) const
	{
		return m_table_size;
	}

	void reset(void)
	{
		m_next=0;
	}
}; // lookup_table

typedef enum {
	RXAPTSTART,
	RXAPTSTOP,
	RXPHASING,
	RXIMAGE,
	TXAPTSTART,
	TXPHASING,
	ENDPHASING,
	TXIMAGE,
	TXAPTSTOP,
	IDLE } fax_state;

static const char * state_to_str(fax_state a_state)
{
	switch( a_state )
	{
		case RXAPTSTART : return _("APT reception start") ;
		case RXAPTSTOP  : return _("APT reception stop") ;
		case RXPHASING  : return _("Phasing reception") ;
		case RXIMAGE    : return _("Receiving") ;
		case TXAPTSTART : return _("APT transmission start") ;
		case TXAPTSTOP  : return _("APT stop") ;
		case TXPHASING  : return _("Phasing transmission") ;
		case ENDPHASING : return _("End phasing") ;
		case TXIMAGE    : return _("Sending image") ;
		case IDLE       : return _("Idle") ;
	}
	return "UNKNOWN" ;
};

/// TODO: This should be hidden to this class.
static const int bytes_per_pixel = 3;

#define IOC_576 576
#define IOC_288 288

/// Index of correlation to image width.
static int ioc_to_width( int ioc )
{
	return ioc * M_PI ;
};

/// Used by bandwidth. Standard shift is 800 Hz, so deviation is 400 Hz.
/// Deutsche Wetterdienst (DWD) shift is 850 Hz.
static int fm_deviation = -1 ;

#define GARBAGE_STR "garbage"

class fax_implementation {
	wefax * m_ptr_wefax ;  // Points to the modem of which this is the implementation.
	fax_state m_rx_state ; // RXPHASING, RXIMAGE etc...
	int m_sample_rate;     // Set at startup: 8000, 11025 etc...
	int m_current_value;   // Latest received pixel value.
	bool m_apt_high;
	int m_apt_trans;       // APT low-high transitions
	int m_apt_count;       // samples counted for m_apt_trans
	int m_apt_start_freq;  // Standard APT frequency. Set at startup.
	int m_apt_stop_freq;   // Standard APT frequency.
	bool m_phase_high;     // When state=RXPHASING
	int m_curr_phase_len;  // Counts the len of the image band used for phasing
	int m_curr_phase_high; // Counts the len of the white part of the phasing image band. 
	int m_phase_lines;
	int m_num_phase_lines;
	int m_phasing_calls_nb;// Number of calls to decode_phasing for the current image.
	double m_lpm_img;      // Lines per minute.
	double m_lpm_sum_rx;   // Sum of the latest LPM values, when RXPHASING.
	int m_default_lpm;     // 120 for WEFAX_576, 60 for WEFAX_288.
	int m_img_width;       // Calculated with IOC=576 or 288.
	int m_img_sample;      // Current received samples number when in RXIMAGE.
	int m_last_col;        // Current col based on samples number, decides to set a pixel when changes.
	int m_pixel_val;       // Accumulates received samples, then averaged to set a pixel.
	int m_pix_samples_nb;  // Number of samples read for the next pixel, and current column. About 4 if 11025 Hz.
	bool m_img_color;      // Whether this is a color image or not.
	fax_state m_tx_state;  // Modem state when transmitting.
	int m_tx_phasing_lin;  // Nb of phasing lines sent when transmitting an image.
	int m_start_duration;  // Number of seconds for sending ATP start.
	int m_stop_duration;   // Number of seconds for sending APT stop.
	int m_img_tx_cols;     // Number of columns when transmitting.
	int m_img_tx_rows;     // Number of rows when transmitting.
	int m_carrier;         // Normalised fax carrier frequency. Should be identical to modem::get_freq().
	int m_fax_pix_num;     // Index of current pixel in received image.
	const unsigned char * m_xmt_pic_buf ; // Bytes to send. Number of pixels by three.
	bool m_freq_mod ;      // Frequency modulation or AM.
	bool m_manual_mode ;   // Tells whether everything is read, or apt+phasing detection.

	/// The number of samples sent for one line. The LPM is given by the GUI. Typically 5512.
	double m_smpl_per_lin ;// Recalculated each time m_lpm_img is updated.

	int m_ix_filt ;        // Index of the current reception filter.

	static const int range_sample = 255 ;

	// At the moment, never found an example where it should be negated.
	static const bool m_phase_inverted = false;

	/// This is always the same filter for all objects.
	static fir_filter_pair_set m_rx_filters ;

	/// These are used for transmission.
	lookup_table<double> m_dbl_sine;
	lookup_table<double> m_dbl_cosine;
	lookup_table<double> m_dbl_arc_sine;

	/// Stores a result based on the previous received sample.
	double m_i_fir_old;
	double m_q_fir_old;

	void decode(const int* buf, int nb_samples);

	/// Used for transmission.
	lookup_table<short> m_short_sine;

	/// This evaluates the average and standard deviation of the current image,
	// using the histogram.
	class statistics
	{
		static const size_t m_sz = 256 ;
		int m_hist[m_sz];
		mutable double m_avg ;
		mutable double m_dev ;
	public:
		void add_bw( int pix_val ) {
			pix_val = pix_val < 0 ? 0 : pix_val > 255 ? 255 : pix_val ;
			m_hist[ pix_val ]++;
		}
		void calc() const {
			m_avg = 0.0;
			int sum = 0 ;
			int sum_vals = 0 ;
			for( size_t i = 0; i < m_sz; ++ i ) {
				int weight = m_hist[i];
				sum_vals +=  i * weight ;
				sum += weight ;
			}
			if( sum == 0 ) {
				m_avg = 0.0 ;
				m_dev = 0.0 ;
				return ;
			}
			m_avg = sum_vals / sum ;

			double sum_dlt_pwr2 = 0.0 ;
			for( size_t i = 0; i < m_sz; ++ i ) {
				int weight = m_hist[i];
				double val = i - m_avg ;
				sum_dlt_pwr2 += val * val * weight ;
			}
			m_dev = sqrt( sum_dlt_pwr2 / sum );
		}
		double average() const { return m_avg; }
		double stddev() const { return m_dev; };
		void reset() {
			std::fill( m_hist, m_hist + m_sz, 0 );
		}
	}; 

	statistics m_statistics ;

	fax_implementation();

	/// Needed when starting image reception, after phasing.
	void reset_counters(void)
	{
		m_last_col       = 0 ;
		m_fax_pix_num    = 0 ;
		m_img_sample     = 0;
		m_pix_samples_nb = 0;
	}

	// These counters used for phasing.
	void reset_phasing_counters()
	{
		m_lpm_sum_rx = 0;
		m_phasing_calls_nb = 0 ;
		m_phase_high = m_current_value>=128 ? true : false;
		m_curr_phase_len=m_curr_phase_high=0;
		m_phase_lines=m_num_phase_lines=0;
	}

	/// Called at init time or when the carrier changes.
	void reset_increments(void)
	{
		/// This might happen at startup. Not a problem.
		if(m_sample_rate == 0) {
			return;
		}
		m_dbl_sine.set_increment(m_dbl_sine.size()*m_carrier/m_sample_rate);
		m_dbl_cosine.set_increment(m_dbl_cosine.size()*m_carrier/m_sample_rate);
		m_dbl_sine.reset();
		m_dbl_cosine.reset();

		m_short_sine.reset();
		if(!m_freq_mod ) {
			m_short_sine.set_increment(m_short_sine.size()*m_carrier/m_sample_rate);
		}
	}

public:
	fax_implementation(int fax_mode, wefax * ptr_wefax );
	void init_rx(int the_smpl_rate);
	void skip_apt_rx(void);
	void skip_phasing_to_image_save(const std::string & comment);
	void skip_phasing_to_image(bool auto_center);
	void skip_phasing_rx(bool auto_center);
	void end_rx(void);
	void rx_new_samples(const double* audio_ptr, int audio_sz);
        void init_tx(int the_smpl_rate);
	void modulate(const double* buffer, int n);
	void tx_params_set(
		int the_lpm,
		const unsigned char * xmtpic_buffer,
		bool is_color,
		int img_w,
		int img_h );
	bool trx_do_next(void);
	void tx_apt_stop(void);

	double carrier(void) const
	{
		return m_carrier ;
	}

	/// The trigo tables have an increment which depends on the carrier frequency.
	void set_carrier(double freq)
	{
		m_carrier = freq ;
		reset_increments();
	}

	int fax_width(void) const
	{
		return m_img_width ;
	}

	void set_filter_rx( int idx_filter )
	{
		m_ix_filt = idx_filter ;
	}

	/// When set, starts receiving faxes without interruption other than manual.
	void manual_mode_set( bool manual_flag )
	{
		m_manual_mode = manual_flag ;
	}

	/// Called by the GUI.
	bool manual_mode_get(void) const
	{
		return m_manual_mode ;
	}

	int lpm_to_samples( int the_lpm ) const {
		return m_sample_rate * 60.0 / the_lpm ;
	}

	/// Called by the GUI.
	void lpm_set( int the_lpm )
	{
		m_lpm_img = the_lpm ;
		m_smpl_per_lin = lpm_to_samples( m_lpm_img );
	}

	/// This generates a filename based on the frequency, current time, internal state etc...
	std::string generate_filename( const char *extra_msg ) const ;

	const char * state_rx_str(void) const
	{
		return state_to_str(m_rx_state);
	}

	/// Returns a string telling the state. Informational purpose.
	std::string state_string(void) const
	{
		std::stringstream result ;
		result	<< "tx:" << state_to_str(m_tx_state)
			<< " "
			<< "rx:" << state_to_str(m_rx_state);
		return result.str();
	};

private:
	/// Centered around the frequency.
	double power_usb_noise(void) const
	{
		static double avg_pwr = 0.0 ;
       		double pwr = wf->powerDensity(m_carrier, 2 * fm_deviation) + 1e-10;

       		return decayavg( avg_pwr, pwr, 10 );
	}

	/// This evaluates the power signal when APT start frequency. This frequency pattern
	/// can be observed on the waterfall.
	double power_usb_apt_start(void) const
	{
		static double avg_pwr = 0.0 ;
		/// Value approximated by watching the waterfall.
		static const int bandwidth_apt_start = 10 ;
       		double pwr
			= wf->powerDensity(m_carrier - 2 * m_apt_start_freq, bandwidth_apt_start)
			+ wf->powerDensity(m_carrier -     m_apt_start_freq, bandwidth_apt_start)
			+ wf->powerDensity(m_carrier                       , bandwidth_apt_start)
			+ wf->powerDensity(m_carrier +     m_apt_start_freq, bandwidth_apt_start);
			+ wf->powerDensity(m_carrier + 2 * m_apt_start_freq, bandwidth_apt_start);

       		return decayavg( avg_pwr, pwr, 10 );
	}

	/// Estimates the signal power when the phasing signal is received.
	double power_usb_phasing(void) const
	{
		static double avg_pwr = 0.0 ;
		/// Rough estimate based on waterfall observation.
		static const int bandwidth_phasing = 1 ;
       		double pwr = wf->powerDensity(m_carrier - fm_deviation, bandwidth_phasing);

       		return decayavg( avg_pwr, pwr, 10 );
	}

	/// There is some power at m_carrier + fm_deviation but this is neglictible.
	double power_usb_image(void) const
	{
		static double avg_pwr = 0.0 ;
		/// This value is obtained by watching the waterfall.
		static const int bandwidth_image = 100 ;
       		double pwr = wf->powerDensity(m_carrier + fm_deviation, bandwidth_image);

       		return decayavg( avg_pwr, pwr, 10 );
	}

	/// Hambourg Radio sends a constant freq which looks like an image.
	/// We eliminate it by comparing with a black signal.
	/// If this is a real image, bith powers will be close.
	/// If not, the image is very important, but not the black.
	/// TODO: Consider removing this, because it is not used.
	double power_usb_black(void) const
	{
		static double avg_pwr = 0.0 ;
		/// This value is obtained by watching the waterfall.
		static const int bandwidth_black = 20 ;
       		double pwr = wf->powerDensity(m_carrier - fm_deviation, bandwidth_black);

       		return decayavg( avg_pwr, pwr, 10 );
	}

	/// Evaluates the signal power for APT stop frequency.
	double power_usb_apt_stop(void) const
	{
		static double avg_pwr = 0.0 ;
		/// This value is obtained by watching the waterfall.
		static const int bandwidth_apt_stop = 50 ;
       		double pwr = wf->powerDensity(m_carrier - m_apt_stop_freq, bandwidth_apt_stop);

       		return decayavg( avg_pwr, pwr, 10 );
	}

	/// This evaluates the signal/noise ratio for some specific bandwidths.
	class fax_signal {
		const fax_implementation * _ptr_fax ;
		int                        _cnt ; /// The value can be reused a couple of times.

		double                     _apt_start ;
		double                     _phasing ;
		double                     _image ;
		double                     _black ;
		double                     _apt_stop ;

		fax_state                  _state ; /// Deduction made based on signal power.
		const char *               _text;
		const char *               _stop_code;

		/// Finer tests can be added. These are all rule-of-thumb values based
		/// on observation of real signals.
		/// TODO: Adds a learning mode using the Hamfax detector to train the signal-based one.
		void set_state(void)
		{
			_state = IDLE ;
			_text = _("No signal detected");
			_stop_code = "" ;

			if( 	( _apt_start   > 20.0 ) &&
				( _phasing     < 10.0 ) &&
				( _image       < 10.0 ) &&
				( _apt_stop    < 10.0 ) ) {
				_state = RXAPTSTART ;
				_text = _("Strong APT start signal: Skip to phasing.");
			}
			if(     /// The image signal may be weak if the others signals are even weaker.
			(	( _apt_start   <  1.0 ) &&
				( _phasing     <  1.0 ) &&
				( _image       > 10.0 ) &&
				( _apt_stop    <  1.0 ) ) ||
			(	( _apt_start   <  1.0 ) &&
				( _phasing     <  0.5 ) &&
				( _image       >  8.0 ) &&
				( _apt_stop    <  0.5 ) ) ||
			(	( _apt_start   <  3.0 ) &&
				( _phasing     <  1.0 ) &&
				( _image       > 15.0 ) &&
				( _apt_stop    <  1.0 ) ) ) {
				_state = RXIMAGE ;
				_text = _("Strong image signal when getting APT start: Starting phasing.");
			}
			if( 	( _apt_start   < 10.0 ) &&
				( _phasing     > 20.0 ) &&
				( _image       < 10.0 ) &&
				( _apt_stop    < 10.0 ) ) {
				_state = RXPHASING ;
				_text = _("Strong phasing signal when getting APT start: Starting phasing.");
			}
			/// TODO: Beware that quite often, it cuts image in the middle.
			// Maybe the levels should be amended.
			if(
			(	( _apt_start   <  2.0 ) &&
				( _phasing     <  2.0 ) &&
				( _image       <  2.0 ) &&
				( _apt_stop    >  8.0 ) ) ||
			(	( _apt_start   <  1.0 ) &&
				( _phasing     <  1.0 ) &&
				( _image       <  1.0 ) &&
				( _apt_stop    >  6.0 ) ) ) {

				/// This test is redundant now that apt_stop_freq is better (On several lines).
				// It had also the disadvantage to be triggered on a plain horizontal line.
				LOG_DEBUG("Strong APT Stop TEMPORARILY DISABLED\n");
				if(false) {
					_state = RXAPTSTOP ;
					_text = _("Strong APT stop signal: Stopping reception.");
					_stop_code = "stop";
				} // END TEMP DISABLED.
			}

			/// Consecutive lines in a wefax image have a strong statistical correlation.
			fax_state state_corr = _ptr_fax->correlation_state( &_stop_code );
			if( ( _state == RXIMAGE ) || ( _state == IDLE ) ) {
				if( state_corr == RXAPTSTOP ) {
					_state = RXAPTSTOP ;
					_text = _("No significant line-to-line correlation: Reception should stop.");
				}
			}

			if( ( _state == IDLE ) || ( _state == RXAPTSTART ) || ( _state == RXPHASING ) ) {
				if( state_corr == RXIMAGE ) {
					_state = RXIMAGE ;
					_text = _("Significant line-to-line correlation: Reception should start.");
				}
			}
		}

		void recalc(void)
		{
			/// Adds a small value to avoid division by zero.
			double noise = _ptr_fax->power_usb_noise() + 1e-10 ;

			/// Multiplications are faster than divisions.
			double inv_noise = 1.0 / noise ;

			_apt_start = _ptr_fax->power_usb_apt_start() * inv_noise ;
			_phasing   = _ptr_fax->power_usb_phasing()   * inv_noise ;
			_image     = _ptr_fax->power_usb_image()     * inv_noise ;
			_black     = _ptr_fax->power_usb_black()     * inv_noise ;
			_apt_stop  = _ptr_fax->power_usb_apt_stop()  * inv_noise ;

			set_state();
		}

		public:
		/// This recomputes, if needed, the various signal power ratios.
		void refresh(void)
		{
			/// Values reevaluated every X input samples. Must be smaller
			/// than the typical audio input frames (512 samples).
			static const int validity = 1000 ;

			/// Calculated at least the first time.
			if( ( _cnt % validity ) == 0 ) {
				recalc();
			}
			/// Does not matter if wrapped beyond 2**31.
			++_cnt ;
		}

		fax_signal( const fax_implementation * ptr_fax)
		: _ptr_fax(ptr_fax)
		, _cnt(0)
		, _text(NULL)
		{}

		fax_state signal_state(void) const { return _state ; }
		const char * signal_text(void) const { return _text; };
		const char * signal_stop_code(void) const { return _stop_code; }

		double image_noise_ratio(void) const { return _image ; }

		/// This updates a Fl_Chart widget.
		void display(void) const
		{
			// Protected with REQ, otherwise will segfault. Do not use REQ_SYNC
			// otherwise it hangs when switching to another mode with a macro.
			REQ( wefax_pic::power,
				_apt_start,
				_phasing,
				_image,
				_black,
				_apt_stop );
		}

		/// For debugging only.
		friend std::ostream & operator<<( std::ostream & refO, const fax_signal & ref_sig )
		{
			refO
				<< " start=" << std::setw(10) << ref_sig._apt_start
				<< " phasing=" << std::setw(10) << ref_sig._phasing
				<< " image=" << std::setw(10) << ref_sig._image
				<< " black=" << std::setw(10) << ref_sig._black
				<< " stop=" << std::setw(10) << ref_sig._apt_stop
				<< " pwr_state=" << state_to_str(ref_sig._state);
			return refO ;
		}

		std::string to_string(void) const
		{
			std::stringstream tmp_strm ;
			tmp_strm << *this ;
			return tmp_strm.str();
		}
	};

	/// We tolerate a small difference between the frequencies.
	bool is_near_freq( int f1, int f2, int margin ) const
	{
		int delta = f1 - f2 ;
		if( delta < 0 )
		{
			delta = -delta ;
		}
		if( delta < margin ) {
			return true ;
		} else {
			return false ;
		}
	}

	void save_automatic(
		const char        * extra_msg,
		const std::string & comment);

	void decode_apt(int x, const fax_signal & the_signal );
	void decode_phasing(int x, const fax_signal & the_signal );
	bool decode_image(int x);

private:
	/// Each received file has its name pushed in this queue.
	syncobj m_sync_rx ;
	std::queue< std::string > m_received_files ;
public:
	/// Called by the engine each time a file is saved.
	void put_received_file( const std::string &filnam )
	{
		guard_lock g( m_sync_rx.mtxp() );
		LOG_INFO("%s", filnam.c_str() );
		m_received_files.push( filnam );
		m_sync_rx.signal();
	}

	/// Returns a received file name, by chronological order.
	std::string get_received_file( int max_seconds )
	{
		guard_lock g( m_sync_rx.mtxp() );

		LOG_INFO(_("delay=%d"), max_seconds );
		if( m_received_files.empty() )
		{
			if( ! m_sync_rx.wait(max_seconds) ) return std::string();
		}
		std::string filnam = m_received_files.front();
		m_received_files.pop();
		return filnam ;
	}
private:
	/// No unicity if the same filename is sent by several clients at the same time.
	/// This does not really matter because the error codes should be the same.
	syncobj m_sync_tx_fil ;
	std::string m_tx_fil ; // Filename being transmitted.

	syncobj m_sync_tx_msg ;
	typedef std::map< std::string, std::string > sent_files_type ;
	sent_files_type m_sent_files ;
public:
	/// If the delay is exceeded, returns with an error message.
	std::string send_file( const std::string & filnam, double max_seconds )
	{
		LOG_INFO("%s rf_carried=%d carrier=%d", filnam.c_str(), 
				static_cast<int>(wf->rfcarrier()), m_carrier );

		bool is_acquired = transmit_lock_acquire(filnam, max_seconds);
		if( ! is_acquired ) return "Timeout sending " + filnam ;

		REQ( wefax_pic::send_image, filnam );
		
		guard_lock g( m_sync_tx_fil.mtxp() );

		sent_files_type::iterator itFi ;
		while(true)
		{
			itFi = m_sent_files.find( filnam );
			if( itFi != m_sent_files.end() )
			{
				break ;
			}
			LOG_INFO("Locking");
			guard_lock g( m_sync_tx_msg.mtxp() );
			LOG_INFO("Waiting %f",max_seconds);
			if( ! m_sync_tx_msg.wait( max_seconds ) )
			{
				LOG_INFO("Timeout %f", max_seconds );
				return "Timeout";
			}
		}
		std::string err_msg = itFi->second ;
		LOG_INFO("err_msg=%s", err_msg.c_str() );
		m_sent_files.erase( itFi );
		return err_msg ;
	}

	/// Called when loading a file from the GUI, or indirectly from XML-RPC.
	bool transmit_lock_acquire(const std::string & filnam, double delay = wefax::max_delay )
	{
		LOG_INFO("Sending %s delay=%f tid=%d", filnam.c_str(), delay, (int)GET_THREAD_ID() );
		guard_lock g( m_sync_tx_fil.mtxp() );
		LOG_INFO("Locked");
		if ( ! m_tx_fil.empty() )
		{
			if( ! m_sync_tx_fil.wait( delay ) ) return false ;
		}
		m_tx_fil = filnam ;
		LOG_INFO("Sent %s", filnam.c_str() );
		return true ;
	}

	/// Allows to send another file. Called by XML-RPC and the GUI.
	void transmit_lock_release( const std::string & err_msg )
	{
		LOG_INFO("err_msg=%s tid=%d", err_msg.c_str(), (int)GET_THREAD_ID() );
		guard_lock g( m_sync_tx_msg.mtxp() );
		LOG_INFO("%s %s", m_tx_fil.c_str(), err_msg.c_str() );
		if( m_tx_fil.empty() )
		{
			LOG_WARN(_("%s: File name should not be empty"), err_msg.c_str() );
		}
		else
		{
			m_sent_files[ m_tx_fil ] = err_msg ;
			m_tx_fil.clear();
		}
		LOG_INFO("Signaling");
		m_sync_tx_msg.signal();
	}

	/// Maybe we could reset this buffer when we change the state so that we could
	/// use the current LPM width.
	struct corr_buffer_t : public std::vector<unsigned char> 
	{
		unsigned char at_mod(size_t i) const {
			return operator[](i % size());
		}
		unsigned char & at_mod(size_t i) {
			return operator[](i % size());
		}
	};
	mutable corr_buffer_t m_correlation_buffer ;
	mutable double m_curr_corr_avg ; // Average line-to-line correlation for the last m_min_corr_lines sampled lines.
	mutable double m_imag_corr_max ; // Max line-to-line correlation for the current image.
	mutable double m_imag_corr_min ; // Min line-to-line correlation for the current image.
	mutable int m_corr_calls_nb;
	static const int m_min_corr_lines = 15 ;

	/// Evaluates the correlation between two lines separated by line_offset pixels.
	double correlation_from_index(size_t line_length, size_t line_offset) const {
                /// This is a ring buffer.
		size_t line_length_plus_img_sample = line_length + m_img_sample ;

		int avg_pred = 0, avg_curr = 0 ;
		for( size_t i = m_img_sample ; i < line_length_plus_img_sample ; ++i ) {
			int pix_pred = m_correlation_buffer.at_mod( i               );
			int pix_curr = m_correlation_buffer.at_mod( i + line_offset );
			avg_pred += pix_pred;
			avg_curr += pix_curr ;
		}
		avg_pred /= line_length;
		avg_curr /= line_length;

		/// Use integers because it is faster. Samples are chars, so no overflow possible.
		int numerator = 0, denom_pred = 0, denom_curr = 0 ;
		for( size_t i = m_img_sample ; i < line_length_plus_img_sample ; ++i ) {
			int pix_pred = m_correlation_buffer.at_mod( i               );
			int pix_curr = m_correlation_buffer.at_mod( i + line_offset );
			int delta_pred = pix_pred - avg_pred ;
			int delta_curr = pix_curr - avg_curr ;
			numerator += delta_pred * delta_curr ;
			denom_pred += delta_pred * delta_pred ;
			denom_curr += delta_curr * delta_curr ;
		}
		double denominator = sqrt( (double)denom_pred * (double)denom_curr );
		if( denominator == 0.0 ) {
			return 0.0 ;
		} else {
			return fabs( numerator / denominator );
		}
	}

	// This is experimental, and is disabled yet.
	// The goal is to eliminate artefact which echoes each pixel a couple of pixels later.
	// This correlates each sample line with itself for each pixel number in a small given
	// range. When the correlation reaches a maximum (A couple of dozens of samples),
	// this estimates the echo delay.
	// This delay is typical of a given emitter, at a given time of the day.
	// Therefore, the array of occurences of a given delay must be refreshed when the frequence
	// is changed.
	// The next step is to estimate the amplitude of the echo, and later to substract
	// each sample to the same sample "delay" mater, with the right attenuation.
	//
	// For an unknown reason, there is frequently a correlation peak after 9 or 10 samples.
	// Just as there were two propagation paths, distant from 10 / 11025 Hz * 300000 = 300 km.
	//
	// This happens with Bracknell (Not far from where I live),
	// and surprisingly the weak signal comes first.
	// This never happens with Deutsche Wetterdienst (Hundredth of kilometers).
	size_t correlation_shift( size_t corr_smpl_lin ) const
	{
		static bool is_init = false ;
		static const size_t max_space_echo = 100 ;

		/// Specific to the antenna and the reception, so no need to clean it up.
		static size_t shift_histogram[max_space_echo];
		static size_t nb_calls = 0 ;

		if( ! is_init ) {
			for( size_t i = 0; i < max_space_echo; ++i ) shift_histogram[i] = 0 ;
			is_init = true ;
		}

		double tmpCorrPrev = correlation_from_index(corr_smpl_lin, 0 );
//		double tmpCorrMax = 0.0 ;
		size_t local_max = 0 ;
		bool is_growing = false ;

		/// We could even start the loop later because we are not interested by small shifts.
		for( size_t i = 1 ; i < max_space_echo; i++ )
		{
			double tmpCorr = correlation_from_index(corr_smpl_lin, i );
			bool is_growing_next = tmpCorr > tmpCorrPrev ;
			if( is_growing && ( ! is_growing_next  ) ) {
				local_max = i - 1 ;
//				tmpCorrMax = tmpCorr ;
				break ;
			}
			is_growing = is_growing_next ;
			tmpCorrPrev = tmpCorr ;
		}

		if( local_max != 0 ) {
			++shift_histogram[local_max];
			// LOG_INFO("Local max: i=%d corr=%lf", local_max, tmpCorrMax );
		}
		++nb_calls ;

		size_t best_shift_idx = 0 ;
		size_t biggest_shift = shift_histogram[best_shift_idx];
		if( ( nb_calls > 100 ) && ( ( nb_calls % 10 ) == 0 ) ) {
			for( size_t i = 0; i < max_space_echo; ++i ) {
				size_t new_hist = shift_histogram[i];
				if( new_hist > biggest_shift ) {
					biggest_shift = new_hist ;
					best_shift_idx = i ;
				}
			}

			LOG_INFO("Shift: i=%d hist=%d", (int)best_shift_idx, (int)biggest_shift );
		}
		return best_shift_idx ;
	}

	static const int min_white_level = 128 ;

	/// Absolute value of statistical correlation between the current line and the previous one.
	/// Called once per maximum sample line.
	void correlation_calc(void) const {
		++m_corr_calls_nb;
		/// We should in fact take the smallest one, 60.
		size_t corr_smpl_lin = lpm_to_samples( m_default_lpm );
		double current_corr = correlation_from_index(corr_smpl_lin, corr_smpl_lin);

		/// This never happened, but who knows (Inaccuracy etc...)?
		if( current_corr > 1.0 ) {
			LOG_WARN("Inconsistent correlation:%lf", current_corr );
			current_corr = 1.0;
		}

		if( m_corr_calls_nb < m_min_corr_lines ) {
			m_curr_corr_avg = current_corr ;
			/// The max value of the correlation must be significative (Does not take peak values).
			m_imag_corr_max = 0.0;
			m_imag_corr_min = 0.0;
		} else {
			/// Equivalent to decayavg with weight= (min_corr_lin +1)/min_corr_lin
			m_curr_corr_avg = ( m_curr_corr_avg * m_min_corr_lines + current_corr ) / ( m_min_corr_lines + 1 );
			m_imag_corr_max = std::max( m_curr_corr_avg, m_imag_corr_max );
			m_imag_corr_min = std::min( m_curr_corr_avg, m_imag_corr_min );
		}

		/// Debugging purpose only.
		if( ( m_corr_calls_nb % 100 ) == 0 ) {
			LOG_DEBUG( "current_corr=%lf m_curr_corr_avg=%lf m_imag_corr_max=%f m_corr_calls_nb=%d "
				"m_min_corr_lines=%d state=%s m_lpm_img=%f",
				current_corr, m_curr_corr_avg, m_imag_corr_max, 
				m_corr_calls_nb, m_min_corr_lines, state_rx_str(), m_lpm_img );
		}
		double metric = m_curr_corr_avg * 100.0 ;
		m_ptr_wefax->display_metric(metric);

		static const bool calc_corr_shift = false ;
		if( calc_corr_shift )
			correlation_shift( corr_smpl_lin );
	}

	/// This is called quite often. It estimates, based on the mobile
	/// average of statistical correlation between consecutive lines, whether this is a wefax
	/// signal or not.
	fax_state correlation_state(const char ** stop_code) const {
		static fax_state stable_state = IDLE ;

		switch( stable_state )
		{
			case RXAPTSTART : *stop_code = "stable_start"  ; break;
			case RXAPTSTOP  : *stop_code = "stable_stop"   ; break;
			case RXPHASING  : *stop_code = "stable_phasing"; break;
			case RXIMAGE    : *stop_code = "stable_image"  ; break;
			case IDLE       : *stop_code = "stable_idle"   ; break;
			default         : *stop_code = "unexpected"    ; break;
		}

		/// If the mobile average is not computed on enough lines, returns IDLE
		/// which means "Do not know" in this context.
		if( m_corr_calls_nb >= m_min_corr_lines ) {
			int crr_row = m_img_sample / m_smpl_per_lin ;

			/// Sometimes, we detected a Stop just after the header, and we create an image
			// of 300 lignes. This is a rule of thumb. The bad consequence is that the image
			// would be a bit too high. This an approximate row number.
			// On the other hand, if we read very few lines, we assume this is just noise.
			const double low_corr
				= ( crr_row < 600 ) ? 0.01   // Short images containing text.
				: ( crr_row < 1300 ) ? 0.02  // Most of images.
				: 0.05 ;                     // Deutsche Wetterdienst sometimes 1900 pixels high.
			/// The thresholds are very approximate. This is a compromise:
			// * If high threshold, we cut images in two.
			// * If low threshold, we go on reading an image after its end if apt stop is not seen,
			//   and might read the beginning of the next image.
			// This might be suppressed if phasing band detection is much better.
			if( m_curr_corr_avg < low_corr ) {
				LOG_DEBUG("Setting to stop m_curr_corr_avg=%f low_corr=%f", m_curr_corr_avg, low_corr );
				*stop_code = "nocorr";
				stable_state = RXAPTSTOP ;

			/// TODO: Beware, this is sometimes triggered with cyclic parasites.
			} else if( m_curr_corr_avg > 0.20 ) {
				stable_state = RXIMAGE ;
			} else if( ( m_imag_corr_max < 0.10 ) && ( crr_row > 200 ) ) {
				// If the correlation was always very low for many lines, 
				// this means that we started to read a wrong image, so
				// there might be a wrong tuning or the DWD constant frequency.
				// So we trash the image.
				// TODO: It is done too many times: SOMETIMES THE MESSAGE IS REPEATED HUNDREDTH.
				// crr8row continues to grow although it makes no sense.
				LOG_INFO("Flushing dummy image m_imag_corr_max=%f crr_row=%d 200 lines.",
					m_imag_corr_max, crr_row );
				static const char * garbage_200 = GARBAGE_STR ".200";
				*stop_code = garbage_200;
				reset_afc();
				stable_state = RXAPTSTOP ;
			} else if( ( m_imag_corr_max < 0.20 ) && ( crr_row > 500 ) ) {
				// If the max line-to-line correlation still low for a bigger image.
				LOG_INFO("Flushing dummy image m_imag_corr_max=%f crr_row=%d 500 lines.",
					m_imag_corr_max, crr_row );
				static const char * garbage_500 = GARBAGE_STR ".500";
				*stop_code = garbage_500;
				reset_afc();
				stable_state = RXAPTSTOP ;
			} else {
				stable_state = IDLE ;
			}
		}

		/// Message for first detection.
		if( (stable_state == IDLE) && (m_corr_calls_nb == m_min_corr_lines) ) {
			// Do not display twice the same value.
			static double last_corr_avg = 0.0 ;
			if( m_curr_corr_avg != last_corr_avg ) {
				LOG_INFO("Correlation average %lf m_imag_corr_max=%f: Detected %s",
					m_curr_corr_avg, m_imag_corr_max, state_to_str(stable_state) );
				last_corr_avg = m_curr_corr_avg ;
			}
		}

		return stable_state;
	}

	/// We compute about the same when plotting a pixel.
	void correlation_update( int the_sample ) {
		size_t corr_smpl_lin = lpm_to_samples( m_default_lpm );
		size_t corr_buff_sz = 2 * corr_smpl_lin ;

		static size_t cnt_upd = 0 ;

		if( m_correlation_buffer.size() != corr_buff_sz ) {
			m_correlation_buffer.resize( corr_buff_sz, 0 );
			m_curr_corr_avg = 0.0 ;
		}

		if( ( cnt_upd % corr_smpl_lin ) == 0 ) {
			correlation_calc();
		}
		++cnt_upd ;

		m_correlation_buffer.at_mod( m_img_sample ) = the_sample ;

	} // correlation_update

	/// Clean trick so that we can reset internal variables of process_afc().
	void reset_afc() const {
		process_afc(true);
	}

	/// Called once for each row of input data.
	void process_afc(bool reset_afc = false) const {
		static int prev_row = -1 ;
		static int total_img_rows = 0 ;
		/// Audio frequency.
		static int stable_carrier = 0 ;

		static const int max_median_freqs = 20 ;
		static double median_freqs[ max_median_freqs ];
		static int nb_median_freqs = 0 ;


		/// Rig frequency.
		static long long stable_rfcarrier = 0 ;

		/// TODO: Maybe we could restrict the range of frequencies.
		if( reset_afc ) {
			/// Displays the message only once.
			if( prev_row != -1 ) {
				LOG_INFO("Resetting AFC total_img_rows=%d",total_img_rows);
			}
			prev_row = -1 ;
			total_img_rows = 0 ;
			stable_carrier = 0;
			stable_rfcarrier = 0 ;
			nb_median_freqs = 0 ;
			return;
		}

		if( progStatus.afconoff == false ) return ;

		/// The LPM might have changed, but this is very improbable.
		/// We do not know the LPM so we assume the default.
		double smpl_per_lin = lpm_to_samples( m_default_lpm );
		int crr_row = m_img_sample / smpl_per_lin ;

		// Actually executed once for each line of samples.
		if( crr_row == prev_row )
		{
			return ;
		}

		long long curr_rfcarr = wf->rfcarrier() ;

		/// If the freq changed, reset the number of good lines read.
		if( ( m_carrier != stable_carrier )
		||  ( curr_rfcarr != stable_rfcarrier ) )
		{
			/// Displays the messages once only.
			if( total_img_rows != 0 ) {
				LOG_INFO("Setting m_carrier=%d curr_rfcarr=%d total_img_rows=%d",
					m_carrier, static_cast<int>(curr_rfcarr), total_img_rows );
			}
			total_img_rows = 0 ;
			stable_carrier = m_carrier ;
			stable_rfcarrier = curr_rfcarr ;
		}

		/// If we could read a big portion of an image, then no adjustment.
		if( m_rx_state == RXIMAGE )
		{
			/// Maybe this is a new image so last_img_rows is from the previous image.
			if( prev_row < crr_row )
			{
				total_img_rows += crr_row - prev_row ;
			}
			else
			{
				LOG_DEBUG("prev_crr_row=%d crr_row=%d total_img_rows=%d", prev_row, crr_row, total_img_rows );
			}
		}
		else
		{
			LOG_DEBUG("State=%s crr_row=%d", state_rx_str(), crr_row );
		}
		prev_row = crr_row ;

		/// If we could succesfully read this number of lines, it must be the right frequency.
		static const int threshold_rows = 200 ;

		/// Consider than not only we could read many lines, but also other criterias such
		// as correlation, or if we could read apt start/stop. Otherwise, it may stabilize a useless frequency.
		if( ( total_img_rows >= threshold_rows ) && ( m_curr_corr_avg > 0.10 ) )
		{
			if( total_img_rows == threshold_rows )
			{
				LOG_INFO("Stable total_img_rows=%d m_curr_corr_avg=%f", total_img_rows, m_curr_corr_avg );
			}

			/// TODO: If the reception is always poor for a long time,
			// and a close frequency is constantly better for a long time, then restart AFC.
			return ;
		}

		/* TODO: If there are parasites, there is an excursion of the frequency. Therefore,
		 * in order to keep constant a frequency which might be temporarily decentered,
		 * it would be better to:
		 * - Stabilize an average of the last X frequencies.
		 * - And/Or maximize as well the correlation. */

		/// This centers the carrier where the activity is the strongest.
		static const int bw_dual[][2] = {
			{ -fm_deviation - 50, -fm_deviation + 50 },
			{  fm_deviation - 50,  fm_deviation + 50 } };
       		double max_carrier_dual = wf->powerDensityMaximum( 2, bw_dual );

		static const int bw_right[][2] = {
			{  fm_deviation - 50,  fm_deviation + 50 } };
       		double max_carrier_right = wf->powerDensityMaximum( 1, bw_right );

		// This might have to be adjusted because DWD has all the energy on the right
		// band, but Northwood has some on the left.
		double max_carrier = 0.0 ;

		/// Maybe there is not enough room on the left.
		if( max_carrier_dual < 0.0 ) {
			LOG_DEBUG("Invalid AFC: Dual band=%f, right band=%f. Take right",
				max_carrier_dual, max_carrier_right );
			max_carrier = max_carrier_right ;
		} else 
		/// Both mask detect approximately the same frequency: This is the consistent case.
		if( fabs( max_carrier_dual - max_carrier_right ) < 20 ) {
			max_carrier = max_carrier_right ;
		} else 
		if( max_carrier_dual < max_carrier_right ) {
			LOG_DEBUG("Inconsistent AFC: Dual band=%f, right band=%f. Take right",
				max_carrier_dual, max_carrier_right );
			max_carrier = max_carrier_right ;
		} else
		/// Single-band mask close to the left frequency of the dual-freq mask
		if( fabs( fabs( max_carrier_dual - max_carrier_right ) - 2 * fm_deviation ) < 20 ) {
			LOG_DEBUG("Dual band=%f instead of right band=%f. Take dual",
				max_carrier_dual, max_carrier_right );
			max_carrier = max_carrier_dual ;
		} else {
			LOG_DEBUG("Inconsistent AFC: Dual band=%f, right band=%f. Take dual.",
				max_carrier_dual, max_carrier_right );
			max_carrier = max_carrier_dual ;
		}

		/// The max power is unreachable, there might be another reason. Stay where we are.
		static bool prevWasRight = true ;

		/// TODO: We might find the next solution, maybe by giving to powerDensityMaximum a smaller range.
		if( ( max_carrier <= fm_deviation ) || ( max_carrier >= IMAGE_WIDTH - fm_deviation ) ) {
			/// Display this message once only.
			if( prevWasRight ) {
				LOG_INFO("Invalid max_carrier=%f", max_carrier);
			}
			prevWasRight = false ;
			return ;
		} else {
			prevWasRight = true ;
		}

		// TODO: If the correlation is below a given threshold, it might be useless to add it.
		median_freqs[ nb_median_freqs % max_median_freqs ] = max_carrier ;
		++nb_median_freqs;
		if( nb_median_freqs >= max_median_freqs ) {
			std::sort( median_freqs, median_freqs + max_median_freqs );
			max_carrier = median_freqs[ max_median_freqs / 2 ];
		}
		double delta = fabs( m_carrier - max_carrier );
		if( delta > 5.0 ) { // Hertz.
			/// Do not change the frequency too quickly if an image is received.
			double next_carr = max_carrier ;
			LOG_DEBUG("m_carrier=%f max_carrier=%f next_carr=%f", (double)m_carrier, max_carrier, next_carr );
			m_ptr_wefax->set_freq(next_carr);
		}

		// So we can detect if it was changed manually.
		stable_carrier = m_carrier ;
	}
}; // class fax_implementation

/// Narrow, middle etc... input filters. Constructed at program startup. Readonly.
fir_filter_pair_set fax_implementation::m_rx_filters ;

// http://www.newearth.demon.co.uk/radio/hfwefax1.htm
// "...the total offset is 1900Hz, so to tune a fax station on 4610kHz, tune to 4608.1kHz USB."
static const double wefax_default_carrier = 1900;

fax_implementation::fax_implementation( int fax_mode, wefax * ptr_wefax  )
	: m_ptr_wefax( ptr_wefax )
	, m_dbl_sine(8192),m_dbl_cosine(8192),m_dbl_arc_sine(256)
	, m_short_sine(8192)
{
	m_freq_mod       = true;
	m_apt_stop_freq  = 450 ;
	m_img_color      = false ;
	m_tx_phasing_lin = 20 ;
	m_carrier        = wefax_default_carrier ;
	m_start_duration = 5 ;
	m_stop_duration  = 5 ;
	m_manual_mode    = false ;
	m_rx_state       = IDLE ;
	m_tx_state       = IDLE ;
	m_sample_rate    = 0 ;
	reset_counters();

	int index_of_correlation ;
	/// http://en.wikipedia.org/wiki/Radiofax
	switch( fax_mode )
	{
	case MODE_WEFAX_576:
		m_apt_start_freq     = 300 ;
		index_of_correlation = IOC_576 ;
		m_default_lpm        = 120 ;
		break;
	case MODE_WEFAX_288:
		m_apt_start_freq     = 675 ;
		index_of_correlation = IOC_288 ;
		m_default_lpm        = 60 ;
		break;
	default:
		LOG_ERROR(_("Invalid fax mode:%d"), fax_mode);
		abort();
	}

	m_img_width = ioc_to_width( index_of_correlation );

	for(size_t i=0; i<m_dbl_sine.size(); i++) {
		m_dbl_sine[i]=32768.0 * std::sin(2.0*M_PI*i/m_dbl_sine.size());
	}
	for(size_t i=0; i<m_dbl_cosine.size(); i++) {
		m_dbl_cosine[i]=32768.0 * std::cos(2.0*M_PI*i/m_dbl_cosine.size());
	}
	for(size_t i=0; i<m_dbl_arc_sine.size(); i++) {
		m_dbl_arc_sine[i]=std::asin(2.0*i/m_dbl_arc_sine.size()-1.0)/2.0/M_PI;
	}
	for(size_t i=0; i<m_short_sine.size(); i++) {
		m_short_sine[i]=static_cast<short>(32767*std::sin(2.0*M_PI*i/m_short_sine.size()));
	}
};

void fax_implementation::init_rx(int the_smpl_rat)
{
	m_sample_rate=the_smpl_rat;
	m_rx_state=RXAPTSTART;
	m_apt_count=m_apt_trans=0;
	m_apt_high=false;
	/// Centers the carriers on the GUI and reinits the trigonometric tables.
	m_ptr_wefax->set_freq(wefax_default_carrier);

	/// Default value, can be the with the GUI.
	m_ix_filt = 0 ; // 0=narrow, 1=middle, 2=wide.

	reset_increments();
	m_i_fir_old=m_q_fir_old=0;
	m_corr_calls_nb = 0;
}

/// Values are between zero and +range_sample
void fax_implementation::decode(const int* buf, int nb_samples)
{
	if(nb_samples==0)
	{
		LOG_WARN(_("Empty buffer."));
		end_rx();
	}
	fax_signal my_signal(this);
	process_afc();
	for(int i=0; i<nb_samples; i++) {
		int crr_val = buf[i];
		my_signal.refresh();
		m_current_value = crr_val;

		{
			static int n = 0 ;

			/// TODO: It could be displayed less often.
			if( 0 == ( n++ % 10000 ) ) {
				/// Must create a temp string otherwise the char pointer may be freed.
				std::string tmp_str = my_signal.to_string();
				LOG_VERBOSE( "%s state=%s", tmp_str.c_str(), state_rx_str() );
				my_signal.display();
			}
		}

		correlation_update(crr_val);

		if( m_manual_mode ) {
			m_rx_state = RXIMAGE;
			bool is_max_lines_reached = decode_image(crr_val);
			if( is_max_lines_reached ) {
				skip_apt_rx();
				skip_phasing_rx(false);
				LOG_INFO(_("Max lines reached in manual mode: Resuming reception."));
				if( m_manual_mode == false ) {
					LOG_ERROR(_("Inconsistent manual mode."));
				}
			}
		} else {
			decode_apt(crr_val,my_signal);
			if( m_rx_state==RXPHASING || m_rx_state==RXIMAGE ) {
				decode_phasing(crr_val,my_signal);
			}
			if( ( m_rx_state==RXPHASING || m_rx_state==RXIMAGE ) && m_lpm_img > 0 ) {
				/// If the maximum number of lines is reached, we stop the reception.
				decode_image(crr_val);
			}
		}
		m_img_sample++;
	}
}

// The number of transitions between black and white is counted. After 1/2 
// second, the frequency is calculated. If it matches the APT start frequency,
// the state skips to the detection of phasing lines, if it matches the apt
// stop frequency two times, the reception is ended.
void fax_implementation::decode_apt(int x, const fax_signal & the_signal )
{
	if( (m_img_sample % 10000) == 0 ) {
		PUT_STATUS( state_rx_str() << " apt_count=" << m_apt_count );
	}

	/// Maybe the thresholds 229 and 25 should be changed if the image is not contrasted enough.
	// if(x>229 && !m_apt_high) {
	if(x>215 && !m_apt_high) {
		m_apt_high=true;
		++m_apt_trans;
	// } else if(x<25 && m_apt_high) {
	} else if(x<40 && m_apt_high) {
		m_apt_high=false;
	}
	++m_apt_count ;

	/// This makes one line if LPM=120: // samples_per_line = m_sample_rate * 60.0 / the_lpm ;
	 if( m_apt_count >= m_sample_rate/2 ) {
		static int curr_freq = 0 ;
		static int cr_1_freq = 0 ;
		static int cr_2_freq = 0 ;
		static int cr_3_freq = 0 ;

		// For APT start, we have several lines with freq=300 nearly exactly.
		// For APT stop, one or two lines maximum with freq=452, and this is not accurate.
		// On top of that, sometimes lines randomly have 300 or 452 black-white transitions,
		// especially if the signal is very noisy. This is why we check the transitions
		// frequency of two consecutive lines.
		cr_3_freq=cr_2_freq;
		cr_2_freq=cr_1_freq;
		cr_1_freq=curr_freq;
		curr_freq=m_sample_rate*m_apt_trans/m_apt_count;

		/// This writes the S/R level on the status bar.
		double tmp_snr = the_signal.image_noise_ratio();
		char snr_buffer[64];
        	snprintf(snr_buffer, sizeof(snr_buffer), "s/n %3.0f dB", 20.0 * log10(tmp_snr));
		put_Status1(snr_buffer);

		LOG_DEBUG("m_apt_count=%d m_apt_trans=%d curr_freq=%d cr_1_freq=%d cr_2_freq=%d",
				m_apt_count, m_apt_trans, curr_freq, cr_1_freq, cr_2_freq );

		m_apt_count=m_apt_trans=0;

		if(m_rx_state==RXAPTSTART) {
			if( is_near_freq(curr_freq,m_apt_start_freq, 8 )
			&&  is_near_freq(cr_1_freq,m_apt_start_freq, 8 ) ) {
				LOG_INFO(_("Skipping APT freq=%d State=%s"),
						curr_freq, state_rx_str() );
				skip_apt_rx();
				PUT_STATUS( state_rx_str() << ", " << _("frequency") << ": "
						<< curr_freq << " Hz. " << _("Skipping.") );
				return ;
			}
			if( is_near_freq(curr_freq,m_apt_stop_freq, 2 )
			&&  is_near_freq(cr_1_freq,m_apt_stop_freq, 2 ) ) {
				LOG_INFO(_("Spurious APT stop frequency=%d Hz as waiting for APT start. SNR=%f State=%s"),
						curr_freq, tmp_snr, state_rx_str() );
				return ;
			}

			PUT_STATUS( state_rx_str() << ", " << _("frequency") << ": " << curr_freq << " Hz." );
		}

		/// If APT start in the middle of an image, maybe the preceding APT Stop was not caught.
		if(m_rx_state==RXIMAGE) {
			/// TODO: We could enhance accuracy by taking into account the number of read lines.
			int crr_row = m_img_sample / m_smpl_per_lin ;

			const char * msg_start = NULL ;
			if( is_near_freq(curr_freq,m_apt_start_freq, 4 )
			&&  is_near_freq(cr_1_freq,m_apt_start_freq, 4 )
			&&  is_near_freq(cr_2_freq,m_apt_start_freq, 4 ) ) {
				msg_start = "apt";
			} else
			if( is_near_freq(curr_freq,m_apt_start_freq, 5 )
			&&  is_near_freq(cr_1_freq,m_apt_start_freq, 5 )
			&&  is_near_freq(cr_2_freq,m_apt_start_freq, 5 )
			&&  is_near_freq(cr_3_freq,m_apt_start_freq, 5 ) ) {
				msg_start = "apt2";
			}

			if( msg_start != NULL ) {
				std::string comment = strformat(
					"APT start: freq=%d cr_1=%d cr_2=%d cr_3=%d crr_row=%d State=%s msg=%s",
					curr_freq, cr_1_freq, cr_2_freq, cr_3_freq, crr_row, state_rx_str(), msg_start );
				LOG_INFO("%s",comment.c_str());
				PUT_STATUS( state_rx_str() << ", " << _("frequency") << ": "
						<< curr_freq << " Hz. " << _("Skipping.") );
				save_automatic(msg_start,comment);
				skip_apt_rx();
				return ;
			}
		}

		/// TODO: Check that the correlation is not too high because there are false stops.
		const char * msg_stop = NULL ;
		if( is_near_freq(curr_freq,m_apt_stop_freq, 6 )
		&&  is_near_freq(cr_1_freq,m_apt_stop_freq, 6 ) ) {
			msg_stop = "ok" ;
		} else
		if( is_near_freq(curr_freq,m_apt_stop_freq, 7 )
		&&  is_near_freq(cr_1_freq,m_apt_stop_freq, 7 )
		&&  is_near_freq(cr_2_freq,m_apt_stop_freq, 7 ) ) {
			msg_stop = "ok2" ;
		}

		if( msg_stop != NULL ) {
			std::string comment = strformat(
				"APT Stop: curr_freq=%d m_img_sample=%d State=%s_ msg=%s",
				curr_freq, m_img_sample, state_rx_str(), msg_stop );
			LOG_INFO("%s",comment.c_str());
			PUT_STATUS( state_rx_str() << " " << _("Apt stop frequency") << ": "
					<< curr_freq << " Hz. " << _("Stopping.") );
			save_automatic(msg_stop,comment);
			return ;
		}

		switch( the_signal.signal_state() ) {
		case RXAPTSTART :
			switch( m_rx_state ) {
				case RXAPTSTART :
					skip_apt_rx();
					LOG_INFO( "Start, start: %s", the_signal.signal_text() );
					break ;
				default :
					LOG_DEBUG( "Start, %s: %s", state_rx_str(), the_signal.signal_text() );
					break ;
			}
			break ;
		case RXPHASING :
			switch( m_rx_state ) {
				case RXAPTSTART :
					skip_apt_rx();
					LOG_INFO( "Phasing, start: %s", the_signal.signal_text() );
					break ;
				default :
					LOG_DEBUG( "Phasing, %s: %s", state_rx_str(), the_signal.signal_text() );
					break ;
			}
			break ;
		case RXIMAGE :
			switch( m_rx_state ) {
				case RXAPTSTART :
					skip_apt_rx();
					/// The phasing step will start receiving the image later. First we try
					/// to phase the image correctly.
					LOG_INFO( "Image, start: %s", the_signal.signal_text() );
					break ;
				default :
					LOG_DEBUG( "Image, %s: %s", state_rx_str(), the_signal.signal_text() );
					break ;
			}
			break ;
		case RXAPTSTOP :
			switch( m_rx_state ) {
				case RXIMAGE    :
					LOG_INFO( "Stop, image: %s", the_signal.signal_text() );
					save_automatic( the_signal.signal_stop_code(), the_signal.signal_text() );
					break;
				default :
					LOG_DEBUG( "Stop, %s: %s", state_rx_str(), the_signal.signal_text() );
					break ;
			}
			break ;
		default :
			LOG_DEBUG( "%s, %s: %s", state_to_str(the_signal.signal_state()), state_rx_str(), the_signal.signal_text() );
			break ;
		}
	}
}

/// This generates a file name with the reception time and the frequency.
std::string fax_implementation::generate_filename( const char *extra_msg ) const
{
	time_t tmp_time = time(NULL);
	struct tm tmp_tm ;
	localtime_r( &tmp_time, &tmp_tm );

	char buf_fil_nam[256] ;
	long long tmp_fl = wf->rfcarrier() ;
	snprintf( buf_fil_nam, sizeof(buf_fil_nam),
		"wefax_%04d%02d%02d_%02d%02d%02d_%d_%s.png",
		1900 + tmp_tm.tm_year,
		1 + tmp_tm.tm_mon,
		tmp_tm.tm_mday,
		tmp_tm.tm_hour,
		tmp_tm.tm_min,
		tmp_tm.tm_sec,
		static_cast<int>(tmp_fl),
		extra_msg );

	return buf_fil_nam ;
}

/// This saves an image and adds the right comments.
void fax_implementation::save_automatic(
		const char        * extra_msg,
		const std::string & comment)
{
	std::string new_filnam ;
	std::stringstream extra_comments ;

	/// These criteria used by rules-of-thumb to eliminate blank images.
	m_statistics.calc();
	double avg = m_statistics.average();
	double stddev = m_statistics.stddev();
	int current_row = m_img_sample / m_smpl_per_lin ;

	/* Sometimes the AFC leaves the right frequency (Because no signal) and it produces
	 * images which are plain wrong (Pure periodic parasites). We might find 
	 * a criteria to suppress them but maybe it's better to enhance AFC. */

	/// Minimum pixel numbers for a valid image.
	static const int max_fax_pix_num = 150000 ;
	if( m_fax_pix_num < max_fax_pix_num ) {
		/// Maybe we should reset AFC ?
		LOG_INFO( _("Do not save small image (%d bytes). Manual=%d"), m_fax_pix_num, m_manual_mode );
		goto cleanup_rx ;
	}

	/// If correlation was always low.
	if( 0 == strncmp( extra_msg, GARBAGE_STR, strlen(GARBAGE_STR) ) ) {
		LOG_INFO( _("Do not save garbage file") );
		goto cleanup_rx ;
	}

	static const double min_max_correlation = 0.20 ;
	if( m_imag_corr_max < min_max_correlation ) {
		LOG_INFO(_("Do not save image with correlation < less than %f"), min_max_correlation );
		goto cleanup_rx ;
	}

	/// Intermediate blank images, low standard deviation and correlation.
	if( ( ( avg > 220 ) && ( m_imag_corr_max < 0.30 ) && ( stddev < 30 ) )
	 || ( ( avg > 240 ) && ( m_imag_corr_max < 0.35 ) && ( stddev < 30 ) )
	 || ( ( avg > 230 ) && ( m_imag_corr_max < 0.25 ) && ( stddev < 35 ) )
	 || ( ( avg > 220 )                               && ( stddev < 20 ) ) ) {
		LOG_INFO(_("Do not save non-significant image, avg=%f, m_imag_corr_max=%f stddev=%f"),
				avg, m_imag_corr_max, stddev );
		goto cleanup_rx ;
	}

	/// Dark images coming from local parasites.
	if( ( avg < 100 ) && ( m_imag_corr_max < 0.30 ) ) {
		LOG_INFO(_("Do not save dark parasite image, avg=%f, m_imag_corr_max=%f"), avg, m_imag_corr_max );
		goto cleanup_rx ;
	}

	/// Same: Dark images due to parasites.
	if( ( avg < 80 ) && ( m_imag_corr_max < 0.35 ) ) {
		LOG_INFO(_("Do not save dark parasite image (II), avg=%f, m_imag_corr_max=%f"), avg, m_imag_corr_max );
		goto cleanup_rx ;
	}

	if( ( avg > 235 ) && ( m_imag_corr_max < 0.65 ) && ( stddev < 40 ) && ( current_row < 100 ) ) {
		LOG_INFO(_("Do not save small white image, avg=%f, m_imag_corr_max=%f stddev=%f current_row=%d"),
				avg, m_imag_corr_max, stddev, current_row );
		goto cleanup_rx ;
	}

	/// Small image cut between APT start and phasing.
	if( ( avg < 130 ) && ( m_imag_corr_max < 0.70 ) && ( stddev < 100 ) && ( current_row < 100 ) ) {
		LOG_INFO(_("Do not save small white image, avg=%f, m_imag_corr_max=%f stddev=%f current_row=%d"),
				avg, m_imag_corr_max, stddev, current_row );
		goto cleanup_rx ;
	}


	new_filnam = generate_filename(extra_msg);
	LOG_INFO( "Saving %d bytes in %s. m_imag_corr_max=%f", m_fax_pix_num, new_filnam.c_str(), m_imag_corr_max );

	extra_comments << "ControlMode:"                      << ( m_manual_mode ? "Manual" : "APT control" ) << "\n" ;
	extra_comments << "LPM:"                              << m_lpm_img << "\n" ;
	extra_comments << "FrequencyMode:"                    << ( m_freq_mod ? "FM" : "AM" ) << "\n" ;
	extra_comments << "Carrier:"                          << m_carrier << "\n" ;
	extra_comments << "Inversion:"                        << ( m_phase_inverted ? "Inverted" : "Normal" ) << "\n" ;
	extra_comments << "Color:"                            << ( m_img_color ? "Color" : "BW" ) << "\n" ;
	extra_comments << "SampleRate:"                       << m_sample_rate << "\n" ;
	extra_comments << "Maximum line-to-line correlation:" << m_imag_corr_max << "\n" ;
	extra_comments << "Minimum line-to-line correlation:" << m_imag_corr_min << "\n" ;
	extra_comments << "Average:"                          << avg << "\n" ;
	extra_comments << "Row number:"                       << current_row << "\n" ;
	extra_comments << "Standard deviation:"               << stddev << "\n" ;
	extra_comments << "Comment:"                          << comment << "\n" ;
	extra_comments << "PID:"                              << getpid() << "\n" ;
	wefax_pic::save_image( new_filnam, extra_comments.str() );

cleanup_rx:
	/// This clears the current image.
	end_rx();
};

// Phasing lines consist of 2.5% white at the beginning, 95% black and again
// 2.5% white at the end (or inverted). In normal phasing lines we try to
// count the length between the white-black transitions. If the line has
// a reasonable amount of black (4.8%--5.2%) and the length fits in the 
// range of 60--360lpm (plus some tolerance) it is considered a valid
// phasing line. Then the start of a line and the lpm is calculated.
void fax_implementation::decode_phasing(int x, const fax_signal & the_signal )
{
	static const bool filter_phasing = true ;

	if( filter_phasing )
	{
		// The input is filtered over X columns because the blacks bands are very wide.
		static const size_t phasing_width = 16 ;
		static size_t phasing_count = 0 ;
		static int phasing_history[ phasing_width ];

		/// Mobile average filters out parasites.
		phasing_history[ phasing_count % phasing_width ] = x ;
		++phasing_count ;
		if( phasing_count >= phasing_width ) {
			x = 0 ;
			for( size_t i = 0; i < phasing_width; ++i ) x += phasing_history[ i ];
			x /= phasing_width ;
		}
	}

	/// Number of samples per line.
	m_curr_phase_len++;
	++m_phasing_calls_nb ;

	/// Number of black pixels.
	if(x>128) {
		m_curr_phase_high++;
	}

	/* If the high level is too high (229) we miss some phasing lines.
	 * but if it is too low (200) the center is not at the right place.
	 * We should instead use an histogram and determine what is the highest level in the image. */
	// if((!m_phase_inverted && x>229 && !m_phase_high) ||
	if((!m_phase_inverted && x>200 && !m_phase_high) ||
	   ( m_phase_inverted && x<25  && m_phase_high))
	{
		m_phase_high=m_phase_inverted?false:true;
	}
	else if((!m_phase_inverted && x<25 && m_phase_high) ||
		  ( m_phase_inverted && x>200 && !m_phase_high))
	//	  ( m_phase_inverted && x>229 && !m_phase_high))
	{
		m_phase_high=m_phase_inverted?true:false;

		/// In the phasing line, there is a white segment of 5% of the total length,
		// so it is approximated with 0.048->0.052.
		if(m_curr_phase_high>=(m_phase_inverted?0.948:0.048)*m_curr_phase_len &&
		   m_curr_phase_high<=(m_phase_inverted?0.952:0.052)*m_curr_phase_len &&
		   m_curr_phase_len <= 1.1  * m_sample_rate &&
		   m_curr_phase_len >= 0.15 * m_sample_rate)
		{
			double tmp_lpm = 60.0 * m_sample_rate / m_curr_phase_len;

			m_lpm_sum_rx += tmp_lpm;
			++m_phase_lines;

			PUT_STATUS( state_rx_str()
				<< ". " << _("Decoding phasing line") << ", lpm = " << tmp_lpm
				<< " " << _("count") << "=" << m_phase_lines );

			m_num_phase_lines=0;
			/// This option was selected just once, in the middle of an image,
			// with many vertical lines.
			if( m_phase_lines >= 5 /* Was 4 */ ) {
				/// The precision cannot really increase because there cannot 
				// be more than a couple of loops. This is used for guessing
				// whether the LPM is around 120 or 60.
				lpm_set( m_lpm_sum_rx / m_phase_lines );

				std::string comment = strformat( 
					"Skipping to reception: m_phase_lines=%d m_num_phase_lines=%d LPM=%f State=%s"
					" m_img_sample=%d m_last_col=%d m_lpm_img=%f m_smpl_per_lin=%f",
					m_phase_lines, m_num_phase_lines, m_lpm_img, state_rx_str(),
				       	m_img_sample, m_last_col, m_lpm_img, m_smpl_per_lin );
				LOG_INFO( "%s", comment.c_str() );

				skip_phasing_to_image_save(comment);

				/// reset_counters sets these to zero, so restore right values.
				// But skip_phasing_to_image normalizes the LPM if not accurate.

				/// Half of the band of the phasing line.
				m_img_sample=static_cast<int>(1.025 * m_smpl_per_lin );

				double tmp_pos=std::fmod(m_img_sample,m_smpl_per_lin) / m_smpl_per_lin;
				m_last_col=static_cast<int>(tmp_pos*m_img_width);

				/// Now the image will start at the right column offset.
				m_fax_pix_num = m_last_col * bytes_per_pixel ;

				LOG_INFO("Center set: m_last_col=%d m_img_width=%d m_smpl_per_lin=%f",
					m_last_col, m_img_width, m_smpl_per_lin );
			}
		} else if( m_rx_state == RXPHASING && m_phase_lines>0 && ++m_num_phase_lines>=5) {
			/// TODO: Compare with m_tx_phasing_lin which indicates the number of phasing
			/// lines sent when transmitting an image.
			std::string comment = strformat(
				"Missed last phasing line m_phase_lines=%d m_num_phase_lines=%d LPM=%f State=%s",
				m_phase_lines, m_num_phase_lines, m_lpm_img, state_rx_str() );
			LOG_INFO("%s", comment.c_str() );
			/// Phasing header is finished but could not get the center.
			skip_phasing_to_image(true);
		} else if(m_curr_phase_len>5*m_sample_rate) {
			m_curr_phase_len=0;
			PUT_STATUS( state_rx_str() << ". " << _("Decoding phasing line, resetting.") );
		} else {
			/// Here, if no phasing is detected. Must be very fast.
		}
		PUT_STATUS( state_rx_str() << ". " << _("Decoding phasing line, reset.") );
		m_curr_phase_len=m_curr_phase_high=0;
	}
	else if ( m_rx_state == RXPHASING )
	{
		/// We do not know the LPM so we assume the default.
		/// TODO: We could take the one given by the GUI. Apparently; problem for Japanese faxes when lpm=60:
		// http://forums.radioreference.com/digital-signals-decoding/228802-problems-decoding-wefax.html
		double smpl_per_lin = lpm_to_samples( m_default_lpm );
		int smpl_per_lin_int = smpl_per_lin ;
		int nb_tested_phasing_lines = m_phasing_calls_nb / smpl_per_lin ;

		/// If this is too big, we lose the beginning of the fax.
		// If this is too small, we start recording when we should not (But why not starting early ?)
		// Value was: 30
		static const int max_tested_phasing_lines = 20 ;
		if(
			(m_phase_lines == 0) &&
			(m_num_phase_lines == 0) &&
		 	(nb_tested_phasing_lines >= max_tested_phasing_lines ) &&
			( (m_phasing_calls_nb % smpl_per_lin_int) == 0 ) ) {
			switch( the_signal.signal_state() ) {
			case RXIMAGE :
				LOG_INFO(_("Starting reception when phasing:%s"), the_signal.signal_text() );
				skip_phasing_to_image(true);
				break ;
			/// If RXPHASING, we stay in phasing mode.
			case RXAPTSTOP :
				/// Do not display the same message over and over.
				LOG_DEBUG( _("Strong APT stop when phasing:%s"), the_signal.signal_text() );
				end_rx();
				skip_apt_rx();
			default : break ;
			}
		}
	}
}

bool fax_implementation::decode_image(int x)
{
	double current_row_dbl = m_img_sample / m_smpl_per_lin ;
	int current_row = current_row_dbl ;
	int curr_col= m_img_width * (current_row_dbl - current_row) ;

	if(curr_col==m_last_col) {
		m_pixel_val+=x;
		m_pix_samples_nb++;
	} else {
		if(m_pix_samples_nb>0) {
			m_pixel_val/=m_pix_samples_nb;

			/// TODO: Put m_fax_pix_num in wefax_pic so that the saving of an image
			/// will only be the number of added pixels. And it will hide
			/// the storage of B/W pixels in three contiguous ones.
			//
			// TODO: If the machine is heavily loaded, it loses a couple of pixel.
			// The consequence is that the image is shifted.
			// We could use the local clock to deduce where the pixel must be written:
			// - Store the first pixel reception time.
			// - Compute m_fax_pix_num = (reception_time * LPM * image_width)
			REQ( wefax_pic::update_rx_pic_bw, m_pixel_val, m_fax_pix_num );
			m_statistics.add_bw( m_pixel_val );
			m_fax_pix_num += bytes_per_pixel ;
		}
		m_last_col=curr_col;
		m_pixel_val=x;
		m_pix_samples_nb=1;
	}

	/// Prints the status from time to time.
	if( (m_img_sample % 10000) == 0 ) {
		PUT_STATUS( state_rx_str()
			<< ". " << _("Image reception")
			<< ", " << _("sample") << "=" << m_img_sample );
	}

	/// Hard-limit to the number of rows.
	if( current_row >= progdefaults.WEFAX_MaxRows ) {
		std::string comment = strformat( _("Maximum number of rows %d reached:%d. m_last_col=%d Manual=%d"),
			progdefaults.WEFAX_MaxRows, current_row, m_last_col, m_manual_mode );
		LOG_INFO("%s", comment.c_str());
		/// The reception might be very poor, so reset AFC.
		reset_afc();
		save_automatic("max",comment);
		return true ;
	} else {
		return false ;
	}
}

/// Called automatically or by the GUI, when clicking "Skip APT"
void fax_implementation::skip_apt_rx(void)
{
	LOG_INFO("state=%s",state_rx_str() );
	REQ( wefax_pic::skip_rx_apt );
	if(m_rx_state!=RXAPTSTART) {
		LOG_ERROR( _("Should be in APT state. State=%s. Manual=%d"), state_rx_str(), m_manual_mode );
	}
	lpm_set( 0 );
	m_rx_state=RXPHASING;
	reset_phasing_counters();
	m_img_sample=0; /// Used for correlation between consecutive lines.
	m_imag_corr_max = 0.0 ; // Used for finally deciding whether it is worth saving the image.
	m_imag_corr_min = 1.0 ; // Used for finally deciding whether it is worth saving the image.
	m_statistics.reset();
}

void fax_implementation::skip_phasing_to_image_save(const std::string & comment)
{
	switch( m_rx_state ) {
		/// Maybe we were still reading an image when the phasing band was detected.
		case RXIMAGE :
			LOG_INFO("Detected phasing when in image state.");
			// TODO: Maybe we could keep the top margin, and not save if less than X lines.
			save_automatic("phasing",comment);
			skip_apt_rx();
			break ;
		default:
			LOG_ERROR( _("Should be in phasing or image state. State=%s"), state_rx_str() );
		case RXPHASING:
			break ;
	}
	/// Because we might find a phasing band when reading an image.
	reset_phasing_counters();
	/// Auto-centering by default. Beware that if an image is saved and
	/// if we lose samples, the alignment is lost.
	skip_phasing_to_image(false);
}

/// Called by the user when skipping phasing,
/// or automatically when phasing is detected.
void fax_implementation::skip_phasing_to_image(bool auto_center)
{
	m_ptr_wefax->qso_rec_init();

	REQ( wefax_pic::skip_rx_phasing, auto_center );
	m_rx_state=RXIMAGE;

	/// For monochrome, LPM=60, 90, 100, 120, 180, 240. For colour, LPM =120, 240
	/// So we round to the nearest integer to avoid slanting.
	int lpm_integer = wefax_pic::normalize_lpm( m_lpm_img );
	if( m_lpm_img != lpm_integer )
	{
		/// If we could not find a valid LPM, then set the default one for this mode (576/288).
		lpm_integer = wefax_pic::normalize_lpm( m_default_lpm );
		LOG_INFO( _("LPM rounded from %f to %d. Manual=%d State=%s"),
				m_lpm_img, lpm_integer, m_manual_mode, state_rx_str() );
	}

	/// From now on, m_lpm_img will never change and has a normalized value.
	REQ( wefax_pic::update_rx_lpm, lpm_integer);
	PUT_STATUS( state_rx_str() << ". " << _("Decoding phasing line LPM=") << lpm_integer );
	lpm_set( lpm_integer );
}

/// Called by the user when clicking button. Never called automatically.
void fax_implementation::skip_phasing_rx(bool auto_center)
{
	if(m_rx_state!=RXPHASING) {
		LOG_ERROR( _("Should be in phasing state. State=%s"), state_rx_str() );
	}
	skip_phasing_to_image(auto_center);

	/// We force these two values because these could not be detected automatically.
	if( m_lpm_img != m_default_lpm ) {
		lpm_set( m_default_lpm );
		LOG_INFO( _("Forcing m_lpm_img=%f. Manual=%d"), m_lpm_img, m_manual_mode );
	}
	m_img_sample=0; /// The image start may not be what the phasing would have told.
}

// Here we want to remove the last detected phasing line and the following
// non phasing line from the beginning of the image and one second of apt stop
// from the end
void fax_implementation::end_rx(void)
{
	/// Synchronized otherwise there might be a crash if something tries to access the data.
	REQ(wefax_pic::abort_rx_viewer );
	m_rx_state=RXAPTSTART;
	reset_counters();
}

/// Receives data from the soundcard.
void fax_implementation::rx_new_samples(const double* audio_ptr, int audio_sz)
{
	int demod[audio_sz];
	static const double half_255 = (double)range_sample * 0.5 ;
	const double ratio_sam_devi = half_255 * static_cast<double>(m_sample_rate)/fm_deviation;

	/// The reception filter may have been changed by the GUI.
	C_FIR_filter & ref_fir_filt_pair = m_rx_filters[ m_ix_filt ];

	const double half_arc_sine_size = m_dbl_arc_sine.size() / 2.0 ;

	for(int i=0; i<audio_sz; i++) {
		double idx_aux = audio_ptr[i] ;

		cmplx firin( idx_aux*m_dbl_cosine.next_value(), idx_aux*m_dbl_sine.next_value() );
		cmplx firout ;

		/// This returns zero if the filter is not yet stable.
		/* int run_status = */ ref_fir_filt_pair.run( firin, firout );

		double ifirout = firout.real();
		double qfirout = firout.imag();

		if(m_freq_mod ) {
			/// Normalize values.
			double abs=std::sqrt(qfirout*qfirout+ifirout*ifirout);
			/// cosine(a)
			ifirout/=abs;
			/// sine(a)
			qfirout/=abs;

			/// Does it mean the signal should not be too strong ? Max is 32767.
			if(abs>10000) {
				/// Real part of the product of current vector,
				/// by previous vector rotated of 90 degrees.
				/// It makes something like sine(a-b).
				/// Maybe the derivative of the phase, that is,
				/// the instantaneous frequency ?
				double y = m_q_fir_old * ifirout - m_i_fir_old * qfirout ;

				/// Mapped to the interval [0 .. size]
				/// m_dbl_arc_sine[i] = asin(2*i/m_dbl_arc_sine.size-1)/2/Pi.
				/// TODO: Therefore it could be simplified ?
				y = ( y + 1.0 ) * half_arc_sine_size;

				/// TODO: y might be rounded with more accuracy: (int)(Y+0.5)
				double x = ratio_sam_devi * m_dbl_arc_sine[static_cast<size_t>(y)];

				int scaled_x = x + half_255 ;
				if(scaled_x < 0) {
					scaled_x=0;
				} else if(scaled_x > range_sample) {
					scaled_x=range_sample;
				}
				demod[i]=scaled_x;
			} else {
				demod[i]=0;
			}
		} else {
			/// Why 96000 ? bytes_per_pixel * 32000 because color ?
			ifirout/=96000;
			qfirout/=96000;
			demod[i]=static_cast<int>
				(std::sqrt(ifirout*ifirout+qfirout*qfirout));
		}

		m_i_fir_old=ifirout;
		m_q_fir_old=qfirout;
	}
	decode( demod, audio_sz);

#ifdef WEFAX_DISPLAY_SCOPE
	/// Nothing really meaningful to display.
	/// Beware that some pixels are lost if too many things are displayed
	double scope_demod[ audio_sz ];
	/// TODO: Do this in the loop, it will avoid conversions.
	for( int i = 0 ; i < audio_sz ; ++i ) {
		scope_demod[ i ] = demod[i];
	}
	set_scope( (double *)scope_demod, audio_sz , true );
#endif // WEFAX_DISPLAY_SCOPE
}

// Init transmission. Called once only.
void fax_implementation::init_tx(int the_smpl_rat)
{
	m_sample_rate=the_smpl_rat;
	set_carrier(wefax_default_carrier);
	/// These reset increments of trigo tables.
	m_ptr_wefax->set_freq(wefax_default_carrier);
	m_tx_state=TXAPTSTART;
}

/// Elements of buffer are between 0.0 and 1.0
void fax_implementation::modulate(const double* buffer, int number)
{
	/// TODO: This should be in m_short_sine
	static const double dbl_max_short_invert = 1.0 / 32768.0 ;

	double stack_xmt_buf[number] ;
	if( m_freq_mod ) {
		for(int i = 0; i < number; i++) {
			double tmp_freq = m_carrier + 2. * ( buffer[i] - 0.5 ) * fm_deviation ;
			m_short_sine.set_increment(m_short_sine.size() * tmp_freq / m_sample_rate);
			stack_xmt_buf[i] = m_short_sine.next_value() * dbl_max_short_invert ;
		}
	} else {
		/// Beware: Not tested !
		for(int i = 0; i < number; i++) {
			stack_xmt_buf[i] = m_short_sine.next_value() * buffer[i] * dbl_max_short_invert ;
		}
	}
	m_ptr_wefax->ModulateXmtr( stack_xmt_buf, number );
}

/// Returns true if succesful
/*
MULTIPSK: 120 LPM / 288 IOC and a transmission
takes with a 200 lines picture 150 sec.

In FLDIGI it took 480 sec. For 120 LPM it's too
slow. "240 LPM" in FLDIGI gives 130 sec which is nearly real 120 LPM.

APT    5 sec     m_start_duration=5
SYNC  30 sec     m_tx_phasing_line=20 lines => 10 seconds if LPM=120
PIC
APT    5 sec     m_stop_duration=5
BLACK 10 sec     No black signal.

This is in summary 50 sec + PIC (60*LINES/LPM).
*/
bool fax_implementation::trx_do_next(void)
{
	LOG_DEBUG("m_lpm_img=%f", m_lpm_img );

	/// The number of samples sent for one line. The LPM is given by the GUI.
	const int smpl_per_lin= m_smpl_per_lin;

	/// Should not be too big because it is allocated on the stack, gcc feature.
	static const int block_len = 256 ;
	double buf[block_len];

	bool end_of_loop = false ;
	bool tx_completed = true ;
	int curr_sample_idx = 0 , nb_samples_to_send  = 0 ;

	for(int num_bytes_to_write=0; ; ++num_bytes_to_write ) {
		bool disp_msg = ( ( num_bytes_to_write % block_len ) == 0 ) && ( num_bytes_to_write > 0 );
	       
		if( disp_msg ) {
			modulate( buf, num_bytes_to_write);

			const char * curr_status_msg = state_to_str( m_tx_state );
			/// TODO: Should be multiplied by 3 when sending in BW ?
			if( m_ptr_wefax->is_tx_finished(curr_sample_idx, nb_samples_to_send, curr_status_msg ) ) {
				end_of_loop = true ;
				tx_completed = false;
				continue ;
			};
			num_bytes_to_write = 0 ;
		};
		if( end_of_loop == true ) {
			break ;
		}
		if(m_tx_state==TXAPTSTART) {
			nb_samples_to_send = m_sample_rate * m_start_duration ;
			if( curr_sample_idx < nb_samples_to_send ) {
				buf[num_bytes_to_write]=(curr_sample_idx*2*m_apt_start_freq/m_sample_rate)%2;
				curr_sample_idx++;
			} else {
				m_tx_state=TXPHASING;
				curr_sample_idx=0;
			}
		}
		if(m_tx_state==TXPHASING) {
			nb_samples_to_send = smpl_per_lin * m_tx_phasing_lin ;
			if( curr_sample_idx < nb_samples_to_send ) {
				double pos= (double)(curr_sample_idx % smpl_per_lin) / (double)smpl_per_lin;
				buf[num_bytes_to_write] = (pos<0.025||pos>=0.975 )
					? (m_phase_inverted?0.0:1.0) 
					: (m_phase_inverted?1.0:0.0);
				curr_sample_idx++;
			} else {
				m_tx_state=ENDPHASING;
				curr_sample_idx=0;
			}
		}
		if(m_tx_state==ENDPHASING) {
			nb_samples_to_send = smpl_per_lin ;
			if( curr_sample_idx < nb_samples_to_send ) {
				buf[num_bytes_to_write]= m_phase_inverted?0.0:1.0;
				curr_sample_idx++;
			} else {
				m_tx_state=TXIMAGE;
				curr_sample_idx=0;
			}
		}
		if(m_tx_state==TXIMAGE) {
			/// The image must be stretched so that its width matches the fax width,
			/// which cannot change because because it depends on the LPM.
			/// Accordingly the height is stretched.
			/// For LPM=120 and sample rate=11025 Hz, smpl_per_lin=5512.
			double ratio_img_to_fax = (double)m_img_width / m_img_tx_cols ;
			double samples_per_pix =  smpl_per_lin / (double)m_img_width ;
			double ratio_pow = ratio_img_to_fax * ratio_img_to_fax * samples_per_pix ;
			nb_samples_to_send = m_img_tx_cols * m_img_tx_rows * ratio_pow ;
			if( curr_sample_idx < nb_samples_to_send ) {
				int tmp_col = (double)( curr_sample_idx % smpl_per_lin ) * (double)m_img_tx_cols / smpl_per_lin;

				int tmp_row = (double)( curr_sample_idx / smpl_per_lin ) * (double)m_img_tx_cols / m_img_width;
				if( tmp_row >= m_img_tx_rows ) {
					LOG_ERROR( "Inconsistent tmp_row=%d m_img_tx_rows=%d "
						"curr_sample_idx=%d smpl_per_lin=%d "
						"ratio_img_to_fax=%f nb_samples_to_send=%d",
							tmp_row, m_img_tx_rows,
							curr_sample_idx, smpl_per_lin,
							ratio_img_to_fax, nb_samples_to_send );
					exit(EXIT_FAILURE);
				}

				int byte_offset = bytes_per_pixel * ( tmp_row * m_img_tx_cols + tmp_col );
				unsigned char temp_pix = m_xmt_pic_buf[ byte_offset ];
				curr_sample_idx++;
				REQ( wefax_pic::set_tx_pic, temp_pix, tmp_col, tmp_row, m_img_color );
				buf[num_bytes_to_write]= (double)temp_pix / 256.0 ;

				if( ( curr_sample_idx % 5000 ) == 0 ) {
					LOG_INFO(
						"curr_sample_idx=%d tmp_row=%d tmp_col=%d smpl_per_lin=%d "
						"samples_per_pix=%f nb_samples_to_send=%d m_img_width=%d",
						curr_sample_idx,
						tmp_row,
						tmp_col,
						smpl_per_lin,
						samples_per_pix,
						nb_samples_to_send,
						m_img_width );
				}

			} else {
				m_tx_state=TXAPTSTOP;
				curr_sample_idx=0;
			}
		}
		if(m_tx_state==TXAPTSTOP) {
			nb_samples_to_send = m_sample_rate * m_stop_duration ;
			if( curr_sample_idx < nb_samples_to_send ) {
				buf[num_bytes_to_write]=curr_sample_idx*2*m_apt_stop_freq/m_sample_rate%2;
				curr_sample_idx++;
			} else {
				m_tx_state=IDLE;
				end_of_loop = true ;
				continue ;
			}
		}
	} // loop
	return tx_completed ;
}

void fax_implementation::tx_params_set(
	int the_lpm,
	const unsigned char * xmtpic_buffer,
	bool is_color,
	int img_w,
	int img_h )
{
	LOG_DEBUG("img_w=%d img_h=%d the_lpm=%d is_color=%d",
			img_w, img_h, the_lpm, (int)is_color);
	m_img_tx_rows=img_h;
	m_img_tx_cols=img_w;
	m_img_color = is_color ;
	lpm_set( the_lpm );
	m_xmt_pic_buf = xmtpic_buffer ;

	PUT_STATUS( _("Sending picture.")
			<< _(" rows=") << m_img_tx_rows
			<< _(" cols=") << m_img_tx_cols );
}

void fax_implementation::tx_apt_stop(void)
{
	m_tx_state=TXAPTSTOP;
}

//=============================================================================
//
//=============================================================================

/// Called by trx_trx_transmit_loop
void  wefax::tx_init(SoundBase *sc)
{
	modem::scard = sc; // SoundBase
	
	videoText(); // In trx/modem.cxx
	m_impl->init_tx(modem::samplerate) ;
}

/// This updates the window label according to the state.
void wefax::update_rx_label(void) const
{
	std::string tmp_label( _("Reception ") );
	tmp_label += mode_info[modem::mode].name ;

	if( m_impl->manual_mode_get() ) {
		tmp_label += _(" - Manual") ;
	} else {
		tmp_label += _(" - APT control") ;
	}

	REQ( wefax_pic::set_rx_label, tmp_label );
}

void  wefax::rx_init()
{
	put_MODEstatus(modem::mode);
	m_impl->init_rx(modem::samplerate) ;

	REQ( wefax_pic::resize_rx_viewer, m_impl->fax_width() );
}

void wefax::init()
{
	modem::init();

	/// TODO: Maybe remove, probably not necessary because called by trx_trx_loop
	rx_init();

	/// TODO: Display scope.
	set_scope_mode(Digiscope::SCOPE);
}

void wefax::shutdown()
{
}

wefax::~wefax()
{
	modem::stopflag = true;

	/// Maybe we are receiving an image, this must be stopped.
	end_reception();

	/// Maybe we are sending an image.
	REQ( wefax_pic::abort_tx_viewer );

	activate_wefax_image_item(false);

	delete m_impl ;

	/// Not really useful, just to help debugging.
	m_impl = 0 ;
}

wefax::wefax(trx_mode wefax_mode) : modem()
{
	/// Beware that it is already set by modem::modem
	modem::cap |= CAP_AFC | CAP_REV | CAP_IMG ;
	LOG_DEBUG("wefax_mode=%d", (int)wefax_mode);

	wefax::mode = wefax_mode;

	modem::samplerate = 11025;

	m_impl = new fax_implementation(wefax_mode, this);

	/// Now this object is usable by wefax_pic.
	wefax_pic::setpicture_link(this);

	int tmpShift = progdefaults.WEFAX_Shift ;
	if(
	          (progdefaults.WEFAX_Shift < 100 )
	       || ( progdefaults.WEFAX_Shift > 1000 ) )
	{
		static const int standard_shift = 800;
		LOG_WARN("Invalid weather fax shift: %d. setting standard value: %d",
			progdefaults.WEFAX_Shift, standard_shift );
		tmpShift = standard_shift ;
	}
	fm_deviation = tmpShift / 2 ;

	modem::bandwidth = fm_deviation * 2 ;
	
	m_abortxmt = false;
	modem::stopflag = false;

	// There is only one instance of the reception and transmission
	// windows, therefore only static methods.
	wefax_pic::create_both( false );

	/// This updates the window label.
	set_rx_manual_mode( false );

	activate_wefax_image_item(true);

	init();
}


//=====================================================================
// receive processing
//=====================================================================

/// This must return the current time in seconds with high precision.

#if defined(__WIN32__) || defined(__APPLE__)
#include <ctime>
/// This is much less accurate.
static double current_time(void)
{
	clock_t clks = clock();

	return clks * 1.0 / CLOCKS_PER_SEC;
}
#else
#if defined(HAVE_CLOCK_GETTIME)

#include <sys/time.h>

static double current_time(void)
{
//  DJV/AB3NR
// Replace call to deprecated ftime() function with call to
// clock_gettime(2).  clock_gettime(2) is POSIX.1 compliant.
// Replace ifdef __linux__ with autoconf generated guard.
// (Note __linux__ fails for *BSD, which all have better functions
// than the fallback Windows routine.  Note, clock(2) on Unix systems
// returns CPU time used so far.  It's not a "clock" in any generally
// accepted sense of the word.
//
// Check system call return codes and abort on failure.
//  DJV/AB3NR

	struct timespec ts;
	double curtime;

// Do not know if CLOCK_MONOTONIC or CLOCK_REALTIME is appropriate. DJV

	if(clock_gettime(CLOCK_REALTIME, &ts)!=0) {
                LOG_PERROR("clock_gettime");
                abort();
        }
	curtime= ts.tv_nsec*1e-09 + (double)ts.tv_sec;
	return curtime;
}
#else
#include <ctime>
// This is much less accurate.
// Add compile time warning  DJV/AB3NR
#warning imprecise clock() call in function current_time in wefax.cxx
static double current_time(void)
{
	clock_t clks = clock();

	return clks * 1.0 / CLOCKS_PER_SEC;
}
#endif // HAVE_CLOCK_GETTIME
#endif //__WIN32__

/// Callback continuously called by fldigi modem class.
int wefax::rx_process(const double *buf, int len)
{
	if( len == 0 )
	{
		return 0 ;
	}

	static const int avg_buf_size = 256 ;

	static int idx = 0 ;

	static double buf_tim[avg_buf_size];
	static int    buf_len[avg_buf_size];

	int idx_mod = idx % avg_buf_size ;

	/// Here we estimate the average number of pixels per second.
	buf_tim[idx_mod] = current_time();
	buf_len[idx_mod] = len ;

	++idx ;

	/// Wait some seconds otherwise not significant.
	if( idx >= avg_buf_size ) {
		if( idx == avg_buf_size ) {
			LOG_INFO( _("Starting samples loss control avg_buf_size=%d"), avg_buf_size);
		}
		int idx_mod_first = idx % avg_buf_size ;
		double total_tim = buf_tim[idx_mod] - buf_tim[idx_mod_first];
		int total_len = 0 ;
		for( int ix = 0 ; ix < avg_buf_size ; ++ix ) {
			total_len += buf_len[ix] ;
		}

		/// Estimate the real sample rate.
		double estim_smpl_rate = (double)total_len / total_tim ;

		/// If too far from what it should be, it means that pixels were lost.
		if( estim_smpl_rate < 0.95 * modem::samplerate ) {
			int expected_samples = (int)( modem::samplerate * total_tim + 0.5 );
			int missing_samples = expected_samples - total_len ;

			LOG_INFO(_("Lost %d samples idx=%d estim_smpl_rate=%f total_tim=%f total_len=%d. Auto-center."),
				missing_samples,
				idx,
				estim_smpl_rate,
				total_tim,
				total_len );

			REQ( wefax_pic::update_auto_center, true );

			if( missing_samples <= 0 ) {
				/// This should practically never happen.
				LOG_WARN(_("Cannot compensate"));
			} else {
				/// Adjust the number of received pixels,
				/// so the lost frames are signaled once only.
				buf_len[idx_mod] += missing_samples ;
			}
		}
	}

	/// Back to normal processing.
	m_impl->rx_new_samples( buf, len );
	return 0;
}

//=====================================================================
// transmit processing
//=====================================================================

/// This is called by wefax-pix.cxx before entering transmission loop.
void wefax::set_tx_parameters(
	int the_lpm,
	const unsigned char * xmtpic_buffer,
	bool is_color,
	int img_w,
	int img_h)
{
	m_impl->tx_params_set(
		the_lpm,
		xmtpic_buffer,
		is_color,
		img_w,
		img_h );
}

/// Callback continuously called by fldigi modem class.
int wefax::tx_process()
{
	bool tx_was_completed = m_impl->trx_do_next();
	std::string status ;
	if( false == tx_was_completed ) {
		status = _("Transmission cancelled");
		LOG_INFO("Sending cancelled" );
		m_qso_rec.putField(NOTES, status.c_str() );
	}
	qso_rec_save();

	REQ_FLUSH(GET_THREAD_ID());

	FL_LOCK_E();
	wefax_pic::restart_tx_viewer();
	transmit_lock_release(status);
	m_abortxmt = false;
	FL_UNLOCK_E();
	m_impl->tx_apt_stop();
	return -1;
}

void wefax::skip_apt(void)
{
	m_impl->skip_apt_rx();
}

/// auto_center indicates whether we try to find the margin of the image
/// automatically. This is the fact when skipping to image reception
/// is triggered manually or based on the signal power.
void wefax::skip_phasing(bool auto_center)
{
	m_impl->skip_phasing_rx(auto_center);
}

void wefax::end_reception(void)
{
	m_impl->end_rx();
}

// Continuous reception or APT control.
void wefax::set_rx_manual_mode( bool manual_flag )
{
	m_impl->manual_mode_set( manual_flag );
	wefax_pic::set_manual( manual_flag );
	update_rx_label();
}

void wefax::set_lpm( int the_lpm )
{
	return m_impl->lpm_set( the_lpm );
}

/// Transmission time in seconds. Factor 3 if b/w image.
int wefax::tx_time( int nb_bytes ) const
{
	return (double)nb_bytes / modem::samplerate ;
}

/// This prints a message about the progress of image sending,
/// then tells whether the user has requested the end.
bool wefax::is_tx_finished( int ix_sample, int nb_sample, const char * msg ) const
{
	static char wefaxmsg[256];
	double fraction_done = nb_sample ? 100.0 * (double)ix_sample / nb_sample : 0.0 ;
	int tm_left = tx_time( nb_sample - ix_sample );
	snprintf(
			wefaxmsg, sizeof(wefaxmsg),
			"%s : %04.1f%% done. Time left: %dm %ds",
			msg,
			fraction_done,
			tm_left / 60,
			tm_left % 60 );
	put_status(wefaxmsg);

	bool is_finished = modem::stopflag || m_abortxmt ;
	if( is_finished ) {
		LOG_INFO("Transmit finished");
	}
	return is_finished ;
}

/// This returns the names of the possible reception filters.
const char ** wefax::rx_filters(void)
{
	return fir_filter_pair_set::filters_list();
}

/// Allows to choose the reception filter.
void wefax::set_rx_filter( int idx_filter )
{
	m_impl->set_filter_rx( idx_filter );
}

std::string wefax::suggested_filename(void) const
{
	return m_impl->generate_filename( "gui" );
};

/// This creates a QSO record to be written to an adif file.
void wefax::qso_rec_init(void)
{
	/// This is always initialised because the flag progdefaults.WEFAX_AdifLog
	/// may be set in the middle of an image reception.

	/// Ideally we should find out the name of the fax station.
	m_qso_rec.putField(CALL, "Wefax");
	m_qso_rec.putField(NAME, "Weather fax");
	m_qso_rec.putField( TX_PWR, "0");
	m_qso_rec.setDateTime(true);
	m_qso_rec.setFrequency( wf->rfcarrier() + m_impl->carrier() );

	m_qso_rec.putField(MODE, mode_info[get_mode()].adif_name );

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
}

/// Called once a QSO rec has been filled with information. Saved to adif file.
void wefax::qso_rec_save(void)
{
	if( progdefaults.WEFAX_AdifLog == false ) {
		return ;
	}

	m_qso_rec.setDateTime(false);

	qsodb.qsoNewRec (&m_qso_rec);
	qsodb.isdirty(0);
	loadBrowser(true);

	adifFile.writeLog (logbook_filename.c_str(), &qsodb);
	// dxcc_entity_cache_add(&rec);
	LOG_INFO( _("Updating log book %s"), logbook_filename.c_str() );
}

/// Called when changing the carrier in the GUI, and by class modem with 1000Hz, when initializing.
void wefax::set_freq(double freq)
{
	modem::set_freq(freq);
	/// This must recompute the increments of the trigonometric tables.
	m_impl->set_carrier( freq );
}

// String telling the tx and rx internal state.
std::string wefax::state_string(void) const
{
	return m_impl->state_string();
}

/// Called when a file is saved, so XML-RPC calls can get the filename.
void wefax::put_received_file( const std::string &filnam )
{
	m_impl->put_received_file( filnam );
}

/// Returns a received file name, by chronological order.
std::string wefax::get_received_file( int max_seconds )
{
	return m_impl->get_received_file( max_seconds );
}

std::string wefax::send_file( const std::string & filnam, double max_seconds )
{
	return m_impl->send_file( filnam, max_seconds );
}

/// Transmitting files is done in exclusive mode.
bool wefax::transmit_lock_acquire(const std::string & filnam, double max_seconds )
{
	return m_impl->transmit_lock_acquire( filnam, max_seconds );
}

/// Called after a file is sent.
void wefax::transmit_lock_release( const std::string & err_msg )
{
	m_impl->transmit_lock_release( err_msg );
}

