// ----------------------------------------------------------------------------
// test_signal.cxx
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

#include "test_signal.h"

Fl_Double_Window* test_signal_window = (Fl_Double_Window *)0;

void show_testdialog(void)
{
	if (!test_signal_window)
		test_signal_window = make_testdialog();
	test_signal_window->show();
}

