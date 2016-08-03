// ----------------------------------------------------------------------------
//
//	data_io.h
//
// support for ARQ server/client system such as pskmail and fl_arq
// support for KISS server/client system such as BPQ and JNOS
//
// Copyright (C) 2014
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

#ifndef fldigi_data_io_h
#define fldigi_data_io_h

#include "gettext.h"

#define IO_START_STR      _("Start")
#define IO_STOP_STR       _("Stop")

#define DEFAULT_ARQ_IP_ADDRESS    "127.0.0.1"
#define DEFAULT_ARQ_IP_PORT       "7322"

#define DEFAULT_KISS_IP_ADDRESS   "127.0.0.1"
#define DEFAULT_KISS_IP_IO_PORT   "7342"
#define DEFAULT_KISS_IP_OUT_PORT  "7343"

#define DEFAULT_XMLPRC_IP_ADDRESS "127.0.0.1"
#define DEFAULT_XMLRPC_IP_PORT    "7362"

#define DEFAULT_FLRIG_IP_ADDRESS  "127.0.0.1"
#define DEFAULT_FLRIG_IP_PORT     "12345"

#define DEFAULT_FLLOG_IP_ADDRESS  "127.0.0.1"
#define DEFAULT_FLLOG_IP_PORT     "8421"

enum {DISABLED_IO, ARQ_IO, KISS_IO, XMLRPC_IO, FLRIG_IO, FLLOG_IO};

#define RSID_KISS_NOTIFY    0x01
#define RSID_KISS_ACTIVE    0x02
#define RSID_KISS_USER      0x03

#define DATA_IO_NA  0x00
#define DATA_IO_TCP 0x01
#define DATA_IO_UDP 0x02

// This variable indepent of progdefaults.data_io_enabled
// and progStatus.data_io_enabled
// Only on start do we assign this variable with progStatus.data_io_enabled.
// This is one way assignment as we dont want to save all of the available states
// this variable will have.
extern int data_io_enabled; // Located in kiss_io.cxx
extern int data_io_type;    // Located in kiss_io.cxx

extern void disable_config_p2p_io_widgets(void);
extern void enable_config_p2p_io_widgets(void);
extern void set_ip_to_default(int which_io);

// KISS implementation
extern void	kiss_init(bool connect_flag);
extern void	kiss_close(bool override_flag);
extern void	kiss_reset(void);
extern void WriteKISS(const char data);
extern void WriteKISS(const char *data);
extern void WriteKISS(const char *data, size_t size);
extern void WriteKISS(std::string data);
extern bool kiss_thread_running(void);

extern bool kiss_auto_connect(void);

extern void check_kiss_modem(void);
extern int  kiss_get_char(void);
extern bool kiss_text_available; // Located in kiss_io.cxx

extern bool kiss_bcast_rsid_reception;
extern bool kiss_bcast_trx_toggle;
extern bool bcast_rsid_kiss_frame(int new_wf_pos, int new_mode, int old_wf_pos, int old_mode, int notify);
extern void bcast_trxc_kiss_frame(void);
extern void kiss_io_set_button_state(void *);
extern void connect_to_kiss_io(bool);

// ARQ implementation
extern void	arq_init(void);
extern void	arq_close(void);
extern bool arq_state(void);
extern void	WriteARQ(unsigned char);
extern void	checkTLF(void);
extern int  arq_get_char(void);
extern bool	arq_text_available;
extern bool	gmfskmail;
extern std::string ArqFilename;
extern bool	mailclient;
extern bool	mailserver;
extern bool	tlfio;
extern time_t inhibit_tx_seconds;

// Misc calls
extern void enable_disable_kpsql(void);
#endif
