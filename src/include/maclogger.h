#ifndef MACLOGGERIO_H
#define MACLOGGERIO_H

#include <vector>

#include "threads.h"
#include "socket.h"
#include "modem.h"

#define MACLOGGER_BUFFER_SIZE 2048

extern void get_maclogger_udp();
extern void *maclogger_loop(void *args);
extern void maclogger_init(void);
extern bool maclogger_start(void);
extern void maclogger_close(void);

#endif // MACLOGGERIO_H
