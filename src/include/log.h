// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
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

#ifndef _LOG_H
#define _LOG_H

#include <cstdio>
#include <string>

class cLogfile {
public:
	enum log_t { LOG_RX, LOG_TX, LOG_START, LOG_STOP };
private:
	FILE*	logfile;
	bool	retflag;
	log_t	logtype;

public:
	cLogfile(const std::string& fname);
	~cLogfile();
	void	log_to_file(log_t type, const std::string& s);
	void	log_to_file_start();
	void	log_to_file_stop();
};
	
#endif
