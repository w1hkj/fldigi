// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//              Remi Chateauneu
//
// This file is part of fldigi
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

#ifndef _WEFAX_H
#define _WEFAX_H

#include "globals.h"
#include "modem.h"
#include "filters.h"
#include "mbuffer.h"
#include "logbook.h"

/// Forward definition.
class fax_implementation ;

class wefax : public modem {

	fax_implementation * m_impl ;
	
	bool	m_abortxmt;

	/// For updating the logbook when loading/saving an image file.
	cQsoRec m_qso_rec ;

	/// Non-copiable object.
	wefax ();
	wefax ( const wefax & );
	wefax & operator=( const wefax & );
public:
	wefax (trx_mode md);
	virtual ~wefax ();
	void init();
	void rx_init();
	void tx_init(SoundBase *sc);
	void restart() {};
	int  rx_process(const double *buf, int len);
	int  tx_process();
	void shutdown();
	int  tx_time( int nb_bytes ) const ;
	bool is_tx_finished( int ix_sample, int nb_sample, const char * msg ) const;

	void skip_apt(void);
	void skip_phasing(bool auto_center);
	void end_reception(void);

	void set_tx_parameters(
		int the_lpm,
		const unsigned char * xmtpic_buffer,
		bool is_color,
		int img_w,
		int img_h );

	void set_tx_abort_flag(void)
	{
		m_abortxmt = true ;
	}

	/// Whether reading without end, or apt/phasing/stop.
	void set_rx_manual_mode( bool manual_flag );

	/// There are several possible input filter designated
	/// by a name, for displaying, and an index.
	static const char ** rx_filters(void);

	/// Set by the GUI.
	void set_rx_filter( int idx_filter );

	void set_lpm( int the_lpm );

	/// Restores the window label by taking into account wefax state mode.
	void update_rx_label(void) const ;

	/// Returns a filename matching current image properties.
	std::string suggested_filename(void) const ;

	cQsoRec & qso_rec(void)
	{
		return m_qso_rec ;
	}

	/// Called before loading/sending an image.
	void qso_rec_init(void);

	/// Called when transmitting/receiving is finished.
	void qso_rec_save(void);

	void set_freq(double);

	/// Helper string indicating the internal state of the wefax engine.
	std::string state_string(void) const;

	/// Maximum wait time when getting information about received and sent files.
	static const int max_delay = 3600 * 24 * 365 ;

	/// Called by the engine when a file is received.
	void put_received_file(const std::string & filnam);

	/// Used by XML-RPC to get the list of received files.
	std::string get_received_file(int max_seconds=max_delay);

	/// Called by XML-RPC to send a file which resides on the machine where fldigi runs.
	std::string send_file( const std::string & filnam, double max_seconds=max_delay);

	/// Called before sending a file. Transmitting is an exclusive process.
	bool transmit_lock_acquire( const std::string & filnam, double max_seconds=max_delay);

	/// Called after sending a file so another sending can take place.
	void transmit_lock_release( const std::string & err_msg );
};

#endif
