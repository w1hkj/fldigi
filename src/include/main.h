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

#define Hmenu 24
#define Hqsoframe 48
#define Hnotes 24
#define Hrcvtxt 200
#define Hxmttxt 100
//#define Hrcvtxt 160
//#define Hxmttxt 100
#define Hwfall 140
//#define Hwfall 118
#define Hstatus 22
#define Hmacros 24
#define Wmode 80
#define Ws2n  100
#define Wimd  100
#define Wwarn 16
#define bwAfcOnOff	(Hwfall -22)/2
#define bwSqlOnOff	(Hwfall -22)/2

#define Wwfall 754
//#define Wwfall 654
#define HNOM (Hwfall + Hxmttxt + Hrcvtxt + Hmenu + (Hstatus + 4) + Hmacros + Hqsoframe + Hnotes)
#define WNOM (Wwfall + Hwfall - 22)
#define Wstatus (WNOM - Wmode - Ws2n - Wimd - Wwarn - bwAfcOnOff - bwSqlOnOff)


extern Fl_Mutex		trx_mutex;
extern Fl_Cond		trx_cond;
extern Fl_Thread	trx_thread;
extern state_t		trx_state;
extern modem		*active_modem;
extern string		HomeDir;
extern string		PskMailDir;
extern string		xmlfname;
extern bool			gmfskmail;

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

