// ----------------------------------------------------------------------------
// kiss_io.cxx
//
// support for KISS interface
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


#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <errno.h>
#include <float.h>

#include <sys/types.h>
#if !defined(__MINGW32__) && !defined(__APPLE__)
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif

#include <signal.h>

#include "config.h"

#ifdef __MINGW32__
#  include "compat.h"
#endif

#include "main.h"
#include "configuration.h"
#include "fl_digi.h"
#include "trx.h"
#include "kiss_io.h"
#include "globals.h"

#include "threads.h"
#include "socket.h"
#include "debug.h"
#include "qrunner.h"
#include "data_io.h"
#include "status.h"
#include "psm/psm.h"

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Check_Button.H>

#include "confdialog.h"
#include "configuration.h"
#include "status.h"

LOG_FILE_SOURCE(debug::LOG_KISSCONTROL);

//#define EXTENED_DEBUG_INFO
//#undef EXTENED_DEBUG_INFO

using namespace std;
//======================================================================
// Socket KISS i/o used on all platforms
//======================================================================

#define KISSLOOP_TIMING   100 // msec
#define KISSLOOP_FRACTION 10  // msec

static string errstring;

// =====================================================================
static pthread_t kiss_thread;
static pthread_t kiss_rx_socket_thread;
static pthread_t kiss_watchdog_thread;

static pthread_cond_t kiss_watchdog_cond    = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t from_host_mutex      = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t from_radio_mutex     = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t kiss_bc_frame_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t kiss_frame_mutex     = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t to_host_arq_mutex    = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t to_host_mutex        = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t to_radio_mutex       = PTHREAD_MUTEX_INITIALIZER;
//static pthread_mutex_t external_mutex       = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t restart_mutex        = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t kiss_encode_mutex    = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t kiss_loop_exit_mutex = PTHREAD_MUTEX_INITIALIZER;

bool kiss_enabled = false;
bool kiss_exit = false;
bool kiss_rx_exit = false;
bool allow_kiss_socket_io  = false;
bool kiss_loop_running     = false;
bool kiss_rx_loop_running  = false;
bool kiss_watchdog_exit    = false;
bool kiss_watchdog_running = false;
bool kiss_tcp_ip_connected = false;
bool tcpip_reset_flag      = false;

static bool smack_crc_enabled  = false;
static int crc_mode            = CRC16_CCITT;
static std::string default_kiss_modem = "BPSK250";
static std::string kiss_modem = "";
static unsigned int transmit_buffer_flush_timeout = 0;

unsigned int duplex = KISS_HALF_DUPLEX; // Default half duplex
unsigned int kiss_port_no = 0;          // Default is 0

/// Any access to shared variables must be protected.
static std::string from_host         = "";
static std::string from_radio        = "";
static std::string from_radio_parsed = "";
static std::string kiss_bc_frame     = "";
static std::string kiss_frame        = "";
static std::string kiss_ip_address   = "";
static std::string kiss_ip_io_port   = "";
static std::string kiss_ip_out_port  = "";
static std::string kiss_one_frame    = "";
static std::string to_arq_host       = "";
static std::string to_host           = "";
static std::string to_radio          = "";
static std::string translated_frame  = "";
static int pText = 0;

bool bcast_tx_buffer_empty_flag = false;
bool kiss_bcast_rsid_reception  = false;
bool kiss_bcast_trx_toggle      = false;
bool kiss_text_available        = false;
static bool kiss_reset_flag     = false;
static int  retry_count         = KISS_CONNECT_RETRY_COUNT;

#define HISTO_COUNT 256
#define HISTO_THRESH_HOLD 48
// In seconds
#define HISTO_RESET_TX_TIME 3

static int histogram[HISTO_COUNT];
static bool init_hist_flag = true;
// static double threshold = 5.0;
// time_t inhibit_tx_seconds = 0;

time_t temp_disable_tx_inhibit = 0;
time_t temp_disable_tx_duration = DISABLE_TX_INHIBIT_DURATION;

static int kpsql_pl = 0;
static double kpsql_threshold = 0.0;

extern int IMAGE_WIDTH;
Socket *kiss_socket = 0;
int data_io_enabled = DISABLED_IO;
int data_io_type    = DATA_IO_UDP;

//program_start_time
bool program_started_flag = 0;

extern const struct mode_info_t mode_info[];
extern void abort_tx();

// Storage for modem list allowed for KISS use.
static std::vector<std::string> availabe_kiss_modems;
static int kiss_raw_enabled = KISS_RAW_DISABLED;
std::string host_name_string;

inline std::string uppercase_string(std::string str);
inline void set_tx_timeout(void);
size_t hdlc_decode(char *src, size_t src_size, char **dst);
size_t hdlc_encode(char *src, size_t src_size, char **dst);
size_t kiss_decode(char *src, size_t src_size, char **dst);
size_t kiss_encode(char *src, size_t src_size, char **dst);
static bool kiss_queue_frame(KISS_QUEUE_FRAME * frame, std::string cmd);
static int  calc_ccitt_crc(char *buf, int n);
static int  calc_fcs_crc(char *buf, int n);
static int  calc_xor_crc(char *buf, int n);
static KISS_QUEUE_FRAME * encap_kiss_frame(std::string data, int frame_type, int port);
static KISS_QUEUE_FRAME *encap_kiss_frame(char *buffer, size_t buffer_size, int frame_type, int port);
static size_t decap_hdlc_frame(char *buffer, size_t data_count);
static size_t encap_hdlc_frame(char *buffer, size_t data_count);
static void *kiss_loop(void *args);
static void *ReadFromHostSocket(void *args);
static void *tcpip_watchdog(void *args);
static void exec_hardware_command(std::string cmd, std::string arg);
static void host_name(char *arg);
static void kiss_tcp_disconnect(char *arg);
static void parse_hardware_frame(std::string frame);
static void parse_kiss_frame(std::string frame_segment);
static void ReadFromHostBuffered(void);
static void reply_active_modem_bw(char * arg);
static void reply_active_modem(char * arg);
static void reply_busy_channel_duration(char * arg);
static void reply_busy_channel_on_off(char * arg);
static void reply_busy_state(char * arg);
static void reply_crc_mode(char *arg);
static void reply_csma_mode(char *arg);
static void reply_fldigi_stat(char *arg);
static void reply_kiss_raw_mode(char * arg);
static void reply_kpsql_fraction_gain(char *arg);
static void reply_kpsql_on_off(char * arg);
static void reply_kpsql_pwr_level(char * arg);
static void reply_kpsql_squelch_level(char * arg);
static void reply_modem_list(char * arg);
static void reply_psm_on_off(char * arg);
static void reply_psm_pwr_level(char * arg);
static void reply_psm_squelch_level(char * arg);
static void reply_rsid_bc_mode(char * arg);
static void reply_rsid_mode_state(char * arg);
static void reply_rsid_rx_state(char * arg);
static void reply_rsid_tx_state(char * arg);
static void reply_sql_level(char * arg);
static void reply_sql_on_off(char * arg);
static void reply_sql_pwr_level(char * arg);
static void reply_tnc_name(char * arg);
static void reply_trx_state(char * arg);
static void reply_trxs_bc_mode(char * arg);
static void reply_tx_buffer_count(char * arg);
static void reply_txbe_bc_mode(char * arg);
static void reply_waterfall_bw(char * arg);
static void reply_wf_freq_pos(char * arg);
static void send_disconnect_msg(void);
static void set_busy_channel_duration(char * arg);
static void set_busy_channel_inhibit(char *arg);
static void set_busy_channel_on_off(char * arg);
static void set_button(Fl_Button * button, bool value);
static void set_counter(Fl_Counter * counter, int value);
static void set_crc_mode(char * arg);
static void set_csma_mode(char * arg);
static void set_default_kiss_modem(void);
static void set_kiss_modem(char * arg);
static void set_kiss_raw_mode(char * arg);
static void set_kpsql_on_off(char * arg);
static void set_kpsql_squelch_level(char * arg);
static void set_psm_fraction_gain(char *arg);
static void set_psm_on_off(char * arg);
static void set_psm_squelch_level(char * arg);
static void set_reply_tx_lock(char * arg);
static void set_rsid_bc_mode(char * arg);
static void set_rsid_mode(char *arg);
static void set_rsid_rx(char *arg);
static void set_rsid_tx(char * arg);
static void set_sql_level(char * arg);
static void set_sql_on_off(char * arg);
static void set_trxs_bc_mode(char * arg);
static void set_txbe_bc_mode(char * arg);
static void set_wf_cursor_pos(char * arg);
static void WriteToHostARQBuffered(void);
static void WriteToHostBuffered(const char *data, size_t size);
static void WriteToRadioBuffered(const char *data, size_t size);
std::string kiss_decode(std::string frame);
std::string kiss_encode(std::string frame);
std::string unencap_kiss_frame(char *buffer, size_t buffer_size, int *frame_type, int *kiss_port_no);
std::string unencap_kiss_frame(std::string package, int *frame_type, int *kiss_port_no);
void kiss_main_thread_close(void *ptr);
void kiss_main_thread_retry_open(void *ptr);
void kiss_reset_buffers(void);
void kiss_reset(void);
void ReadFromRadioBuffered(void);
void WriteKISS(const char *data, size_t size);
void WriteKISS(const char *data);
void WriteKISS(const char data);
void WriteKISS(std::string data);
void WriteToHostBCastFramesBuffered(void);
void WriteToHostSocket(void);

/**********************************************************************************
 * Not all modems are created equal. Translate!
 * There must be at least HDLC_CNT_OFFSET count offset (+/-, but still in range)
 * between real values and translated values.
 **********************************************************************************/
static int not_allowed[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, //  16
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //  32
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //  48
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //  64
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //  80
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //  96
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 112
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, // 128
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 144
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 160
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 176
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 192
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 208
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 224
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 240
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1  // 256
};

/**********************************************************************************
 * KISS hardware frame commands strings and calling functions
 **********************************************************************************/
EXEC_HARDWARE_CMD_MATCH exec_match[] = {
	{ (char *) "BCHN",      set_busy_channel_on_off },
	{ (char *) "BCHNS",     set_busy_channel_duration },
	{ (char *) "BUSY",      reply_busy_state },
	{ (char *) "CSMA",      set_csma_mode },
	{ (char *) "DISC",      kiss_tcp_disconnect },
	{ (char *) "FLSTAT",    reply_fldigi_stat },
	{ (char *) "HOST",      host_name },
	{ (char *) "IBCHN",     set_busy_channel_inhibit },
	{ (char *) "KISSCRCM",  set_crc_mode },
	{ (char *) "KISSRAW",   set_kiss_raw_mode },
	{ (char *) "KPSATT",    set_psm_fraction_gain },   // Depreciated
	{ (char *) "PSMATT",    set_psm_fraction_gain },
	{ (char *) "PSM",       set_psm_on_off },
	{ (char *) "PSMP",      reply_psm_pwr_level },
	{ (char *) "PSMS",      set_psm_squelch_level },
	{ (char *) "KPSQL",     set_kpsql_on_off },        // Depreciated
	{ (char *) "KPSQLP",    reply_kpsql_pwr_level },   // Depreciated
	{ (char *) "KPSQLS",    set_kpsql_squelch_level }, // Depreciated
	{ (char *) "MODEM",     set_kiss_modem },
	{ (char *) "MODEMBW",   reply_active_modem_bw },
	{ (char *) "MODEML",    reply_modem_list },
	{ (char *) "RSIDBCAST", set_rsid_bc_mode },
	{ (char *) "RSIDM",     set_rsid_mode },
	{ (char *) "RSIDRX",    set_rsid_rx },
	{ (char *) "RSIDTX",    set_rsid_tx },
	{ (char *) "SQL",       set_sql_on_off },
	{ (char *) "SQLP",      reply_sql_pwr_level },
	{ (char *) "SQLS",      set_sql_level },
	{ (char *) "TNC",       reply_tnc_name },
	{ (char *) "TRXS",      reply_trx_state },
	{ (char *) "TRXSBCAST", set_trxs_bc_mode },
	{ (char *) "TXBEBCAST", set_txbe_bc_mode },
	{ (char *) "TXBUF",     reply_tx_buffer_count },
	{ (char *) "TXLOCK",    set_reply_tx_lock },
	{ (char *) "WFBW",      reply_waterfall_bw },
	{ (char *) "WFF",       set_wf_cursor_pos },
	{ (char *) 0,           0 }
};


#ifdef USE_NOCTRL

static std::string noctrl(string src);

static const char *asc[128] = {
	"<NUL>", "<SOH>", "<STX>", "<ETX>",
	"<EOT>", "<ENQ>", "<ACK>", "<BEL>",
	"<BS>",  "<TAB>", "\n",    "<VT>",
	"<FF>",  "",      "<SO>",  "<SI>",
	"<DLE>", "<DC1>", "<DC2>", "<DC3>",
	"<DC4>", "<NAK>", "<SYN>", "<ETB>",
	"<CAN>", "<EM>",  "<SUB>", "<ESC>",
	"<FS>",  "<GS>",  "<RS>",  "<US>",
	" ",     "!",     "\"",    "#",
	"$",     "%",     "&",     "\'",
	"(",     ")",     "*",     "+",
	",",     "-",     ".",     "/",
	"0",     "1",     "2",     "3",
	"4",     "5",     "6",     "7",
	"8",     "9",     ":",     ";",
	"<",     "=",     ">",     "?",
	"@",     "A",     "B",     "C",
	"D",     "E",     "F",     "G",
	"H",     "I",     "J",     "K",
	"L",     "M",     "N",     "O",
	"P",     "Q",     "R",     "S",
	"T",     "U",     "V",     "W",
	"X",     "Y",     "Z",     "[",
	"\\",    "]",     "^",     "_",
	"`",     "a",     "b",     "c",
	"d",     "e",     "f",     "g",
	"h",     "i",     "j",     "k",
	"l",     "m",     "n",     "o",
	"p",     "q",     "r",     "s",
	"t",     "u",     "v",     "w",
	"x",     "y",     "z",     "{",
	"|",     "}",     "~",     "<DEL>"
};

/**********************************************************************************
 *
 **********************************************************************************/
static string noctrl(string src)
{
	static string retstr;
	retstr.clear();
	char hexstr[10];
	int c;
	for (size_t i = 0; i < src.length(); i++)  {
		c = src[i];
		if ( c > 0 && c < 128)
			retstr.append(asc[c]);
		else {
			snprintf(hexstr, sizeof(hexstr), "<%0X>", c & 0xFF);
			retstr.append(hexstr);
		}
	}
	return retstr;
}
#endif // USE_NOCTRL

/**********************************************************************************
 * For SMACK CRC validation
 **********************************************************************************/
static int  calc_ccitt_crc(char *buf, int n)
{
	static int  crc_table[] = {
		0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
		0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
		0xcc01, 0xcc0,  0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
		0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
		0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
		0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
		0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
		0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
		0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
		0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
		0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
		0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
		0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
		0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
		0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
		0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
		0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
		0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
		0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
		0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
		0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
		0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
		0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
		0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
		0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
		0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
		0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
		0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
		0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
		0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
		0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
		0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
	};

	int crc;
	crc = 0;
	while (--n >= 0)
		crc = ((crc >> 8) & 0xff) ^ crc_table[(crc ^ *buf++) & 0xff];
	return crc;
}

/**********************************************************************************
 * For SMACK CRC validation (BPQ XOR CRC implimentation).
 **********************************************************************************/
static int  calc_xor_crc(char *buf, int n)
{
	int crc;
	crc = 0;
	while (--n >= 0)
		crc ^= (*buf++ & 0xff);
	return crc;
}

/**********************************************************************************
 * For FCS CRC.
 **********************************************************************************/
static int  calc_fcs_crc(char *buf, int n)
{

	static int fcstab[256] = {
		0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
		0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
		0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
		0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
		0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
		0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
		0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
		0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
		0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
		0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
		0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
		0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
		0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
		0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
		0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
		0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
		0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
		0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
		0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
		0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
		0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
		0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
		0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
		0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
		0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
		0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
		0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
		0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
		0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
		0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
		0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
		0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
	};

	int crc;
	crc = 0xFFFF;
	while (--n >= 0)
		crc = ((crc >> 8) & 0xff) ^ fcstab[(crc ^ *buf++) & 0xff];
	return crc;
}

/**********************************************************************************
 *
 **********************************************************************************/
static void set_button(Fl_Button * button, bool value)
{
	button->value(value);
	button->do_callback();
}


/**********************************************************************************
 *
 **********************************************************************************/
static void set_counter(Fl_Counter * counter, int value)
{
	counter->value(value);
	counter->do_callback();
}

/**********************************************************************************
 *
 **********************************************************************************/
static void set_slider2(Fl_Slider2 * sider, int value)
{
	sider->value(value);
	sider->do_callback();
}

/**********************************************************************************
 *
 **********************************************************************************/
bool valid_kiss_modem(std::string _modem)
{
	if(_modem.empty()) return false;

	int index = 0;
	int count = availabe_kiss_modems.size();
	std::string _tmp_str;

	if(count < 1) {
		for(index = 0; index < NUM_MODES; index++) {
			if(mode_info[index].iface_io & KISS_IO) {
				_tmp_str = uppercase_string(mode_info[index].sname);
				availabe_kiss_modems.push_back(_tmp_str);
			}
		}
		count = availabe_kiss_modems.size();
	}

	std::string cmp_str = "";
	index = 0;

	_modem = uppercase_string(_modem);

	while(index < count) {
		cmp_str = availabe_kiss_modems[index];
		if(cmp_str.empty()) return false;

		if(_modem.compare(cmp_str) == 0) {
			return true;
		}

		index++;
	}

	return false;
}

/**********************************************************************************
 *
 **********************************************************************************/
void check_kiss_modem(void)
{
	int mode = active_modem->get_mode();
	std::string modem_name;
	modem_name.assign(mode_info[mode].sname);
	bool valid = valid_kiss_modem(modem_name);

	if(!valid)
		set_default_kiss_modem();
}

/**********************************************************************************
 *
 **********************************************************************************/
static void set_default_kiss_modem(void)
{
	set_kiss_modem((char *) default_kiss_modem.c_str());
}

/**********************************************************************************
 *
 **********************************************************************************/
inline std::string uppercase_string(std::string str)
{
	int index = 0;
	int count = str.size();
	std::string ret_str = "";

	if(!count) return ret_str;

	ret_str.reserve(count + 1);
	ret_str.clear();

	for(index = 0; index < count; index++)
		ret_str += toupper(str[index]);

	return ret_str;
}

/**********************************************************************************
 * MODEM:<modem_id_string>
 **********************************************************************************/
static void set_kiss_modem(char * arg)
{
	if(!arg) return;

	std::string _modem = "";
	std::string _cmp_modem = "";

	_modem.assign(arg);

	if(_modem.empty()) {
		return reply_active_modem(arg);
	}

	bool valid = valid_kiss_modem(_modem);

	_modem = uppercase_string(_modem);

	if(valid) {
		for (size_t i = 0; i < NUM_MODES; i++) {
			_cmp_modem = uppercase_string(mode_info[i].sname);
			if (_modem == _cmp_modem) {
				REQ_SYNC(init_modem_sync, i, 0);
				kiss_modem.assign(_modem);
				break;
			}
		}
		return;
	}

	if(!valid)
		LOG_INFO("Modem %s invalid for KISS use. Must support 8bit.", _modem.c_str());

	return;
}

/**********************************************************************************
 * DISC:  Disconnect TCP/IP connection.
 **********************************************************************************/
static void kiss_tcp_disconnect(char *arg)
{
	if(kiss_socket && data_io_type == DATA_IO_TCP) {
		Fl::awake(kiss_main_thread_close, (void *) 0);
	}
}

/**********************************************************************************
 * Send a Disconnect message to the HOST called from the main thread only.
 **********************************************************************************/
static void send_disconnect_msg(void)
{
	if(kiss_socket && data_io_type == DATA_IO_TCP) {
		std::string package = "";
		std::string cmd = "DISC:";
		package.assign(cmd);
		kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
	}
}

/**********************************************************************************
 * FLSTAT:<INIT|OK>,<HH:MM:SS>
 **********************************************************************************/
static void reply_fldigi_stat(char *arg)
{
	std::string package = "";
	std::string cmd = "FLSTAT:";

	unsigned int hours = 0;
	unsigned int mins = 0;
	unsigned int secs = 0;

	time_t current_time = time(0);
	time_t diff_time = 0;

	char buffer[64];

	package.assign(cmd);

	if(program_started_flag) {
		package.append("OK");
	} else {
		package.append("INIT");
		program_started_flag = true;
	}

	if(program_start_time == 0)
		program_start_time = time(0);

	diff_time = current_time - program_start_time;

	hours = (unsigned int) (diff_time / 3600);
	diff_time -= (time_t) (hours * 3600);

	mins = (unsigned int)(diff_time / 60);
	diff_time -= (time_t) (mins * 60);

	secs = (unsigned int) diff_time;

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer)-1, ",%02u:%02u:%02u", hours, mins, secs);
	package.append(buffer);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);

}


/**********************************************************************************
 * CSMA:<ON|OFF>
 **********************************************************************************/
static void set_csma_mode(char * arg)
{
	if(!arg)
		return;

	std::string rsid_tx_state = "";
	rsid_tx_state.assign(arg);

	if(rsid_tx_state.empty())
		return reply_csma_mode(arg);

	std::string state = uppercase_string(rsid_tx_state);

	if(state.find("ON") != std::string::npos) {
		REQ(set_button, btnEnable_csma, 1);
		return;
	}

	if(state.find("OFF") != std::string::npos) {
		REQ(set_button, btnEnable_csma, 0);
		return;
	}
}

/**********************************************************************************
 * CSMA:<ON|OFF>
 **********************************************************************************/
static void reply_csma_mode(char *arg)
{
	std::string package = "";
	std::string cmd = "CSMA:";

	package.assign(cmd);

	if(progdefaults.csma_enabled)
		package.append("ON");
	else
		package.append("OFF");

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);

}

/**********************************************************************************
 * IBCHN:I       // Inhibit busy channel temporarily. 'I' (character)
 * IBCHN:<N>     // Set Inhibit busy channel duration to N seconds
 * IBCHN:0       // Resets temporary duration to default setting (5).
 * IBCHN:        // Returns IBCHN:<SECONDS>
 **********************************************************************************/
static void set_busy_channel_inhibit(char *arg)
{
	std::string argstr = "";
	time_t temp = 0;

	if(arg) {
		argstr.assign(arg);
		if(!argstr.empty()) {
			if(argstr[0] == 'I' || argstr[0] == 'i') {
				if(progdefaults.enableBusyChannel && inhibit_tx_seconds) {
					temp_disable_tx_inhibit = time(0) + temp_disable_tx_duration;
				}
			} else if(isdigit(argstr[0])) {
				sscanf(arg, "%lu", &temp);
				if(temp == 0)
					temp_disable_tx_duration = DISABLE_TX_INHIBIT_DURATION;
				else
					temp_disable_tx_duration = temp;
			}
			return;
		}
	}

	std::string cmd = "IBCHN:";
	std::string package = "";
	char buff[32];

	package.assign(cmd);

	memset(buff, 0, sizeof(buff));
	snprintf(buff, sizeof(buff)-1, "%lu", temp_disable_tx_duration);
	package.append(buff);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * KPSATT:<value>        // Set the fractional ratio gain value (1/value)
 **********************************************************************************/
static void set_psm_fraction_gain(char *arg)
{
	if(!arg) return;
	std::string args;

	args.assign(arg);

	if(args.empty())
		return reply_kpsql_fraction_gain(arg);

	unsigned int value = 0;

	sscanf(args.c_str(), "%u", &value);

	update_kpsql_fractional_gain(value);

	REQ(set_counter, cntKPSQLAttenuation, progdefaults.kpsql_attenuation);
}

/**********************************************************************************
 * KPSQLG:          // Return the fractional ratio gain value (1/value)
 **********************************************************************************/
static void reply_kpsql_fraction_gain(char *arg)
{
	std::string package = "";
	std::string cmd = "KPSATT:";
	char buffer[128];

	package.assign(cmd);
	memset(buffer, 0, sizeof(buffer));

	snprintf(buffer, sizeof(buffer)-1, "%u", progdefaults.kpsql_attenuation);
	package.append(buffer);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);

}

/**********************************************************************************
 * WFF:<integer value> Move TXRX cursor to Frequency.
 **********************************************************************************/
static void set_wf_cursor_pos(char * arg)
{
	if(!arg) return;

	std::string wf_cursor_pos = "";
	wf_cursor_pos.assign(arg);

	if(wf_cursor_pos.empty())
		return reply_wf_freq_pos(arg);

	int cursor = 0;
	int mode_bw = active_modem->get_bandwidth();

	sscanf(wf_cursor_pos.c_str(), "%d", &cursor);

	mode_bw >>= 1;

	if((cursor - mode_bw) < 0) cursor = mode_bw;
	if((cursor + mode_bw) >= IMAGE_WIDTH) cursor = IMAGE_WIDTH - mode_bw;

	active_modem->set_freq((double) cursor);

}

/**********************************************************************************
 * RSIDTX:<ON|OFF>
 **********************************************************************************/
static void set_rsid_tx(char * arg)
{
	if(!arg)
		return;

	std::string rsid_tx_state = "";
	rsid_tx_state.assign(arg);

	if(rsid_tx_state.empty())
		return reply_rsid_tx_state(arg);

	std::string state = uppercase_string(rsid_tx_state);

	if(state.find("ON") != std::string::npos) {
		REQ(set_button, btnTxRSID, 1);
		return;
	}

	if(state.find("OFF") != std::string::npos) {
		REQ(set_button, btnTxRSID, 0);
		return;
	}
}

/**********************************************************************************
 * RSIDRX:<ON|OFF>
 **********************************************************************************/
static void set_rsid_rx(char *arg)
{
	if(!arg)
		return;

	std::string rsid_rx_state = "";

	rsid_rx_state.assign(arg);

	if(rsid_rx_state.empty())
		return reply_rsid_rx_state(arg);

	std::string state = uppercase_string(rsid_rx_state);

	if(state.find("ON") != std::string::npos) {
		REQ(set_button, btnRSID, 1);
		return;
	}

	if(state.find("OFF") != std::string::npos) {
		REQ(set_button, btnRSID, 0);
		return;
	}
}

/**********************************************************************************
 * RSIDM:<BANDPASS|MODEM> <ACTIVE|NOTIFY>
 **********************************************************************************/
static void set_rsid_mode(char *arg)
{
	if(!arg)
		return;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_rsid_mode_state(arg);

	std::string state = uppercase_string(strarg);

	if(state.find("BANDPASS") != std::string::npos) {
		REQ(set_button, chkRSidWideSearch, 1);
	}

	if(state.find("MODEM") != std::string::npos) {
		REQ(set_button, chkRSidWideSearch, 0);
	}

	if(state.find("NOTIFY") != std::string::npos) {
		REQ(set_button, chkRSidNotifyOnly, 1);
	}

	if(state.find("ACTIVE") != std::string::npos) {
		REQ(set_button, chkRSidNotifyOnly, 0);
	}
}

/**********************************************************************************
 * RSIDBCAST:<ON|OFF>
 **********************************************************************************/
static void set_rsid_bc_mode(char * arg)
{
	if(!arg)
		return;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_rsid_bc_mode(arg);

	std::string state = uppercase_string(strarg);

	if(state.find("ON") != std::string::npos) {
		kiss_bcast_rsid_reception = true;
		return;
	}

	if(state.find("OFF") != std::string::npos) {
		kiss_bcast_rsid_reception = false;
		return;
	}
}

/**********************************************************************************
 * RSIDBCAST:<ON|OFF>
 **********************************************************************************/
static void reply_rsid_bc_mode(char * arg)
{
	std::string package = "";
	std::string cmd = "RSIDBCAST:";

	package.assign(cmd);

	if(kiss_bcast_rsid_reception)
		package.append("ON");
	else
		package.append("OFF");

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * TRXSBCAST:<ON|OFF>
 **********************************************************************************/
static void set_trxs_bc_mode(char * arg)
{
	if(!arg)
		return;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_trxs_bc_mode(arg);

	std::string state = uppercase_string(strarg);

	if(state.find("ON") != std::string::npos) {
		kiss_bcast_trx_toggle = true;
		return;
	}

	if(state.find("OFF") != std::string::npos) {
		kiss_bcast_trx_toggle = false;
		return;
	}
}

/**********************************************************************************
 * TRXSBCAST:<ON|OFF>
 **********************************************************************************/
static void reply_trxs_bc_mode(char * arg)
{
	std::string package = "";
	std::string cmd = "TRXSBCAST:";

	package.assign(cmd);

	if(kiss_bcast_trx_toggle)
		package.append("ON");
	else
		package.append("OFF");

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * SET TXBEBCAST:<ON|OFF>
 **********************************************************************************/
static void set_txbe_bc_mode(char * arg)
{
	if(!arg)
		return;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_txbe_bc_mode(arg);

	std::string state = uppercase_string(strarg);

	if(state.find("ON") != std::string::npos) {
		bcast_tx_buffer_empty_flag = true;
		return;
	}

	if(state.find("OFF") != std::string::npos) {
		bcast_tx_buffer_empty_flag = false;
		return;
	}
}

/**********************************************************************************
 * REPLY TXBEBCAST:<ON|OFF>
 **********************************************************************************/
static void reply_txbe_bc_mode(char * arg)
{
	std::string package = "";
	std::string cmd = "TXBEBCAST:";

	package.assign(cmd);

	if(bcast_tx_buffer_empty_flag)
		package.append("ON");
	else
		package.append("OFF");

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * TNC:      FLDIGI returns TNC:FLDIGI <Version Number>
 **********************************************************************************/
static void reply_tnc_name(char * arg)
{
	std::string package = "";
	std::string cmd = "TNC:";

	package.assign(cmd).append("FLDIGI ").append(PACKAGE_VERSION);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 *  TRXS:  FLDIGI returns TRXSQ:<TX|RX>
 **********************************************************************************/
static void reply_trx_state(char * arg)
{
	std::string package = "";
	std::string cmd = "TRXS:";

	package.assign(cmd);

	if((trx_state == STATE_TX) || (trx_state == STATE_TUNE))
		package.append("TX");
	else
		package.append("RX");

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 *  RSIDRX:  FLDIGI returns RSIDRXQ:<ON|OFF>
 **********************************************************************************/
static void reply_rsid_rx_state(char * arg)
{
	std::string package = "";
	std::string cmd = "RSIDRX:";

	package.assign(cmd);

	if(progdefaults.rsid)
		package.append("ON");
	else
		package.append("OFF");

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 *  RSIDTX:  FLDIGI returns RSIDTXQ:<ON|OFF>
 **********************************************************************************/
static void reply_rsid_tx_state(char * arg)
{
	std::string package = "";
	std::string cmd = "RSIDTX:";

	package.assign(cmd);

	if(progdefaults.TransmitRSid)
		package.append("ON");
	else
		package.append("OFF");

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 *  TXLOCK:  FLDIGI TXLOCK:<ON|OFF> or without arg return lock state.
 **********************************************************************************/
static void set_reply_tx_lock(char * arg)
{
	if(!arg) return;

	std::string strarg = "";
	std::string package = "";
	std::string cmd = "TXLOCK:";

	strarg.assign(arg);

	if(strarg.empty()) {
		package.assign(cmd);

		if (!active_modem)
			package.append("INOP");
		else if(active_modem->freqlocked())
			package.append("ON");
		else
			package.append("OFF");

		kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);

		return;
	}

	if (!active_modem) return;

	if(strarg.find("ON") != std::string::npos) {
		active_modem->set_freqlock(true);
		REQ(set_button, (Fl_Button *) wf->xmtlock, 1);
		return;
	}

	if(strarg.find("OFF") != std::string::npos) {
		active_modem->set_freqlock(false);
		REQ(set_button, (Fl_Button *) wf->xmtlock, 0);
		return;
	}
}

/**********************************************************************************
 * WFF:     FLDIGI returns WFFQ:<integer value> (0-4000) Current waterfall limit
 **********************************************************************************/
static void reply_wf_freq_pos(char * arg)
{
	std::string package = "";
	std::string cmd = "WFF:";
	char buff[32];

	package.assign(cmd);

	memset(buff, 0, sizeof(buff));
	snprintf(buff, sizeof(buff) - 1, "%d", (int) active_modem->get_txfreq());

	package.append(buff);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 *  RSIDM:   FLDIGI returns RSIDMQ:<BANDPASS|MODEM>,<ACTIVE|NOTIFY>
 **********************************************************************************/
static void reply_rsid_mode_state(char * arg)
{
	std::string package = "";
	std::string cmd = "RSIDM:";

	package.assign(cmd);

	if(progdefaults.rsidWideSearch)
		package.append("BANDPASS,");
	else
		package.append("MODEM,");

	if(progdefaults.rsid_notify_only)
		package.append("NOTIFY");
	else
		package.append("ACTIVE");

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * MODEM:   FLDIGI returns MODEMQ:<Modem ID String>   // Current Modem
 **********************************************************************************/
static void reply_active_modem(char * arg)
{
	std::string package = "";
	std::string cmd = "MODEM:";
	int mode = active_modem->get_mode();
	package.assign(cmd);
	package.append(mode_info[mode].sname);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * HOST: Sent from FLDIGI to instuct the HOST program to return it's name and
 * version number.
 **********************************************************************************/
static void host_name(char *arg)
{
	if(arg) {
		if(*arg) {
			host_name_string.assign(arg);
			LOG_INFO("%s", host_name_string.c_str());
		}
	}
}

/**********************************************************************************
 * MODEMBW: FLDIGI returns MODEMBWQ:<Bandwidth in Hz> // Current Modem Bandwidth
 **********************************************************************************/
static void reply_active_modem_bw(char * arg)
{
	std::string package = "";
	std::string cmd = "MODEMBW:";
	char buff[32];

	memset(buff, 0, sizeof(buff));
	snprintf(buff, sizeof(buff) - 1, "%d", (int) active_modem->get_bandwidth());

	package.assign(cmd).append(buff);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * WFBW:    FLDIGI returns WFBWQ:<LOWER HZ>,<UPPER HZ>
 **********************************************************************************/
static void reply_waterfall_bw(char * arg)
{
	std::string package = "";
	std::string cmd = "WFBW:";
	char buff[32];

	memset(buff, 0, sizeof(buff));
	snprintf(buff, sizeof(buff) - 1, "%d,%d", (int) progdefaults.LowFreqCutoff,
			 progdefaults.HighFreqCutoff );

	package.assign(cmd).append(buff);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * MODEML:  FLDIGI returns MODEML:Modem1,Modem2,...
 * A List of comma delimited modem ID strings.
 **********************************************************************************/
static void reply_modem_list(char * arg)
{
	int index = 0;
	int count = 0;
	std::string package = "";
	std::string cmd = "MODEML:";

	package.assign(cmd);

	count = availabe_kiss_modems.size();

	for(index = 0; index < count - 1; index++)
		package.append(availabe_kiss_modems[index]).append(",");
	package.append(availabe_kiss_modems[index]);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * TXBUF: FLDIGI returns TXBUFQ:<number_of_bytes_in_the_tx_buffer>
 **********************************************************************************/
static void reply_tx_buffer_count(char * arg)
{
	char *buffer = (char *)0;
	unsigned int buffer_size = 64;
	unsigned tx_buffer_count = 0;
	std::string package = "";
	std::string cmd = "TXBUF:";

	buffer = new char[buffer_size];

	if(!buffer) {
		LOG_DEBUG("%s", "Buffer allocation Error");
		return;
	}

	{
		guard_lock to_radio_lock(&to_radio_mutex);
		tx_buffer_count = to_radio.size();
	}

	memset(buffer, 0, buffer_size);
	snprintf(buffer, buffer_size - 1, "%u", tx_buffer_count);

	package.assign(cmd).append(buffer);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);

	if(buffer) delete [] buffer;

}

/**********************************************************************************
 * SQL:<ON|OFF>        // SQL On/Off
 **********************************************************************************/
static void set_sql_on_off(char * arg)
{
	if(!arg)
		return;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_sql_on_off(arg);

	if(strarg.find("ON") != std::string::npos) {
		REQ(set_button, btnSQL, 1);
		return;
	}

	if(strarg.find("OFF") != std::string::npos) {
		REQ(set_button, btnSQL, 0);
		return;
	}
}

/**********************************************************************************
 * SQL:      FLDIGI returns SQLQ:<ON|OFF>
 **********************************************************************************/
static void reply_sql_on_off(char * arg)
{
	std::string package = "";
	std::string cmd = "SQL:";

	package.assign(cmd);

	if(progStatus.sqlonoff)
		package.append("ON");
	else
		package.append("OFF");

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * SQLP:<0-100>        // Current Symbol Quality Level
 **********************************************************************************/
static void reply_sql_pwr_level(char * arg)
{
	std::string package = "";
	std::string cmd = "SQLP:";
	char buffer[64];

	package.assign(cmd);

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer)-1, "%u", (unsigned int) progStatus.squelch_value);
	package.append(buffer);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);

}

/**********************************************************************************
 * SQLS:<0-100> // Set SQL Level (percent)
 **********************************************************************************/
static void set_sql_level(char * arg)
{
	if(!arg)
		return;
	int value = 0;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_sql_level(arg);

	sscanf(strarg.c_str(), "%d", &value);
	if(value < 1) value = 1;
	if(value > 100) value = 100;

	progStatus.sldrSquelchValue = value;

	if(!progStatus.kpsql_enabled)
		REQ(set_slider2, sldrSquelch, value);
}

/**********************************************************************************
 * SQLS:     FLDIGI returns SQLSQ:<0-100>  // Set SQL Level Query (percent)
 **********************************************************************************/
static void reply_sql_level(char * arg)
{
	std::string package = "";
	std::string cmd = "SQLS:";
	char buffer[64];

	package.assign(cmd);

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer)-1, "%u", (unsigned int) progStatus.sldrSquelchValue);
	package.append(buffer);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * BCHN:<ON|OFF>       // Busy Channel On/Off
 **********************************************************************************/
static void set_busy_channel_on_off(char * arg)
{
	if(!arg)
		return;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_busy_channel_on_off(arg);

	if(strarg.find("ON") != std::string::npos) {
		REQ(set_button, btnEnableBusyChannel, 1);
		return;
	}

	if(strarg.find("OFF") != std::string::npos) {
		REQ(set_button, btnEnableBusyChannel, 0);
		return;
	}

}

/**********************************************************************************
 * BCHN:     FLDIGI returns BCHNQ:<ON|OFF>  // Busy Channel State On/Off Query
 **********************************************************************************/
static void reply_busy_channel_on_off(char * arg)
{
	std::string package = "";
	std::string cmd = "BCHN:";

	package.assign(cmd);

	if(progdefaults.enableBusyChannel)
		package.append("ON");
	else
		package.append("OFF");

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * BCHNS<0-999>        // Busy Channel Wait Duration (seconds)
 **********************************************************************************/
static void set_busy_channel_duration(char * arg)
{
	if(!arg)
		return;
	int value = 0;
	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_busy_channel_duration(arg);

	sscanf(strarg.c_str(), "%d", &value);
	if(value < 1) value = 1;

	REQ(set_counter, cntBusyChannelSeconds, value);
}

/**********************************************************************************
 * BCHNS:    FLDIGI returns BCHNSQ:<0-999> // Busy Channel Wait Duration Query (seconds)
 **********************************************************************************/
static void reply_busy_channel_duration(char * arg)
{
	std::string package = "";
	std::string cmd = "BCHNS:";
	char buffer[64];

	package.assign(cmd);

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer)-1, "%d", progdefaults.busyChannelSeconds);
	package.append(buffer);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);

}

/**********************************************************************************
 * BUSY:     FLDIGI returns BUSY:<T|F>  // Modem band pass signal presents
 **********************************************************************************/
static void reply_busy_state(char * arg)
{
	std::string package = "";
	std::string cmd = "BUSY:";

	package.assign(cmd);

	if((trx_state == STATE_TX) || \
	   (kpsql_pl > kpsql_threshold) || \
	   (inhibit_tx_seconds)) {
		package.append("T");
	} else {
		package.append("F");
	}

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * KPSQL:<ON|OFF> Depreciated
 **********************************************************************************/
static void set_kpsql_on_off(char * arg)
{
	if(!arg)
		return;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_kpsql_on_off(arg);

	if(strarg.find("ON") != std::string::npos) {
		REQ(set_button, btnPSQL, 1);
		return;
	}

	if(strarg.find("OFF") != std::string::npos) {
		REQ(set_button, btnPSQL, 0);
		return;
	}

}

/**********************************************************************************
 * PSM:<ON|OFF>
 **********************************************************************************/
static void set_psm_on_off(char * arg)
{
	if(!arg)
		return;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_psm_on_off(arg);

	if(strarg.find("ON") != std::string::npos) {
		REQ(set_button, btnPSQL, 1);
		return;
	}

	if(strarg.find("OFF") != std::string::npos) {
		REQ(set_button, btnPSQL, 0);
		return;
	}

}

/**********************************************************************************
 * KPSQL:<ON|OFF> Depreciated
 **********************************************************************************/
static void reply_kpsql_on_off(char * arg)
{
	std::string package = "";
	std::string cmd = "KPSQL:";

	package.assign(cmd);

	if(progStatus.kpsql_enabled)
		package.append("ON");
	else
		package.append("OFF");

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * PSM:<ON|OFF>
 **********************************************************************************/
static void reply_psm_on_off(char * arg)
{
	std::string package = "";
	std::string cmd = "PSM:";

	package.assign(cmd);

	if(progStatus.kpsql_enabled)
		package.append("ON");
	else
		package.append("OFF");

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * KPSQLP:<0-100> Depreciated
 **********************************************************************************/
static void reply_kpsql_pwr_level(char * arg)
{
	std::string package = "";
	std::string cmd = "KPSQLP:";
	char buffer[64];
	float plevel = 0;
	float scale = 100.0 / ((float) HISTO_COUNT);

	package.assign(cmd);

	if(kpsql_pl > (double) HISTO_COUNT) {
		plevel = HISTO_COUNT;
	} else {
		plevel = kpsql_pl;
	}

	plevel *= scale;

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer)-1, "%u", (unsigned int) plevel);
	package.append(buffer);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * PSMP:<0-100>
 **********************************************************************************/
static void reply_psm_pwr_level(char * arg)
{
	std::string package = "";
	std::string cmd = "PSMP:";
	char buffer[64];
	float plevel = 0;
	float scale = 100.0 / ((float) HISTO_COUNT);

	package.assign(cmd);

	if(kpsql_pl > (double) HISTO_COUNT) {
		plevel = HISTO_COUNT;
	} else {
		plevel = kpsql_pl;
	}

	plevel *= scale;

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer)-1, "%u", (unsigned int) plevel);
	package.append(buffer);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * PSMS:<0-100>
 **********************************************************************************/
static void set_psm_squelch_level(char * arg)
{
	if(!arg)
		return;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_psm_squelch_level(arg);

	int value = 0;

	sscanf(strarg.c_str(), "%d", &value);

	if(value < 1) value = 1;
	if(value > 100) value = 100;

	progStatus.sldrPwrSquelchValue = value;

	if(progStatus.kpsql_enabled)
		REQ(set_slider2, sldrSquelch, value);

}

/**********************************************************************************
 * KPSQLS:<0-100> Depreciated
 **********************************************************************************/
static void set_kpsql_squelch_level(char * arg)
{
	if(!arg)
		return;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_kpsql_squelch_level(arg);

	int value = 0;

	sscanf(strarg.c_str(), "%d", &value);

	if(value < 1) value = 1;
	if(value > 100) value = 100;

	progStatus.sldrPwrSquelchValue = value;

	if(progStatus.kpsql_enabled)
		REQ(set_slider2, sldrSquelch, value);

}

/**********************************************************************************
 * KPSQLS: Depreciated
 **********************************************************************************/
static void reply_kpsql_squelch_level(char * arg)
{
	std::string package = "";
	std::string cmd = "KPSQLS:";
	char buffer[64];

	package.assign(cmd);

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer)-1, "%u", (unsigned int) progStatus.sldrPwrSquelchValue);
	package.append(buffer);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * PSMS:
 **********************************************************************************/
static void reply_psm_squelch_level(char * arg)
{
	std::string package = "";
	std::string cmd = "PSMS:";
	char buffer[64];

	package.assign(cmd);

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer)-1, "%u", (unsigned int) progStatus.sldrPwrSquelchValue);
	package.append(buffer);

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * KISSRAW:<ON|OFF|ONLY>    // Enable RAW unaltered data over KISS
 **********************************************************************************/
static void set_kiss_raw_mode(char * arg)
{
	if(!arg)
		return;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_kiss_raw_mode(arg);

	// Longer compares first
	if(strarg.find("ONLY") != std::string::npos) {
		kiss_raw_enabled = KISS_RAW_ONLY;
		return;
	}

	if(strarg.find("ON") != std::string::npos) {
		kiss_raw_enabled = KISS_RAW_ON;
		return;
	}

	if(strarg.find("OFF") != std::string::npos) {
		kiss_raw_enabled = KISS_RAW_DISABLED;
		return;
	}

}

/**********************************************************************************
 * KISSRAW:<ON|OFF|ONLY>    // Enable RAW unaltered data over KISS
 **********************************************************************************/
static void reply_kiss_raw_mode(char * arg)
{
	std::string package = "";
	std::string cmd = "KISSRAW:";

	package.assign(cmd);

	switch(kiss_raw_enabled) {
		case KISS_RAW_ONLY:
			package.append("ONLY");
			break;

		case KISS_RAW_ON:
			package.append("ON");
			break;

		case KISS_RAW_DISABLED:
			package.append("OFF");
			break;

		default:
			return;
	}

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * KISSCRCM:<NONE|SMACK|CCITT|XOR|FCS>
 **********************************************************************************/
static void set_crc_mode(char * arg)
{
	if(!arg)
		return;

	std::string strarg = "";

	strarg.assign(arg);

	if(strarg.empty())
		return reply_crc_mode(arg);

	if(strarg.find("SMACK") != std::string::npos) {
		smack_crc_enabled = true;
		crc_mode = CRC16_CCITT;
		return;
	}

	if(strarg.find("CCITT") != std::string::npos) {
		smack_crc_enabled = true;
		crc_mode = CRC16_CCITT;
		return;
	}

	if(strarg.find("FCS") != std::string::npos) {
		smack_crc_enabled = true;
		crc_mode = CRC16_FCS;
		return;
	}

	if(strarg.find("XOR") != std::string::npos) {
		smack_crc_enabled = true;
		crc_mode = CRC8_XOR;
		return;
	}

	if(strarg.find("NONE") != std::string::npos) {
		smack_crc_enabled = false;
		return;
	}

}

/**********************************************************************************
 * KISSCRCM:<NONE|SMACK>,<NONE|CCITT|XOR|FCS>
 **********************************************************************************/
static void reply_crc_mode(char *arg)
{
	std::string package = "";
	std::string cmd = "KISSCRCM:";

	package.assign(cmd);

	if(smack_crc_enabled)
		package.append("SMACK,");
	else
		package.append("NONE,");

	switch(crc_mode) {
		case CRC16_NONE:
			package.append("NONE");
			break;

		case CRC16_CCITT:
			package.append("CCITT");
			break;

		case CRC16_FCS:
			package.append("FCS");
			break;

		case CRC8_XOR:
			package.append("XOR");
			break;

		default:
			package.append("UNDEFINED");

	}

	kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
}

/**********************************************************************************
 * RSIDN:NEW_WF_OFFSET,NEW_MODEM,OLD_WF_OFFSET,OLD_MODEM,<ACTIVE|NOTIFY>
 **********************************************************************************/
bool bcast_rsid_kiss_frame(int new_wf_pos, int new_mode, int old_wf_pos, int old_mode, int notify)
{
	guard_lock kiss_bc_frame_lock(&kiss_bc_frame_mutex);

	char buffer[256];
	char old_modem_name[64];
	char new_modem_name[64];
	char *notify_str = (char *) "";
	KISS_QUEUE_FRAME *frame = (KISS_QUEUE_FRAME *)0;
	std::string package = "";

	if(new_mode >= NUM_MODES || old_mode >= NUM_MODES)
		return false;

	if(new_mode < 0 || old_mode < 0)
		return false;

	if(!(mode_info[new_mode].iface_io & KISS_IO))
		return false;

	if(old_mode != new_mode || new_wf_pos != old_wf_pos) {
		psm_reset_histogram();
	}

	if(!kiss_bcast_rsid_reception) return true;

	if(new_wf_pos == 0) {
		new_wf_pos = old_wf_pos;
		notify = RSID_KISS_USER;
	}

	switch(notify) {
		case RSID_KISS_NOTIFY:
			notify_str = (char *) "NOTIFY";
			break;

		case RSID_KISS_ACTIVE:
			notify_str = (char *) "ACTIVE";
			break;

		case RSID_KISS_USER:
			notify_str = (char *) "USER";
			break;

		default:
			LOG_DEBUG("%s", "Unknown KISS frame RSID BC Source");
			return false;
	}

	// Send all modem names in capital letters

	memset(old_modem_name, 0, sizeof(old_modem_name));
	strncpy(old_modem_name, mode_info[old_mode].sname, sizeof(old_modem_name) - 1);

	for(size_t i = 0; i < sizeof(old_modem_name); i++) {
		if(old_modem_name[i])
			old_modem_name[i] = toupper(old_modem_name[i]);
		else
			break;
	}

	memset(new_modem_name, 0, sizeof(new_modem_name));
	strncpy(new_modem_name, mode_info[new_mode].sname, sizeof(new_modem_name) - 1);

	for(size_t i = 0; i < sizeof(new_modem_name); i++) {
		if(new_modem_name[i])
			new_modem_name[i] = toupper(new_modem_name[i]);
		else
			break;
	}

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer)-1, "RSIDN:%d,%s,%d,%s,%s", new_wf_pos, new_modem_name, \
			 old_wf_pos, old_modem_name, notify_str);

	package.assign(buffer);

	frame = encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no);

	if(!frame) {
		LOG_DEBUG("%s", "Broadcast Hardware Frame Assembly Failure");
		return true;
	}

	kiss_bc_frame.append((const char *) frame->data, (size_t) frame->size);

	if(frame->data)	delete [] frame->data;
	if(frame) delete frame;

	return true;
}

/**********************************************************************************
 *  TRXS:<RX|TX> // Transmit to HOST during a state change between RX/TX or TX/RX.
 **********************************************************************************/
void bcast_trxs_kiss_frame(int state)
{
	guard_lock kiss_bc_frame_lock(&kiss_bc_frame_mutex);

	KISS_QUEUE_FRAME *frame = (KISS_QUEUE_FRAME *)0;
	std::string package;

	package.assign("TRXS:");

	switch(state) {
		case STATE_RX:
			package.append("RX") ;
			break;

		case STATE_TUNE:
		case STATE_TX:
			package.append("TX") ;
			break;

		default:
			LOG_DEBUG("%s", "Unknown Transmit State");
			return;

	}

	frame = encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no);

	if(!frame) {
		LOG_DEBUG("%s", "Broadcast Hardware Frame Assembly Failure");
		return;
	}

	kiss_bc_frame.append((const char *) frame->data, (size_t) frame->size);

	if(frame->data)	delete [] frame->data;
	if(frame) delete frame;
}

/**********************************************************************************
 * TXBE: // Broadcast empty transmit buffer state
 **********************************************************************************/
void bcast_tx_buffer_empty_kiss_frame(void)
{
	if(!bcast_tx_buffer_empty_flag) return;

	guard_lock kiss_bc_frame_lock(&kiss_bc_frame_mutex);

	KISS_QUEUE_FRAME *frame = (KISS_QUEUE_FRAME *)0;
	std::string package;

	package.assign("TXBE:");

	frame = encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no);

	if(!frame) {
		LOG_DEBUG("%s", "Broadcast Hardware Frame Assembly Failure");
		return;
	}

	kiss_bc_frame.append((const char *) frame->data, (size_t) frame->size);

	if(frame->data)	delete [] frame->data;
	if(frame) delete frame;
}

/**********************************************************************************
 *
 **********************************************************************************/
static void exec_hardware_command(std::string cmd, std::string arg)
{
	if(cmd.empty()) return;
	if(kiss_reset_flag) return;
	int pos = 0;
	int a = 0;
	int b = 0;
	int comp_size = 0;
	int index = 0;
	int count = sizeof(exec_match) / sizeof(EXEC_HARDWARE_CMD_MATCH);
	string cmp = "";

	for(index = 0; index < count; index++) {
		if(exec_match[index].cmd == (char *)0) return;
		cmp.assign(exec_match[index].cmd);
		if((pos = cmp.find(cmd)) != (int)(string::npos)) {
			a = cmp.size();
			b = cmd.size();

			if(a > b)
				comp_size = a;
			else
				comp_size = b;

			if(cmd.compare(pos, comp_size, cmp) == 0) {
				if(exec_match[index].cmd_func)
					(*exec_match[index].cmd_func)((char *) arg.c_str());
				return;
			}
		}
	}
}

/**********************************************************************************
 *
 **********************************************************************************/
static bool kiss_queue_frame(KISS_QUEUE_FRAME * frame, std::string cmd)
{
	if(!frame) {
		LOG_DEBUG("Null frame (%s)", cmd.c_str());
		return false;
	}

	if(frame->size == 0 || frame->data == (char *)0) {
		LOG_DEBUG("Frame null content (%s)", cmd.c_str());
		if(frame->data) delete[] frame->data;
		delete frame;
		return false;
	}

	WriteToHostBuffered((const char *) frame->data, (size_t) frame->size);

	delete[] frame->data;
	delete frame;

	return true;
}

/**********************************************************************************
 *
 **********************************************************************************/
size_t kiss_encode(char *src, size_t src_size, char **dst)
{
	if(!src || !dst || src_size < 1) return 0;

	size_t index = 0;
	int count = 0;
	int buffer_size = 0;
	int byte = 0;
	char *buffer = (char *)0;

	buffer_size = (src_size * KISS_BUFFER_FACTOR) + BUFFER_PADDING;
	buffer = new char[buffer_size];

	if(!buffer) {
		LOG_DEBUG("Memory allocation error near line %d", __LINE__);
		*dst = (char *)0;
		return 0;
	}

	memset(buffer, 0, buffer_size);
	count = 0;
	buffer[count++] = KISS_FEND;

	for(index = 0; index < src_size; index++) {
		byte = (int) src[index] & 0xFF;
		switch(byte) {
			case KISS_FESC:
				buffer[count++] = KISS_FESC;
				buffer[count++] = KISS_TFESC;
				break;

			case KISS_FEND:
				buffer[count++] = KISS_FESC;
				buffer[count++] = KISS_TFEND;
				break;

			default:
				buffer[count++] = byte;
		}
	}

	buffer[count++] = KISS_FEND;
	*dst = (char *) buffer;

	return count;
}

/**********************************************************************************
 *
 **********************************************************************************/
size_t kiss_decode(char *src, size_t src_size, char **dst)
{
	if(!src || !dst || src_size < 1) return 0;

	size_t index = 0;
	int count = 0;
	int buffer_size = 0;
	int byte = 0;
	int last_byte = 0;
	char *buffer = (char *)0;

	buffer_size = src_size + BUFFER_PADDING;
	buffer = new char[buffer_size];

	if(!buffer) {
		LOG_DEBUG("Memory allocation error near line %d", __LINE__);
		*dst = (char *)0;
		return 0;
	}

	memset(buffer, 0, buffer_size);
	count = 0;
	last_byte = KISS_INVALID;

	for(index = 0; index < src_size; index++) {

		byte = src[index] & 0xFF;

		switch(byte) {
			case KISS_FEND:
				continue;

			case KISS_FESC:
				break;

			case KISS_TFEND:
				if(last_byte == KISS_FESC)
					buffer[count++] = KISS_FEND;
				else
					buffer[count++] = byte;
				break;

			case KISS_TFESC:
				if(last_byte == KISS_FESC)
					buffer[count++] = KISS_FESC;
				else
					buffer[count++] = byte;
				break;

			default:
				buffer[count++] = byte;
		}

		last_byte = byte;
	}

	*dst = (char *) buffer;
	return count;
}

#if 0
/**********************************************************************************
 *
 **********************************************************************************/
std::string kiss_decode(std::string frame)
{
	int count = 0;
	int frame_size = 0;
	char *dst = (char *)0;

	static std::string ret_str = "";

	if(frame.empty()) return frame;

	frame_size = frame.size();

	ret_str.clear();
	ret_str.reserve(frame_size + BUFFER_PADDING);

	count = kiss_decode((char *) frame.c_str(), frame.size(), &dst);

	if(count && dst) {
		ret_str.assign(dst, count);
		dst[0] = 0;
		delete [] dst;
	}

	return ret_str;
}
#endif // 0

/**********************************************************************************
 *
 **********************************************************************************/
std::string kiss_encode(std::string frame)
{
	int count = 0;
	int frame_size = 0;
	char *dst = (char *)0;

	static std::string ret_str = "";

	if(frame.empty()) return frame;

	frame_size = frame.size();

	ret_str.clear();
	ret_str.reserve((frame_size * 2) + BUFFER_PADDING);

	count = kiss_encode((char *) frame.c_str(), frame.size(), &dst);

	if(count && dst) {
		ret_str.assign(dst, count);
		dst[0] = 0;
		delete [] dst;
	}

	return ret_str;
}


/**********************************************************************************
 *
 **********************************************************************************/
size_t hdlc_encode(char *src, size_t src_size, char **dst)
{
	if(!src || !dst || src_size < 1) return 0;

	size_t index = 0;
	int count = 0;
	int buffer_size = 0;
	int byte = 0;
	char *buffer = (char *)0;

	buffer_size = (src_size * HDLC_BUFFER_FACTOR) + BUFFER_PADDING;
	buffer = new char[buffer_size];

	if(!buffer) {
		LOG_DEBUG("Memory allocation error near line %d", __LINE__);
		*dst = (char *)0;
		return 0;
	}

	memset(buffer, 0, buffer_size);
	count = 0;
	buffer[count++] = ' ';
	buffer[count++] = KISS_FEND;

	for(index = 0; index < src_size; index++) {
		byte = (int) src[index] & 0xFF;

		if(not_allowed[byte]) {
			buffer[count++] = HDLC_CNT;
			if((byte + HDLC_CNT_OFFSET) > 255)
				buffer[count++] = ((byte - HDLC_CNT_OFFSET) & 0xFF);
			else
				buffer[count++] = ((byte + HDLC_CNT_OFFSET) & 0xFF);
			continue;
		}

		switch(byte) {
			case KISS_FESC:
				buffer[count++] = KISS_FESC;
				buffer[count++] = KISS_TFESC;
				break;

			case KISS_FEND:
				buffer[count++] = KISS_FESC;
				buffer[count++] = KISS_TFEND;
				break;

			case HDLC_CNT:
				buffer[count++] = KISS_FESC;
				buffer[count++] = HDLC_TCNT;
				break;

			default:
				buffer[count++] = byte;
		}
	}

	buffer[count++] = KISS_FEND;
	buffer[count++] = ' ';

	*dst = (char *) buffer;

	return count;
}

/**********************************************************************************
 *
 **********************************************************************************/
size_t hdlc_decode(char *src, size_t src_size, char **dst)
{
	if(!src || !dst || src_size < 1) return 0;

	size_t index = 0;
	int count = 0;
	int buffer_size = 0;
	int byte = 0;
	int last_byte = 0;
	int check_byte = 0;
	char *buffer = (char *)0;

	buffer_size = src_size + BUFFER_PADDING;
	buffer = new char[buffer_size];

	if(!buffer) {
		LOG_DEBUG("Memory allocation error near line %d", __LINE__);
		*dst = (char *)0;
		return 0;
	}

	memset(buffer, 0, buffer_size);
	count = 0;
	last_byte = KISS_INVALID;

	for(index = 0; index < src_size; index++) {

		byte = src[index] & 0xFF;

		if(last_byte == HDLC_CNT) {
			check_byte = byte - HDLC_CNT_OFFSET;
			if((check_byte > -1) && (check_byte < 256) && not_allowed[check_byte]) {
				buffer[count++] = check_byte;
				last_byte = byte;
				continue;
			}

			check_byte = byte + HDLC_CNT_OFFSET;
			if((check_byte > -1) && (check_byte < 256) && not_allowed[check_byte]) {
				buffer[count++] = check_byte;
			}

			last_byte = byte;
			continue;
		}

		switch(byte) {
			case KISS_FEND:
				continue;

			case KISS_FESC:
			case HDLC_CNT:
				last_byte = byte;
				continue;

			case KISS_TFEND:
				if(last_byte == KISS_FESC)
					byte = KISS_FEND;
				break;

			case KISS_TFESC:
				if(last_byte == KISS_FESC)
					byte = KISS_FESC;
				break;

			case HDLC_TCNT:
				if(last_byte == KISS_FESC)
					byte = HDLC_CNT;
				break;
		}

		last_byte = buffer[count++] = byte;
	}

	*dst = (char *) buffer;
	return count;

}

/**********************************************************************************
 *  Buffer must be at least twice the size of the data provided + BUFFER_PADDING
 *  data_count: Number of bytes in the buffer to be converted.
 *  Return is the converted byte count. Buffer is overwritten with converted data.
 **********************************************************************************/
static size_t encap_hdlc_frame(char *buffer, size_t data_count)
{
	if(!buffer || !data_count) {
		LOG_DEBUG("%s", "Parameter Data Error [NULL]");
		return false;
	}

	size_t count = 0;
	unsigned int crc_value = 0;
	char *kiss_encap = (char *)0;

	if(progdefaults.ax25_decode_enabled) {
		ax25_decode((unsigned char *) buffer, data_count, true, true);
	}

	crc_value = calc_fcs_crc(buffer, (int) data_count);

	buffer[data_count++] = CRC_LOW(crc_value);
	buffer[data_count++] = CRC_HIGH(crc_value);

	count = hdlc_encode(buffer, data_count, &kiss_encap);

	if(kiss_encap && count) {
		memcpy(buffer, kiss_encap, count);
#ifdef EXTENED_DEBUG_INFO
		LOG_HEX(buffer, count);
#endif
		delete [] kiss_encap;
	} else {
		LOG_DEBUG("%s", "Kiss Encode Memory Allocation Error");
		return 0;
	}

	return count;
}

/*********************************************************************************
 * Buffer is presently larger then what will be returned.
 * data_count: Number of bytes in the buffer to process.
 * Returns the converted byte count. Buffer is over written with converted data.
 *********************************************************************************/
static size_t decap_hdlc_frame(char *buffer, size_t data_count)
{
	if(!buffer || !data_count) {
		LOG_DEBUG("%s", "Parameter Data Error/NULL");
		return false;
	}

	size_t count = 0;
	//	size_t index = 0;
	unsigned int crc_value = 0;
	unsigned int calc_crc_value = 0;
	char *kiss_decap = (char *)0;

	count = hdlc_decode(buffer, data_count, &kiss_decap);

#ifdef EXTENED_DEBUG_INFO
	if(data_count && buffer)
		LOG_HEX(buffer, data_count);

	if(count && kiss_decap)
		LOG_HEX(kiss_decap, count);
#endif

	do  {

		if(count > data_count || !kiss_decap) {
			LOG_DEBUG("%s", "Kiss decode error");
			count = 0;
			break;
		}

		if(count > 2)
			count -= 2;

		if(count) {
			calc_crc_value = calc_fcs_crc(kiss_decap, (int) count);
		}
		else {
			LOG_DEBUG("%s", "Kiss decode error");
			count = 0;
			break;
		}

		crc_value = CRC_LOW_HIGH(kiss_decap[count], kiss_decap[count + 1]);

		if(crc_value != calc_crc_value) {
			count = 0;
			break;
		}

		temp_disable_tx_inhibit = time(0) + DISABLE_TX_INHIBIT_DURATION; // valid packet, disable busy channel inhitbit for x duration.

		kiss_decap[count] = kiss_decap[count + 1] = 0;
		memcpy(buffer, kiss_decap, count);

		if(progdefaults.ax25_decode_enabled) {
			ax25_decode((unsigned char *) buffer, count, true, false);
		}

		break;
	} while(1);

	if(kiss_decap) {
		kiss_decap[0] = 0;
		delete [] kiss_decap;
	}

	return count;
}


/**********************************************************************************
 *
 **********************************************************************************/
static KISS_QUEUE_FRAME *encap_kiss_frame(char *buffer, size_t buffer_size, int frame_type, int port)
{

	guard_lock kfenc(&kiss_encode_mutex);

	if(!buffer || buffer_size < 1) {
		LOG_DEBUG("%s", "KISS encap argument 'data' contains no data");
		return (KISS_QUEUE_FRAME *)0;
	}

	if(port > 0xF || port < 0) {
		LOG_DEBUG("Invalid KISS port number (%d)", port);
		return (KISS_QUEUE_FRAME *)0;
	}

	switch(frame_type) {
		case KISS_DATA:
		case KISS_RAW:
		case KISS_TXDELAY:
		case KISS_PERSIST:
		case KISS_SLOTTIME:
		case KISS_TXTAIL:
		case KISS_DUPLEX:
		case KISS_HARDWARE:
			break;

		default:
			LOG_DEBUG("Invalid KISS frame type (%d)", frame_type);
			return (KISS_QUEUE_FRAME *)0;
	}

	int size = 0;
	int index = 0;
	unsigned int crc_value = 0;
	KISS_QUEUE_FRAME *frame = (KISS_QUEUE_FRAME *)0;

	frame = new KISS_QUEUE_FRAME;

	if(!frame) {
		LOG_DEBUG("%s", "KISS struct frame memory allocation error");
		return (KISS_QUEUE_FRAME *)0;
	}

	size = (buffer_size * KISS_BUFFER_FACTOR) + BUFFER_PADDING; // Resulting data space could be 2 fold higher.
	frame->data = (char *) new char[size];

	if(!frame->data) {
		delete frame;
		LOG_DEBUG("%s", "KISS buffer frame memory allocation error");
		return (KISS_QUEUE_FRAME *)0;
	}

	memset(frame->data, 0, size);
	size = buffer_size;

	frame->data[0] = SET_KISS_TYPE_PORT(frame_type, port);
	memcpy(&frame->data[1], buffer, size);

	size++;

	if((frame_type == KISS_DATA) || (frame_type == KISS_RAW)) {

		if(smack_crc_enabled) {

			frame->data[0] = SMACK_CRC_ASSIGN(frame->data[0]);

			switch(crc_mode) {
				case CRC16_CCITT:
					crc_value = calc_ccitt_crc((char *) frame->data, size);
					frame->data[size++] = CRC_LOW(crc_value);
					frame->data[size++] = CRC_HIGH(crc_value);
					break;

				case CRC16_FCS:
					crc_value = calc_fcs_crc((char *) frame->data, size);
					frame->data[size++] = CRC_LOW(crc_value);
					frame->data[size++] = CRC_HIGH(crc_value);
					break;

				case CRC8_XOR:
					crc_value = calc_xor_crc((char *) frame->data, size);
					frame->data[size++] = CRC_LOW(crc_value);
					break;

				default:
					break;
			}
		}
	}

	char *tmp = (char *)0;

	index = kiss_encode((char *) frame->data, size, &tmp);

	if(tmp) {

#ifdef EXTENED_DEBUG_INFO
		LOG_HEX(tmp, index);
#endif

		frame->data[0] = 0;
		delete [] frame->data;
		frame->data = (char *) tmp;
		frame->size = index;
	} else {
		LOG_DEBUG("KISS encode allocation error near line %d", __LINE__);
		delete [] frame->data;
		delete frame;
		frame = (KISS_QUEUE_FRAME *)0;
	}

	return frame;
}

/**********************************************************************************
 *
 **********************************************************************************/
static KISS_QUEUE_FRAME * encap_kiss_frame(std::string data, int frame_type, int port)
{
	if(data.empty()) return (KISS_QUEUE_FRAME *)0;
	return encap_kiss_frame((char *) data.c_str(), (size_t) data.size(), frame_type, port);
}

/**********************************************************************************
 *
 **********************************************************************************/
std::string unencap_kiss_frame(char *buffer, size_t buffer_size, int *frame_type, int *kiss_port_no)
{
	if(!buffer || buffer_size < 1 || !frame_type || !kiss_port_no)
		return std::string("");

	char *decoded_buffer = (char *)0;
	size_t count = 0;
	unsigned int crc_extracted = 0;
	unsigned int crc_calc = 0;
	unsigned int port = 0;
	unsigned int ftype = 0;

	static std::string ret_str = "";

	ret_str.clear();

#ifdef EXTENED_DEBUG_INFO
	LOG_HEX(buffer, buffer_size);
#endif

	count = kiss_decode(buffer, buffer_size, &decoded_buffer);

	if(!count || !decoded_buffer) {
		LOG_DEBUG("Kiss decoder memory allocation error near line %d", __LINE__);
		return ret_str;
	}

	ftype = KISS_CMD(decoded_buffer[0]);
	port = KISS_PORT(decoded_buffer[0]);

	if((ftype == KISS_DATA) || (ftype == KISS_RAW)) {
		smack_crc_enabled = SMACK_CRC(port);
		port = SMACK_CRC_MASK(port);

		if(smack_crc_enabled) {
			switch(crc_mode) {
				case CRC16_CCITT:
					count -= 2;
					if(count > 2) {
						crc_calc = calc_ccitt_crc(decoded_buffer, count);
						crc_extracted = CRC_LOW_HIGH(decoded_buffer[count], decoded_buffer[count + 1]);
					} else {
						crc_calc = crc_extracted + 1; // Force a fail
					}
					break;

				case CRC16_FCS:
					count -= 2;
					if(count > 2) {
						crc_calc = calc_fcs_crc(decoded_buffer, count);
						crc_extracted = CRC_LOW_HIGH(decoded_buffer[count], decoded_buffer[count + 1]);
					} else {
						crc_calc = crc_extracted + 1;
					}
					break;

				case CRC8_XOR:
					count -= 1;
					if(count > 1) {
						crc_calc = CRC_LOW(calc_fcs_crc(decoded_buffer, count));
						crc_extracted = CRC_LOW(decoded_buffer[count]);
					} else {
						crc_calc = crc_extracted + 1;
					}
					break;

				default:
					LOG_DEBUG("CRC type not found %d", crc_mode);
			}

			if(crc_calc != crc_extracted) {

				if(frame_type) *frame_type = ftype;
				if(kiss_port_no) *kiss_port_no = port;
				if(decoded_buffer) delete [] decoded_buffer;

				ret_str.clear();
				return ret_str;
			}
		}
	}

	if(count > 0)
		count--;

#ifdef EXTENED_DEBUG_INFO
	LOG_HEX(&decoded_buffer[1], count);
#endif

	ret_str.assign(&decoded_buffer[1], count);

	if(frame_type) *frame_type = ftype;
	if(kiss_port_no) *kiss_port_no = port;
	if(decoded_buffer) delete [] decoded_buffer;

	return ret_str;
}

/**********************************************************************************
 *
 **********************************************************************************/
std::string unencap_kiss_frame(std::string package, int *frame_type, int *kiss_port_no)
{
	if(package.empty() || !frame_type || !kiss_port_no)
		return std::string("");

	return unencap_kiss_frame((char *) package.c_str(), (size_t) package.size(), frame_type, kiss_port_no);
}

/**********************************************************************************
 *
 **********************************************************************************/
static void parse_hardware_frame(std::string frame)
{
	if(frame.empty()) return;

	string cmd = "";
	string arg = "";
	static char buffer[512];
	string parse_frame = "";
	char bofmsg[] = "Temp Buffer overflow";
	size_t count = frame.size();
	size_t index = 0;
	size_t pos = 0;
	size_t j = 0;

	parse_frame.assign(frame);

#ifdef EXTENED_DEBUG_INFO
	LOG_HEX(frame.c_str(), frame.size());
#endif

	do {
		if(kiss_reset_flag) return;

		pos = parse_frame.find(":");

		if(pos == string::npos) return;

		j = 0;
		memset(buffer, 0, sizeof(buffer));
		for(index = 0; index < pos; index++) {
			if(parse_frame[index] <= ' ') continue;
			buffer[j++] = toupper(parse_frame[index]);
			if(j >= sizeof(buffer)) {
				LOG_DEBUG("%s", bofmsg);
				return;
			}
		}
		cmd.assign(buffer);

		j = 0;
		memset(buffer, 0, sizeof(buffer));
		for(index = pos + 1; index < count; index++) {
			if(parse_frame[index] <= ' ') break;
			buffer[j++] = parse_frame[index];
			if(j >= sizeof(buffer)) {
				LOG_DEBUG("%s", bofmsg);
				return;
			}
		}
		arg.assign(buffer);

		if(cmd.empty()) return;

		exec_hardware_command(cmd, arg);

		if(index > count)
			index = count;

		parse_frame.erase(0, index);
		count = parse_frame.size();

	} while(count > 0);

}

/**********************************************************************************
 *
 **********************************************************************************/
static void parse_kiss_frame(std::string frame_segment)
{
	guard_lock kiss_rx_lock(&kiss_frame_mutex);

	unsigned int cur_byte    = KISS_INVALID;
	unsigned int frame_size  = 0;
	unsigned int index       = 0;
	unsigned int fend_count  = 0;
	unsigned int cmsa_data   = 0;
	int buffer_size = 0;
	int port_no     = KISS_INVALID;
	int frame_type  = KISS_INVALID;
	int data_count = 0;

	bool process_one_frame = false;
	char *buffer = (char *)0;

	kiss_frame.append(frame_segment);

	while(1) {

		if(kiss_frame.empty()) return;

		frame_size = kiss_frame.size();
		process_one_frame = false;
		fend_count = 0;
		kiss_one_frame.clear();

		for(index = 0; index < frame_size; index++) {
			cur_byte = kiss_frame[index] & 0xFF;

			if(cur_byte == KISS_FEND) {
				fend_count++;
			}

			if(fend_count) {
				kiss_one_frame += cur_byte;
			}

			if(fend_count == 2) {
				kiss_frame.erase(0, index);
				process_one_frame = true;
				break;
			}
		}

		if(!process_one_frame)
			return;

		frame_size = kiss_one_frame.size();

		if(frame_size < 3) {
			continue; // Invalid Frame size
		}

		kiss_one_frame = unencap_kiss_frame(kiss_one_frame, &frame_type, &port_no);

		if(kiss_one_frame.empty())
			continue;

		if(port_no != (int)kiss_port_no) {
			continue;
		}

		switch(frame_type) {
			case KISS_TXDELAY:
			case KISS_PERSIST:
			case KISS_SLOTTIME:
			case KISS_TXTAIL:
			case KISS_DUPLEX:
				cmsa_data = kiss_one_frame[0] & 0xFF;
				break;

			case KISS_DATA:
				if(kiss_raw_enabled == KISS_RAW_ONLY)
					continue;
				break;

			case KISS_RAW:
				if(kiss_raw_enabled == KISS_RAW_DISABLED)
					continue;
				break;

			case KISS_HARDWARE:
				break;

			default:
				continue; // Unreconized frame_type.

		}

		switch(frame_type) {
			case KISS_DATA:
				buffer_size = (frame_size * HDLC_BUFFER_FACTOR) + BUFFER_PADDING;
				buffer = new char[buffer_size];

				if(!buffer) {
					LOG_DEBUG("%s", "Buffer Allocation Error");
					return;
				}
				data_count = kiss_one_frame.size();
				memset(buffer, 0, buffer_size);
				memcpy(buffer, kiss_one_frame.c_str(), data_count);

				data_count = encap_hdlc_frame(buffer, data_count);

				WriteToRadioBuffered((const char *) buffer, (size_t) data_count);

				buffer[0] = 0;
				delete [] buffer;
				buffer = 0;

				break;

			case KISS_RAW:
				WriteToRadioBuffered((const char *) kiss_one_frame.c_str(), (size_t) kiss_one_frame.size());
				break;

			case KISS_TXDELAY:
				progStatus.csma_transmit_delay   = cmsa_data;
				progdefaults.csma_transmit_delay = cmsa_data;
				REQ(update_csma_io_config, (int) CSMA_TX_DELAY);
				break;

			case KISS_PERSIST:
				progStatus.csma_persistance   = cmsa_data;
				progdefaults.csma_persistance = cmsa_data;
				REQ(update_csma_io_config, (int) CSMA_PERSISTANCE);
				break;

			case KISS_SLOTTIME:
				progStatus.csma_slot_time   = cmsa_data;
				progdefaults.csma_slot_time = cmsa_data;
				REQ(update_csma_io_config, (int) CSMA_SLOT_TIME);
				break;

			case KISS_TXTAIL:
				break;

			case KISS_DUPLEX:
				if(cmsa_data)
					duplex = KISS_FULL_DUPLEX;
				else
					duplex = KISS_HALF_DUPLEX;
				break;

			case KISS_HARDWARE:
				parse_hardware_frame(kiss_one_frame);
				break;
		}
	} // while(1)
}

/**********************************************************************************
 *
 **********************************************************************************/
static void WriteToHostBuffered(const char *data, size_t size)
{
	guard_lock to_host_lock(&to_host_mutex);
	to_host.append(data, size);
}

/**********************************************************************************
 *
 **********************************************************************************/
static void WriteToRadioBuffered(const char *data, size_t size)
{
	guard_lock to_radio_lock(&to_radio_mutex);
	if(!data || size < 1) return;
	set_tx_timeout();
	to_radio.append(data, size);
}

/**********************************************************************************
 * Must be call in WriteToRadioBuffered() and no other.
 **********************************************************************************/
inline void set_tx_timeout(void)
{
	if(to_radio.empty()) {
		transmit_buffer_flush_timeout = time(0) + TX_BUFFER_TIMEOUT;
	}
}

/**********************************************************************************
 *
 **********************************************************************************/
void flush_kiss_tx_buffer(void)
{
	int data_count = 0;

	{
		guard_lock to_host_lock(&to_radio_mutex);
		kiss_text_available = false;
		pText = 0;
		data_count = to_radio.size();

		if(data_count)
			to_radio.clear();
	}

	if(data_count)
		bcast_tx_buffer_empty_kiss_frame();
}

/**********************************************************************************
 *
 **********************************************************************************/
static void WriteToHostARQBuffered(void)
{
	std::string arq_data = "";

	{
		guard_lock to_host_arq_lock(&to_host_arq_mutex);
		int data_available = to_arq_host.size();

		if(kiss_raw_enabled == KISS_RAW_DISABLED) {
			if(data_available) {
				to_arq_host.clear();
			}
			return;
		}

		if(data_available < 1) return;

#ifdef EXTENED_DEBUG_INFO
		LOG_HEX(to_arq_host.c_str(), to_arq_host.size());
#endif

		arq_data.assign(to_arq_host);
		to_arq_host.clear();
	}

	kiss_queue_frame(encap_kiss_frame(arq_data, KISS_RAW, kiss_port_no), string("ARQ"));

}

/**********************************************************************************
 *
 **********************************************************************************/
static void *ReadFromHostSocket(void *args)
{
	if(!kiss_socket) return (void *)0;

	static char buffer[2048];
	string str_buffer;
	size_t count = 0;
	Socket *tmp_socket = (Socket *)0;
	memset(buffer, 0, sizeof(buffer));
	str_buffer.reserve(sizeof(buffer));

	if(progStatus.kiss_tcp_io && progStatus.kiss_tcp_listen) {
		tmp_socket = kiss_socket->accept2();
		kiss_socket->shut_down();
		kiss_socket->close();

		if(tmp_socket)
			kiss_socket = tmp_socket;
		tmp_socket = 0;
	}

	LOG_INFO("%s", "Kiss RX loop started. ");
	kiss_rx_exit = false;
	kiss_rx_loop_running = true;

	while(!kiss_rx_exit) {

		memset(buffer, 0, sizeof(buffer));

		try {
			if(progStatus.kiss_tcp_io)
				count = kiss_socket->recv((void *) buffer, sizeof(buffer) - 1);
			else
				count = kiss_socket->recvFrom((void *) buffer, sizeof(buffer) - 1);
		} catch (...) {
			if (errno)
				LOG_INFO("recv/recvFrom Socket Error %d", errno);
			count = 0;

			if(!tcpip_reset_flag) {
				kiss_tcp_disconnect((char *)"");
			}
			break;
		}

		if(count && (data_io_enabled == KISS_IO)) {
#ifdef EXTENED_DEBUG_INFO
			LOG_HEX(buffer, count);
#endif
			guard_lock from_host_lock(&from_host_mutex);
			from_host.append(buffer, count);
		}
	}

	LOG_INFO("%s", "Kiss RX loop exit. ");

	kiss_rx_loop_running = false;
	return (void *)0;
}

extern Fl_Slider2 *sldrSquelch;
extern Progress	*pgrsSquelch;

/**********************************************************************************
 *
 **********************************************************************************/
static void *kiss_loop(void *args)
{
	SET_THREAD_ID(KISS_TID);

	int old_trx_state = STATE_TX;

	LOG_INFO("%s", "Kiss loop started. ");

	kiss_loop_running = true;

	while(!kiss_exit){
		MilliSleep(100);

		if(data_io_enabled != KISS_IO) {
			kiss_text_available = false;
			kiss_reset_buffers();
			continue;
		}

		if(old_trx_state != trx_state) {
			if(kiss_bcast_trx_toggle) {
				switch(trx_state) {
					case STATE_TX:
					case STATE_RX:
					case STATE_TUNE:
						bcast_trxs_kiss_frame(trx_state);
					default :
						break;
				}
			}

			old_trx_state = trx_state;
		}

		ReadFromHostBuffered();
		ReadFromRadioBuffered();
		WriteToHostBCastFramesBuffered();
		WriteToHostARQBuffered();
		WriteToHostSocket();

		if(!to_radio.empty()) {
			kiss_text_available = true;
			active_modem->set_stopflag(false);
			trx_transmit();
		}
	}

	kiss_loop_running = false;

	// exit the kiss thread
	return NULL;
}


/**********************************************************************************
 *
 **********************************************************************************/
void WriteKISS(const char data)
{
	if (active_modem->get_mode() == MODE_FSQ) return;

	if(kiss_reset_flag) return;

	{
		guard_lock from_radio_lock(&from_radio_mutex);
		from_radio += data;
	}

	if(kiss_raw_enabled != KISS_RAW_DISABLED) {
		guard_lock to_host_arq_lock(&to_host_arq_mutex);
		to_arq_host += data;
	}
}

/**********************************************************************************
 *
 **********************************************************************************/
void WriteKISS(const char *data)
{
	if (active_modem->get_mode() == MODE_FSQ) return;

	if(kiss_reset_flag) return;

	{
		guard_lock from_radio_lock(&from_radio_mutex);
		if(data)
			from_radio.append(data);
	}

	{
		guard_lock to_host_arq_lock(&to_host_arq_mutex);
		if(data && (kiss_raw_enabled != KISS_RAW_DISABLED))
			to_arq_host.append(data);
	}
}

/**********************************************************************************
 *
 **********************************************************************************/
void WriteKISS(const char *data, size_t size)
{
	if (active_modem->get_mode() == MODE_FSQ) return;

	if(kiss_reset_flag) return;

	{
		guard_lock from_radio_lock(&from_radio_mutex);
		if(data && size) {
			from_radio.append(data, size);
		}
	}

	{
		guard_lock to_host_arq_lock(&to_host_arq_mutex);
		if(data && size && (kiss_raw_enabled != KISS_RAW_DISABLED)) {
			to_arq_host.append(data, size);
		}
	}
}

/**********************************************************************************
 *
 **********************************************************************************/
void WriteKISS(std::string data)
{
	if (active_modem->get_mode() == MODE_FSQ) return;

	if(kiss_reset_flag) return;

	{
		guard_lock from_radio_lock(&from_radio_mutex);
		if(!data.empty()) {
			from_radio.append(data);
		}
	}

	{
		guard_lock to_host_arq_lock(&to_host_arq_mutex);
		if(!data.empty() && (kiss_raw_enabled != KISS_RAW_DISABLED)) {
			to_arq_host.append(data);
		}
	}
}

/**********************************************************************************
 *
 **********************************************************************************/
void WriteToHostSocket(void)
{
	guard_lock to_host_lock(&to_host_mutex);
	size_t count = 0;

	if(to_host.empty()) return;

	if(kiss_socket && data_io_enabled == KISS_IO) {
		try {
			if(progStatus.kiss_tcp_io)
				count = kiss_socket->send(to_host.c_str(), to_host.size());
			else
				count = kiss_socket->sendTo(to_host.c_str(), to_host.size());
#ifdef EXTENED_DEBUG_INFO
			LOG_HEX(to_host.c_str(), to_host.size());
#endif
		} catch (...) {
			if(kiss_reset_flag == false) {
				kiss_tcp_disconnect((char *)"");
			}
			LOG_INFO("Write error error to KISS socket: %d", static_cast<int>(count));
		}
	}

	to_host.clear();
}

/**********************************************************************************
 *
 **********************************************************************************/
static void ReadFromHostBuffered(void)
{
	if(!kiss_socket) return;

	guard_lock from_host_lock(&from_host_mutex);
	if(from_host.empty()) return;
#ifdef EXTENED_DEBUG_INFO
	LOG_HEX(from_host.c_str(), from_host.size());
#endif
	parse_kiss_frame(from_host);
	from_host.clear();
}

/**********************************************************************************
 *
 **********************************************************************************/
void ReadFromRadioBuffered(void)
{
	if(kiss_reset_flag) return;

	guard_lock from_radio_lock(&from_radio_mutex);
	if(from_radio.empty()) return;

	int pos = 0;
	int pos2 = 0;
	//	unsigned int crc = 0;
	KISS_QUEUE_FRAME *frame = (KISS_QUEUE_FRAME *)0;
	static char frame_marker[2] = { (char)(KISS_FEND), 0 };
	std::string one_frame = "";

	pos = from_radio.find(frame_marker);

	if(pos == (int)(std::string::npos)) {
		from_radio.clear();
		return;
	}

	if(pos != 0) {
		from_radio.erase(0, pos);
		pos = 0;
	}

	pos2 = from_radio.find(frame_marker, pos + 1);
	if(pos2 != (int)(std::string::npos)) {
		one_frame.assign(from_radio, pos, pos2 - pos + 1);
	} else {
		if(from_radio.size() > MAX_TEMP_BUFFER_SIZE)
			from_radio.clear();
		return;
	}

	char *buffer = (char *)0;
	size_t buffer_size = one_frame.size() + BUFFER_PADDING;

	buffer = new char [buffer_size];

	if(!buffer) {
		LOG_DEBUG("Memory Allocation Error Near Line No. %d", __LINE__);
		goto EXIT;
	}

	memset(buffer, 0, buffer_size);

	buffer_size = one_frame.size();
	memcpy(buffer, one_frame.c_str(), buffer_size);

#ifdef EXTENED_DEBUG_INFO
	LOG_HEX(buffer, buffer_size);
#endif

	buffer_size = decap_hdlc_frame(buffer, buffer_size);

	if(buffer_size) {
		from_radio.erase(pos, pos2 - pos + 1);

		if(kiss_raw_enabled != KISS_RAW_ONLY) {
			frame = encap_kiss_frame(buffer, buffer_size, KISS_DATA, kiss_port_no);

			if(!frame || !frame->data) {
				LOG_DEBUG("Frame Allocation Error Near Line %d", __LINE__);
				goto EXIT;
			}

			WriteToHostBuffered((const char *) frame->data, (size_t) frame->size);
		}
	} else {
		from_radio.erase(pos, pos + 1);
	}

EXIT:;

	if(frame) {
		if(frame->data) {
			frame->data[0] = 0;
			delete [] frame->data;
		}
		delete frame;
	}

	if(buffer) {
		buffer[0] = 0;
		delete [] buffer;
	}
}

/**********************************************************************************
 *
 **********************************************************************************/
void WriteToHostBCastFramesBuffered(void)
{
	guard_lock kiss_bc_frame_lock(&kiss_bc_frame_mutex);
	if(kiss_bc_frame.empty()) return;
	WriteToHostBuffered((const char *) kiss_bc_frame.c_str(), (size_t) kiss_bc_frame.size());

#ifdef EXTENED_DEBUG_INFO
	LOG_HEX(kiss_bc_frame.c_str(), (size_t) kiss_bc_frame.size());
#endif

	kiss_bc_frame.clear();
}

/**********************************************************************************
 *
 **********************************************************************************/
bool tcp_init(bool connect_flag)
{
	if(progdefaults.kiss_address.empty() || progdefaults.kiss_io_port.empty()) {
		LOG_DEBUG("%s", "KISS IP Address or Port null");
		return false;
	}

	kiss_ip_address.assign(progdefaults.kiss_address);
	kiss_ip_io_port.assign(progdefaults.kiss_io_port);
	kiss_ip_out_port.assign(progdefaults.kiss_out_port);

	try {
		kiss_socket = new Socket(Address(kiss_ip_address.c_str(), kiss_ip_io_port.c_str(), "tcp"));
		kiss_socket->set_autoclose(true);
		kiss_socket->set_nonblocking(false);

		if(progdefaults.kiss_tcp_listen)
			kiss_socket->bind();
	}
	catch (const SocketException& e) {
		LOG_ERROR("Could not resolve %s: %s", kiss_ip_address.c_str(), e.what());
		if(kiss_socket) {
			kiss_socket->shut_down();
			kiss_socket->close();
			delete kiss_socket;
			kiss_socket = 0;
			kiss_enabled = 0;
		}
		return false;
	}

	if(connect_flag) {
		if(kiss_socket->connect1() == false) {
			LOG_INFO("Connection Failed: Host program present?");
			kiss_socket->shut_down();
			kiss_socket->close();
			delete kiss_socket;
			kiss_socket = 0;
			kiss_enabled = 0;
			return false;
		}
	}

	return true;
}

/**********************************************************************************
 *
 **********************************************************************************/
bool udp_init(void)
{
	if(progdefaults.kiss_address.empty() || progdefaults.kiss_io_port.empty()) {
		LOG_DEBUG("%s", "KISS IP Address or Port null");
		return false;
	}

	kiss_ip_address.assign(progdefaults.kiss_address);
	kiss_ip_io_port.assign(progdefaults.kiss_io_port);
	kiss_ip_out_port.assign(progdefaults.kiss_out_port);

	try {
		kiss_socket = new Socket(Address(kiss_ip_address.c_str(), kiss_ip_io_port.c_str(), "udp"));
		kiss_socket->dual_port(&progdefaults.kiss_dual_port_enabled);
		kiss_socket->set_dual_port_number(kiss_ip_out_port);
		kiss_socket->set_autoclose(true);
		kiss_socket->set_nonblocking(false);

		if(progdefaults.kiss_tcp_listen) // Listen flag indcates server mode.
			kiss_socket->bindUDP();

	} catch (const SocketException& e) {
		LOG_ERROR("Could not resolve %s: %s", kiss_ip_address.c_str(), e.what());
		if(kiss_socket) {
			kiss_socket->shut_down();
			kiss_socket->close();
			delete kiss_socket;
			kiss_socket = 0;
			kiss_enabled = 0;
		}
		return false;
	}

	return true;
}

/**********************************************************************************
 *
 **********************************************************************************/
void kiss_reset_buffers(void)
{
	{
		guard_lock to_host_lock(&to_host_mutex);
		if(!to_host.empty())
			to_host.clear();
	}

	{
		guard_lock to_host_lock(&from_radio_mutex);
		if(!from_radio.empty())
			from_radio.clear();
	}

	{
		guard_lock to_host_lock(&to_radio_mutex);
		pText = 0;
		kiss_text_available = false;
		if(!to_radio.empty())
			to_radio.clear();
	}

	{
		guard_lock kiss_bc_frame_lock(&kiss_bc_frame_mutex);
		if(!kiss_bc_frame.empty())
			kiss_bc_frame.clear();
	}
}

/**********************************************************************************
 *
 **********************************************************************************/
void kiss_reset(void)
{
	kiss_reset_flag     = true;
	kiss_text_available = false;
	duplex              = KISS_HALF_DUPLEX;
	crc_mode            = CRC16_NONE;
	smack_crc_enabled   = false;
	pText = 0;

	if(data_io_enabled == KISS_IO)
		data_io_enabled = DISABLED_IO;

	MilliSleep(1000);

	kiss_reset_buffers();

	if (trx_state == STATE_TX || trx_state == STATE_TUNE) {
		REQ(abort_tx);
	}

	kiss_reset_flag = false;

	if(data_io_enabled == DISABLED_IO)
		data_io_enabled = KISS_IO;
}

/**********************************************************************************
 *
 **********************************************************************************/
void kiss_init(bool connect_flag)
{
	kiss_enabled     = false;
	kiss_exit        = false;
	tcpip_reset_flag = false;

	// progStatus.data_io_enabled (widget state), data_io_enabled (program state)

	if(progStatus.data_io_enabled == KISS_IO) {
		if(!(active_modem->iface_io() & KISS_IO)) {
			set_default_kiss_modem();
		}
	}

	if(init_hist_flag) {
		memset(histogram, 0, sizeof(histogram));
		init_hist_flag = false;
	}

	srand(time(0)); // For CSMA persistance
	update_kpsql_fractional_gain(progdefaults.kpsql_attenuation);

	data_io_type = DATA_IO_NA;

	if(progStatus.kiss_tcp_io) {

		if(retry_count > KISS_CONNECT_RETRY_COUNT)
			retry_count = KISS_CONNECT_RETRY_COUNT;

		if(progStatus.kiss_tcp_listen)
			connect_flag = false;

		do {
			if(tcp_init(connect_flag)) break;

			if(progStatus.kiss_tcp_listen) return;

			MilliSleep(KISS_RETRY_WAIT_TIME);

			if(retry_count-- > 0) continue;
			else return;

		} while(1);

		LOG_INFO("%s", "TCP Init - OK");
	} else {
		if(!udp_init()) return;
		LOG_INFO("%s", "UDP Init - OK");
	}

	Fl::awake(kiss_io_set_button_state, (void *) IO_START_STR);

	kiss_loop_running = false;
	if (pthread_create(&kiss_thread, NULL, kiss_loop, NULL) < 0) {
		LOG_ERROR("KISS kiss_thread: pthread_create failed");
		return;
	}

	kiss_rx_loop_running = false;
	if (pthread_create(&kiss_rx_socket_thread, NULL, ReadFromHostSocket, NULL) < 0) {
		LOG_ERROR("KISS kiss_rx_socket_thread: pthread_create failed");
		kiss_exit = true;
		pthread_join(kiss_thread, NULL);
		return;
	}

	if(progStatus.kiss_tcp_io) {
		data_io_type = DATA_IO_TCP;
		kiss_watchdog_running = false;
		if (pthread_create(&kiss_watchdog_thread, NULL, tcpip_watchdog, NULL) < 0) {
			LOG_ERROR("KISS kiss_watchdog_thread: pthread_create failed");
		}
	} else {
		data_io_type = DATA_IO_UDP;
	}

	Fl::awake(kiss_io_set_button_state, (void *) IO_STOP_STR);

	if(progdefaults.data_io_enabled == KISS_IO)
		data_io_enabled = KISS_IO;

	kiss_enabled = true;
	allow_kiss_socket_io = true;
}

/**********************************************************************************
 *
 **********************************************************************************/
static void *tcpip_watchdog(void *args)
{
	kiss_watchdog_running = true;
	kiss_watchdog_exit = false;
	struct timespec timeout;
	struct timeval    tp;
	std::string package = "";
	std::string cmd = "HOST:";

	package.assign(cmd);

	memset(&timeout, 0, sizeof(timeout));
	LOG_INFO("%s", "TCP/IP watch dog started");

	kiss_watchdog_running = true;

	while(!kiss_watchdog_exit) {
		gettimeofday(&tp, NULL);
		
		timeout.tv_sec  = tp.tv_sec + TEST_INTERVAL_SECONDS;
		timeout.tv_nsec = tp.tv_usec * 1000;
		
		pthread_mutex_lock(&kiss_loop_exit_mutex);
		pthread_cond_timedwait(&kiss_watchdog_cond, &kiss_loop_exit_mutex, &timeout);
		pthread_mutex_unlock(&kiss_loop_exit_mutex);
		
		// Send somthing every once in awhile to check if other side is still connected.
		// Data transfer fail handled by the data transfer routines.
		if(kiss_socket && !tcpip_reset_flag)
			if(kiss_socket->is_connected())
				kiss_queue_frame(encap_kiss_frame(package, KISS_HARDWARE, kiss_port_no), cmd);
	}
	
	kiss_watchdog_running = false;
	LOG_INFO("%s", "TCP/IP watch dog stopped");
	return (void *)0;
}

/**********************************************************************************
 *
 **********************************************************************************/
void kiss_main_thread_close(void *ptr)
{
	kiss_close(true);
}

/**********************************************************************************
 * Only called from the main thread.
 **********************************************************************************/
void kiss_close(bool override_flag)
{
	int max_loops = 100;
	int loop_delay_ms = 10;
	
	if(tcpip_reset_flag && !override_flag) return;
	
	if(kiss_socket) {
		if(kiss_socket->is_connected()) {
			kiss_reset_buffers();
			send_disconnect_msg();
			MilliSleep(500); // Wait a short period for outstanding
							 // messages to be sent.
		}
	} else {
		return;
	}
	
	tcpip_reset_flag = true;
	kiss_text_available = false;
	allow_kiss_socket_io = false;
	
	if(data_io_type == DATA_IO_TCP && kiss_watchdog_running) {
		kiss_watchdog_exit = true;
		pthread_cond_signal(&kiss_watchdog_cond);
		
		for(int i = 0; i < max_loops; i++) {
			if(kiss_watchdog_running == false) break;
			MilliSleep(loop_delay_ms);
		}
		
		if(!kiss_watchdog_running) {
			pthread_join(kiss_watchdog_thread, NULL);
			LOG_INFO("%s", "kiss_watchdog_running - join");
		} else {
			CANCEL_THREAD(kiss_watchdog_thread);
			LOG_INFO("%s", "kiss_watchdog_running - cancel");
		}
	}
	
	if(data_io_enabled == KISS_IO) {
		data_io_enabled = DISABLED_IO;
		data_io_type = DATA_IO_NA;
	}
	
	kiss_rx_exit = true;
	
	if(kiss_socket) {
		kiss_socket->shut_down();
		kiss_socket->close();
	}
	
	for(int i = 0; i < max_loops; i++) {
		if(kiss_rx_loop_running == false) break;
		MilliSleep(loop_delay_ms);
	}
	
	if(!kiss_rx_loop_running) {
		pthread_join(kiss_rx_socket_thread, NULL);
		LOG_INFO("%s", "kiss_rx_loop_running - join");
	} else {
		CANCEL_THREAD(kiss_rx_socket_thread);
		LOG_INFO("%s", "kiss_rx_loop_running - cancel");
	}
	
	kiss_exit = 1;
	
	for(int i = 0; i < max_loops; i++) {
		if(kiss_loop_running == false) break;
		MilliSleep(loop_delay_ms);
	}
	
	if(!kiss_loop_running) {
		pthread_join(kiss_thread, NULL);
		LOG_INFO("%s", "kiss_loop_running - join");
	} else {
		CANCEL_THREAD(kiss_thread);
		LOG_INFO("%s", "kiss_loop_running - cancel");
	}
	
	LOG_INFO("%s", "Kiss loop terminated. ");
	
	kiss_socket = 0;
	kiss_enabled = false;
	kiss_loop_running = false;
	kiss_rx_loop_running = false;
	kiss_watchdog_running = false;
	
	Fl::awake(kiss_io_set_button_state, (void *) IO_START_STR);
	
	tcpip_reset_flag = false;
	
	if((retry_count > 0) && progStatus.kiss_tcp_io)
		Fl::awake(kiss_main_thread_retry_open, (void *) 0);
}

/**********************************************************************************
 *
 **********************************************************************************/
void kiss_main_thread_retry_open(void *ptr)
{
	if(!progStatus.kiss_tcp_listen)
		MilliSleep(KISS_RETRY_WAIT_TIME);
	connect_to_kiss_io(false);
}

/**********************************************************************************
 *
 **********************************************************************************/
void connect_to_kiss_io(bool user_requested)
{
	guard_lock external_lock(&restart_mutex);
	
	if(kiss_socket) {
		if(user_requested)
			retry_count = 0;
		
		kiss_close(true);
		
		if(user_requested)
			return;
	}
	
	if(user_requested || progStatus.kiss_tcp_listen)
		retry_count = KISS_CONNECT_RETRY_COUNT;
	
	if(retry_count > 0) {
		retry_count--;
		kiss_init(progdefaults.kiss_tcp_listen ? false : true);
	}
}

/**********************************************************************************
 *
 **********************************************************************************/
bool kiss_thread_running(void)
{
	return (bool) kiss_enabled;
}

/**********************************************************************************
 *
 **********************************************************************************/
int kiss_get_char(void)
{
	/// Mutex is unlocked when returning from function
	guard_lock to_radio_lock(&to_radio_mutex);
	int c = 0;
	static bool toggle_flag = 0;
	
	if (kiss_text_available) {
		if (pText != (int)to_radio.length()) {
			c = to_radio[pText++] & 0xFF;
			toggle_flag = true;
		} else {
			kiss_text_available = false;
			to_radio.clear();
			pText = 0;
			c = GET_TX_CHAR_ETX;
			
			if(toggle_flag) {
				bcast_tx_buffer_empty_kiss_frame();
				toggle_flag = 0;
			}
		}
	}
	
	return c;
}
