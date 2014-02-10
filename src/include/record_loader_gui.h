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

#ifndef RECORD_LOADER_GUI_H
#define RECORD_LOADER_GUI_H

#include <FL/Fl_Table.H>
#include <FL/Fl_Double_Window.H>

class RecordLoaderInterface ;

class DerivedRecordLst : public Fl_Table
{
	DerivedRecordLst();
	DerivedRecordLst( const DerivedRecordLst & );
	DerivedRecordLst & operator=( const DerivedRecordLst & );
public:
	DerivedRecordLst(int, int, int, int, const char * title = 0);
	virtual ~DerivedRecordLst();

	static void cbGuiUpdate();
	static void cbGuiReset();

	void AddRow(int R);
	void DrawRow(int R);
protected:
	void draw_cell(TableContext context,  		// table cell drawing
    		   int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0);
};

#endif // RECORD_LOADER_GUI_H
