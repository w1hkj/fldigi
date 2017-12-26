// ----------------------------------------------------------------------------
// spectrum_viewer.cxx  --  spectrum dialog
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

#ifndef SPECTRUM_VIEWER_H
#define SPECTRUM_VIEWER_H

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Output.H>

#include "spectrum.h"

#include "digiscope.h"

//extern Digiscope	*fftscope;
extern spectrum			*fftscope;
extern Fl_Box			*pause_label;
extern Fl_Output		*values;
extern Fl_Output		*db_diffs;
extern Fl_Output		*f_diffs;

extern Fl_Double_Window	*spectrum_viewer;

extern void open_spectrum_viewer();
extern void close_spectrum_viewer();
extern void recenter_spectrum_viewer();

#endif

