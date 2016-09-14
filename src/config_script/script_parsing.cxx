// ----------------------------------------------------------------------------
// Copyright (C) 2015
//              Robert Stiles
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <string.h>
#include "config.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

#include <FL/fl_utf8.h>
#include "debug.h"
#include "gettext.h"
#include "script_parsing.h"
#include "run_script.h"

// Do not use directly. It's copied for each instance

static const SCRIPT_COMMANDS default_operator_command_table[] = {
	{ CMD_CALLSIGN,                SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_callsign_info, 0, 0},
	{ CMD_NAME,                    SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_name_info,     0, 0},
	{ CMD_QTH,                     SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_qth_info,      0, 0},
	{ CMD_LOCATOR,                 SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_locator_info,  0, 0},
	{ CMD_ANTENNA,                 SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_antenna_info,  0, 0},
	{ CMD_END_CMD,                 SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_audio_device_command_table[] = {
	{ CMD_OSS_AUDIO,               SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_use_oss_audio_device,    0, 0},
	{ CMD_OSS_AUDIO_DEV_PATH,      SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_oss_audio_device_path,   0, 0},
	{ CMD_PORT_AUDIO,              SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_use_port_audio_device,   0, 0},
	{ CMD_PORTA_CAP,               SCRIPT_COMMAND, 0,  2, {0}, { p_int, p_dev_name }, 0, 0, 0, process_capture_path,            0, 0},
	{ CMD_PORTA_PLAY,              SCRIPT_COMMAND, 0,  2, {0}, { p_int, p_dev_name }, 0, 0, 0, process_playback_path,           0, 0},
	{ CMD_PULSEA,                  SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_use_pulse_audio_device,  0, 0},
	{ CMD_PULSEA_SERVER,           SCRIPT_COMMAND, 0,  1, {0}, { p_dev_path }, 0, 0, 0, process_pulse_audio_device_path, 0, 0},
	{ CMD_END_CMD,                 SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_audio_settings_command_table[] = {
	{ CMD_CAPTURE_SAMPLE_RATE,     SCRIPT_COMMAND, 0,  1, {0}, { p_string   },     0, 0, 0, process_audio_device_sample_rate_capture, 0, 0},
	{ CMD_PLAYBACK_SAMPLE_RATE,    SCRIPT_COMMAND, 0,  1, {0}, { p_string   },     0, 0, 0, process_audio_device_sample_rate_playback, 0, 0},
	{ CMD_AUDIO_CONVERTER,         SCRIPT_COMMAND, 0,  1, {0}, { p_string   },     0, 0, 0, process_audio_device_converter, 0, 0},
	{ CMD_RX_PPM,                  SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_rx_ppm, 0, 0},
	{ CMD_TX_PPM,                  SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_tx_ppm, 0, 0},
	{ CMD_TX_OFFSET,               SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_tx_offset, 0, 0},
	{ CMD_END_CMD,                 SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_audio_rt_channel_command_table[] = {
	{ CMD_AUDIO_L_R,               SCRIPT_COMMAND, 0,  1, {0}, { p_bool }, 0, 0, 0, process_modem_signal_left_right,  0, 0},
	{ CMD_AUDIO_REV_L_R,           SCRIPT_COMMAND, 0,  1, {0}, { p_bool }, 0, 0, 0, process_reverse_left_right,       0, 0},
	{ CMD_PTT_RIGHT_CHAN,          SCRIPT_COMMAND, 0,  1, {0}, { p_bool }, 0, 0, 0, process_ptt_tone_right_channel,   0, 0},
	{ CMD_CW_QSK_RT_CHAN,          SCRIPT_COMMAND, 0,  1, {0}, { p_bool }, 0, 0, 0, process_cw_qsk_right_channel,     0, 0},
	{ CMD_PSEUDO_FSK_RT_CHAN,      SCRIPT_COMMAND, 0,  1, {0}, { p_bool }, 0, 0, 0, process_pseudo_fsk_right_channel, 0, 0},
	{ CMD_END_CMD,                 SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_audio_wave_command_table[] = {
	{ CMD_WAVE_SR, SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_wave_file_sample_rate, 0, 0},
	{ CMD_END_CMD, SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_rig_hrdwr_ptt_command_table[] = {
	{ CMD_HPPT_PTT_RT,             SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_hrdw_ptt_right_audio_channel, 0, 0},
	{ CMD_HPTT_SP2,                SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_hrdw_ptt_sep_serial_port, 0, 0},
	{ CMD_HPTT_SP2_PATH,           SCRIPT_COMMAND, 0,  1, {0}, { p_dev_path },     0, 0, 0, process_hrdw_ptt_sep_serial_port_path, 0, 0},
	{ CMD_HPTT_SP2_RTS,            SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_hrdw_ptt_sep_serial_port_rts, 0, 0},
	{ CMD_HPTT_SP2_DTR,            SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_hrdw_ptt_sep_serial_port_dtr, 0, 0},
	{ CMD_HPTT_SP2_RTS_V,          SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_hrdw_ptt_sep_serial_port_rts_v, 0, 0},
	{ CMD_HPTT_SP2_DTR_V,          SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_hrdw_ptt_sep_serial_port_dtr_v, 0, 0},
	{ CMD_HPTT_SP2_START_DELAY,    SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_hrdw_ptt_start_delay, 0, 0},
	{ CMD_HPTT_SP2_END_DELAY,      SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_hrdw_ptt_end_delay, 0, 0},
	{ CMD_HPTT_UHROUTER,           SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_hrdw_ptt_uhrouter, 0, 0},
	{ CMD_HPTT_SP2_INITIALIZE,     SCRIPT_COMMAND, 0,  0, {0}, { p_void },         0, 0, 0, process_hrdw_ptt_initialize, 0, 0},
	{ CMD_END_CMD,                 SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_rigcat_command_table[] = {
	{ CMD_RIGCAT_STATE,            SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_use_rigcat, 0, 0},
	{ CMD_RIGCAT_DESC_FILE,        SCRIPT_COMMAND, 0,  1, {0}, { p_filename },     0, 0, 0, process_rigcat_desc_file, 0, 0},
	{ CMD_RIGCAT_DEV_PATH,         SCRIPT_COMMAND, 0,  1, {0}, { p_dev_path },     0, 0, 0, process_rigcat_device_path, 0, 0},
	{ CMD_RIGCAT_RETRIES,          SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_rigcat_retries, 0, 0},
	{ CMD_RIGCAT_RETRY_INTERVAL,   SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_rigcat_retry_interval, 0, 0},
	{ CMD_RIGCAT_WRITE_DELAY,      SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_rigcat_write_delay, 0, 0},
	{ CMD_RIGCAT_INTIAL_DELAY,     SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_rigcat_init_delay, 0, 0},
	{ CMD_RIGCAT_BAUD_RATE,        SCRIPT_COMMAND, 0,  1, {0}, { p_string },       0, 0, 0, process_rigcat_baud_rate, 0, 0},
	{ CMD_RIGCAT_STOP_BITS,        SCRIPT_COMMAND, 0,  1, {0}, { p_float  },       0, 0, 0, process_rigcat_stop_bits, 0, 0},
	{ CMD_RIGCAT_ECHO,             SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_rigcat_commands_echoed, 0, 0},
	{ CMD_RIGCAT_TOGGLE_RTS_PTT,   SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_rigcat_toggle_rts_ptt, 0, 0},
	{ CMD_RIGCAT_RESTORE,          SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_rigcat_restore_on_close, 0, 0},
	{ CMD_RIGCAT_PTT_COMMAND,      SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_rigcat_cat_command_ptt, 0, 0},
	{ CMD_RIGCAT_RTS_12V,          SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_rigcat_rts_12v, 0, 0},
	{ CMD_RIGCAT_DTR_12V,          SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_rigcat_dtr_12v, 0, 0},
	{ CMD_RIGCAT_HRDWR_FLOW,       SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_rigcat_hrdwr_flow, 0, 0},
	{ CMD_RIGCAT_VSP,              SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_rigcat_vsp_enable, 0, 0},
	{ CMD_RIGCAT_INITIALIZE,       SCRIPT_COMMAND, 0,  0, {0}, { p_void },         0, 0, 0, process_rigcat_initialize, 0, 0},
	{ CMD_END_CMD,                 SCRIPT_COMMAND, 0,  0, {0}, { p_list_end },     0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_hamlib_command_table[] = {
	{ CMD_HAMLIB_STATE,            SCRIPT_COMMAND, 0,  1, {0}, { p_bool },         0, 0, 0, process_use_hamlib,              0, 0},
	{ CMD_HAMLIB_RIG,              SCRIPT_COMMAND, 0,  1, {0}, { p_dev_name },     0, 0, 0, process_hamlib_rig,              0, 0},
	{ CMD_HAMLIB_DEV_PATH,         SCRIPT_COMMAND, 0,  1, {0}, { p_dev_path },     0, 0, 0, process_hamlib_device_path,      0, 0},
	{ CMD_HAMLIB_RETRIES,          SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_hamlib_retries,          0, 0},
	{ CMD_HAMLIB_RETRY_INTERVAL,   SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_hamlib_retry_interval,   0, 0},
	{ CMD_HAMLIB_WRITE_DELAY,      SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_hamlib_write_delay,      0, 0},
	{ CMD_HAMLIB_POST_WRITE_DELAY, SCRIPT_COMMAND, 0,  1, {0}, { p_unsigned_int }, 0, 0, 0, process_hamlib_post_write_delay, 0, 0},
	{ CMD_HAMLIB_BAUD_RATE,        SCRIPT_COMMAND, 0,  1, {0}, { p_string },       0, 0, 0, process_hamlib_baud_rate,        0, 0},
	{ CMD_HAMLIB_STOP_BITS,        SCRIPT_COMMAND, 0,  1, {0}, { p_string },       0, 0, 0, process_hamlib_stop_bits,        0, 0},
	{ CMD_HAMLIB_SIDE_BAND,        SCRIPT_COMMAND, 0,  1, {0}, { p_string },       0, 0, 0, process_hamlib_sideband,         0, 0},
	{ CMD_HAMLIB_PTT_COMMAND,      SCRIPT_COMMAND, 0,  1, {0}, { p_bool   },       0, 0, 0, process_hamlib_ptt_hl_command,   0, 0},
	{ CMD_HAMLIB_DTR_12V,          SCRIPT_COMMAND, 0,  1, {0}, { p_bool   },       0, 0, 0, process_hamlib_dtr_12,           0, 0},
	{ CMD_HAMLIB_RTS_12V,          SCRIPT_COMMAND, 0,  1, {0}, { p_bool   },       0, 0, 0, process_hamlib_rts_12,           0, 0},
	{ CMD_HAMLIB_HRDWR_FLOW,       SCRIPT_COMMAND, 0,  1, {0}, { p_bool   },       0, 0, 0, process_hamlib_rts_cts_flow,     0, 0},
	{ CMD_HAMLIB_SFTWR_FLOW,       SCRIPT_COMMAND, 0,  1, {0}, { p_bool   },       0, 0, 0, process_hamlib_xon_xoff_flow,    0, 0},
	{ CMD_HAMLIB_ADV_CONFIG,       SCRIPT_COMMAND, 0,  1, {0}, { p_string },       0, 0, 0, process_hamlib_advanced_config,  0, 0},
	{ CMD_HAMLIB_INITIALIZE,       SCRIPT_COMMAND, 0,  0, {0}, { p_void   },       0, 0, 0, process_hamlib_initialize,       0, 0},
	{ CMD_END_CMD,                 SCRIPT_COMMAND, 0,  0, {0}, { p_list_end },     0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_rc_xmlrpc_command_table[] = {
	{ CMD_RC_XMLRPC_STATE,         SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_use_xml_rpc,           0, 0},
	{ CMD_RC_XMLRPC_BW_DELAY,      SCRIPT_COMMAND, 0,  1, {0}, { p_float    }, 0, 0, 0, process_xml_rpc_mode_bw_delay, 0, 0},
	{ CMD_RC_XMLRPC_INITIALIZE,    SCRIPT_COMMAND, 0,  0, {0}, { p_void     }, 0, 0, 0, process_xml_rpc_initialize,    0, 0},
	{ CMD_END_CMD,                 SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};


static const SCRIPT_COMMANDS default_kiss_command_table[] = {
	{ CMD_IO_KISS_IPA,             SCRIPT_COMMAND, 0,  1, {0}, { p_string  },  0, 0, 0, process_io_kiss_ip_address,     0, 0},
	{ CMD_IO_KISS_IOPN,            SCRIPT_COMMAND, 0,  1, {0}, { p_int  },     0, 0, 0, process_io_kiss_io_port_no,     0, 0},
	{ CMD_IO_KISS_OPN,             SCRIPT_COMMAND, 0,  1, {0}, { p_int  },     0, 0, 0, process_io_kiss_o_port_no,      0, 0},
	{ CMD_IO_KISS_DP,              SCRIPT_COMMAND, 0,  1, {0}, { p_bool  },    0, 0, 0, process_io_kiss_dual_port,      0, 0},
	{ CMD_IO_KISS_BUSY,            SCRIPT_COMMAND, 0,  1, {0}, { p_bool  },    0, 0, 0, process_io_kiss_busy_channel,   0, 0},
	{ CMD_IO_KISS_CONT,            SCRIPT_COMMAND, 0,  1, {0}, { p_int  },     0, 0, 0, process_io_kiss_continue_after, 0, 0},
	{ CMD_IO_KISS_ATTEN,           SCRIPT_COMMAND, 0,  1, {0}, { p_int  },     0, 0, 0, process_io_kiss_kpsql_atten,    0, 0},
	{ CMD_END_CMD,                 SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_arq_command_table[] = {
	{ CMD_IO_ARQ_IPA,             SCRIPT_COMMAND, 0,  1, {0}, { p_string  },  0, 0, 0, process_io_arq_ip_address, 0, 0},
	{ CMD_IO_ARQ_IOPN,            SCRIPT_COMMAND, 0,  1, {0}, { p_int  },     0, 0, 0, process_io_arq_io_port_no, 0, 0},
	{ CMD_END_CMD,                SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_xmlrpc_command_table[] = {
	{ CMD_IO_XMLRPC_IPA,          SCRIPT_COMMAND, 0,  1, {0}, { p_string  },  0, 0, 0, process_io_xmlrpc_ip_address, 0, 0},
	{ CMD_IO_XMLRPC_IOPN,         SCRIPT_COMMAND, 0,  1, {0}, { p_int  },     0, 0, 0, process_io_xmlrpc_io_port_no, 0, 0},
	{ CMD_END_CMD,                SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_io_command_table[] = {
	{ CMD_IO_LOCK,         SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_io_lock,        0, 0},
	{ CMD_IO_ACT_PORT,     SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_io_active_port, 0, 0},
	{ CMD_IO_AX25_DECODE,  SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_io_ax25_decode, 0, 0},
	{ CMD_IO_CSMA,         SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_io_csma,        0, 0},
	{ CMD_IO_KISS,         SCRIPT_COMMAND, 0,  0, {0}, { p_list     }, 0, 0, 0, 0, (struct script_cmds *) &default_kiss_command_table,   sizeof(default_kiss_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_IO_ARQ,          SCRIPT_COMMAND, 0,  0, {0}, { p_list     }, 0, 0, 0, 0, (struct script_cmds *) &default_arq_command_table,    sizeof(default_arq_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_IO_XMLRPC,       SCRIPT_COMMAND, 0,  0, {0}, { p_list     }, 0, 0, 0, 0, (struct script_cmds *) &default_xmlrpc_command_table, sizeof(default_xmlrpc_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_END_CMD,         SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_fldigi_command_table[] = {
	{ CMD_FLDIGI_FREQ,      SCRIPT_COMMAND, 0,  1, {0}, { p_double   }, 0, 0, 0, process_rig_freq,     0, 0},
	{ CMD_FLDIGI_MODE,      SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_rig_mode,     0, 0},
	{ CMD_FLDIGI_WFHZ,      SCRIPT_COMMAND, 0,  1, {0}, { p_int      }, 0, 0, 0, process_wf_hz_offset, 0, 0},
	{ CMD_FLDIGI_RXID,      SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_rx_rsid,      0, 0},
	{ CMD_FLDIGI_TXID,      SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_tx_rsid,      0, 0},
	{ CMD_FLDIGI_SPOT,      SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_spot,         0, 0},
	{ CMD_FLDIGI_REV,       SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_rev,          0, 0},
	{ CMD_FLDIGI_AFC,       SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_afc,          0, 0},
	{ CMD_FLDIGI_LOCK,      SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_lock,         0, 0},
	{ CMD_FLDIGI_SQL,       SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_sql,          0, 0},
	{ CMD_FLDIGI_KPSQL,     SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_kpsql,        0, 0},
	{ CMD_FLDIGI_KPSM,      SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_kpsql,        0, 0},
	{ CMD_FLDIGI_MODEM,     SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_modem,        0, 0},
	{ CMD_END_CMD,          SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};


static const SCRIPT_COMMANDS default_nbems_command_table[] = {
	{ CMD_NBEMS_FLMSG_PATH, SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_misc_nbems_flmsg_path, 0, 0},
	{ CMD_NBEMS_TIMEOUT,    SCRIPT_COMMAND, 0,  1, {0}, { p_double   }, 0, 0, 0, process_misc_nbems_timeout,    0, 0},
	{ CMD_NBEMS_STATE,      SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_misc_nbems_state,      0, 0},
	{ CMD_NBEMS_MSG,        SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_misc_nbems_open_msg,   0, 0},
	{ CMD_NBEMS_FLMSG,      SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_misc_nbems_open_flmsg, 0, 0},
	{ CMD_NBEMS_BRWSR,      SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_misc_nbems_open_brwsr, 0, 0},
	{ CMD_END_CMD,          SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_id_rsid_command_table[] = {
	{ CMD_ID_RSID_NOTIFY,       SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_rsid_notify,       0, 0},
	{ CMD_ID_RSID_SRCH_BP,      SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_rsid_search_bp,    0, 0},
	{ CMD_ID_RSID_MARK_PREV,    SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_rsid_mark_prev,    0, 0},
	{ CMD_ID_RSID_DETECTOR,     SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_rsid_detector,     0, 0},
	{ CMD_ID_RSID_ALRT_DIALOG,  SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_rsid_alert_dialog, 0, 0},
	{ CMD_ID_RSID_TX_FREQ_LOCK, SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_rsid_tx_freq_lock, 0, 0},
	{ CMD_ID_RSID_FREQ_CHANGE,  SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_rsid_freq_change,  0, 0},
	{ CMD_ID_RSID_ALLOW_ERRORS, SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_rsid_allow_errors, 0, 0},
	{ CMD_ID_RSID_SQL_OPEN,     SCRIPT_COMMAND, 0,  1, {0}, { p_int      }, 0, 0, 0, process_rsid_sql_open,     0, 0},
	{ CMD_ID_RSID_PRETONE,      SCRIPT_COMMAND, 0,  1, {0}, { p_double   }, 0, 0, 0, process_rsid_pretone,      0, 0},
	{ CMD_ID_RSID_END_XMT_ID,   SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_rsid_end_xmt_id,   0, 0},
	{ CMD_END_CMD,              SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_id_video_command_table[] = {
	{ CMD_ID_VIDEO_TX_ID_MODE,   SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_video_tx_id_mode,  0, 0},
	{ CMD_ID_VIDEO_TX_VIDEO_TXT, SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_video_tx_vid_txt,  0, 0},
	{ CMD_ID_VIDEO_TX_TXT_INP,   SCRIPT_COMMAND, 0,  1, {0}, { p_string   }, 0, 0, 0, process_video_txt_input,   0, 0},
	{ CMD_ID_VIDEO_SMALL_FONT,   SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_video_small_font,  0, 0},
	{ CMD_ID_VIDEO_500_HZ,       SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_video_500hz,       0, 0},
	{ CMD_ID_VIDEO_WIDTH_LIMIT,  SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_video_width_limit, 0, 0},
	{ CMD_ID_VIDEO_CHAR_ROW,     SCRIPT_COMMAND, 0,  1, {0}, { p_int      }, 0, 0, 0, process_rsid_char_per_row, 0, 0},
	{ CMD_END_CMD,               SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_id_cw_command_table[] = {
	{ CMD_ID_CW_TX_CALLSIGN, SCRIPT_COMMAND, 0,  1, {0}, { p_bool     }, 0, 0, 0, process_cw_callsign,      0, 0},
	{ CMD_ID_CW_SPEED,       SCRIPT_COMMAND, 0,  1, {0}, { p_int      }, 0, 0, 0, process_cw_speed,         0, 0},
	{ CMD_END_CMD,           SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_id_command_table[] = {
	{ CMD_ID_RSID,          SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_id_rsid_command_table,          sizeof(default_id_rsid_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_ID_VIDEO,         SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_id_video_command_table,         sizeof(default_id_video_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_ID_CW,            SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_id_cw_command_table,            sizeof(default_id_cw_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_END_CMD,          SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};

static const SCRIPT_COMMANDS default_top_level_command_table[] = {
	{ CMD_FLDIGI,           SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_fldigi_command_table,           sizeof(default_fldigi_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_OPERATOR,         SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_operator_command_table,         sizeof(default_operator_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_AUDIO_DEVICE,     SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_audio_device_command_table,     sizeof(default_audio_device_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_AUDIO_SETTINGS,   SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_audio_settings_command_table,   sizeof(default_audio_settings_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_AUDIO_RT_CHANNEL, SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_audio_rt_channel_command_table, sizeof(default_audio_rt_channel_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_AUDIO_WAVE,       SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_audio_wave_command_table,       sizeof(default_audio_wave_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_HRDWR_PTT,        SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_rig_hrdwr_ptt_command_table,    sizeof(default_rig_hrdwr_ptt_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_RIGCAT,           SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_rigcat_command_table,           sizeof(default_rigcat_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_HAMLIB,           SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_hamlib_command_table,           sizeof(default_hamlib_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_RC_XMLRPC,        SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_rc_xmlrpc_command_table,        sizeof(default_rc_xmlrpc_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_IO,               SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_io_command_table,               sizeof(default_io_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_NBEMS,            SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_nbems_command_table,            sizeof(default_nbems_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_ID,               SCRIPT_COMMAND, 0,  0, {0}, { p_list   }, 0, 0, 0, 0, (struct script_cmds *) &default_id_command_table,               sizeof(default_id_command_table)/sizeof(SCRIPT_COMMANDS)},
	{ CMD_LOAD_MACRO,       SCRIPT_COMMAND | SCRIPT_STRUCTURED_ONLY, 0,  3, {0}, { p_int, p_int, p_string   }, 0, 0, 0, process_load_macro, 0, 0},
	{ CMD_END_CMD,          SCRIPT_COMMAND, 0,  0, {0}, { p_list_end }, 0, 0, 0, 0, 0, 0},
	{ {0} }
};


/** **************************************************************
 * \brief Macro command requires special procesing.
 * \param cmd Pointer to matching struct script_cmds script
 * command.
 * \return SCRIPT_CODES error see \ref script_codes
 *****************************************************************/
SCRIPT_CODES ScriptParsing::sc_macros(struct script_cmds *cmd)
{
	bool not_done = false;
	std::string temp;
	SCRIPT_CODES error = script_no_errors;
	int line_number_start = line_number;
	char *local_buffer = (char *)0;

	_macro_command.clear();
	temp.clear();

	std::string search_cmd;
	search_cmd.assign(CMD_END_CMD).append(":");

	local_buffer = new char[MAX_READLINE_LENGTH];

	if(!local_buffer)
		return script_memory_allocation_error;

	while(!not_done) {

		if(ferror(fd)) {
			error = script_file_read_error;
			break;
		}

		if(feof(fd)) {
			error = script_unexpected_eof;
			break;
		}

		memset(local_buffer, 0, MAX_READLINE_LENGTH);
		if (fgets(local_buffer, MAX_READLINE_LENGTH, fd))
			line_number++;

		temp.assign(local_buffer);
		trim(temp);


		if(temp.find(search_cmd) == std::string::npos)
			_macro_command.append(local_buffer);
		else
			break;
	}

	int pos = _macro_command.length() - 1;
	bool lf_removed = false;

	// Remove the last new line in the macro command (script syntax requirement)

	if(pos > 1) { // Windows way
		if(_macro_command[pos-1] == '\r' && _macro_command[pos] == '\n') {
			_macro_command.erase(pos - 1, 2);
			lf_removed = true;
		}
	}

	if((lf_removed == false) && (pos > 0)) { // Unix/Linux/MacOSX way
		if(_macro_command[pos] == '\n')
			_macro_command.erase(pos, 1);
	}

	if(error == script_unexpected_eof) {
		LOG_INFO(_("Missing command %s after line %d"), search_cmd.c_str(), line_number_start);
	}

	delete [] local_buffer;

	return error;
}

/** **************************************************************
 * \brief Used for initialization of the function vector table.
 * \param cmd Pointer to matching struct script_cmds script
 * command.
 * \return SCRIPT_CODES error see \ref script_codes
 *****************************************************************/
SCRIPT_CODES ScriptParsing::sc_dummy(struct script_cmds *cmd)
{
	return script_no_errors;
}

/** **************************************************************
 * \brief Convert error numbers into human readable form.
 * \param error_no Error number to convert.
 * \param ln The offending line number in the script file.
 * \param cmd The script command is question.
 * \return SCRIPT_CODES error see \ref script_codes
 *****************************************************************/
char * ScriptParsing::script_error_string(SCRIPT_CODES error_no, int ln, char *cmd)
{
	char *es = (char *) "";

	memset(error_buffer,     0, sizeof(error_buffer));
	memset(error_cmd_buffer, 0, sizeof(error_cmd_buffer));
	memset(error_string,     0, sizeof(error_string));

	if(cmd) {
		strncpy(error_cmd_buffer, cmd, sizeof(error_cmd_buffer)-1);
	}

	if(error_no < 0) {
		_script_error_detected = true;
	}

	switch(error_no) {
		case script_command_not_found:
			es =  (char *) _("Command Not Found");
			break;

		case script_non_script_file:
			es = (char *) _("Not a script file/tag not found");
			break;

		case script_parameter_error:
			es = (char *) _("Invalid parameter");
			break;

		case script_function_parameter_error:
			es = (char *) _("Invalid function parameter (internal non-script error)");
			break;

		case script_mismatched_quotes:
			es = (char *) _("Missing paired quotes (\")");
			break;

		case script_general_error:
			es = (char *) _("General Error");
			break;

		case script_no_errors:
			es = (char *) _("No Errors");
			break;

		case script_char_match_not_found:
			es = (char *) _("Character searched not found");
			break;

		case script_end_of_line_reached:
			es = (char *) _("End of line reached");
			break;

		case script_file_not_found:
			es = (char *) _("File not found");
			break;

		case script_path_not_found:
			es = (char *) _("Directory path not found");
			break;

		case script_args_eol:
			es = (char *) _("Unexpected end of parameter (args[]) list found");
			break;

		case script_param_check_eol:
			es = (char *) _("Unexpected end of parameter check list found");
			break;

		case script_paramter_exceeds_length:
			es = (char *) _("Character count in args[] parameter exceeds expectations");
			break;

		case script_memory_allocation_error:
			es = (char *) _("Memory Allocation Error (internal non-script error)");
			break;

		case script_incorrectly_assigned_value:
			es = (char *) _("Passed parameter is not of the expected type.");
			break;

		case script_invalid_parameter:
			es = (char *) _("Parameter is not valid.");
			break;

		case script_command_seperator_missing:
			es = (char *) _("Command missing ':'.");
			break;

		case script_max_sub_script_reached:
			es = (char *) _("Maximum open subscripts reached.");
			break;

		case script_subscript_exec_fail:
			es = (char *) _("Subscript execution fail (internal).");
			break;

		case script_device_path_not_found:
			es = (char *) _("Script device path not found.");
			break;

		case script_unexpected_eof:
			es = (char *) _("Unexpected end of file reached.");
			break;

		case script_file_read_error:
			es = (char *) _("File read error");
			break;

		default:
			es = (char *) _("Undefined error");
	}

	snprintf(error_buffer, sizeof(error_buffer)-1, _("Line: %d Error:%d %s (%s)"),
			 ln, error_no, es, error_cmd_buffer);

	return error_buffer;
}

/** **************************************************************
 * \brief Search for first occurrence of a non white space
 * \param data Data pointer to search.
 * \param limit Number of bytes in the data buffer.
 * \param error returned error code.
 * \return Pointer to character if found. Otherwise, return null
 * \par Note:<br>
 * The searched condition is ignored if the expected content is
 * encapsulated in quotes \(\"\"\).
 *****************************************************************/
char * ScriptParsing::skip_white_spaces(char * data, char * limit, SCRIPT_CODES &error)
{
	char *cPtr      = (char *) 0;

	if(!data || !limit) {
		error = script_function_parameter_error;
		return (char *)0;
	}

	for(cPtr = data; cPtr < limit; cPtr++) {
		if(*cPtr > ' ') {
			error = script_no_errors;
			return cPtr;
		}
	}

	error = script_end_of_line_reached;


	return (char *)0;        // End of line reached.
}

/** **************************************************************
 * \brief Search for the first occurrence on a non number.
 * \param data Data pointer to search.
 * \param limit Number of bytes in the data buffer.
 * \param error returned error code.
 * \return Pointer to character if found. Otherwise, return null
 * \par Note:<br>
 * The searched condition is ignored if the expected content is
 * encapsulated in quotes \(\"\"\).
 *****************************************************************/
char * ScriptParsing::skip_numbers(char * data, char * limit, SCRIPT_CODES &error)
{
	char *cPtr  = (char *) 0;
	int  q_flag = 0;

	if(!data || !limit) {
		error = script_function_parameter_error;
		return (char *)0;
	}

	for(cPtr = data; cPtr < limit; cPtr++) {
		if(*cPtr == '"')     // Check for encapsulated strings ("")
			q_flag++;

		if((q_flag & 0x1))   // Continue if string is encapsulated
			continue;

		if(!isdigit(*cPtr)) {
			error = script_no_errors;
			return cPtr;
		}
	}

	if(q_flag & 0x1) {
		error = script_mismatched_quotes;
	} else {
		error = script_end_of_line_reached;
	}

	return (char *)0;        // End of line reached.
}

/** **************************************************************
 * \brief Skip characters until either a number or white space is
 * found.
 * \param data Data pointer to search.
 * \param limit Number of bytes in the data buffer.
 * \param error returned error code.
 * \return Pointer to character if found. Otherwise, return null
 * \par Note:<br>
 * The searched condition is ignored if the expected content is
 * encapsulated in quotes \(\"\"\).
 *****************************************************************/
char * ScriptParsing::skip_characters(char * data, char * limit, SCRIPT_CODES &error)
{
	char *cPtr  = (char *) 0;
	int  q_flag = 0;

	if(!data || !limit) {
		error = script_function_parameter_error;
		return (char *)0;
	}

	for(cPtr = data; cPtr < limit; cPtr++) {
		if(*cPtr == '"')     // Check for encapsulated strings ("")
			q_flag++;

		if((q_flag & 0x1))   // Continue if string is encapsulated
			continue;

		if(isdigit(*cPtr) || *cPtr <= ' ') {
			error = script_no_errors;
			return cPtr;
		}
	}

	if(q_flag & 0x1) {
		error = script_mismatched_quotes;
	} else {
		error = script_end_of_line_reached;
	}

	return (char *)0;        // End of line reached.
}

/** **************************************************************
 * \brief Search for the first occurrence of a white space.
 * \param data Data pointer to search.
 * \param limit Number of bytes in the data buffer.
 * \param error returned error code.
 * \return Pointer to character if found. Otherwise, return null
 * \par Note:<br>
 * The searched condition is ignored if the expected content is
 * encapsulated in quotes \(\"\"\).
 *****************************************************************/
char * ScriptParsing::skip_alpha_numbers(char * data, char * limit, SCRIPT_CODES &error)
{
	char *cPtr  = (char *) 0;
	int  q_flag = 0;

	if(!data || !limit) {
		error = script_function_parameter_error;
		return (char *)0;
	}

	for(cPtr = data; cPtr < limit; cPtr++) {

		if(*cPtr == '"')     // Check for encapsulated strings ("")
			q_flag++;

		if((q_flag & 0x1))   // Continue if string is encapsulated
			continue;

		if(*cPtr <= ' ') {
			error = script_no_errors;
			return cPtr;
		}
	}

	if(q_flag & 0x1) {
		error = script_mismatched_quotes;
	} else {
		error = script_end_of_line_reached;
	}

	return (char *)0;        // End of line reached.
}

/** **************************************************************
 * \brief Search for first occurrence of 'character'
 * \param c Character to search for
 * \param data Pointer to Data to search for character in.
 * \param limit Number of bytes in the data buffer.
 * \param error returned error code.
 * \return Pointer to character if found. Otherwise, return null
 * \par Note:<br>
 * The searched condition is ignored if the expected content is
 * encapsulated in quotes \(\"\"\).
 *****************************************************************/
char * ScriptParsing::skip_to_character(char c, char * data, char * limit, SCRIPT_CODES &error)
{
	char *cPtr  = (char *) 0;
	int  q_flag = 0;

	if(!data || !limit) {
		error = script_function_parameter_error;
		return (char *)0;
	}

	for(cPtr = data; cPtr < limit; cPtr++) {
		if(*cPtr == '"')     // Check for encapsulated strings ("")
			q_flag++;

		if((q_flag & 0x1))   // Continue if string is encapsulated
			continue;

		if(*cPtr == c)   {    // Match found. Return pointer to it's location
			error = script_no_errors;
			return cPtr;
		}
	}

	if(q_flag & 0x1) {
		error = script_mismatched_quotes;
	} else {
		error = script_end_of_line_reached;
	}

	return (char *)0;        // End of line reached.
}

/** **************************************************************
 * \brief Replace CR, LF, and '#' with '0' (by value)
 * \param data Search data pointer
 * \param limit data buffer size
 * \return void (none)
 * \par Note:<br>
 * The searched condition is ignored if the remark character \(#\)
 * is encapsulated in quotes \(\"\"\).
 *****************************************************************/
SCRIPT_CODES ScriptParsing::remove_crlf_comments(char *data, char *limit, size_t &count)
{
	char *cPtr  = (char *) 0;
	int  q_flag = 0;

	SCRIPT_CODES error = script_no_errors;

	if(!data || !limit)
		return script_function_parameter_error;

	count = 0;

	for(cPtr = data; cPtr < limit; cPtr++) {
		if(*cPtr == '\r' || *cPtr == '\n') {
			*cPtr = 0;
			return script_no_errors;
		}

		if(*cPtr == '"')
			q_flag++;

		if((q_flag & 0x1))
			continue;

		if(*cPtr == '#') {
			*cPtr = 0;
			break;
		}

		if(*cPtr > ' ')
			count++;

	}

	// Remove trailing white spaces.
	while(cPtr >= data) {
		if(*cPtr <= ' ') *cPtr = 0;
		else break;
		cPtr--;
	}

	if(q_flag & 0x1) {
		error = script_mismatched_quotes;
	} else {
		error = script_end_of_line_reached;
	}

	return error;
}

/** **************************************************************
 * \brief Copy memory from address to address.
 * \param buffer Destination buffer
 * \param sPtr Start of the copy Address
 * \param ePtr End of the copy Address
 * \param limit Destination buffer size
 * command.
 * \return SCRIPT_CODES error see \ref script_codes
 *****************************************************************/
SCRIPT_CODES ScriptParsing::copy_string_uppercase(char *buffer, char *sPtr, char *ePtr, size_t limit)
{
	if(!buffer || !sPtr || !ePtr || limit < 1) {
		return script_function_parameter_error;
	}

	char *dPtr   = buffer;
	size_t index = 0;

	for(index = 0; index < limit; index++) {
		*dPtr++ = toupper(*sPtr++);
		if(sPtr >= ePtr) break;
	}

	return script_no_errors;
}

/** **************************************************************
 * \brief Parse the parameters and seperate into individual components.
 * \param s char pointer to the start of the string.
 * \param e char pointer to the end of the string.
 * \param matching_command pointer to the data strucure of the matching
 * command. See \ref SCRIPT_COMMANDS
 * \return SCRIPT_CODES error see \ref script_codes
 *****************************************************************/
SCRIPT_CODES ScriptParsing::parse_parameters(char *s, char *e, SCRIPT_COMMANDS *matching_command)
{
	char *c   = s;
	char *d   = (char *)0;
	int index = 0;
	int parameter_count = matching_command->argc;
	int count = 0;
	long tmp  = 0;

	SCRIPT_CODES error = script_no_errors;

	// Clear the old pointers.
	for(index = 0; index < MAX_CMD_PARAMETERS; index++) {
		matching_command->args[index] = (char *)0;
	}

	if(parameter_count > 0) {
		count = parameter_count - 1;
		for(index = 0; index < count; index++) {
			c = skip_white_spaces(c, e, error);

			if(error != script_no_errors)
				return script_parameter_error;

			d = skip_to_character(',', c, e, error);

			if(error != script_no_errors)
				return script_parameter_error;

			*d = 0;
			tmp = (long) (d - c);
			if(tmp > 0)
				trim(c, (size_t)(tmp));
			matching_command->args[index] = c;
			c = d + 1;
		}

		c = skip_white_spaces(c, e, error);
		if(error) return error;

		d = skip_alpha_numbers(c, e, error);
		if(error) return error;

		*d = 0;
		tmp = (long) (d - c);
		if(tmp > 0)
			trim(c, (size_t)(tmp));

		matching_command->args[index] = c;
	}

#ifdef TESTING
	for(int i = 0; i < parameter_count;  i++)
		if(matching_command->args[i])
			printf("parameters %d (%s)\n", i, matching_command->args[i]);
#endif

	error = check_parameters(matching_command);

	if(error != script_no_errors)
		return error;

	// If assigned then special processing is required.
	if(matching_command->func)
		error = (this->*matching_command->func)(matching_command);
	if(error) return error;


	return script_no_errors;
}

/** **************************************************************
 * \brief Execute callback function.
 * \param cb_data Pointer for making a copy of the data to prevent
 * exterior alteration of source information.
 * \return 0 = No error<br> \< 0 = Error<br>
 *****************************************************************/
int ScriptParsing::call_callback(SCRIPT_COMMANDS *cb_data)
{
	int argc     = 0;
	int error    = 0;
	int index    = 0;
	SCRIPT_COMMANDS *tmp = (SCRIPT_COMMANDS *)0;
	size_t count = 0;

	if(!cb_data || !cb_data->cb) return -1;

	argc = cb_data->argc;

	tmp = new SCRIPT_COMMANDS;

	if(!tmp) return -1;

	memset(tmp, 0, sizeof(SCRIPT_COMMANDS));

	for(index = 0; index < argc; index++) {
		if(cb_data->args[index]) {
			count = strnlen(cb_data->args[index], MAX_PARAMETER_LENGTH-1);
			tmp->args[index] = new char[count+1];
			if(tmp->args[index]) {
				memset(tmp->args[index], 0, count+1);
				strncpy(tmp->args[index], cb_data->args[index], count);
			} else {
				error = -1;
				break;
			}
		} else break;
	}

	if(error > -1) {
		// Fill SCRIPT_COMMANDS (tmp) struct with useful data.
		tmp->flags          = cb_data->flags;
		tmp->command_length = cb_data->command_length;
		tmp->argc           = cb_data->argc;
		strncpy(tmp->command, cb_data->command, MAX_COMMAND_LENGTH);

		// Initialize with do nothing functions
		tmp->func = &ScriptParsing::sc_dummy;
		tmp->cb   = callback_dummy;

		error = (*cb_data->cb)(this, tmp);
	}

	if(tmp) {
		for(index = 0; index < argc; index++) {
			if(tmp->args[index]) {
				delete [] tmp->args[index];
			}
		}

		delete tmp;
	}

	return error;
}

/** **************************************************************
 * \brief Parse a single line of data from the script file being read.
 * \param data Pointer the the script scring in question
 * \param buffer_size buffer size of the data pointer.
 * command.
 * \return SCRIPT_CODES error see \ref script_codes
 *****************************************************************/
SCRIPT_CODES ScriptParsing::parse_hierarchy_command(char *data, size_t buffer_size)
{
	char *buffer = (char *)0;
	char *cPtr   = (char *)0;
	char *endPtr = (char *)0;
	char *ePtr   = (char *)0;
	int allocated_buffer_size = MAX_COMMAND_LENGTH;

	SCRIPT_CODES error = script_no_errors;
	SCRIPT_COMMANDS *local_list = _script_commands;
	size_t local_limit = _script_command_count;

	cPtr = data;
	endPtr = &data[buffer_size];

	cPtr = skip_white_spaces(cPtr, endPtr, error);
	if(error != script_no_errors) return error;

	ePtr = skip_to_character(':', cPtr, endPtr, error);
	if(error != script_no_errors) return script_command_seperator_missing;

	buffer = new char [allocated_buffer_size];
	if(!buffer) {
		LOG_INFO(_("Buffer allocation Error near File: %s Line %d"), __FILE__, __LINE__);
		return script_memory_allocation_error;
	}

	memset(buffer, 0, allocated_buffer_size);
	error = copy_string_uppercase(buffer, cPtr, ePtr, allocated_buffer_size-1);

	if(error != script_no_errors) {
		buffer[0] = 0;
		delete [] buffer;
		return error;
	}

	int str_count = str_cnt(buffer, allocated_buffer_size);
	trim(buffer, (size_t) str_count);

	char sub_command[MAX_COMMAND_LENGTH];

	bool not_found = true;
	char *endCmd = ePtr;
	char *endCmdPtr = endPtr;

	cPtr = buffer;
	endPtr = &buffer[str_count];

	while(not_found) {
		memset(sub_command, 0, sizeof(sub_command));

		ePtr = skip_to_character('.', cPtr, endPtr, error);

		if(error == script_end_of_line_reached) {
			ePtr = endPtr;
			error = script_no_errors;
		}

		if(error == script_no_errors) {
			copy_string_uppercase(sub_command, cPtr, ePtr, MAX_COMMAND_LENGTH-1);
			cPtr = ePtr + 1;
		}
		else
			break;

		for(size_t i = 0; i < local_limit; i++) {

			if(local_list[i].command[0] == 0) {
				not_found = false;
				error = script_command_not_found;
				break;
			}

			if(strncmp(sub_command, local_list[i].command, MAX_COMMAND_LENGTH) == 0) {

				if((local_list[i].param_check[0] == p_list) && local_list[i].sub_commands) {
					local_limit = local_list[i].sub_command_count;
					local_list  = local_list[i].sub_commands;
					break;
				}

				if((file_type() & local_list[i].flags) && !(SCRIPT_STRUCTURED_ONLY & local_list[i].flags)) {
					error = parse_parameters(++endCmd, endCmdPtr, &local_list[i]);

					if(error)  {
						buffer[0] = 0;
						delete [] buffer;
						return error;
					}

					if(local_list[i].cb) {
						error = (SCRIPT_CODES) call_callback(&local_list[i]);
						if(error < script_no_errors)
							LOG_INFO(_("Call back for script command %s reported an Error"), local_list[local_limit].command);
						not_found = false;
						error = script_command_handled;
						break;
					}
				} else {
					LOG_INFO(_("Command %s ignored, dot notation not supported"), buffer);
					not_found = false;
					error = script_general_error;
					break;
				}
			}
		}
	}

	buffer[0] = 0;
	delete [] buffer;

	return error;
}

/** **************************************************************
 * \brief Parse a single line of data from the script file being read.
 * \param data Pointer the the script scring in question
 * \param buffer_size buffer size of the data pointer.
 * command.
 * \return SCRIPT_CODES error see \ref script_codes
 *****************************************************************/
SCRIPT_CODES ScriptParsing::parse_single_command(FILE *fd, SCRIPT_COMMANDS *cur_list, size_t limit, char *data, size_t buffer_size)
{
	char *buffer = (char *)0;
	char *cPtr   = (char *)0;
	char *endPtr = (char *)0;
	char *ePtr   = (char *)0;
	int allocated_buffer_size = MAX_COMMAND_LENGTH;

	size_t cmd_size    = 0;
	size_t cmp_results = 0;
	size_t index       = 0;
	size_t size        = 0;
	bool dot_notation  = false;

	SCRIPT_CODES error = script_no_errors;
	SCRIPT_COMMANDS *local_list = cur_list;
	size_t local_limit = limit;

	cPtr = data;
	endPtr = &data[buffer_size];

	cPtr = skip_white_spaces(cPtr, endPtr, error);
	if(error != script_no_errors) return error;

	ePtr = skip_to_character(':', cPtr, endPtr, error);
	if(error != script_no_errors) return script_command_seperator_missing;

	buffer = new char [allocated_buffer_size];
	if(!buffer) {
		LOG_INFO(_("Buffer allocation Error near File: %s Line %d"), __FILE__, __LINE__);
		return script_memory_allocation_error;
	}

	memset(buffer, 0, allocated_buffer_size);
	error = copy_string_uppercase(buffer, cPtr, ePtr, allocated_buffer_size-1);
	if(error != script_no_errors) {
		buffer[0] = 0;
		delete [] buffer;
		return error;
	}

	int str_count = (int) strnlen(buffer, allocated_buffer_size);
	trim(buffer, (size_t) str_count);


	for(int i = 0; i < str_count; i++) {
		if(buffer[i] == '.') {
			error = parse_hierarchy_command(data, buffer_size);
			if(error == script_no_errors)
				dot_notation = true;
			break;
		}
	}

	if(dot_notation == false && error == script_no_errors) {
		for(index = 0; index < local_limit; index++) {
			size = strnlen(local_list[index].command, MAX_COMMAND_LENGTH);
			cmd_size = strnlen(buffer, MAX_COMMAND_LENGTH);
			cmp_results = memcmp(buffer, local_list[index].command, size);

			if(cmp_results == 0 && (cmd_size == size)) {

				if(local_list[index].param_check[0] == p_list_end) {
					return script_end_of_list_reached;
				}

				if(local_list[index].sub_commands && local_list[index].param_check[0] == p_list) {
					if(recursive_count <= RECURSIVE_LIMIT) {
						read_file(fd, local_list[index].sub_commands, local_list[index].sub_command_count);
					} else {
						error = script_recursive_limit_reached;
					}
					break;
				}

				if((file_type() & local_list[index].flags) && !(SCRIPT_DOT_NOTATION_ONLY & local_list[index].flags)) {
					if(local_list[index].argc > 0) {
						error = parse_parameters(++ePtr, endPtr, &local_list[index]);
						if(error)  {
							buffer[0] = 0;
							delete [] buffer;
							return error;
						}
					}

					if(local_list[index].cb) {
						error = (SCRIPT_CODES) call_callback(&local_list[index]);
						if(error < script_no_errors)
							LOG_INFO(_("Call back for script command %s reported an Error"), local_list[index].command);
					}

				} else {
					LOG_INFO(_("Command %s ignored, structured command not supported"), buffer);
					error = script_general_error;
				}
				break;
			}
		}
	}
	buffer[0] = 0;
	delete [] buffer;

	return error;
}

/** **************************************************************
 * \brief Script entry point for parsing the script file.
 * \param file_name_path path and file name for the script to parse.
 * \return SCRIPT_CODES error see \ref script_codes
 *****************************************************************/
SCRIPT_CODES ScriptParsing::read_file(FILE *fd, SCRIPT_COMMANDS *cur_list, size_t limit)
{
	SCRIPT_CODES return_code = script_no_errors;
	SCRIPT_COMMANDS *local_list = cur_list;

	size_t count    = 0;
	bool errors_reported = false;

	recursive_count++;

	if(recursive_count > RECURSIVE_LIMIT)
		return script_recursive_limit_reached;

	while(1) {
		if(ferror(fd) || feof(fd)) break;

		memset(line_buffer, 0, sizeof(line_buffer));
		if (fgets(line_buffer, sizeof(line_buffer) - 1, fd))
			line_number++;

#ifdef TESTING
		printf("Reading: %s", line_buffer);
#endif

		return_code = remove_crlf_comments(line_buffer, &line_buffer[sizeof(line_buffer)], count);

		if(count < 1) {
			continue;
		}

		if(return_code >= script_no_errors) {
			return_code = parse_single_command(fd, local_list, limit, line_buffer, sizeof(line_buffer) - 1);
		}

		if(return_code == script_end_of_list_reached)
			break;

		if(return_code < script_no_errors) {
			LOG_INFO("%s", script_error_string(return_code, line_number, line_buffer));
			errors_reported = true;
		}
	}

	recursive_count--;

	if(errors_reported)
		return script_errors_reported;

	return script_no_errors;
}

/** **************************************************************
 * \brief Script entry point for parsing the script file.
 * \param file_name_path path and file name for the script to parse.
 * \return SCRIPT_CODES error see \ref script_codes
 *****************************************************************/
SCRIPT_CODES ScriptParsing::parse_commands(char *file_name_path)
{
	char *cPtr      = (char *)0;
	SCRIPT_CODES error_code = script_no_errors;
	size_t tmp      = 0;
	SCRIPT_COMMANDS *local_script_commands = _script_commands;
	size_t local_script_command_count      = _script_command_count;

	if(!file_name_path) {
		LOG_INFO(_("Invalid function parameter 'char *file_name_path' (null)"));
		return script_function_parameter_error;
	}

	fd = fl_fopen(file_name_path, "r");
	line_number = 0;

	if(!fd) {
		LOG_INFO(_("Unable to open file %s"), file_name_path);
		return script_file_not_found;
	}

	memset(line_buffer, 0, sizeof(line_buffer));

	char *retval = fgets(line_buffer, sizeof(line_buffer) - 1, fd);
	line_number++;

	tmp = strlen(SCRIPT_FILE_TAG);
	line_buffer[tmp] = 0;
	tmp = strncmp(SCRIPT_FILE_TAG, line_buffer, tmp);

	if(!retval || tmp) {
		cPtr = script_error_string(script_non_script_file, line_number, line_buffer);
		LOG_INFO("%s", cPtr);
		fclose(fd);
		return script_non_script_file;
	}

	error_code = read_file(fd, local_script_commands, local_script_command_count);

	fclose(fd);

	return error_code;
}
/** **************************************************************
 * \brief Assign a list of valid parameters for verification checks.
 * \param array An Array of pointers to each element.
 * \param array_count Number of entries in the array.
 * \return the array count or '0' if error.
 * \par Note:
 * This array is limited to the first parameter of the command
 * used in it's comparison.
 *****************************************************************/
int ScriptParsing::assign_valid_parameters(const char *command, const char **array, const int array_count)
{
	if(!array || array_count < 1 || !command) return 0;

	int index = 0;
	int count = 0;

	SCRIPT_COMMANDS * cmd_sc = search_command(_script_commands, 0, command);

	if(!cmd_sc) {
		return 0;
	}

	for(index = 0; index < array_count; index++) {
		if(*array[index]) count++;
	}

	if(count != array_count) return 0;

	cmd_sc->valid_values = array;
	cmd_sc->valid_value_count = array_count;

	return array_count;
}

/** **************************************************************
 * \brief Return true state if string is matched.
 * \param state Referenced value to assign results to.
 * \param string Pointer to the data string.
 * \param true_state Pointer to the data to match with.
 * \return SCRIPT_CODES error code.
 *****************************************************************/
inline SCRIPT_CODES ScriptParsing::test_on_off_state(bool &state, char *string, char *true_state=(char *)"ON")
{
	if(!string || !true_state) {
		return script_function_parameter_error;
	}

	bool flag = false;

	if(strncmp(string, true_state, MAX_PARAMETER_LENGTH) == 0)
		flag = true;

	state = flag;

	return script_no_errors;
}

/** **************************************************************
 * \brief Validate if file is located in the specified location.
 * \param filename Pointer to a series of charcters
 * \return SCRIPT_CODES error code.
 *****************************************************************/
SCRIPT_CODES ScriptParsing::check_filename(char *filename)
{
	SCRIPT_CODES error = script_no_errors;

	if(!filename) {
		return script_function_parameter_error;
	}

	FILE *fd = (FILE *)0;

	fd = fl_fopen(filename, "r");

	if(!fd) {
		error = script_file_not_found;
	} else {
		fclose(fd);
	}

	return error;
}

/** **************************************************************
 * \brief Validate if path is present.
 * \param path The path to verify.
 * \return SCRIPT_CODES error code.
 *****************************************************************/
SCRIPT_CODES ScriptParsing::check_path(const char *path)
{
	if(!path) {
		return script_function_parameter_error;
	}

	struct stat st;
	memset(&st, 0, sizeof(struct stat));

	if(stat(path, &st) == 0) {
		if(st.st_mode & S_IFDIR)
			return script_no_errors;
	}

	return script_path_not_found;
}

/** **************************************************************
 * \brief Validate if device path is present.
 * \param path The path to verify.
 * \return SCRIPT_CODES error code.
 *****************************************************************/
SCRIPT_CODES ScriptParsing::check_dev_path(const char *path)
{
	if(!path) {
		return script_function_parameter_error;
	}

	struct stat st;
	memset(&st, 0, sizeof(struct stat));

#ifdef __WIN32__
	std::string alt_path;
	int value = 0;
	int cnt   = 0;

	alt_path.assign(path);

	if(!alt_path.empty()) {
		if(alt_path.find("COM") != std::string::npos) {
			cnt = sscanf(alt_path.c_str(), "COM%d", &value);
			if(cnt && (value > 0))
				return script_no_errors;
		}
	}
#endif

	if(stat(path, &st) == 0) {
		if(st.st_mode & S_IFCHR)
			return script_no_errors;
	}

	return script_device_path_not_found;
}

/** **************************************************************
 * \brief Validate bool representation.
 * \param value String data <ENABLE|YES|1> or <DISABLE|NO|0>
 * \param flag  Depending on string content a value of true or false assigned
 * \return SCRIPT_CODES error code.
 *****************************************************************/
SCRIPT_CODES ScriptParsing::check_bool(const char *value, bool &flag)
{
	if(!value) {
		return script_function_parameter_error;
	}

	flag = false;
	std::string uc_value;

	uc_value.assign(value);
	to_uppercase(uc_value);

	static char *bool_true_flags[] = {
		(char *)PARM_ENABLE, (char *)PARM_YES, (char *)_("1"), (char *)0
	};

	static char *bool_false_flags[] = {
		(char *) PARM_DISABLE, (char *)PARM_NO, (char *)_("0"), (char *)0
	};

	for(size_t i = 0; i < sizeof(bool_true_flags)/sizeof(char *); i++) {
		if(bool_true_flags[i] == (char *)0) break;
		if(strncmp(uc_value.c_str(), bool_true_flags[i], 7) == 0) {
			flag = true;
			return script_no_errors;
		}
	}

	for(size_t i = 0; i < sizeof(bool_false_flags)/sizeof(char *); i++) {
		if(bool_false_flags[i] == (char *)0) break;
		if(strncmp(uc_value.c_str(), bool_false_flags[i], 7) == 0) {
			flag = false;
			return script_no_errors;
		}
	}

	flag = false;
	return script_invalid_parameter;
}

/** **************************************************************
 * \brief Validate if the parameter is a value.
 * \param value The string in question.
 * \param p format verification.
 * \return SCRIPT_CODES error code.
 *****************************************************************/
SCRIPT_CODES ScriptParsing::check_numbers(char *value, paramter_types p)
{
	SCRIPT_CODES error = script_no_errors;
	size_t length = 0;
	size_t index = 0;
	int data_count = 0;
	int signed_value = 0;
	int decimal_point = 0;

	if(!value)
		return script_function_parameter_error;

	length = strnlen(value, MAX_PARAMETER_LENGTH);

	if(length < 1)
		return script_parameter_error;

	// Skip any leading white spaces.
	for(index = 0; index < length; index++) {
		if(value[index] > ' ')
			break;
	}

	if((index >= length))
		return script_parameter_error;

	switch(p) {
		case p_int:
		case p_long:

			if(value[0] == '-' || value[0] == '+') {
				index++;
				signed_value++;
			}

		case p_unsigned_int:
		case p_unsigned_long:

			for(; index< length; index++) {
				if(isdigit(value[index]))
					data_count++;
				else
					break;
			}
			break;

			if(data_count)
				return script_no_errors;

		case p_float:
		case p_double:
			if(value[0] == '-' || value[0] == '+') {
				index++;
				signed_value++;
			}

			for(; index< length; index++) {
				if(isdigit(value[index]))
					data_count++;

				if(value[index] == '.')
					decimal_point++;

				if(decimal_point > 1)
					return script_parameter_error;
			}

			if(data_count)
				return script_no_errors;

			break;

		default:;

	}

	return error;
}

/** **************************************************************
 * \brief Validate the script parameter(s) are of the expected format.
 * \param cmd Matching command data structure.
 * \param p A table of expected parameters types (null terminated).
 * \param p_count the number of 'p[]' items in the table (includes null termination).
 * \return SCRIPT_CODES error code.
 *****************************************************************/
SCRIPT_CODES ScriptParsing::check_parameters(struct script_cmds *cmd)
{
	SCRIPT_CODES error = script_no_errors;
	int count   = 0;
	int index   = 0;
	size_t size = 0;
	bool flag = false;

	if(!cmd)
		return script_function_parameter_error;

	count = cmd->argc;

	if(count < 1)
		return script_no_errors;

	for(index = 0; index < count; index++) {

		if(!cmd->args[index]) {
			return script_args_eol;
		}

		if(cmd->param_check[index] == p_null) {
			size = 0;
		} else {
			size = strnlen(cmd->args[index], MAX_COMMAND_LENGTH);
		}

		switch(cmd->param_check[index]) {
			case p_null:
				error = script_param_check_eol;
				break;

			case p_char:
				if(size > 1)
					error = script_paramter_exceeds_length;
				break;

			case p_bool:
				error = check_bool(cmd->args[index], flag);
				break;

			case p_int:
			case p_long:
			case p_unsigned_int:
			case p_unsigned_long:
			case p_float:
			case p_double:
				error = check_numbers(cmd->args[index], cmd->param_check[index]);
				break;

			case p_dev_path:
				error = check_dev_path(cmd->args[index]);
				break;

			case p_dev_name:
			case p_string:
				if(size < 1)
					error = script_parameter_error;
				break;

			case p_path:
				error = check_path(cmd->args[index]);
				break;

			case p_filename:
				error = check_filename(cmd->args[index]);
				break;

			case p_list:
			case p_list_end:
			case p_void:
				break;
		}

		if(error != script_no_errors)
			break;
	}

	return error;
}

/** **************************************************************
 * \brief Search the content of SCRIPT_COMMANDS structure table
 * for the specified command.
 * \param command The command to search for.
 * \return Pointer to the matching SCRIPT_COMMANDS entry. Null if
 * not found.
 *****************************************************************/
SCRIPT_COMMANDS * ScriptParsing::search_command(SCRIPT_COMMANDS * table, size_t limit, const char *command)
{
	char *cmd_buffer = (char *)0;
	int diff = 0;
	SCRIPT_COMMANDS * found = (SCRIPT_COMMANDS *) 0;
	size_t count = limit;
	size_t index = 0;

	if(!command) return found;

	cmd_buffer = new char [MAX_COMMAND_LENGTH];

	if(!cmd_buffer) {
		LOG_INFO(_("cmd_buffer allocation error near line %d"), __LINE__);
		return found;
	}

	memset(cmd_buffer, 0, MAX_COMMAND_LENGTH);
	strncpy(cmd_buffer, command, MAX_COMMAND_LENGTH-1);

	to_uppercase(cmd_buffer, (int) MAX_COMMAND_LENGTH);
	trim(cmd_buffer, (size_t) MAX_COMMAND_LENGTH);

	for(index = 0; index < count; index++) {
		diff = strncmp(cmd_buffer, table[index].command, MAX_COMMAND_LENGTH);
		if(diff == 0) {
			found = &table[index];
			break;
		}
	}

	cmd_buffer[0] = 0;
	delete [] cmd_buffer;

	return found;
}

/** **************************************************************
 * \brief Convert string to uppercase characters.<br>
 * \par str Pointer to data.
 * \par limit data buffer size
 * \return void
 *****************************************************************/
void ScriptParsing::to_uppercase(char *str, int limit)
{
	if(!str || limit < 1) return;

	int character = 0;
	int count     = 0;
	int index     = 0;

	count = (int) strnlen(str, limit);

	for(index = 0; index < count; index++) {
		character = str[index];
		if(character == 0) break;
		character = (char) toupper(character);
		str[index] = character;
	}
}

/** **************************************************************
 * \brief Convert string to uppercase characters.<br>
 * \par str String storage passed by reference.
 * \return void
 *****************************************************************/
void ScriptParsing::to_uppercase(std::string &str)
{
	int character = 0;
	int count     = 0;
	int index     = 0;

	count = (int) str.length();

	for(index = 0; index < count; index++) {
		character = str[index];
		if(character == 0) break;
		character = (char) toupper(character);
		str[index] = character;
	}
}

#if 0
/** **************************************************************
 * \brief Assign Call back function to a given script command.<br>
 * \param scriptCommand Script command string<br>
 * \param cb Pointer to call back function. int (*cb)(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
 *****************************************************************/
int ScriptParsing::assign_callback(const char *scriptCommand, int (*cb)(ScriptParsing *sp, SCRIPT_COMMANDS *sc))
{
	char *cmd_buffer = (char *)0;
	int diff = 0;
	size_t count = _script_command_count;
	size_t index = 0;

	if(!scriptCommand || !cb) return 0;

	cmd_buffer = new char[MAX_COMMAND_LENGTH];

	if(!cmd_buffer) {
		LOG_INFO(_("cmd_buffer allocation error near line %d"), __LINE__);
		return 0;
	}

	memset(cmd_buffer, 0, MAX_COMMAND_LENGTH);
	strncpy(cmd_buffer, scriptCommand, MAX_COMMAND_LENGTH-1);

	to_uppercase(cmd_buffer, (int) MAX_COMMAND_LENGTH);
	trim(cmd_buffer, (size_t) MAX_COMMAND_LENGTH);

	for(index = 0; index < count; index++) {
		diff = strncmp(cmd_buffer, _script_commands[index].command, MAX_COMMAND_LENGTH);
		if(diff == 0) {
			if(_script_commands[index].cb)
				LOG_INFO(_("Over writing call back funcion for \"%s\""), cmd_buffer);
			_script_commands[index].cb = cb;
			break;
		}
	}

	cmd_buffer[0] = 0;
	delete [] cmd_buffer;

	return 0;
}
#endif // #if 0

/** **************************************************************
 * \brief Assign member calling function to a command
 * \param top_command Top Command
 * \param sub_command Sub command
 * \param func Assigned calling function member
 * \return void (nothing)
 *****************************************************************/
SCRIPT_CODES ScriptParsing::assign_member_func(char *cmd, SCRIPT_CODES (ScriptParsing::*func)(struct script_cmds *))
{
	char *buffer = (char *)0;
	char *cPtr   = (char *)0;
	char *endPtr = (char *)0;
	char *ePtr   = (char *)0;
	int allocated_buffer_size = MAX_COMMAND_LENGTH;
	size_t size        = 0;

	SCRIPT_CODES error = script_no_errors;
	SCRIPT_COMMANDS *local_list = _script_commands;
	size_t local_limit = _script_command_count;

	if(!cmd || !func)
		return script_invalid_parameter;

	size = strnlen(cmd, MAX_COMMAND_LENGTH);
	cPtr = cmd;
	endPtr = &cmd[size];
	size = 0;

	cPtr = skip_white_spaces(cPtr, endPtr, error);
	if(error != script_no_errors) return error;

	ePtr = skip_to_character(':', cPtr, endPtr, error);
	if(error != script_no_errors) return script_command_seperator_missing;

	buffer = new char [allocated_buffer_size];
	if(!buffer) {
		LOG_INFO(_("Buffer allocation Error near File: %s Line %d"), __FILE__, __LINE__);
		return script_memory_allocation_error;
	}

	memset(buffer, 0, allocated_buffer_size);
	error = copy_string_uppercase(buffer, cPtr, ePtr, allocated_buffer_size-1);

	if(error != script_no_errors) {
		buffer[0] = 0;
		delete [] buffer;
		return error;
	}

	int str_count = str_cnt(buffer, allocated_buffer_size);
	trim(buffer, (size_t) str_count);

	char sub_command[MAX_COMMAND_LENGTH];

	bool not_found = true;

	cPtr = buffer;
	endPtr = &buffer[str_count];

	while(not_found) {
		memset(sub_command, 0, sizeof(sub_command));

		ePtr = skip_to_character('.', cPtr, endPtr, error);

		if(error == script_end_of_line_reached) {
			ePtr = endPtr;
			error = script_no_errors;
		}

		if(error == script_no_errors) {
			copy_string_uppercase(sub_command, cPtr, ePtr, MAX_COMMAND_LENGTH-1);
			cPtr = ePtr + 1;
		}
		else
			break;

		for(size_t i = 0; i < local_limit; i++) {

			if(local_list[i].command[0] == 0) {
				not_found = false;
				error = script_command_not_found;
				break;
			}

			if(strncmp(sub_command, local_list[i].command, MAX_COMMAND_LENGTH) == 0) {

				if((local_list[i].param_check[0] == p_list) && local_list[i].sub_commands) {
					local_limit = local_list[i].sub_command_count;
					local_list  = local_list[i].sub_commands;
					break;
				}

				local_list[i].func = func;
				local_list[i].command_length = strnlen(local_list[i].command, MAX_COMMAND_LENGTH);
				not_found = false;
				error = script_no_errors;
				break;
			}
		}
	}

	buffer[0] = 0;
	delete [] buffer;

	return error;

}

#if 0
/** **************************************************************
 * \brief Assign member calling function to a command
 * \param top_command Top Command
 * \param sub_command Sub command
 * \param func Assigned calling function member
 * \return void (nothing)
 *****************************************************************/
void ScriptParsing::assign_member_func(char *cmd, SCRIPT_CODES (ScriptParsing::*func)(struct script_cmds *))
{
	if(!top_command || !func) return;

	SCRIPT_COMMANDS *top_table = (SCRIPT_COMMANDS *)0;
	SCRIPT_COMMANDS *modify_table = (SCRIPT_COMMANDS *)0;

	if(top_cache) {
		if(strncmp(top_cache->command, top_command, MAX_COMMAND_LENGTH) == 0) {
			top_table = top_cache;
		}
	}

	if(!top_table) {
		for(int i = 0; i < _script_command_count; i++) {
			if(strncmp(_script_commands[i].command, top_command, MAX_COMMAND_LENGTH) == 0) {
				top_cache = &_script_commands[i];
				top_table = top_cache;
				break;
			}
		}
	}

	if(!top_table)
		return;

	if(!sub_command)
		modify_table = top_table;

	if(modify_table == (SCRIPT_COMMANDS *)0) {
		if(top_table->sub_commands) {
			for(int i = 0; i < top_table->sub_command_count; i++) {
				if(strncmp(sub_command, top_table->sub_commands[i].command, MAX_COMMAND_LENGTH) == 0) {
					modify_table = &top_table->sub_commands[i];
					break;
				}
			}
		}
	}

	if(modify_table != (SCRIPT_COMMANDS *)0) {
		modify_table->func = func;
		modify_table->command_length = strnlen(modify_table->command, MAX_COMMAND_LENGTH);
	}
}
#endif // #if 0

/** **************************************************************
 * \brief Initialize variables
 * \return void (nothing)
 *****************************************************************/
void ScriptParsing::clear_script_parameters(bool all)
{
	_path.clear();
	_file_type = SCRIPT_COMMAND;
	_macro_command.clear();
}


/** **************************************************************
 * \brief Initialize callable members.
 * \return void (nothing)
 *****************************************************************/
void ScriptParsing::initialize_function_members(void)
{
	std::string cmd;

	cmd.assign(CMD_LOAD_MACRO).append(":"); // For localization
	assign_member_func((char *) cmd.c_str(), &ScriptParsing::sc_macros);
}

/** **************************************************************
 * \brief Copy environment to the sub ScriptParsing class
 * \param src Source Class pointer to copy from.
 *****************************************************************/
int ScriptParsing::CopyScriptParsingEnv(ScriptParsing *src)
{
	if(!src || (src == this)) return -1;


	parent(src);
	script_commands(src->script_commands());
	script_command_count(src->script_command_count());
	initialize_function_members();

	return 0;
}
/** **************************************************************
 * \brief Constructor: Copy and initialize.<br>
 *****************************************************************/
int ScriptParsing::copy_tables(struct script_cmds * subcmds, size_t count)
{
	size_t index = 0;
	size_t table_count = 0;

	SCRIPT_COMMANDS * dst_table = 0;
	SCRIPT_COMMANDS * src_table = 0;

	recursive_count++;
	if(recursive_count > RECURSIVE_LIMIT) return 0;

	for(index = 0; index < count; index++) {
		src_table   = subcmds[index].sub_commands;
		table_count = subcmds[index].sub_command_count;

		if(src_table && table_count) {
			dst_table = new SCRIPT_COMMANDS[table_count];
			if(dst_table) {
				add_to_delete_list(dst_table);
				memcpy(dst_table, src_table, sizeof(SCRIPT_COMMANDS) * table_count);
				subcmds[index].sub_commands      = dst_table;
				subcmds[index].sub_command_count = table_count;
				copy_tables(dst_table, table_count);
			}
		}
	}

	recursive_count--;
	return 0;
}

/** **************************************************************
 * \brief Constructor: Copy and initialize function arrays.<br>
 *****************************************************************/
ScriptParsing::ScriptParsing()
{
	size_t count = 0;

	clear_script_parameters(true);

	memset(line_buffer,      0, sizeof(line_buffer));
	memset(error_cmd_buffer, 0, sizeof(error_cmd_buffer));
	memset(error_string,     0, sizeof(error_string));
	memset(error_buffer,     0, sizeof(error_buffer));

	memset(memory_delete_list, 0, sizeof(memory_delete_list));
	delete_list_count = 0;

	top_cache = 0;
	recursive_count = 0;
	_script_error_detected = false;
	_restart_flag = false;

	fd = 0;

	count = sizeof(default_top_level_command_table)/sizeof(SCRIPT_COMMANDS);

	_script_commands  = new SCRIPT_COMMANDS[count];

	if(_script_commands) {
		add_to_delete_list(_script_commands);
		_script_command_count = count;
		memcpy(_script_commands, default_top_level_command_table, sizeof(SCRIPT_COMMANDS) * _script_command_count);
		copy_tables(_script_commands, _script_command_count);
	}

	recursive_count = 0;
	initialize_function_members();

}

/** **************************************************************
 * \brief Destructor
 *****************************************************************/
ScriptParsing::~ScriptParsing()
{
	int index = 0;
	for(index = 0; index < MAX_DELETE_LIST_COUNT; index++) {
		if(memory_delete_list[index]) {
			delete [] memory_delete_list[index];
		}
	}
}

/** **************************************************************
 * \brief Keep track of memory allocation for easy deallocation.
 *****************************************************************/
void ScriptParsing::add_to_delete_list(SCRIPT_COMMANDS *ptr)
{
	if(!ptr) return;

	if(delete_list_count < MAX_DELETE_LIST_COUNT) {
		memory_delete_list[delete_list_count] = ptr;
		delete_list_count++;
	}
}

/** **************************************************************
 * \brief Dummy callback function for initialization of
 * function pointers.
 * \param sp The calling ScriptParsing Class
 * \param sc Command data structure pointer to the matching script
 * command.
 * \return 0 = No error<br> \< 0 = Error<br>
 *****************************************************************/
int callback_dummy(ScriptParsing *sp, struct script_cmds *sc)
{
	return 0;
}

/** ********************************************************
 * \brief Determine the length of the string with a count
 * limitation.
 * \return signed integer. The number of characters in the
 * array not to excede count limit.
 ***********************************************************/
int ScriptParsing::str_cnt(char * str, int count_limit)
{
	if(!str || (count_limit < 1))
		return 0;

	int value = 0;

	for(int index = 0; index < count_limit; index++) {
		if(str[index] == 0) break;
		value++;
	}

	return value;
}

/** ********************************************************
 * \brief Trim leading and trailing spaces from string.
 * \param s String to modify
 * \return s modified string.
 ***********************************************************/
std::string &ScriptParsing::trim(std::string &s) {
	char *buffer = (char *)0;
	char *dst    = (char *)0;
	char *end    = (char *)0;
	char *src    = (char *)0;
	long count   = s.size();

	buffer = new char[count + 1];
	if(!buffer) return s;

	memcpy(buffer, s.c_str(), count);
	buffer[count] = 0;

	dst = src = buffer;
	end = &buffer[count];

	while(src < end) {
		if(*src > ' ') break;
		src++;
	}

	if(src > dst) {
		while((dst < end) && (src < end))
			*dst++ = *src++;
		*dst = 0;
	}

	while(end >= buffer) {
		if(*end > ' ') break;
		*end-- = 0;
	}

	s.assign(buffer);

	delete [] buffer;

	return s;
}

/** **************************************************************
 * \brief Remove leading/trailing white spaces and quotes.
 * \param buffer Destination buffer
 * \param limit passed buffer size
 * \return void
 *****************************************************************/
void ScriptParsing::trim(char *buffer, size_t limit)
{
	char *s      = (char *)0;
	char *e      = (char *)0;
	char *dst    = (char *)0;
	size_t count = 0;

	if(!buffer || limit < 1) {
		return;
	}

	for(count = 0; count < limit; count++)
		if(buffer[count] == 0) break;

	if(count < 1) return;

	s = buffer;
	e = &buffer[count-1];

	for(size_t i = 0; i < count; i++) {
		if((*s <= ' ') || (*s == '"')) s++;
		else break;
	}

	while(e > s) {
		if(*e == '"') {	*e-- = 0; break; }
		if(*e > ' ') break;
		if(*e <= ' ') *e = 0;
		e--;
	}

	dst = buffer;
	while(s <= e) {
		*dst++ = *s++;
	}
	*dst = 0;
}
