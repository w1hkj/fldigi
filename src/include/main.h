#ifndef _MAIN_H
#define _MAIN_H

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/time.h>
#include <string>
#include <FL/Fl_Double_Window.H>
#include <FL/filename.H>

#include "trx.h"
#include "modem.h"
#include "ScopeDialog.h"
#include "fl_digi.h"
#include "ptt.h"
#include "log.h"

#ifndef NOHAMLIB
	#include "rigclass.h"
#endif


extern Fl_Mutex		trx_mutex;
extern Fl_Cond		trx_cond;
extern Fl_Thread	trx_thread;
extern state_t		trx_state;
extern modem		*active_modem;
extern string		HomeDir;
extern string		PskMailDir;
extern string		xmlfname;
extern bool			gmfskmail;
extern bool			testmenu;

extern std::string	 scDevice;
extern PTT			*push2talk;
#ifndef NOHAMLIB
extern Rig			*xcvr;
#endif

extern cLogfile		*Maillogfile;
extern cLogfile		*logfile;
extern bool			mailclient;
extern bool			mailserver;
extern bool			pskmail_text_available;
extern char			pskmail_get_char();
extern void			pskmail_loop(void *);

#endif

