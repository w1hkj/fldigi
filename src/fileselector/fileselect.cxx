// ----------------------------------------------------------------------------
//
// fileselect.cxx -- file selector front end
//
// Copyright (C) 2008-2009
//		Stelios Bounanos, M0GLD
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

#include <string>
#include <cstdlib>
#include <libgen.h>

#include "config.h"

#include "fileselect.h"
#include "debug.h"

#include <FL/fl_ask.H>
#include <FL/Fl_Native_File_Chooser.H>

/**
  \class Fl_Native_File_Chooser

  This class lets an FLTK application easily and consistently access 
  the operating system's native file chooser. Some operating systems 
  have very complex and specific file choosers that many users want 
  access to specifically, instead of FLTK's default file chooser(s). 

  In cases where there is no native file browser, FLTK's own file browser
  is used instead.

  To use this widget correctly, use the following include in your code:
  \code
  #include <FL/Fl_Native_File_Chooser.H>
  \endcode
  Do not include the other Fl_Native_File_Choser_XXX.H files in your code;
  those are platform specific files that will be included automatically
  depending on your build platform.

  The following example shows how to pick a single file:
  \code
  // Create and post the local native file chooser
  #include <FL/Fl_Native_File_Chooser.H>
  [..]
  Fl_Native_File_Chooser fnfc;
  fnfc.title("Pick a file");
  fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
  fnfc.filter("Text\t*.txt\n"
              "C Files\t*.{cxx,h,c}");
  fnfc.directory("/var/tmp");           // default directory to use
  // Show native chooser
  switch ( fnfc.show() ) {
    case -1: printf("ERROR: %s\n", fnfc.errmsg());    break;  // ERROR
    case  1: printf("CANCEL\n");                      break;  // CANCEL
    default: printf("PICKED: %s\n", fnfc.filename()); break;  // FILE CHOSEN
  }
  \endcode

  <B>Platform Specific Caveats</B>

  - Under X windows, it's best if you call Fl_File_Icon::load_system_icons()
    at the start of main(), to enable the nicer looking file browser widgets.
    Use the static public attributes of class Fl_File_Chooser to localize
    the browser.
  - Some operating systems support certain OS specific options; see 
    Fl_Native_File_Chooser::options() for a list.

  \image html Fl_Native_File_Chooser.png "The Fl_Native_File_Chooser on different platforms."
  \image latex Fl_Native_File_Chooser.png "The Fl_Native_File_Chooser on different platforms" width=14cm

  enum Type {
    BROWSE_FILE = 0,			///< browse files (lets user choose one file)
    BROWSE_DIRECTORY,			///< browse directories (lets user choose one directory)
    BROWSE_MULTI_FILE,			///< browse files (lets user choose multiple files)
    BROWSE_MULTI_DIRECTORY,		///< browse directories (lets user choose multiple directories)
    BROWSE_SAVE_FILE,			///< browse to save a file
    BROWSE_SAVE_DIRECTORY		///< browse to save a directory
  };
  enum Option {
    NO_OPTIONS     = 0x0000,		///< no options enabled
    SAVEAS_CONFIRM = 0x0001,		///< Show native 'Save As' overwrite confirm dialog (if supported)
    NEW_FOLDER     = 0x0002,		///< Show 'New Folder' icon (if supported)
    PREVIEW        = 0x0004		///< enable preview mode
  };

IMPORTANT NOTICE:

The filter type must be terminated with a '\n' on OS X or the application crashes with a Bus timeout

*/

using namespace std;

namespace FSEL {

string filename;

void create(void) {};
void destroy(void) {};

string stitle, sfilter, sdef;

const char* select(const char* title, const char* filter, const char* def, int* fsel)
{
	Fl_Native_File_Chooser native;

	stitle.clear();
	sfilter.clear();
	sdef.clear();
	if (title) stitle.assign(title);
	if (filter) sfilter.assign(filter);
	if (def) sdef.assign(def);
	if (!sfilter.empty() && sfilter[sfilter.length()-1] != '\n') sfilter += '\n';

	if (!stitle.empty()) native.title(stitle.c_str());
	native.type(Fl_Native_File_Chooser::BROWSE_FILE);
	if (!sfilter.empty()) native.filter(sfilter.c_str());
	native.options(Fl_Native_File_Chooser::PREVIEW);
	if (!sdef.empty()) native.preset_file(sdef.c_str());

	filename.clear();
	switch ( native.show() ) {
		case -1: 
			LOG_INFO("ERROR: %s\n", native.errmsg()); // Error fall through
		case  1: 
			return 0;
			break;
		default:
			if ( native.filename() ) {
				filename = native.filename();
			} else {
				filename = "";
		}
		break;
	}

	if (fsel)
		*fsel = native.filter_value();

	return filename.c_str();
}

const char* saveas(const char* title, const char* filter, const char* def, int* fsel)
{
	Fl_Native_File_Chooser native;

	stitle.clear();
	sfilter.clear();
	sdef.clear();
	if (title) stitle.assign(title);
	if (filter) sfilter.assign(filter);
	if (def) sdef.assign(def);
	if (!sfilter.empty() && sfilter[sfilter.length()-1] != '\n') sfilter += '\n';

	if (!stitle.empty()) native.title(stitle.c_str());
	native.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	if (!sfilter.empty()) native.filter(sfilter.c_str());
	native.options(Fl_Native_File_Chooser::NEW_FOLDER || Fl_Native_File_Chooser::SAVEAS_CONFIRM);
	if (!sdef.empty()) native.preset_file(sdef.c_str());

	filename.clear();
	switch ( native.show() ) {
		case -1: LOG_INFO("ERROR: %s\n", native.errmsg()); break;	// ERROR
		case  1: break;		// CANCEL
		default: 
			if ( native.filename() ) {
				filename = native.filename();
			} else {
				filename = "";
		}
		break;
	}

	if (fsel)
		*fsel = native.filter_value();

	return filename.c_str();

}

const char* dir_select(const char* title, const char* filter, const char* def)
{
	Fl_Native_File_Chooser native;

	stitle.clear();
	sfilter.clear();
	sdef.clear();
	if (title) stitle.assign(title);
	if (filter) sfilter.assign(filter);
	if (def) sdef.assign(def);
	if (!sfilter.empty() && sfilter[sfilter.length()-1] != '\n') sfilter += '\n';

	if (!stitle.empty()) native.title(stitle.c_str());
	native.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
	if (!sfilter.empty()) native.filter(sfilter.c_str());
	native.options(Fl_Native_File_Chooser::NO_OPTIONS);
	if (!sdef.empty()) native.directory(sdef.c_str());

	filename.clear();
	switch ( native.show() ) {
		case -1: LOG_INFO("ERROR: %s\n", native.errmsg()); break;	// ERROR
		case  1: break;		// CANCEL
		default:
			if ( native.filename() ) {
				filename = native.filename();
			} else {
				filename = "";
		}
		break;
	}

	return filename.c_str();
}

} // FSEL
