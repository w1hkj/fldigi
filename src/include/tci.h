// ----------------------------------------------------------------------------
// tci.h  --  ExpertSDR3 / Zeus TCI client (rig + RX/TX audio)
//
// Copyright (C) 2026
//		Chris, KC2RGW
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

#ifndef TCI_H_
#define TCI_H_

class Fl_Widget;

// Start the background client. Safe to call once from the FLTK main thread.
void tci_start();

// Open the setup dialog (Configure menu).
void tci_show_dialog();
void cb_mnu_tci(Fl_Widget*, void*);

// Save the setup and drop the socket so the client reconnects.
// An empty host or port keeps the value already saved. rx below 0 becomes 0.
void tci_apply(bool enable, bool audio, const char* host, const char* port, int rx);

// Status line, such as "TCI disabled" or "TCI connecting to 127.0.0.1:50001".
void tci_copy_status(char* buf, int cap);

// True when TCI rig control is enabled and the WebSocket is up.
bool tci_is_active();

// User asked for modem audio over TCI (independent of the socket being up).
bool tci_audio_wanted();

void tci_set_ptt(bool on);
void tci_set_freq(unsigned long hz);
void tci_set_mode(const char* fldigi_mode);
void tci_set_filter(const char* label);

#endif
