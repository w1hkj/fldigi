//
// "$Id: fl_file_dir.cxx 4288 2005-04-16 00:13:17Z mike $"
//
// File chooser widget for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

#include <FL/filename.H>
#include <FL/fl_ask.H>

#include "File_Selector.h"
#include "FileString.h"


static File_Selector	*fc = (File_Selector *)0;
static void		(*current_callback)(const char*) = 0;
static const char	*current_label = fl_ok;


// Do a file chooser callback...
static void callback(File_Selector *, void*) {
  if (current_callback && fc->value())
    (*current_callback)(fc->value());
}


// Set the file chooser callback
void File_Selector_callback(void (*cb)(const char*)) {
  current_callback = cb;
}


// Set the "OK" button label
void File_Selector_ok_label(const char *l) {
  if (l) current_label = l;
  else current_label = fl_ok;
}


//
// 'File_Selector()' - Show a file chooser dialog and get a filename.
//

char *					// O - Filename or NULL
File_Select(const char *message,	// I - Message in titlebar
                const char *pat,	// I - Filename pattern
		const char *fname,	// I - Initial filename selection
		int        relative) {	// I - 0 for absolute path
  static char	retname[1024];		// Returned filename

  if (!fc) {
    if (!fname || !*fname) fname = ".";

    fc = new File_Selector(fname, pat, File_Selector::CREATE, message);
    fc->callback(callback, 0);
  } else {
    fc->type(File_Selector::CREATE);
    fc->filter(pat);
    fc->label(message);

    if (!fname || !*fname) {
      if (fc->filter() != pat && (!pat || !fc->filter() ||
          strcmp(pat, fc->filter())) && fc->value()) {
	// if pattern is different, remove name but leave old directory:
	strlcpy(retname, fc->value(), sizeof(retname));

	char *p = strrchr(retname, '/');

        if (p) {
	  // If the filename is "/foo", then the directory will be "/", not
	  // ""...
	  if (p == retname)
	    retname[1] = '\0';
	  else
	    *p = '\0';
	}

	// Set the directory...
	fc->directory(retname);
      }
    }
    else
      fc->value(fname);
  }

  fc->ok_label(current_label);
  fc->show();

  while (fc->shown())
    Fl::wait();

  if (fc->value() && relative) {
    fl_filename_relative(retname, sizeof(retname), fc->value());

    return retname;
  } else if (fc->value()) return (char *)fc->value();
  else return 0;
}


//
// 'fl_dir_chooser()' - Show a file chooser dialog and get a directory.
//

char *					// O - Directory or NULL
Dir_Chooser(const char *message,	// I - Message for titlebar
               const char *fname,	// I - Initial directory name
	       int        relative)	// I - 0 for absolute
{
  static char	retname[1024];		// Returned directory name

  if (!fc) {
    if (!fname || !*fname) fname = ".";

    fc = new File_Selector(fname, "*", File_Selector::CREATE |
                                         File_Selector::DIRECTORY, message);
    fc->callback(callback, 0);
  } else {
    fc->type(File_Selector::CREATE | File_Selector::DIRECTORY);
    fc->filter("*");
    if (fname && *fname) fc->value(fname);
    fc->label(message);
  }

  fc->show();

  while (fc->shown())
    Fl::wait();

  if (fc->value() && relative) {
    fl_filename_relative(retname, sizeof(retname), fc->value());

    return retname;
  } else if (fc->value()) return (char *)fc->value();
  else return 0;
}


//
// End of "$Id: fl_file_dir.cxx 4288 2005-04-16 00:13:17Z mike $".
//
