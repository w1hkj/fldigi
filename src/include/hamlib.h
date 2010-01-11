/*
 *    hamlib.h  --  Hamlib (rig control) interface
 */

#ifndef _HAMLIB_H
#define _HAMLIB_H

#include <hamlib/rig.h>

extern void hamlib_get_rigs(void);
extern size_t hamlib_get_index(rig_model_t model);
extern rig_model_t hamlib_get_rig_model(size_t i);
extern rig_model_t hamlib_get_rig_model_compat(const char* name);
extern void hamlib_get_rig_str(int (*func)(const char*));

extern void hamlib_close();
extern bool hamlib_init(bool bPtt);
extern void hamlib_set_ptt(int);
extern void hamlib_set_qsy(long long f);
extern int	hamlib_setfreq(long int);
extern int	hamlib_setmode(rmode_t);
extern rmode_t hamlib_getmode();
extern int	hamlib_setwidth(pbwidth_t);
extern pbwidth_t hamlib_getwidth();
extern void hamlib_get_defaults();
extern void hamlib_restore_defaults();
extern void hamlib_init_defaults();

#endif

