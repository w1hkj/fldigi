#ifndef N3FJP_LOGGER_H
#define N3FJP_LOGGER_H

#include "qso_db.h"

extern void get_n3fjp_udp();
extern void *n3fjp_loop(void *args);
extern void n3fjp_init(void);
extern void n3fjp_start();
extern void n3fjp_restart();
extern void n3fjp_close(void);

extern bool n3fjp_dupcheck();
extern void n3fjp_add_record(cQsoRec &rec);
extern void n3fjp_get_record(string rec);
extern void n3fjp_set_freq(long f);
extern void n3fjp_set_ptt(int on);
extern void n3fjp_clear_record();

extern bool n3fjp_connected;
extern bool n3fjp_calltab;

#endif // N3FJP_LOGGER_H
