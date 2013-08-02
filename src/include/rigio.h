#ifndef RIGIO_H
#define RIGIO_H

#include <string>

#include "serial.h"

extern Cserial rigio;

extern bool hexout(const std::string&);

extern bool sendCommand(std::string, int retnbr);

extern long long rigCAT_getfreq(int retries, bool &failed);
extern void rigCAT_setfreq(long long);

extern std::string rigCAT_getmode();
extern void rigCAT_setmode(const std::string&);

extern std::string rigCAT_getwidth();
extern void rigCAT_setwidth(const std::string&);

extern void rigCAT_close();
extern bool rigCAT_init(bool);

extern void rigCAT_set_ptt(int);
extern void rigCAT_set_qsy(long long f);

extern void rigCAT_defaults();
extern void rigCAT_restore_defaults();

#endif

