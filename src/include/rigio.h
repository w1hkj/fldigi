#ifndef RIGIO_H
#define RIGIO_H

#include <string>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <iostream>
#include <string>
#include <list>
#include <vector>

#include "serial.h"

using namespace std;

extern Cserial rigio;

extern bool hexout( string);

extern long long rigCAT_getfreq();
extern void rigCAT_setfreq(long long);

extern string rigCAT_getmode();
extern void rigCAT_setmode(string);

extern string rigCAT_getwidth();
extern void rigCAT_setwidth(string);

extern void rigCAT_close();
extern bool rigCAT_init();

extern void rigCAT_set_ptt(int);
extern void rigCAT_set_qsy();

#endif

