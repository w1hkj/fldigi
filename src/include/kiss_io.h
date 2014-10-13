#ifndef KISSIO_H
#define KISSIO_H

#include <vector>

#include "threads.h"
#include "socket.h"
#include "modem.h"

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

#define MAX_TEMP_BUFFER_SIZE 32000

#define TX_BUFFER_TIMEOUT (60 * 10) // Ten minute timeout
//#define TX_BUFFER_TIMEOUT (20) // Ten minute timeout
#define DISABLE_TX_INHIBIT_DURATION 5

#define KPSQL_MIN_BANDWIDTH 400

typedef struct {
	char *data;
	size_t size;
} KISS_QUEUE_FRAME;

typedef struct {
	char *cmd;
	void (*cmd_func) (char *arg);
} EXEC_HARDWARE_CMD_MATCH;

#define AX25_FRAME_MARKER 0xFF
//#undef KISS_RX_THREAD
#define KISS_RX_THREAD

extern void AbortKiss();

bool bcast_rsid_kiss_frame(int new_wf_pos, int new_mode, int old_wf_pos, int old_mode, int notify);
inline std::string uppercase_string(std::string str);
static double detect_signal(int freq, int bw, double *low, double *high);
static bool kiss_queue_frame(KISS_QUEUE_FRAME * frame, std::string cmd);
static bool valid_kiss_modem(std::string _modem);
static KISS_QUEUE_FRAME *encap_kiss_frame(char *buffer, size_t size, int frame_type, int kiss_port_no);
static KISS_QUEUE_FRAME *encap_kiss_frame(std::string data, int kiss_frame_type, int port);
static KISS_QUEUE_FRAME *encap_kiss_frame(std::string package, int frame_type, int kiss_port_no);
static size_t decap_hdlc_frame(char *buffer, size_t data_count);
static size_t encap_hdlc_frame(char *buffer, size_t data_count);
static size_t kiss_decode(char *src, size_t src_size, char **dst);
static size_t kiss_encode(char *src, size_t src_size, char **dst);
//static std::string kiss_decode(std::string frame);
//static std::string kiss_encode(std::string frame);
static std::string unencap_kiss_frame(std::string package, int *frame_type, int *kiss_port_no);
static void exec_hardware_command(std::string cmd, std::string arg);
static void flush_tx_buffer(void);
static void parse_hardware_frame(std::string frame);
static void parse_kiss_frame(std::string frame_segment);
static void reply_active_modem_bw(char *);
static void reply_active_modem(char *);
static void reply_busy_channel_duration(char *);
static void reply_busy_channel_on_off(char *);
static void reply_crc_mode(char *);
static void reply_csma_mode(char *arg);
static void reply_fldigi_stat(char *arg);
static void reply_kiss_raw_mode(char *);
static void reply_kpsql_fraction_gain(char *arg);
static void reply_kpsql_on_off(char *);
static void reply_kpsql_pwr_level(char *);
static void reply_kpsql_squelch_level(char *);
static void reply_modem_list(char *);
static void reply_rsid_bc_mode(char *);
static void reply_rsid_mode_state(char *);
static void reply_rsid_rx_state(char *);
static void reply_rsid_tx_state(char *);
static void reply_sql_level(char *);
static void reply_sql_on_off(char *);
static void reply_sql_pwr_level(char *);
static void reply_tnc_name(char *);
static void reply_trx_state(char *);
static void reply_trxs_bc_mode(char *);
static void reply_tx_buffer_count(char *);
static void reply_txbe_bc_mode(char *);
static void reply_waterfall_bw(char *);
static void reply_wf_freq_pos(char *);
static void reset_histogram(void);
static void set_busy_channel_duration(char *);
static void set_busy_channel_on_off(char *);
static void set_busy_channel_inhibit(char *arg);
static void set_button(Fl_Button* button, bool value);
static void set_crc_mode(char *);
static void set_csma_mode(char * arg);
static void set_default_kiss_modem(void);
static void set_kiss_modem(char *);
static void set_kiss_raw_mode(char *);
static void set_kpsql_fraction_gain(char *arg);
static void set_kpsql_on_off(char *);
static void set_kpsql_squelch_level(char *);
static void set_rsid_bc_mode(char *);
static void set_rsid_mode(char *);
static void set_rsid_rx(char *);
static void set_rsid_tx(char *);
static void set_sql_level(char *);
static void set_sql_on_off(char *);
static void set_trxs_bc_mode(char *);
static void set_txbe_bc_mode(char *);
static void set_wf_cursor_pos(char *);
static void TransmitCSMA();
static void WriteToHostBuffered(const char *data, size_t size);
//static void WriteToHostBuffered(const char *data);
//static void WriteToHostBuffered(const char data);
inline void set_tx_timeout(void);
static void WriteToHostSocket(void);
static void WriteToRadioBuffered(const char *data, size_t size);
//static void WriteToRadioBuffered(const char *data);
//static void WriteToRadioBuffered(unsigned char data);
void bcast_trxs_kiss_frame(int state);
void bcast_tx_buffer_empty_kiss_frame(void);
void kiss_reset_buffers(void);
void kiss_reset(void);
void ReadFromRadioBuffered(void);
void WriteToHostBCastFramesBuffered(void);
static void ReadFromHostBuffered(void);
void ReadFromHostSocket(void);
//static void set_busy_state_bc_mode(char * arg);
static void reply_busy_state(char * arg);

#ifdef KISS_RX_THREAD
static void *ReadFromHostSocket(void *args);
#else
static void ReadFromHostSocket();
#endif

#endif // KISSIO_H
