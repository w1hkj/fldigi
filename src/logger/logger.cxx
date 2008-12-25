// ====================================================================
//  logger.cxx Remote Log Interface for fldigi
//
// Copyright W1HKJ, Dave Freese 2006
//
// This library is free software; you can RGBredistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "w1hkj@w1hkj.com".
//
// ====================================================================

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef __CYGWIN__
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif
#include <errno.h>
#include <string>

#include "logger.h"
#include "lgbook.h"
#include "main.h"
#include "modem.h"
#include "debug.h"
#include "macros.h"
#include "status.h"
#include "spot.h"

#include "adif_io.h"

#include "logsupport.h"

#include <FL/fl_ask.H>

//---------------------------------------------------------------------
const char *logmode;
char logdate[32];
char logtime[32];

static string log_msg;
static string errmsg;

//---------------------------------------------------------------------
// the following IPC message is compatible with xlog remote data spec.
//---------------------------------------------------------------------

#ifndef __CYGWIN__

#define addtomsg(x, y)	log_msg = log_msg + (x) + (y) + LOG_MSEPARATOR

static void send_IPC_log(void)
{
	msgtype msgbuf;
	const char   LOG_MSEPARATOR[2] = {1,0};
	char strFreqMhz[20];
	int msqid, len;
	
	snprintf(strFreqMhz, sizeof(strFreqMhz), "%-10f", wf->dFreq()/1.0e6);

	log_msg = "";
	log_msg = log_msg + "program:"	+ PACKAGE_NAME + " v " 	+ PACKAGE_VERSION + LOG_MSEPARATOR;
	addtomsg("version:",	LOG_MVERSION);
	addtomsg("date:",		logdate);
	addtomsg("time:", 		inpTimeOn_log->value());
	addtomsg("endtime:", 	inpTimeOff_log->value());
	addtomsg("call:",		inpCall_log->value());
	addtomsg("mhz:",		strFreqMhz);
	addtomsg("mode:",		logmode);
	addtomsg("tx:",			inpRstS_log->value());
	addtomsg("rx:",			inpRstR_log->value());
	addtomsg("name:",		inpName_log->value());
	addtomsg("qth:",		inpQth_log->value());
	addtomsg("state:",		inpState_log->value());
	addtomsg("province:",	inpVE_Prov_log->value());
	addtomsg("country:",	inpCountry_log->value());
	addtomsg("locator:",	inpLoc_log->value());
	addtomsg("serialout:",	inpSerNoOut_log->value());
	addtomsg("serialin:",	inpSerNoIn_log->value());
	addtomsg("free1:",		inpXchg1_log->value());
	addtomsg("free2:",		inpXchg2_log->value());
	addtomsg("notes:",		inpComment_log->value());
	addtomsg("power:",		inpTX_pwr_log->value());
	
	strncpy(msgbuf.mtext, log_msg.c_str(), sizeof(msgbuf.mtext));
	msgbuf.mtext[sizeof(msgbuf.mtext) - 1] = '\0';

	if ((msqid = msgget(LOG_MKEY, 0666 | IPC_CREAT)) == -1) {
		errmsg = "msgget: ";
		errmsg.append(strerror(errno));
		LOG_ERROR("%s", errmsg.c_str());
		fl_message(errmsg.c_str());
		return;
	}
	msgbuf.mtype = LOG_MTYPE;
	len = strlen(msgbuf.mtext) + 1;
	if (msgsnd(msqid, &msgbuf, len, IPC_NOWAIT) < 0) {
		errmsg = "msgsnd: ";
		fl_message(errmsg.c_str());
	}
}

#endif

//---------------------------------------------------------------------

void submit_log(void)
{
	if (progStatus.spot_log)
		spot_log(inpCall->value(), inpLoc->value());

	struct tm *tm;
	time_t t;

	time(&t);
        tm = gmtime(&t);
		strftime(logdate, sizeof(logdate), "%d %b %Y", tm);
		strftime(logtime, sizeof(logtime), "%H%M", tm);
	logmode = mode_info[active_modem->get_mode()].adif_name;

	AddRecord();

#ifndef __CYGWIN__
	send_IPC_log();
#endif

}

