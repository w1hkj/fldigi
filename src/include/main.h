#ifndef _MAIN_H
#define _MAIN_H

#include <config.h>
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

extern string		appname;
extern string		HomeDir;
extern string		RigsDir;
extern string		ScriptsDir;
extern string		PalettesDir;
extern string		LogsDir;
extern string		PicsDir;
extern string		HelpDir;
extern string		MacrosDir;
extern string		TempDir;
extern string		PskMailDir;

extern string		xmlfname;

extern std::string	 scDevice[2];
extern PTT			*push2talk;
#if USE_HAMLIB
extern Rig			*xcvr;
#endif

extern cLogfile		*Maillogfile;
extern cLogfile		*logfile;
extern bool			gmfskmail;
extern bool			arqmode;
extern string		ArqFilename;
extern bool			mailclient;
extern bool			mailserver;
extern bool			tlfio;
extern bool			arq_text_available;
extern char			arq_get_char();

// ARQ mail implementation
extern void			arq_init();
extern void			arq_close();
extern void			WriteARQ(unsigned int);
extern void			checkTLF();

#define ARQBUFSIZ 8192

struct RXMSGSTRUC {
	long int msg_type;
	char c;
};

struct TXMSGSTRUC {
	long int msg_type;
	char buffer[ARQBUFSIZ];
};

extern RXMSGSTRUC rxmsgst;
extern int rxmsgid;
extern TXMSGSTRUC txmsgst;
extern int txmsgid;

#endif

