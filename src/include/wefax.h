//
//    wefax.h  --  Weather Fax modem

#ifndef _WEFAX_H
#define _WEFAX_H

#include "globals.h"
#include "modem.h"
#include "filters.h"
#include "mbuffer.h"
#include "logbook.h"

#define WEFAXSampleRate 11025
// #define WEFAXSampleRate 8000


/// Forward definition.
class fax_implementation ;

class wefax : public modem {

	fax_implementation * m_impl ;
	
	bool	m_abortxmt;

	// Tells whether received or transmit images are logged to Adif file.
	bool    m_adif_log ;

	/// For updating the logbook when loading/saving an image file.
	cQsoRec m_qso_rec ;
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
		int img_h,
		int xmt_bytes );

	void set_tx_abort_flag(void)
	{
		m_abortxmt = true ;
	}

	/// Whether reading without end, or apt/phasing/stop.
	void set_rx_manual_mode( bool manual_flag );

	/// If a fax it bigger than that, it is automatically saved, and the image is reset.
	void set_max_lines( int max_lines_per_fax );

	int get_max_lines(void) const ;

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

	bool get_adif_log(void) const
	{
		return m_adif_log ;
	}

	void set_adif_log(bool the_flag)
	{
		m_adif_log = the_flag ;
	}
};

#endif
