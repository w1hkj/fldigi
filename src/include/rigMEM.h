#ifndef RIG_MEM_H
#define RIG_MEM_H

#include <string>

extern bool rigMEM_init(void);
extern void rigMEM_close(void);
extern bool rigMEM_active(void);
extern void setrigMEM_PTT (bool);
extern void rigMEM_set_qsy(long long f);
extern void rigMEM_set_freq(long long f);
extern bool rigMEM_CanPTT(void);
extern void rigMEM_setmode(const std::string&);

#endif
