// ---------------------------------------------------------------------
//
//      xml_server.h, a part of flarq
//
// Copyflarqht (C) 2016
//               Dave Freese, W1HKJ
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the program; if not, write to the
//
//  Free Software Foundation, Inc.
//  51 Franklin Street, Fifth Floor
//  Boston, MA  02110-1301 USA.
//
// ---------------------------------------------------------------------

#ifndef XML_SERVER_H
#define XML_SERVER_H

#include <fstream>
#include <vector>
#include <string>

#include <math.h>
#ifndef WIN32
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#endif

#include "status.h"

#include <FL/fl_show_colormap.H>
#include <FL/fl_ask.H>

extern void start_xml_server(int port = 12345);
extern void exit_server();

extern bool xml_rx_text_ready;

#endif
