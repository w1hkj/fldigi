#ifndef RIG_MEM_H
#define RIG_MEM_H


extern void rigMEM_init(void);
extern void rigMEM_close(void);
extern bool rigMEM_active(void);
extern void setrigMEM_PTT (bool);
extern void rigMEM_set_qsy(void);
extern bool rigMEM_CanPTT(void);

#endif
