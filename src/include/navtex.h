//
//	navtex.h

#ifndef _NAVTEX_H
#define _NAVTEX_H

/// Forward definition.
class navtex_implementation ;

#include <string>

#include "modem.h"

class navtex : public modem {
	navtex_implementation * m_impl ;

	/// Non-copiable object.
	navtex();
	navtex(const navtex *);
	navtex & operator=(const navtex *);
public:
	navtex (trx_mode md);
	virtual ~navtex();
	void rx_init();
	void tx_init(SoundBase *sc);
	void restart();
	int  rx_process(const double *buf, int len);
	int  tx_process();
	void set_freq( double );

	std::string get_message(int max_seconds);
	std::string send_message(const std::string & msg);
};
#endif

