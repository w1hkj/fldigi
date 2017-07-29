// ----------------------------------------------------------------------------
// test_signal.h
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

#ifndef TEST_SIGNAL_H
#define TEST_SIGNAL_H

#include <config.h>

#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <cstdlib>

#ifdef __MINGW32__
#  include <windows.h>
#endif

#include "testsigs.h"

extern Fl_Double_Window* test_signal_window;

extern void show_testdialog(void);

#endif
