// ----------------------------------------------------------------------------
// kiss_io.h
//
// Support for KISS interface
//
// Copyright (c) 2014, 2016
//		Robert Stiles, KK5VD
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef KISSIO_H
#define KISSIO_H

#include <vector>
#include <time.h>

#include "threads.h"
#include "socket.h"
#include "modem.h"
#include "psm/psm.h"

#define HDLC_CNT_OFFSET   32
#define HDLC_CNT          0xDE
#define HDLC_TCNT         0xE0

#define KISS_FEND         0xC0
#define KISS_FESC         0xDB
#define KISS_TFEND        0xDC
#define KISS_TFESC        0xDD

#define KISS_DATA         0x00
#define KISS_TXDELAY      0x01
#define KISS_PERSIST      0x02
#define KISS_SLOTTIME     0x03
#define KISS_TXTAIL       0x04
#define KISS_DUPLEX       0x05
#define KISS_HARDWARE     0x06
#define KISS_RAW          0x07 // Non standard frame type for ARQ use
#define KISS_RETURN       0x0F

#define KISS_RAW_DISABLED 0x00
#define KISS_RAW_ONLY     0x01
#define KISS_RAW_ON       0x02

#define KISS_CMD(a)             (a & 0x0F)
#define KISS_PORT(a)            ((a & 0xF0) >> 4)
#define SET_KISS_TYPE_PORT(t,p) ((t & 0x0F) | ((p << 4) & 0xF0))

#define SMACK_CRC(a)            (a & 0x08)
#define SMACK_CRC_MASK(a)       (a & 0xF7)
#define SMACK_CRC_ASSIGN(a)     ((a | 0x08) & 0xFF)
#define CRC_LOW(a)              (a & 0xFF)
#define CRC_HIGH(a)             ((a >> 8) & 0xFF)
#define CRC_LOW_HIGH(l,h)       (((h << 8) & 0xFF00) | (l & 0xFF))

#define CRC16_NONE  0
#define CRC16_CCITT 1
#define CRC16_FCS   2
#define CRC8_XOR    3

#define KISS_INVALID 512

#define KISS_HALF_DUPLEX 0
#define KISS_FULL_DUPLEX 1

#define BUFFER_PADDING 32
#define KISS_BUFFER_FACTOR 2
#define HDLC_BUFFER_FACTOR 3

#define KISS_CONNECT_RETRY_COUNT 10
#define KISS_RETRY_WAIT_TIME 1000

#define MAX_TEMP_BUFFER_SIZE 32000

#define TX_BUFFER_TIMEOUT (60 * 10) // Ten minute timeout
#define DISABLE_TX_INHIBIT_DURATION 5
#define KPSQL_MIN_BANDWIDTH 400

#define TEST_INTERVAL_SECONDS 15

typedef struct {
	char *data;
	size_t size;
} KISS_QUEUE_FRAME;

typedef struct {
	char *cmd;
	void (*cmd_func) (char *arg);
} EXEC_HARDWARE_CMD_MATCH;

#define AX25_FRAME_MARKER 0xFF

extern time_t temp_disable_tx_inhibit;
extern time_t temp_disable_tx_duration;

extern void AbortKiss();

extern bool bcast_rsid_kiss_frame(int new_wf_pos, int new_mode, int old_wf_pos, int old_mode, int notify);
extern bool valid_kiss_modem(std::string _modem);
extern void check_kiss_modem(void);
extern void bcast_trxs_kiss_frame(int state);
extern void bcast_tx_buffer_empty_kiss_frame(void);
extern bool tcp_init(bool connect_flag);
extern bool udp_init(void);
extern void kiss_init(bool connect_flag);
extern void kiss_close(bool override_flag);
extern void connect_to_kiss_io(bool user_requested);
extern bool kiss_thread_running(void);
extern int kiss_get_char(void);
extern void ax25_decode(unsigned char *buffer, size_t count, bool pad, bool tx_flag);
extern void flush_kiss_tx_buffer(void);

#endif // KISSIO_H
