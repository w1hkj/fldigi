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

