#ifndef _MAIN_H
#define _MAIN_H

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/time.h>
#include <string>
#include <glob.h>
#include <FL/Fl_Double_Window.H>
#include <FL/filename.H>

#include "trx.h"
#include "modem.h"
#include "fl_digi.h"
#include "ptt.h"
#include "log.h"

#if USE_HAMLIB
	#include "rigclass.h"
#endif

extern Fl_Mutex		trx_mutex;
extern Fl_Cond		trx_cond;
extern Fl_Thread	trx_thread;
extern state_t		trx_state;
extern modem		*active_modem;
extern string		HomeDir;
extern string		xmlfname;

extern std::string	 scDevice;
extern PTT			*push2talk;
#if USE_HAMLIB
extern Rig			*xcvr;
#endif

extern cLogfile		*Maillogfile;
extern cLogfile		*logfile;
extern string		PskMailDir;
extern bool			gmfskmail;
extern bool			arqmode;
extern string		ArqFilename;
extern bool			mailclient;
extern bool			mailserver;
extern bool			pskmail_text_available;
extern char			pskmail_get_char();
extern void			pskmail_loop(void *);

struct RXMSGSTRUC {
	long int msg_type;
	char c;
};

struct TXMSGSTRUC {
	long int msg_type;
	char buffer[BUFSIZ];
};

extern RXMSGSTRUC rxmsgst;
extern int rxmsgid;
extern TXMSGSTRUC txmsgst;
extern int txmsgid;

#endif

