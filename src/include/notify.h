#ifndef NOTIFY_H_
#define NOTIFY_H_

void notify_start(void);
void notify_stop(void);
void notify_show(void);
void notify_dxcc_show(bool readonly = true);
void notify_change_callsign(void);

#endif // NOTIFY_H_
