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

#ifndef MAIN_H_
#define MAIN_H_

#include <config.h>

#include <string>

#include "ptt.h"
#include "log.h"
#include "data_io.h"

#if USE_HAMLIB
#  include "rigclass.h"
#endif

extern std::string		appname;
extern std::string		BaseDir;
extern std::string		HomeDir;
extern std::string		RigsDir;
extern std::string		ScriptsDir;
extern std::string		PalettesDir;
extern std::string		LogsDir;
extern std::string		PicsDir;
extern std::string		AvatarDir;
extern std::string		HelpDir;
extern std::string		MacrosDir;
extern std::string		WrapDir;
extern std::string		TalkDir;
extern std::string		TempDir;
extern std::string		LoTWDir;
extern std::string		KmlDir;
extern std::string		PskMailDir;
extern std::string		DATA_dir;
extern std::string		NBEMS_dir;
extern std::string		ARQ_dir;
extern std::string		ARQ_files_dir;
extern std::string		ARQ_recv_dir;
extern std::string		ARQ_send;
extern std::string		WRAP_dir;
extern std::string		WRAP_recv_dir;
extern std::string		WRAP_send_dir;
extern std::string		WRAP_auto_dir;
extern std::string		ICS_dir;
extern std::string		ICS_msg_dir;
extern std::string		ICS_tmp_dir;

extern std::string		FLMSG_dir;
extern std::string		FLMSG_WRAP_dir;
extern std::string		FLMSG_WRAP_recv_dir;
extern std::string		FLMSG_WRAP_send_dir;
extern std::string		FLMSG_WRAP_auto_dir;
extern std::string		FLMSG_ICS_dir;
extern std::string		FLMSG_ICS_msg_dir;
extern std::string		FLMSG_ICS_tmp_dir;

extern std::string		FLAMP_dir;
extern std::string		FLAMP_rx_dir;
extern std::string		FLAMP_tx_dir;

extern std::string		xmlfname;

extern std::string	 	scDevice[2];
extern PTT			*push2talk;

#if USE_HAMLIB
extern Rig			*xcvr;
#endif

extern cLogfile			*Maillogfile;
extern cLogfile			*logfile;
extern bool			gmfskmail;
extern bool			arqmode;
extern std::string		ArqFilename;
extern bool			mailclient;
extern bool			mailserver;
extern bool			tlfio;
extern bool			arq_text_available;
extern int			arq_get_char();

// ARQ mail implementation
extern void			arq_init();
extern void			arq_close();
extern void			arq_restart();
extern void			WriteARQ(unsigned char);
extern void			checkTLF();

void check_nbems_dirs(void);
void check_data_dir(void);
extern bool nbems_dirs_checked;

// This inits or reinits everything related to KML: Reloads params etc...
void kml_init(bool load_files = false);

// close down remaining threads just before exiting UI
extern void exit_process();

int directory_is_created( const char * strdir );

// autostart an external program
extern void start_process(std::string executable);


#endif
