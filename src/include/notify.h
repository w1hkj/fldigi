#ifndef NOTIFY_H_
#define NOTIFY_H_

#include "globals.h"

void notify_start(void);
void notify_stop(void);
void notify_show(void);
void notify_dxcc_show(bool readonly = true);
void notify_change_callsign(void);
void notify_rsid(trx_mode mode, int afreq);
void notify_create_rsid_event(bool val);

#endif // NOTIFY_H_
