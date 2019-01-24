// ----------------------------------------------------------------------------
// winkeyer.h  --  Interface to k1el WinKeyer hardware
//
// Copyright (C) 2017
//		Dave Freese, W1HKJ
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

#ifndef _WINKEYER_H
#define _WINKEYER_H

extern void WK_change_btn_swap();
extern void WK_change_btn_auto_space();
extern void WK_change_btn_ct_space();
extern void WK_change_btn_paddledog();
extern void WK_change_btn_cut_zeronine();
extern void WK_change_btn_paddle_echo();
extern void WK_change_btn_serial_echo();
extern void WK_change_btn_sidetone_on();
extern void WK_change_btn_tone_on();
extern void WK_change_btn_ptt_on();
extern void WK_change_cntr_min_wpm();
extern void WK_change_cntr_rng_wpm();
extern void WK_change_cntr_farnsworth();
extern void WK_change_cntr_cmd_wpm();
extern void WK_change_cntr_ratio();
extern void WK_change_cntr_comp();
extern void WK_change_cntr_first_ext();
extern void WK_change_cntr_sample();
extern void WK_change_cntr_weight();
extern void WK_change_cntr_leadin();
extern void WK_change_cntr_tail();
extern void WK_change_choice_keyer_mode();
extern void WK_change_choice_hang();
extern void WK_change_choice_sidetone();
extern void WK_change_choice_output_pins();
extern void WK_use_pot_changed();
extern void WKCW_connect(bool start);

extern int  WK_send_char(int c);
extern void WK_set_wpm();
extern void WK_tune(bool on);

extern void WK_exit();

extern void  WKFSK_init();
extern void WKFSK_connect(bool start);
extern void WKFSK_send_char(int ch);

#endif
