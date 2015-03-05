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

#ifndef _run_script_h
#define _run_script_h

extern int process_callsign_info(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_name_info(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_qth_info(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_locator_info(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_antenna_info(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

extern int process_use_oss_audio_device(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_oss_audio_device_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_use_port_audio_device(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_capture_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_playback_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_use_pulse_audio_device(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_pulse_audio_device_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

extern int process_audio_device_sample_rate_capture(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_audio_device_sample_rate_playback(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_audio_device_converter(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rx_ppm(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_tx_ppm(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_tx_offset(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

extern int process_modem_signal_left_right(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_reverse_left_right(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_ptt_tone_right_channel(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_cw_qsk_right_channel(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_pseudo_fsk_right_channel(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

extern int process_wave_file_sample_rate(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

extern int process_hrdw_ptt_right_audio_channel(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hrdw_ptt_sep_serial_port(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hrdw_ptt_sep_serial_port_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hrdw_ptt_sep_serial_port_rts(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hrdw_ptt_sep_serial_port_dtr(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hrdw_ptt_sep_serial_port_rts_v(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hrdw_ptt_sep_serial_port_dtr_v(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hrdw_ptt_start_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hrdw_ptt_end_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hrdw_ptt_uhrouter(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hrdw_ptt_initialize(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

extern int process_use_rigcat(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_desc_file(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_device_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_retries(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_retry_interval(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_write_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_init_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_baud_rate(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_stop_bits(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_commands_echoed(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_toggle_rts_ptt(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_restore_on_close(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_cat_command_ptt(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_dtr_12v(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_rts_12v(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_hrdwr_flow(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_vsp_enable(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rigcat_initialize(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

extern int process_use_hamlib(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_rig(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_device_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_retries(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_retry_interval(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_write_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_post_write_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_baud_rate(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_stop_bits(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_sideband(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_ptt_hl_command(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_dtr_12(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_rts_12(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_rts_cts_flow(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_xon_xoff_flow(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_advanced_config(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_hamlib_initialize(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

extern int process_use_xml_rpc(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_xml_rpc_mode_bw_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_xml_rpc_initialize(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

extern int process_io_kiss_ip_address(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_kiss_io_port_no(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_kiss_o_port_no(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_kiss_dual_port(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_kiss_busy_channel(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_kiss_continue_after(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_kiss_kpsql_atten(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_arq_ip_address(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_arq_io_port_no(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_xmlrpc_ip_address(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_xmlrpc_io_port_no(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_lock(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_active_port(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_ax25_decode(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_io_csma(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

extern int process_misc_nbems_state(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_misc_nbems_open_flmsg(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_misc_nbems_open_msg(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_misc_nbems_open_brwsr(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_misc_nbems_flmsg_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_misc_nbems_timeout(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

extern int process_rsid_notify(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rsid_search_bp(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rsid_mark_prev(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rsid_detector(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rsid_alert_dialog(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rsid_tx_freq_lock(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rsid_freq_change(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rsid_allow_errors(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rsid_sql_open(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rsid_pretone(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rsid_end_xmt_id(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_video_tx_id_mode(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_video_tx_vid_txt(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_video_txt_input(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_video_small_font(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_video_500hz(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_video_width_limit(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rsid_char_per_row(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_cw_callsign(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_cw_speed(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

// FLDIGI main windows widgets

extern int process_rig_freq(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rig_mode(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_wf_hz_offset(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rx_rsid(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_tx_rsid(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_spot(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_rev(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_afc(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_lock(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_sql(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_kpsql(ScriptParsing *sp, SCRIPT_COMMANDS *sc);
extern int process_modem(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

extern int process_load_macro(ScriptParsing *sp, SCRIPT_COMMANDS *sc);

#endif
