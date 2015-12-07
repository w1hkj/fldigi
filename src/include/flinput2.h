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

#ifndef FL_INPUT2_
#define FL_INPUT2_

#include <FL/Fl_Input.H>

class Fl_Input2 : public Fl_Input
{
private:
	int 	ascii_cnt; // restart the numeric keypad entries.
	int		ascii_chr; // character value of ASCII > 0x80
	int		handle_key_ascii(int key);
	char	*utf8text;
	int		utf8cnt;

public:
	Fl_Input2(int x, int y, int w, int h, const char* l = 0);
	int handle(int event);
};

#endif // FL_INPUT2_
