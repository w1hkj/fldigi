// ----------------------------------------------------------------------------
// record_loader.cxx
//
// Copyright (C) 2013
//		Remi Chateauneu, F4ECW
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

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef __MINGW32__
#  include "compat.h"
#endif

#include "record_loader.h"
#include "record_browse.h"
#include "debug.h"
#include "main.h"
#include "icons.h"
#include "fl_digi.h"
#include "strutil.h"
#include "gettext.h"
#include "network.h"

#include <stdexcept>
#include <fstream>
#include <iostream>

#include <sys/stat.h>

#include "FL/Fl_Double_Window.H"
#include "FL/Fl_Output.H"
#include "FL/fl_ask.H"
#include "FL/Fl_Check_Button.H"

Fl_Double_Window *dlgRecordLoader = (Fl_Double_Window *)0;

/// Loads a file and stores it for later lookup.
int RecordLoaderInterface::LoadAndRegister()
{
	Clear();

	std::string filnam = storage_filename().first;

	time_t cntTim = time(NULL);
	LOG_INFO("Opening:%s", filnam.c_str());

	std::ifstream ifs( filnam.c_str() );

	/// Reuse the same string for each new record.
	std::string input_str ;
	size_t nbRec = 0 ;
	while( ! ifs.eof() )
	{
		if( ! std::getline( ifs, input_str ) ) break;

		/// Comments are legal with # as first character.
		if( input_str[0] == '#' ) continue;

		imemstream str_strm( input_str );
		try
		{
			if( ReadRecord( str_strm ) ) {
				++nbRec;
			} else {
				LOG_WARN( "Cannot process '%s'", input_str.c_str() );
			}
		}
		catch(const std::exception & exc)
		{
			LOG_WARN( "%s: Caught <%s> when reading '%s'",
				base_filename().c_str(),
				exc.what(),
				input_str.c_str() );
			return -1 ;
		}
	}
	ifs.close();
	LOG_INFO( "Read:%s with %d records in %d seconds",
		filnam.c_str(), static_cast<int>(nbRec), static_cast<int>( time(NULL) - cntTim ) );
	return nbRec ;
}

// ----------------------------------------------------------------------------

struct Row
{
	Fl_Output             * m_timestamp ;
	Fl_Check_Button       * m_select ;
	Fl_Output             * m_content_size ;
	Fl_Output             * m_nb_rows ;
	Fl_Button             * m_url ;
	RecordLoaderInterface * m_itf ;

	bool UpdateRow()
	{
		const std::string str = m_itf->Timestamp();
		m_timestamp->value(str.c_str());

		const std::string & strSz = m_itf->ContentSize();
		m_content_size->value(strSz.c_str());
		int nb_recs = m_itf->LoadAndRegister();
		char nb_recs_str[64];
		bool isGood = ( nb_recs >= 0 );

		if( isGood )
			sprintf( nb_recs_str, "%6d", nb_recs );
		else
			strcpy( nb_recs_str, "   N/A" );
		m_nb_rows->value(nb_recs_str);
		const char * strurl = m_itf->Url();

		if( strurl != NULL ) {
			const std::string strnam = m_itf->base_filename();
			m_url->tooltip( strurl );
		}
		if (dlgRecordLoader)
			dlgRecordLoader->damage();
		return isGood ;
	}
};

/// Array all data loaders. It is setup at start time.
static Row * all_recs = NULL ;

/// Number of data loaders, it is a very small integer.
static int dataloader_nb = 0 ;

static const int nb_cols = 5 ;

// ----------------------------------------------------------------------------

/// This is a virtual class, therefore it must have a default constructor.
RecordLoaderInterface::RecordLoaderInterface()
{
	++dataloader_nb ;
	/// We prefer tp use realloc because it is ready before main() is called.
	all_recs = (Row *)realloc( all_recs, dataloader_nb * sizeof( Row ) );

	all_recs[ dataloader_nb - 1 ].m_itf = this ;
}

/// This happens very rarely, so performance is not an issue.
RecordLoaderInterface::~RecordLoaderInterface()
{
	for( int i = 0; i < dataloader_nb; ++i )
	{
		if( all_recs[i].m_itf == this ) {
			memmove( all_recs + i, all_recs + i + 1, sizeof( Row ) * ( dataloader_nb - i - 1 ) );
			--dataloader_nb ;
			return ;
		}
	}
	LOG_ERROR("Inconsistent %d", dataloader_nb );

}

/// This takes only the filename from the complete HTTP or FTP URL, or file path.
std::string RecordLoaderInterface::base_filename() const
{
	const char * pFil = strrchr( Url(), '/' );
	if( pFil == NULL )
		pFil =  Url();
	else
		++pFil ;

	/// This might be an URL so we take only the beginning.
	const char * quest = strchr( pFil, '?' );
	if( quest == NULL ) quest = pFil + strlen(pFil);
	return std::string( pFil, quest );
}

std::pair< std::string, bool > RecordLoaderInterface::storage_filename(bool create_dir) const
{
	/// We check if it is changed, it is not performance-critical.
	if( create_dir ) {
		if( ask_dir_creation( DATA_dir ) ) {
			const char * err = create_directory( DATA_dir.c_str() );
			if( err ) {
				fl_alert("Error:%s",err);
			}
		}
	}

	std::string filnam_data = DATA_dir;
	filnam_data.append(base_filename());
	if( create_dir ) {
		return std::make_pair( filnam_data, false );
	}

	/// This is for a read access.
	std::ifstream ifs( filnam_data.c_str() );
	if( ifs )
	{
		ifs.close();
		return std::make_pair( filnam_data, false );
	}

	if( errno != ENOENT ) {
		LOG_WARN( "Cannot read '%s': %s", filnam_data.c_str(), strerror(errno) );
	}

	// Second try with a file maybe installed by "make install".
	std::string filnam_inst = PKGDATADIR "/" + base_filename();
	LOG_INFO("Errno=%s with %s. Trying %s", strerror(errno), filnam_data.c_str(), filnam_inst.c_str() );
	ifs.open( filnam_inst.c_str() );
	if( ifs )
	{
		ifs.close();
		return std::make_pair( filnam_inst, true );
	}
	/// But the file is not there.
	return std::make_pair( filnam_data, false );
}

std::string RecordLoaderInterface::Timestamp() const
{
	std::string filnam = storage_filename().first;

	struct stat st;
	if (stat(filnam.c_str(), &st) == -1 ) return "N/A";

	struct tm tmLastMod = *localtime( & st.st_mtime );

	char buf[64];
	sprintf(buf, "%d/%d/%d %02d:%02d",
			tmLastMod.tm_year + 1900,
			tmLastMod.tm_mon + 1,
			tmLastMod.tm_mday,
			tmLastMod.tm_hour,
			tmLastMod.tm_min );

	return buf ;
}

std::string RecordLoaderInterface::ContentSize() const
{
	/// It would be faster to cache this result in the object.
	std::string filnam = storage_filename().first;

	struct stat st;
	if (stat(filnam.c_str(), &st) == -1 ) return "      N/A";

	char buf[64];
	sprintf(buf, "%9" PRIuSZ, (size_t)st.st_size );
	return buf ;
}

// ----------------------------------------------------------------------------

static void cb_record_url(Fl_Widget *w, void* ptr)
{
	const RecordLoaderInterface * it = static_cast< const RecordLoaderInterface * >(ptr);
	cb_mnuVisitURL( NULL, const_cast< char * >( it->Url() ) );
}

void DerivedRecordLst::AddRow( int row )
{
	Row * ptRow = all_recs + row ;

	int X,Y,W,H;
	int col=0;
	{
		col_width( col, 110 );
		find_cell(CONTEXT_TABLE, row, col, X, Y, W, H);

		ptRow->m_timestamp = new Fl_Output(X,Y,W,H);
		ptRow->m_timestamp->tooltip( _("Data file creation date") );
	}
	++col;
	{
		col_width( col, 16 );
		find_cell(CONTEXT_TABLE, row, col, X, Y, W, H);

		ptRow->m_select = new Fl_Check_Button(X,Y,W,H);
		ptRow->m_select->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
		ptRow->m_select->value(1);
	}
	++col;
	{
		col_width( col, 80 );
		find_cell(CONTEXT_TABLE, row, col, X, Y, W, H);

		ptRow->m_content_size = new Fl_Output(X,Y,W,H);
		ptRow->m_content_size->tooltip( _("Size in bytes") );
	}
	++col;
	{
		col_width( col, 50 );
		find_cell(CONTEXT_TABLE, row, col, X, Y, W, H);

		ptRow->m_nb_rows = new Fl_Output(X,Y,W,H);
		ptRow->m_nb_rows->tooltip( _("Number of lines in data file") );
	}
	++col;
	{
		col_width( col, 166 );
		find_cell(CONTEXT_TABLE, row, col, X, Y, W, H);
		const char * strurl = ptRow->m_itf->Url();

		ptRow->m_url = NULL ;
		if( strurl != NULL ) {
			const std::string strnam = ptRow->m_itf->base_filename();

			ptRow->m_url = new Fl_Button(X,Y,W,H, strdup(strnam.c_str()) );
			ptRow->m_url->tooltip( strurl );
			ptRow->m_url->callback(cb_record_url, ptRow->m_itf);
		}
	}
	ptRow->UpdateRow();
}

DerivedRecordLst::DerivedRecordLst(int x, int y, int w, int h, const char *title)
: Fl_Table(x,y,w,h,title)
{
	col_header(1);
	col_resize(1);
	col_header_height(25);
	row_header(1);
	row_resize(0);
	row_header_width(105);

	rows(dataloader_nb);
	cols(nb_cols);

	begin();		// start adding widgets to group
	{
		for( int row = 0; row < dataloader_nb; ++row )
		{
			AddRow( row );
		}
	}
	end();
}

DerivedRecordLst::~DerivedRecordLst()
{
}

// Handle drawing all cells in table
void DerivedRecordLst::draw_cell(TableContext context, 
			  int R, int C, int X, int Y, int W, int H)
{
	switch ( context )
	{
	case CONTEXT_STARTPAGE:
		fl_font(FL_HELVETICA, 12);		// font used by all headers
		break;

	case CONTEXT_RC_RESIZE:
	{
		int X, Y, W, H;
		int index = 0;
		for ( int r = 0; r<rows(); r++ )
		{
			for ( int c = 0; c<cols(); c++ )
			{
				if ( index >= children() ) break;
				find_cell(CONTEXT_TABLE, r, c, X, Y, W, H);
				child(index++)->resize(X,Y,W,H);
			}
		}
		init_sizes();			// tell group children resized
		return;
	}

	case CONTEXT_ROW_HEADER:
		fl_push_clip(X, Y, W, H);
		{
			RecordLoaderInterface * it = ( (R >= 0) && ( R < dataloader_nb ) ) ? all_recs[ R ].m_itf : NULL;
			if( it == NULL ) {
				LOG_ERROR("R=%d",R);
				return;
			}

			const char * str = it ? it->Description() : "Unknown" ;

			fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, row_header_color());
			fl_color(FL_BLACK);
			fl_draw(str, X, Y, W, H, FL_ALIGN_CENTER);
		}
		fl_pop_clip();
		return;

	case CONTEXT_COL_HEADER:
		fl_push_clip(X, Y, W, H);
		{
			static const char * col_names[nb_cols] = {
				_("Timestamp"),
				_(" "),
				_("Size"),
				_("# recs"),
				_("WWW"),
			};

			const char * title = ( ( C >= 0 ) && ( C < nb_cols ) ) ? col_names[C] : "?" ;

			fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, col_header_color());
			fl_color(FL_BLACK);
			fl_draw(title, X, Y, W, H, FL_ALIGN_CENTER);
		}
		fl_pop_clip();
		return;
	case CONTEXT_CELL:
		// fl_push_clip(X, Y, W, H);
		return;	// fltk handles drawing the widgets

	default:
		return;
	}
}

void DerivedRecordLst::cbGuiUpdate()
{
	std::string server = inpDataSources->value();
	if( server.empty() ) {
		fl_alert(_("No server selected"));
		return ;
	}

	if( server[server.size()-1] != '/' ) server += '/' ;

	for( int row = 0; row < dataloader_nb; ++row )
	{
		Row * ptrRow = all_recs + row;
		if( ! ptrRow->m_select->value() ) continue ;
		RecordLoaderInterface * it = ptrRow->m_itf;

		std::string url = server + it->base_filename();
		std::string reply ;

		double timeout=600.0;
		// Consider truncating the HTTP header.
		int res = fetch_http_gui(url, reply, timeout );
		LOG_INFO("Loaded %s : %d chars. res=%d", url.c_str(), (int)reply.size(), res );
		if( ! res )
		{
			int ok = fl_choice2(
					_("Could not download %s"),
					_("Continue"),
					_("Stop"),
					NULL,
					url.c_str() );
			if( ok == 1 ) break ;
			continue ;
		}

		static const char *notFound404 = "HTTP/1.1 404 Not Found";
		if( 0 == strncmp( reply.c_str(), notFound404, strlen(notFound404) ) )
		{
			int ok = fl_choice2(
					_("Non-existent URL: %s"),
					_("Continue"),
					_("Stop"),
					NULL,
					url.c_str() );
			if( ok == 1 ) break ;
			continue ;
		}

		/// This creates the directory if necessary;
		std::string filnam = it->storage_filename(true).first;
		std::ofstream ofstrm( filnam.c_str() );
		if( ofstrm )
			ofstrm.write( &reply[0], reply.size() );
		if( ! ofstrm ) {
			int ok = fl_choice2(
					_("Error saving %s to %s:%s"),
					_("Continue"),
					_("Stop"),
					NULL,
					url.c_str(), filnam.c_str(), strerror(errno) );
			if( ok == 1 ) break ;
			continue ;
		}
		ofstrm.close();

		bool isGood = all_recs[row].UpdateRow();
		if( ! isGood ) {
			int ok = fl_choice2(
					_("Error loading %s to %s: %s."),
					_("Continue"),
					_("Stop"),
					NULL,
					url.c_str(), filnam.c_str(), strerror(errno) );
			if( ok == 0 ) break ;
			continue ;
		}
	}
	btnDataSourceUpdate->value(0);
}

void DerivedRecordLst::cbGuiReset()
{
	fprintf(stderr, "%s\n", __FUNCTION__ );

	for( int row = 0; row < dataloader_nb; ++row )
	{
		Row * ptrRow = all_recs + row;
		if( ! ptrRow->m_select->value() ) continue ;
		RecordLoaderInterface * it = ptrRow->m_itf;

		std::pair< std::string, bool > stofil_pair = it->storage_filename(true);

		it->Clear();

		const char * stofil = stofil_pair.first.c_str() ;
		if( stofil_pair.second ) {
			fl_alert("Cannot erase installed data file %s", stofil );
			continue ;
		} else {
			LOG_INFO("Erasing %s", stofil );
			int res = ::remove( stofil );
			if( ( res != 0 ) && ( res != ENOENT ) ) {
				fl_alert("Error erasing data file %s:%s", stofil, strerror(errno) );
				continue ;
			}
		all_recs[row].UpdateRow();
		}
	}
}

// ----------------------------------------------------------------------------

/// Necessary because in a Fl_Menu, a slash has a special meaning.
static std::string fl_escape( const char * str )
{
	std::string res ;
	for( char ch ; ( ch = *str ) != '\0' ; ++str )
	{
		if( ch == '/' ) res += '\\';
		res += ch  ;
	}
	return res ;
}

static void fl_input_add( const char * str )
{
	inpDataSources->add( fl_escape( str ).c_str() );
}

void createRecordLoader()
{
	if (dlgRecordLoader) return; 
	dlgRecordLoader = make_record_loader_window();
	fl_input_add("http://www.w1hkj.com/support_files/");
	fl_input_add("http://primhillcomputers.com/fldigi/data");

	inpDataSources->value(0);
}
// ----------------------------------------------------------------------------
