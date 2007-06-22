/*
 *    hamlib.h  --  Hamlib (rig control) interface
 */

#ifndef _HAMLIB_H
#define _HAMLIB_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <iostream>
#include <string>
#include <list>

#include <hamlib/rig.h>

#include "config.h"

using namespace std;

extern void get_rignames();
extern void get_riglist();

extern void hamlib_close();
extern bool hamlib_init(bool bPtt);
extern void hamlib_set_ptt(int);
extern void hamlib_set_qsy();
extern int	hamlib_setfreq(long int);
extern int	hamlib_setmode(rmode_t);
extern rmode_t hamlib_getmode();
extern int	hamlib_setwidth(pbwidth_t);
extern pbwidth_t hamlib_getwidth();

extern list<string> rignames;

#endif

