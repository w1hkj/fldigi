// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
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

#ifndef RIGCAT_H_
#define RIGCAT_H_

#ifndef RIGCATTEST
#  error FIXME: file should not have been included
#endif

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <iostream>
#include <fstream>
#include <string>

extern Fl_Double_Window *window;
extern char *homedir;
extern string xmlfname;

extern void MilliSleep(long msecs);

#endif
