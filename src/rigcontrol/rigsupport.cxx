// ----------------------------------------------------------------------------
// rigsupport.cxx - support functions file
//
// Copyright (C) 2007-2009
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2009
//		Stelios Bounanos, M0GLD
// Copyright (C) 2013
//              Remi Chateauneu, F4ECW
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

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <iterator>
#include <cstring>
#include <memory>

#include "logsupport.h"
#include "rigsupport.h"
#include "rigxml.h"
#include "rigio.h"
#include "threads.h"
#include "record_loader.h"
#include "catalog.h"
#include "main.h"
#include "fl_digi.h"
#include "fileselect.h"
#include "trx.h"
#include "strutil.h"
#include "icons.h"
#include "coordinate.h"

#include "configuration.h"

#include "globals.h"

#include "debug.h"

#include "gettext.h"

#include <FL/Fl_Choice.H>

LOG_FILE_SOURCE(debug::LOG_RIGCONTROL);

using namespace std;

string windowTitle;

// ----------------------------------------------------------------------------

/// This models a frequency as saved in the frequencies list.
struct qrg_mode_extended_t : public qrg_mode_t
{
	std::string m_rmode;
	std::string m_name;
	std::string m_description;

	short       m_time_on;  // 0000 to 2359
	short       m_time_off; // 0000 to 2359

	typedef short FreqSourceId ;
	static const FreqSourceId FREQS_UNKNOWN = 0;

	/// Colour associated to the frequencies in our file.
	static const FreqSourceId FREQS_NONE = 1 ;

	/// The optional location are used to filter the displayed sttaions.
	CoordinateT::Pair m_coordinates ;

	/// Tells where the data come from: File frequencies2.txt, EIBI, MWList etc...
	FreqSourceId m_data_source ;

	/// Used to select in STL algorithms, freqs of a given source only.
	bool is_source(FreqSourceId src) const { return m_data_source == src ; }

	/// Empty strings take no space, so no init.
	qrg_mode_extended_t(long long rfcarr=0)
	: qrg_mode_t(rfcarr)
	, m_data_source(FREQS_UNKNOWN)
	{
		/// GCC has strings reference counting.
		static const std::string none_mode("NONE");

		/// Very quick copy.
		m_rmode = none_mode ;
	}

	/// This frequency will be saved to file because FREQS_NONE.
	qrg_mode_extended_t(
		long long           rfcarr,
		const char        * rmd,
		int                 carr,
		const trx_mode    & trxmd,
		const std::string & nam,
		const std::string & descr )
	: qrg_mode_t( rfcarr, carr, trxmd )
	, m_rmode(rmd)
	, m_name(nam)
	, m_description(descr)
	, m_data_source(FREQS_NONE) {}

	/// Returns the color associated to a data source.
	static int disp_color(FreqSourceId source)
	{
		static const int colors[] = {
			FL_RED,
			FL_BLACK,
			FL_BLUE,
			FL_MAGENTA,
			FL_DARK_BLUE,
			FL_DARK_GREEN,
			FL_DARK_CYAN,
			FL_DARK_MAGENTA,
			FL_DARK_RED,
			FL_DARK_YELLOW,
			FL_CYAN,
			FL_GREEN,
			FL_YELLOW
		};
		static const size_t nb = sizeof(colors)/sizeof(*colors);

		/// Should never happen.
		if( source < 0 ) return FL_RED;
		return colors[ source % nb ];
	}

	/// Comparison based on the frequency only. Used for pointing to the right line.
	static bool cmp_rfcarrier_only( const qrg_mode_extended_t & a, const qrg_mode_extended_t & b )
	{
		return a.rfcarrier < b.rfcarrier ;
	}

	/// Used to eliminate duplicates in frequencies lists.
	bool similar( const qrg_mode_extended_t & other ) const
	{
		return  ( rfcarrier   == other.rfcarrier )
		&&	( mode        == other.mode )
		&&	( m_rmode     == other.m_rmode )
		&&	( levenshtein( m_name, other.m_name ) < 2 );
	}

	/// This allows to pass directly a buffer for display. No memory allocation needed.
	typedef char buffer_type[256] ;

	/// Writes a descriptive string in this buffer of a given length.
	void str(buffer_type buf) const
	{
		/// Longest string is 12 chars. "NULL" replaced by empty string.
		const char * modstr = ( mode == 0 )
			? ""
			: ( mode > 0 ) && (mode < NUM_MODES)
				? mode_info[mode].sname
				: "?";

	/// FLTK allows to change the text color.
	#define FMT_COLOR "@C%d"
		int clr = disp_color( m_data_source );

		/// If the name is too long, we pack name and description.
		const char * fmtstr = ( m_name.size() >= 12 )
		?	m_description.size() == 0
			?	FMT_COLOR "%11.3lf %-7s%-9s %5d %s%s"
			:	FMT_COLOR "%11.3lf %-7s%-9s %5d %s, %s"
		:		FMT_COLOR "%11.3lf %-7s%-9s %5d %-12.12s %s";

		const char * tmp_rmode = m_rmode == "NULL" ? "" : m_rmode.c_str();

		/// Max realistic value is 1 GHz. Beware of rounding.
		double rfcarr_disp = rfcarrier * 0.001 + 0.00049;

		/// snprintf faster than iostream.
		snprintf(
			buf,
			sizeof(buffer_type),
			fmtstr,
			clr,
			rfcarr_disp,
			tmp_rmode,
			modstr,
			carrier,
			m_name.c_str(),
			m_description.c_str() );
	}
private:
	/// Tells which frequencies are displayed or not.
	friend class visibility_context ;

public:

	/// It is stored in a vector sorted by frequency.
	bool operator<(const qrg_mode_extended_t& rhs) const
	{
		int ret ;
		( ret = rfcarrier - rhs.rfcarrier      ) ||
		( ret =	mode - rhs.mode                ) ||
		( ret =	carrier - rhs.carrier          ) ||
		( ret =	m_rmode.compare( rhs.m_rmode ) ) ||
		( ret =	m_name.compare( rhs.m_name ) );
		return ret < 0 ;
	}

	/// Compare first the integer members because it is faster.
	bool operator==(const qrg_mode_extended_t& rhs) const
	{
		return rfcarrier == rhs.rfcarrier
		&&     mode      == rhs.mode
		&&     carrier   == rhs.carrier
		&&     m_rmode   == rhs.m_rmode
		&&     m_name    == rhs.m_name ;
	}

	/// For saving to frequencies list file.
	friend std::ostream& operator<<(std::ostream& s, const qrg_mode_extended_t& m)
	{
		s << m.rfcarrier << ' ' << m.m_rmode << ' ' << m.carrier << ' ' << mode_info[m.mode].sname;

		s << ' ';
		string_escape(s,m.m_name);
		s << ' ';
		string_escape(s,m.m_description);

		return s ;
	}

	/// Reads from the private frequencies file.
	friend std::istream& operator>>(std::istream& s, qrg_mode_extended_t& m)
	{
		string sMode;
		int mnbr;

		/// The frequencies should be integer, but we tolerate floating point values.
		double dbl_rfcarrier, dbl_carrier ;
		s >> dbl_rfcarrier >> m.m_rmode >> dbl_carrier >> sMode;
		m.rfcarrier = dbl_rfcarrier ;
		m.carrier = dbl_carrier ;

		/// handle case for reading older type of specification string
		if (sscanf(sMode.c_str(), "%d", &mnbr)) {
			m.mode = mnbr;
		} else {
			m.mode = trx_mode_lookup( sMode );
			if( m.mode == NUM_MODES ) m.mode = MODE_PSK31 ;
		}

		/// Now there might be optional information.
		string_unescape(s,m.m_name);
		string_unescape(s,m.m_description);

		/// These frequencies must always be saved.
		m.m_data_source = qrg_mode_extended_t::FREQS_NONE;

		return s;
	}

};

/// All frequencies loaded from various sources, whether they are currently displayed or not.
typedef vector<qrg_mode_extended_t> FreqListType ;

/// Contains all the frequencies displayed in the frequency list.
static FreqListType freqlist;

// ----------------------------------------------------------------------------

/// Stores the global parameters deciding if a frequency is visibile or not.
class visibility_context
{
	/// One bit per source of frequences.
	typedef std::vector<bool> bit_mask_t ;

	/// Maximum distance of displayed frequencies wrt user locator.
	double m_dist_maxi ;

	/// Tells which data sources are currently displayed.
	bit_mask_t m_visible_sources;
public:
	visibility_context()
	: m_dist_maxi(1000000.0) /// No distance like that on the Earth, in kilometers.
	, m_visible_sources(33) /// Ugly BUT OK FOR THE MOMENT !!! Any big number OK !!!!!!!!!!!!!!!!!!!!!
	{
		m_visible_sources[qrg_mode_extended_t::FREQS_UNKNOWN] = true;
		m_visible_sources[qrg_mode_extended_t::FREQS_NONE] = true; // Black.
	}

	/// This returns a new unique source number, which is set to false.
	qrg_mode_extended_t::FreqSourceId allocate_source()
	{
		/// TODO: RecordMWList::freq_source could be dynamic as well.
		static qrg_mode_extended_t::FreqSourceId highestSource = 3 ;
		++highestSource ;
		/// Maybe the vector is not big enough. By default, all sources are IN-visible.
		m_visible_sources.resize( highestSource + 1, false );
		LOG_INFO("highestSource=%d", (int)highestSource );
		return highestSource ;
	}

	/// We can choose which frequencies are displayed by on the file their are loaded from.
	void switch_visible_source( qrg_mode_extended_t::FreqSourceId source )
	{
		if(source >= (int)m_visible_sources.size())
		{
			m_visible_sources.resize( source + 1, false );
		}
		bool statusVisible = m_visible_sources[ source ];
		LOG_INFO("source=%d state=%d", (int)source, (int)statusVisible );
		/// Checks that element is accessible.
		m_visible_sources[ source ] = ! statusVisible;
	}

	/// Display emitters only if closer than X kilometers.
	void set_max_distance( double new_dist_maxi, bool if_lower = false )
	{
		LOG_INFO("new_dist_maxi=%f", new_dist_maxi );
		/// This flag allows to increase only the distance.
		if(if_lower)
			m_dist_maxi = std::max( m_dist_maxi, new_dist_maxi );
		else
			m_dist_maxi = new_dist_maxi ;
	}

	/// Tells if each line is visible or not. Do not change the browser.
	void updateVisibility()
	{
		bool user_coo_available = true ;

		CoordinateT::Pair user_coordinates;

		/// If the userlocator is empty or invalid, do not take into account.
		try
		{
			user_coordinates = CoordinateT::Pair( progdefaults.myLocator );
		}
		catch( const std::exception & )
		{
			user_coo_available = false ;
		}

		int nb = freqlist.size();
		int shown = 0;
		LOG_INFO("nb=%d",nb);
		for (int i = 0; i < nb; i++)
		{
			bool is_visib_freq ;
			const qrg_mode_extended_t & refQrg = freqlist[i];

			if( ! m_visible_sources[ refQrg.m_data_source ] )
				/// If we do not want to see any frequency from this source.
				is_visib_freq = false ;
			else if( ! user_coo_available )
				/// All frequencies visible if not valid user locator.
				is_visib_freq = true ;
			else if( ! refQrg.m_coordinates.is_valid() )
				is_visib_freq = true ;
			else
			{
				/// Station is too far from user ?
				double dist_to_user = refQrg.m_coordinates.distance( user_coordinates );
				is_visib_freq = dist_to_user < m_dist_maxi ;
			}
			if( is_visib_freq )
			{
				++shown;
				qso_opBrowser->show( i + 1 );
			}
			else
				/// Slow if we are at the beginning of the list.
				qso_opBrowser->hide( i + 1 );
		}
		LOG_INFO("shown=%d",shown);
	};
}; // visibility_context

/// This holds the criterias to choose the frequencies to show/hide in the list.
static visibility_context visi_ctxt ;

// ----------------------------------------------------------------------------

static void FreqListToBrowser();

/// Base class of each line in the frequency list catalog.
class FrequencyListBase
{
	bool m_triedToLoad ;

public:
	FrequencyListBase()
	: m_triedToLoad(false) {}

	virtual qrg_mode_extended_t::FreqSourceId frequency_source_id() = 0 ;
	virtual int loadFrequencyFile() = 0;

	void switchFrequencyCheckBox()
	try
	{
		qrg_mode_extended_t::FreqSourceId source = frequency_source_id();
		visi_ctxt.switch_visible_source( source );
		LOG_INFO("freq_source=%d", (int)source );

		if( m_triedToLoad == false ) {
			m_triedToLoad = true ;
			int nbRec = loadFrequencyFile();
			LOG_INFO("Adding %d elements to %d", nbRec, (int)freqlist.size() );
			FreqListToBrowser();
		}
		else
		{
			visi_ctxt.updateVisibility();
		}
	}
	catch( const std::exception & exc )
	{
		LOG_WARN("Caught %s when loading file", exc.what() );
	}
};

// ----------------------------------------------------------------------------

/// Similar to Fl_Check_Browser but with colored lines and each click can be handled.
class Sources_Check_Browser : public Fl_Browser_
{
	/// One such structure per line. The adresses must be stable.
	struct cb_item {
		FrequencyListBase * m_frqLstBase;
		cb_item           * m_prev;
		cb_item           * m_next;
		std::string         m_textItem;	
		Fl_Color            m_textColor ;
		bool                m_checked;
		bool                m_selected;

		cb_item()
		: m_frqLstBase(NULL)
		, m_prev(NULL)
		, m_next(NULL) {}
	};

	/// Items are a linked list because the base class needs stable pointers.
	mutable cb_item * m_itemsFirst ;
	mutable cb_item * m_itemsLast ;

	/// Required routines for Fl_Browser_ subclass
	void *item_first() const { return m_itemsFirst; }

	void *item_next(void *l) const {
		cb_item * itm = (cb_item *)l;
		return itm->m_next;
	}
	void *item_prev(void *l) const {
		cb_item * itm = (cb_item *)l;
		return itm->m_prev;
	}

	int item_height(void *) const { return textsize() + 2; }

	int check_size() const { return textsize()-2; }

	int item_width(void *v) const {
		int tmpCheckSize = check_size();
		fl_font(textfont(), textsize());
		return int(fl_width(((cb_item *)v)->m_textItem.c_str())) + tmpCheckSize + 8;
	}

	void item_draw(void *v, int X, int Y, int, int) const {
		int tmpCheckSize = check_size();
		cb_item *i = (cb_item *)v;
		int tsize = textsize();

		Fl_Color tmpCol = i->m_textColor ;
		Fl_Color col = active_r() ? tmpCol : fl_inactive(tmpCol);

		int cy = Y + (tsize + 1 - tmpCheckSize) / 2;
		X += 2;

		fl_color(active_r() ? FL_FOREGROUND_COLOR : fl_inactive(FL_FOREGROUND_COLOR));
		fl_loop(X, cy, X, cy + tmpCheckSize,
			X + tmpCheckSize, cy + tmpCheckSize, X + tmpCheckSize, cy);
		if (i->m_checked) {
			int tx = X + 3;
			int tw = tmpCheckSize - 4;
			int d1 = tw/3;
			int d2 = tw-d1;
			int ty = cy + (tmpCheckSize+d2)/2-d1-2;
			for (int n = 0; n < 3; n++, ty++) {
				fl_line(tx, ty, tx+d1, ty+d1);
				fl_line(tx+d1, ty+d1, tx+tw-1, ty+d1-d2+1);
			}
		}
		fl_font(textfont(), tsize);
		if (i->m_selected) {
			col = fl_contrast(col, selection_color());
		}
		fl_color(col);
		fl_draw(i->m_textItem.c_str(), X + tmpCheckSize + 8, Y + tsize - 1);
	}

	void item_select(void *v, int state) {
		cb_item *i = (cb_item *)v;

		if (state) {
			i->m_checked = ! i->m_checked ;
		}
	}

	int item_selected(void *v) const {
		cb_item *i = (cb_item *)v;
		return i->m_selected;
	}

	cb_item *find_item(int n) const {
		cb_item * p = m_itemsFirst;
		while( ( n > 1 ) && ( p != NULL ) )
		{
			--n;
			p = p->m_next;
		}
		return p;
	}

public:
	/// No horizontal scroll bar because the window is not high enough.
	Sources_Check_Browser(int X, int Y, int W, int H)
	: Fl_Browser_(X, Y, W, H)
	, m_itemsFirst(NULL)
	, m_itemsLast(NULL)
	{
		type(FL_SELECT_BROWSER);
		when(FL_WHEN_NEVER);
		has_scrollbar(VERTICAL);
	}

	~Sources_Check_Browser()
	{
		for( cb_item * p = m_itemsFirst, * nxt = NULL; p ; p = nxt )
		{
			delete p->m_frqLstBase ;
			nxt = p->m_next;
			delete p;
		}
	}

	/// Adds a new line with a function pointer called with ticking/unticking the checkbox.
	void add_line(
			const char *txt,
			FrequencyListBase * ptrFrqLstBase,
			bool status )
	{
		/// Does not matter if several times the same file.
		cb_item * ptrItem = new cb_item ;
		ptrItem->m_prev = m_itemsLast;

		if( m_itemsFirst == NULL )
		{
			m_itemsLast = m_itemsFirst = ptrItem;
		}
		else
		{
			m_itemsLast->m_next = ptrItem;
		}
		m_itemsLast = ptrItem;

		ptrItem->m_textItem = txt;
		ptrItem->m_textColor = qrg_mode_extended_t::disp_color( ptrFrqLstBase->frequency_source_id() ),
		ptrItem->m_frqLstBase = ptrFrqLstBase; /// Must be deleted by this object.
		ptrItem->m_checked = status;
		ptrItem->m_selected = false;

		if(status)
		{
			ptrFrqLstBase->switchFrequencyCheckBox();
		}
	}

	/// Index of the currently ticked checkbox.
	int value() const {
		cb_item *p0 = (cb_item *)selection();
		cb_item * p = m_itemsFirst;
		int idx = 1 ;
		while( p != p0 )
		{
			if( p == NULL ) return 0 ;
			++idx;
			p = p->m_next;
		}
		return idx ;
	}

protected:
	/// If the mouse is clicked anywhere on this widget.
	int handle(int event) {
		bool isPushed = false ;
		if (event==FL_PUSH)
		{
			deselect();
			isPushed = true ;
		}
		int ret = Fl_Browser_::handle(event);
		if(isPushed && (ret == 1) )
		{
			size_t idx = value() ;
			cb_item * ptr = find_item( idx );
			if( ptr )
				ptr->m_frqLstBase->switchFrequencyCheckBox();
		}
		return ret ;
	}
};

/// This unique browser contains all the various frequency lists sources.
static Sources_Check_Browser * g_SourcesBrowser = NULL ;

// ----------------------------------------------------------------------------

#if !USE_HAMLIB

typedef enum {
	RIG_MODE_NONE =  	0,	/*!< '' -- None */
	RIG_MODE_AM =    	(1<<0),	/*!< \c AM -- Amplitude Modulation */
	RIG_MODE_CW =    	(1<<1),	/*!< \c CW -- CW "normal" sideband */
	RIG_MODE_USB =		(1<<2),	/*!< \c USB -- Upper Side Band */
	RIG_MODE_LSB =		(1<<3),	/*!< \c LSB -- Lower Side Band */
	RIG_MODE_RTTY =		(1<<4),	/*!< \c RTTY -- Radio Teletype */
	RIG_MODE_FM =    	(1<<5),	/*!< \c FM -- "narrow" band FM */
	RIG_MODE_WFM =   	(1<<6),	/*!< \c WFM -- broadcast wide FM */
	RIG_MODE_CWR =   	(1<<7),	/*!< \c CWR -- CW "reverse" sideband */
	RIG_MODE_RTTYR =	(1<<8),	/*!< \c RTTYR -- RTTY "reverse" sideband */
	RIG_MODE_AMS =    	(1<<9),	/*!< \c AMS -- Amplitude Modulation Synchronous */
	RIG_MODE_PKTLSB =       (1<<10),/*!< \c PKTLSB -- Packet/Digital LSB mode (dedicated port) */
	RIG_MODE_PKTUSB =       (1<<11),/*!< \c PKTUSB -- Packet/Digital USB mode (dedicated port) */
	RIG_MODE_PKTFM =        (1<<12),/*!< \c PKTFM -- Packet/Digital FM mode (dedicated port) */
	RIG_MODE_ECSSUSB =      (1<<13),/*!< \c ECSSUSB -- Exalted Carrier Single Sideband USB */
	RIG_MODE_ECSSLSB =      (1<<14),/*!< \c ECSSLSB -- Exalted Carrier Single Sideband LSB */
	RIG_MODE_FAX =          (1<<15),/*!< \c FAX -- Facsimile Mode */
	RIG_MODE_SAM =          (1<<16),/*!< \c SAM -- Synchronous AM double sideband */
	RIG_MODE_SAL =          (1<<17),/*!< \c SAL -- Synchronous AM lower sideband */
	RIG_MODE_SAH =          (1<<18),/*!< \c SAH -- Synchronous AM upper (higher) sideband */
	RIG_MODE_DSB =			(1<<19), /*!< \c DSB -- Double sideband suppressed carrier */
} rmode_t;

#endif

static const struct rmode_name_t {
	rmode_t mode;
	const char *name;
} modes[] = {
	{ RIG_MODE_NONE, "NONE" },
	{ RIG_MODE_AM, "AM" },
	{ RIG_MODE_CW, "CW" },
	{ RIG_MODE_USB, "USB" },
	{ RIG_MODE_LSB, "LSB" },
	{ RIG_MODE_RTTY, "RTTY" },
	{ RIG_MODE_FM, "FM" },
	{ RIG_MODE_WFM, "WFM" },
	{ RIG_MODE_CWR, "CWR" },
	{ RIG_MODE_RTTYR, "RTTYR" },
	{ RIG_MODE_AMS, "AMS" },
	{ RIG_MODE_PKTLSB, "PKTLSB" },
	{ RIG_MODE_PKTUSB, "PKTUSB" },
	{ RIG_MODE_PKTFM, "PKTFM" }
//, // C99 trailing commas in enumerations not yet in the C++ standard
//	{ RIG_MODE_ECSSUSB, "ECSSUSB" },
//	{ RIG_MODE_ECSSLSB, "ECSSLSB" },
//	{ RIG_MODE_FAX, "FAX" }
// the above are covered by our requirement that hamlib be >= 1.2.4
#if (defined(RIG_MODE_SAM) && defined(RIG_MODE_SAL) && defined(RIG_MODE_SAH))
	, // C99 trailing commas in enumerations not yet in the C++ standard
	{ RIG_MODE_SAM, "SAM" },
	{ RIG_MODE_SAL, "SAL" },
	{ RIG_MODE_SAH, "SAH" }
#endif
};

static map<string, rmode_t> mode_nums;
static map<rmode_t, string> mode_names;

void qso_selMode(rmode_t m)
{
	qso_opMODE->value(mode_names[m].c_str());
}

const string & modeString(rmode_t m)
{
	return mode_names[m];
}

static void initOptionMenus()
{
	qso_opMODE->clear();
	list<RIGMODE>::iterator MD;
	list<RIGMODE> *pMD = 0;
	if (lmodes.empty() == false)
		pMD = &lmodes;
	else if (lmodeCMD.empty() == false)
		pMD = &lmodeCMD;

	if (pMD) {
		MD = pMD->begin();
		while (MD != pMD->end()) {
			qso_opMODE->add( (*MD).SYMBOL.c_str());
			MD++;
		}
		qso_opMODE->activate();
		qso_opMODE->index(0);
	}
	else {
		qso_opMODE->deactivate();
	}

	qso_opBW->clear();
	list<BW>::iterator bw;
	list<BW> *pBW = 0;
	if (lbws.empty() == false)
		pBW = &lbws;
	else if (lbwCMD.empty() == false)
		pBW = &lbwCMD;

	if (pBW) {
		bw = pBW->begin();
		while (bw != pBW->end()) {
			qso_opBW->add( (*bw).SYMBOL.c_str());
			bw++;
		}
		qso_opBW->activate();
		qso_opBW->index(0);
	}
	else {
		qso_opBW->deactivate();
	}
}

void clearList()
{
	freqlist.clear();
	qso_opBrowser->clear();
}

/// Copies the frequency list into the GUI browers.
static void updateSelect()
{
	int nb = freqlist.size();
	LOG_INFO("Sz=%d", nb );
	/// Reloaded from scratch.
	qso_opBrowser->clear();
	qrg_mode_extended_t::buffer_type qrg_buf ;
	for (int i = 0; i < nb; i++)
	{
		/// This avoids a string allocation.
		freqlist[i].str( qrg_buf );
		qso_opBrowser->add(qrg_buf);
	}
	visi_ctxt.updateVisibility();
	qso_displayFreq( wf->rfcarrier() );
}


/// Sorts and deduplicates the frequency list.
static void SortAndDeduplicateFreqList()
{
	sort(freqlist.begin(), freqlist.end());

	/// Detect duplicates using frequency and Levenshtein distance.
	int index = 1, unidx = 0 ;

	/// We should check the similarity of all identical frequencies.
	for( int nb = freqlist.size(); index < nb ; ++index )
	{
		if( freqlist[index].similar( freqlist[unidx] ) ) continue ;

		++unidx ;
		if( unidx != index ) freqlist[unidx] = freqlist[index];
	}
	freqlist.resize(unidx+1);
	LOG_INFO("Sorted frequency file, %d unique records out of %d", unidx, index );

}

/// Prepares the frequency list and copies it into the browser.
static void FreqListToBrowser()
{
	SortAndDeduplicateFreqList();
	updateSelect();
}

/** TODO: EIBI and MWList should be in a separate commit,
 * coming after frequency list. The reasons are:
 * It has to come with big data files. And it makes the code
 * more complicated. */

/** EIBI is a web site which collects an big database of frequencies. 
 * Its data is easily loaded in CVS format.
 * Note: These structs are sliced when inserted into a frequency list
 * but it does not matter because they do not contain any private data.
 */
class RecordEIBI : public qrg_mode_extended_t
{
	/// From the CVS file.
	static const char m_delim = ';';
public:
	static const qrg_mode_extended_t::FreqSourceId freq_source = 2 ;

	friend std::istream & operator>>( std::istream & istrm, RecordEIBI & rec )
	{
		/// Explanations here: http://www.eibispace.de/dx/README.TXT
		std::string ITU_cntry;

		/// One-letter or two-letter codes for different transmitter sites within one country.
		std::string trx_site_code ;

		// kHz:75;Time(UTC):93;Days:59;ITU:49;Station:201;Lng:49;Target:62;Remarks:135;P:35;Start:60;Stop:60;
		// 16.4;0000-2400;;NOR;JXN Marine Norway;;NEu;no;1;;
		double khz ;
		if( read_until_delim( m_delim, istrm, khz               )
		&&  read_until_delim( m_delim, istrm  /* time */        )
		&&  read_until_delim( m_delim, istrm  /* days */        ) // "24Dec", "Sa-Mo", "Tu-Fr", "Su-Th", "Su"
		&&  read_until_delim( m_delim, istrm, ITU_cntry         )
		&&  read_until_delim( m_delim, istrm, rec.m_name        )
		&&  read_until_delim( m_delim, istrm  /* language */    )
		&&  read_until_delim( m_delim, istrm  /* target */      )
		&&  read_until_delim( m_delim, istrm, trx_site_code     ) // Transmitter site code.
		&&  read_until_delim( m_delim, istrm  /* start */       ) // 2810
		&&  read_until_delim( m_delim, istrm  /* stop */        ) // 3112
		) 
		{
			rec.rfcarrier = 1000.0 * khz ;

			/// Beneficial because of reference counting.
			static const std::string rmode_none("AM");
			rec.m_rmode = rmode_none ;

			rec.carrier = 0 ;
			rec.mode = MODE_NULL;

			rec.m_description = strjoin( ITU_cntry, trx_site_code );

			return istrm ;
		}

		istrm.setstate(std::ios::badbit);
		return istrm ;
	}
}; // RecordEIBI

/** MWList is a huge database of various short-wave frequencies. It provides an URL
 * allowing to download everything in one CSV zipped file.
 * Note: These objects are sliced when inserted into the frequency list,
 * but it does not matter.
 */
class RecordMWList : public qrg_mode_extended_t
{
	/// From the CVS file.
	static const char m_delim = '|';

public:
	/// Specifically allocated source code for these data.
	static const qrg_mode_extended_t::FreqSourceId freq_source = 3 ;

	// MHZ|ITU|Program|Location|Region|Power|ID|latitude|longitude|schedule|la
	// 16.400000|NOR|JXN|Gildeskål|no|45.000000|86009|66.982778|13.873056|0000-2200||
	// 23.400000|D|DHO38|Rhauderfehn Marinefunksendestelle|nds||86043|53.081944|7.616389|24h||
	// 171.000000|RUS|Voice of Russia/R.Chechnya Svobodnaya|Tbilisskaya|KDA|1200.000000|18|45.485431|40.089333|0200-2000|ce,ru|
	friend std::istream & operator>>( std::istream & istrm, RecordMWList & rec )
	{
		std::string ITU_cntry, location, region;

		/// One-letter or two-letter codes for different transmitter sites within one country.
		std::string trx_site_code ;

		/// Coordinates are optional. We could use also read_until_delim with a default value.
		static const double invalid_coo = -65536 ;
		double latitude = invalid_coo, longitude = invalid_coo;

		// kHz:75;Time(UTC):93;Days:59;ITU:49;Station:201;Lng:49;Target:62;Remarks:135;P:35;Start:60;Stop:60;
		// 16.4;0000-2400;;NOR;JXN Marine Norway;;NEu;no;1;;
		double khz ;

		if( read_until_delim( m_delim, istrm, khz                             )
		&&  read_until_delim( m_delim, istrm, ITU_cntry                       )
		&&  read_until_delim( m_delim, istrm, rec.m_name                      )
		&&  read_until_delim( m_delim, istrm, location                        )
		&&  read_until_delim( m_delim, istrm, region                          )
		&&  read_until_delim( m_delim, istrm  /* power */                     )
		&&  read_until_delim( m_delim, istrm  /* ID */                        )
		&&  read_until_delim( m_delim, istrm, latitude                        )
		&&  read_until_delim( m_delim, istrm, longitude                       )
		&&  read_until_delim( m_delim, istrm  /* time */                      ) // "0200-2000", "24h"
		&&  read_until_delim( m_delim, istrm  /* language */                  )

		) 
		{
			rec.rfcarrier = 1000.0 * khz ;

			/// Beneficial because of reference counting.
			static const std::string rmode_none("AM");
			rec.m_rmode = rmode_none ;

			rec.carrier = 0 ;
			rec.mode = MODE_NULL;

			/// This adds comma between words only if they are not empty.
			rec.m_description = strjoin( location, region, ITU_cntry );

			/// Intentionaly left in invalid state.
			rec.m_coordinates = CoordinateT::Pair();

			if( (latitude != invalid_coo) && (longitude != invalid_coo) )
			try
			{
				rec.m_coordinates = CoordinateT::Pair( longitude, latitude );
			}
			catch( const std::exception & exc )
			{
				LOG_INFO("Invalid coordinates: %s", exc.what() );
			}
			return istrm ;
		}

		istrm.setstate(std::ios::badbit);
		return istrm ;
	}
}; // RecordMWList

/// Path to the file containing the frequency list.
static std::string HomeDirFreqFile()
{
	return HomeDir + "frequencies2.txt";
}

/// Reads a space-separated file containing one frequency (Radio station etc...) per line.
static bool readFreqList(const std::string & filname)
{
	LOG_INFO("Loading frequency file:%s", filname.c_str() );
	ifstream freqfile( filname.c_str() );
	if (!freqfile)
		return false;

	string line;
	qrg_mode_extended_t tempFreq;
	while (!getline(freqfile, line).eof()) {
		/// Lines starting with hash are not loaded.
		if (line[0] == '#')
			continue;
		/// This stream dallocates no memory, therefore faster.
		imemstream is(line);
		is >> tempFreq;
		freqlist.push_back(tempFreq);
	}
	freqfile.close();

	return true ;
}

/// Do not save frequencies loaded from other sources than frequencies2.txt and unaltered.
void saveFreqList()
{
	ofstream freqfile(HomeDirFreqFile().c_str());
	if (!freqfile)
		return;
	freqfile << "# rfcarrier rig_mode carrier mode\n";

	if (freqlist.empty()) {
		freqfile.close();
		return;
	}

	for( FreqListType::const_iterator beg = freqlist.begin(), en = freqlist.end(); beg != en; ++beg )
	{
		/// Do not save frequencies loaded from an external source and not modified.
		if( beg->is_source( qrg_mode_extended_t::FREQS_NONE ) ) {
			freqfile << *beg << "\n";
		}
	}
	freqfile.close();
}

/// Builds an initial file of well known frequencies.
static void addDefaultFreqs()
{
	/// A static init is more efficient than several function calls.
	static const struct freqdef_t {
		long long rfcarrier ;
		int carrier ;
		const char * rmode ;
		trx_mode mode ;
		const char * name;
		const char * description ;
	} cstData[] = {
        	{ 1807000L, 1000, "USB", MODE_PSK31,      "", "Common frequency, USA" },
        	{ 3505000L,  800, "USB", MODE_CW},
        	{ 3580000L, 1000, "USB", MODE_PSK31,      "", "Common operating frequency" },
        	{ 1000500L,  800, "USB", MODE_CW},
        	{10135000L, 1000, "USB", MODE_PSK31 },
        	{ 7005000L,  800, "USB", MODE_CW},
        	{ 7030000L, 1000, "USB", MODE_PSK31 },
        	{ 7070000L, 1000, "USB", MODE_PSK31,      "", "Common frequency, USA" },
        	{14005000L,  800, "USB", MODE_CW},
        	{14070000L, 1000, "USB", MODE_PSK31 },
        	{18100000L, 1000, "USB", MODE_PSK31 },
        	{21005000L,  800, "USB", MODE_CW},
        	{21070000L, 1000, "USB", MODE_PSK31 },
        	{24920000L, 1000, "USB", MODE_PSK31 },
        	{28005000L,  800, "USB", MODE_CW},
        	{ 28120000, 1000, "USB", MODE_PSK31 },
		{   424000, 1000, "USB", MODE_NAVTEX,     "Navtex", _("Japan") },
		{   490000, 1000, "USB", MODE_NAVTEX,     "Navtex", _("Local") },
		{   518000, 1000, "USB", MODE_NAVTEX,     "Navtex", _("International") },
		{  4209500, 1000, "USB", MODE_NAVTEX,     "Navtex", _("Tropical") },
		{   145300, 1000, "USB", MODE_RTTY,       "DDH47", "FM12 / FM13 Synop Code" },
		{  4582000, 1000, "USB", MODE_RTTY,       "DDK2", "FM12 / FM13 Synop Code" },
		{  7645000, 1000, "USB", MODE_RTTY,       "DDH7", "FM12 / FM13 Synop Code" },
		{ 10099800, 1000, "USB", MODE_RTTY,       "DDK9", "FM12 / FM13 Synop Code" },
		{ 11038000, 1000, "USB", MODE_RTTY,       "DDH9", "FM12 / FM13 Synop Code" },
		{ 14466300, 1000, "USB", MODE_RTTY,       "DDH8", "FM12 / FM13 Synop Code" },
		{  2617500, 1000, "USB", MODE_WEFAX_576,  "Meteo Northwood (UK)" },
		{  3854000, 1000, "USB", MODE_WEFAX_576,  "DDH3", "DWD (Deutscher Wetterdienst)" },
		{  4609000, 1000, "USB", MODE_WEFAX_576,  "", "Meteo Northwood (UK)" },
		{  5849000, 1000, "USB", MODE_WEFAX_576,  "", "Meteo Copenhagen (Dänemark)" },
		{  7879000, 1000, "USB", MODE_WEFAX_576,  "DDK3", "DWD (Deutscher Wetterdienst)" },
		{  8039000, 1000, "USB", MODE_WEFAX_576,  "", "Meteo Northwood (UK)" },
		{  9359000, 1000, "USB", MODE_WEFAX_576,  "", "Meteo Copenhagen (Dänemark)" },
		{ 11085500, 1000, "USB", MODE_WEFAX_576,  "", "Meteo Northwood (UK)" },
		{ 13854000, 1000, "USB", MODE_WEFAX_576,  "", "Meteo Copenhagen (Dänemark)" },
		{ 13881500, 1000, "USB", MODE_WEFAX_576,  "DDK6", "DWD (Deutscher Wetterdienst)" },
		{ 17509000, 1000, "USB", MODE_WEFAX_576,  "", "Meteo Copenhagen (Dänemark)" }
	};
/*
If you want to try copying the VOA stuff, here's the usual schedule.

VOA Radiogram transmission schedule

(all days and times UTC)

Sat 1600-1630 17860 kHz
Sun 0230-0300 5745 kHz
Sun 1300-1330 6095 kHz
Sun 1930-2000 15670 kHz

All via the Edward R. Murrow transmitting station in North Carolina.

They start out on MFSK-16 and then each week they have a different
schedule of modes to test. 
*/
	static const size_t nbData = sizeof(cstData)/sizeof(cstData[0]);
	/// Adds new frequencies to the list and the browser. Reorder things.
	for( size_t i = 0; i < nbData; ++i )
	{
		static const std::string emptyStr;

		const freqdef_t * ptr = cstData + i ;
		qrg_mode_extended_t m(
			ptr->rfcarrier,
			ptr->rmode,
			ptr->carrier,
			ptr->mode,
			ptr->name ? std::string(ptr->name) : emptyStr,
			ptr->description ? std::string(ptr->description) : emptyStr );
        	freqlist.push_back(m);
	}
}

/// Builds an initial file of well known frequencies.
static void buildlist()
{
	/// Do not do anything if the frequencies file is already there.
	if (readFreqList( HomeDirFreqFile() ) == false)
	{
		addDefaultFreqs();
	}
	FreqListToBrowser();
}

int cb_qso_opMODE()
{
#if USE_HAMLIB
	if (progdefaults.chkUSEHAMLIBis) {
		if( 1 != hamlib_setmode(mode_nums[qso_opMODE->value()]) ) {
			LOG_WARN("Invalid qso mode:%s",qso_opMODE->value() );
		}
	}
	else
#endif
		rigCAT_setmode(qso_opMODE->value());
	return 0;
}

// ----------------------------------------------------------------------------

/*
These files may be used to download the list of language codes with
their language names, for example into a database. To read the files,
please note that one line of text contains one entry.
An alpha-3 (bibliographic) code,
an alpha-3 (terminologic) code (when given),
an alpha-2 code (when given),
an English name,
and a French name of a language are all separated by pipe (|) characters

If one of these elements is not applicable to the entry,
the field is left empty, i.e., a pipe (|) character immediately
follows the preceding entry. The Line terminator is the LF character. 

eng||en|English|anglais
enm|||English, Middle (1100-1500)|anglais moyen (1100-1500)
epo||eo|Esperanto|espéranto
*/

/// http://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
// ISO 639-1 or ISO 639-2-T
// http://www.loc.gov/standards/iso639-2/ISO-639-2_8859-1.txt
struct RecordISO639 {
	std::string m_ISO639_2_B;
	std::string m_ISO639_1;
	std::string m_nameEnglish ;

	static const char m_delim = '|';
public:
	const std::string & ISO639_2_B(void) const { return m_ISO639_2_B; }
	const std::string & ISO639_1(void) const { return m_ISO639_1; }
	const std::string & languageName(void) const { return m_nameEnglish; }

	friend std::istream & operator>>( std::istream & istrm, RecordISO639 & rec )
	{
		if( read_until_delim( m_delim, istrm, rec.m_ISO639_2_B  )
		&&  read_until_delim( m_delim, istrm                    )
		&&  read_until_delim( m_delim, istrm, rec.m_ISO639_1    )
		&&  read_until_delim( m_delim, istrm, rec.m_nameEnglish )
		&&  read_until_delim( m_delim, istrm                  )
		) 
		{
			strtrim( rec.m_ISO639_2_B );
			rec.m_ISO639_2_B = uppercase( rec.m_ISO639_2_B );
			strtrim( rec.m_ISO639_1 );
			rec.m_ISO639_1 = uppercase( rec.m_ISO639_1 );
			strtrim( rec.m_nameEnglish );

			// std::cout << "2_B=" << rec.m_ISO639_2_B << " 1=" << rec.m_ISO639_1 << " c=" << rec.m_nameEnglish << "\n";
			return istrm ;
		}

		istrm.setstate(std::ios::badbit);
		return istrm ;
	}
}; // RecordISO639

struct CatalogISO639;

/// In Perseus files, two different languages codes are often used.
typedef Catalog< const std::string &, RecordISO639, &RecordISO639::ISO639_2_B, CatalogISO639 > CatalogISO639_2_B;
typedef Catalog< const std::string &, RecordISO639, &RecordISO639::ISO639_1, CatalogISO639 > CatalogISO639_1;

/// A catalog with two different indices on the same data.
struct CatalogISO639
: public CatalogISO639_2_B
, public CatalogISO639_1
{
	/// A static file with the same name is also installed in directory data.
	const char * Url(void) const {
		return "http://www.loc.gov/standards/iso639-2/ISO-639-2_8859-1.txt";
	}

	const char * Description() const {
		return _("ISO639 code for languages description");
	}

	virtual void Clear()
	{
		CatalogISO639_2_B::Clear();
		CatalogISO639_1::Clear();
	}

	/// Each record is inserted twice. TODO: Have only one copy of the record.
	virtual bool ReadRecord(std::istream& istrm)
	{
		std::string str ;
		if( ! std::getline( istrm, str ) ) return false;

		std::istringstream strm_2_B(str);
		if( ! CatalogISO639_2_B::ReadRecord(strm_2_B) ) return false;

		std::istringstream strm_1(str);
		if( ! CatalogISO639_1::ReadRecord(strm_1) ) return false;

		return true ;
	}

	/// Returns a language given the three-letters ISO639_2_B code.
	static const RecordISO639 * Find_2_B(const std::string & str )
	{
		/// Seems often used instead of the normalised "JPN".
		if( str == "JAP" ) 
		{
			static const RecordISO639 jap = { "JAP", "JAP", "Japanese" };
			return & jap;
		}

		return CatalogISO639_2_B::FindFromKey(str);
	}

	/// Returns a language given the two-letters ISO639_1 code.
	static const RecordISO639 * Find_1(const std::string & str )
	{
		return CatalogISO639_1::FindFromKey(str);
	}
};

// ----------------------------------------------------------------------------

/*
0123456789012345678901234567890123456789012345678901234567890123456789
Country Name                            ITU   ISO     M    C    L

Adelie Land                             ADL           N    N    F  
Afghanistan                             AFG   AFG     Y    N    F/E
Albania                                 ALB   ALB     Y    N    F  
Algeria                                 ALG   DZA     Y    Y    F  
*/


/// This reads the file defining the ITU countries.
class RecordITU {
	/// For example AFG, ALB.
	std::string m_itu;

	/// For example Afghanistan or Albania.
	std::string m_name ;

public:
	const std::string & itu(void) const { return m_itu; }
	const std::string & countryName(void) const { return m_name; }

	friend std::istream & operator>>( std::istream & istrm, RecordITU & rec )
	{
		std::string newLine ;

		std::getline( istrm, newLine );

		/// Minimum length per record.
		if(newLine.size() <= 53 )
		{
			istrm.setstate(std::ios::badbit);
			return istrm ;
		}

		rec.m_name = newLine.substr( 0, 40 );
		strtrim( rec.m_name );
		rec.m_itu = newLine.substr( 40, 3 );
		strtrim( rec.m_itu );

		// std::cout << "XXX(" << rec.m_name << "=" << rec.m_itu << ")" << "\n";

		return istrm ;
	}
}; // RecordITU

/// This builds a catalog of countries indexed by their ITU code.
struct CatalogITU : public Catalog< const std::string &, RecordITU, &RecordITU::itu, CatalogITU >
{
	/// A static file with the same name is also installed in directory data.
	const char * Url(void) const {
		return "http://www.ominous-valve.com/itucode.txt";
	}

	/// Displayed in the data files menu.
	const char * Description() const {
		return _("ITU letter code");
	}
};

// ----------------------------------------------------------------------------

/// Transforms a string of the form "2359" into its integer equivalent, otherwise -1.
short ReadHHMM( const std::string & str )
{
	if( str.size() != 4 ) return -1 ;
	int h, m;
	if( 2 != sscanf( str.c_str(), "%2d%2d", &h, &m ) ) return -1 ;
	if( h > 24 ) return -1 ;
	if( m >= 60 ) return -1 ;
	if( ( h == 24 ) && ( m != 0 ) ) return -1 ;
	return h * 100 + m ;
}

// ----------------------------------------------------------------------------

/*
Perseus files:

# Japan
kHz           Time(UTC)       ITU Station                Lang. Location    Days 
+-------------+---------+-----+---+------------------------+---+-----------+----

40            0000-2400       J   Time Signal              A1B Otakadoyama 1-7  
153           0400-1900       D   Deutschlandfunk          Ger Donebach    1-7  

# EIBI There are two EIBI formats: One is in Perseus format:
kHz           Time(UTC) Days  ITU Station                Lang. Target   Remarks
================================================================================

16.4          0000-2400       NOR JXN Marine Norway            NEu         no
18.3          0000-2400       F   HWU French Navy              WEu         wu
171           0005-0100 Tu-Fr MRC Medi 1                   F   NAf         n
60            0000-2400       G   MSF Time from NPL        -TS Eu          an

# FMScan
===================================================================================
87500         0000-2400       I   532A _MALVISI /30v/1153  it  I           Pellegr
87500         0000-2400       E   7878 DYNAMIS_ /15v/1292  es  E           Alcorcó
87500         0000-2400       E   EC93 RADIO___ ESPIN/982      E           Valle d

# emwg http://www.hermanboel.eu/emwg/emwg.txt
kHz                 utc       Ctry        Station          Lan  location   remarks
0,1           0000-2359       ----ELWG 2013                ---
153,0         0000-2400       NOR NRK 1/NRK 2/NRK Finnmark no              100
153,0         0000-2400       D   Deutschlandfunk          de              500
153,0         0100-0200       ALG Radio Alger Internationalar              2000

# http://www.tapiokalmi.net/dx/koje/koje5.html
0540          1100-0700       2   XEMIT Comitan de Domi CS                 VAR
1460          0000-2400       1   KDWA Hastings MN         N               NWS
1460          0000-2400       1   WPON Walled Lake MI      -               OLD

# http://www.ndxc.org/db/jpmw/userlist2.txt
MW Frequency list in Japan    Jan 26, 2013  1140UTC
kHz           Time(UTC) PowKW ITU Station                Lang. Location    Call 
+-------------+---------+-----+---+------------------------+---+-----------+----

531           0000-2400 0.5   J   NHK 1 Matsuyama          Jap Niihama          
531           0000-2400 10    J   NHK 1 Morioka            Jap Morioka     JOQG 

87900         0000-2400       AUT A201 __OE_1__ 32° m      10  AUT         Leutasc^M
88300         0000-2400       AUT A203 HITRADIO 329° h X   10  AUT         Sankt L^M
88700         0000-2400       D   D312 Bayern_2 324° h     10  D           Grünten^M

11840         0315-0415       IND ALL INDIA RADIO          Hin Panaji      1-7  
11840         1145-1315       IND * ALL INDIA RADIO        Chi Delhi (King 1-7  
11840         1000-1018       GUM KTWR GUAM                Eng Agana       2-7  

*/

/**
 * http://en.wikipedia.org/wiki/List_of_ITU_letter_codes
 * The radiocommunication division of the International Telecommunication
 * Union uses the following letter codes to identify its member countries.
 * Eight countries are assigned single-letter codes,
 * while the rest have codes three letters in length.
 */

class RecordPerseus : public qrg_mode_extended_t
{
public:
	friend std::istream & operator>>( std::istream & istrm, RecordPerseus & rec )
	{
		std::string newLine ;

		std::getline( istrm, newLine );

		do
		{
			static const size_t sizePerseus = 75 ;
			if( newLine.size() < sizePerseus ) newLine.resize( sizePerseus );

			/// We ensure that the line is long enough.

			{
				std::string strFreq = newLine.substr( 0, 13 );

				/// Sometimes the delimiter is a comma
				std::replace( strFreq.begin(), strFreq.end(), ',', '.' );

				double khz = atof( strFreq.c_str() );
				if( khz <= 0 ) break;
				rec.rfcarrier = 1000.0 * khz ; 
			}

			rec.m_time_on = ReadHHMM( newLine.substr( 14, 4 ) );  // 0000 to 2359
			rec.m_time_off = ReadHHMM( newLine.substr( 19, 4 ) );  // 0000 to 2359

			// char        m_week_schedule : 7 ;

			std::string description;

			// http://en.wikipedia.org/wiki/List_of_ITU_letter_codes : "J F NOR 2 1 MRC" ;
			std::string strCountry;
			std::string strCodeITU = uppercase(newLine.substr( 30, 3 ));
			strtrim( strCodeITU );

			const RecordITU * ptrITU = CatalogITU::FindFromKey( strCodeITU );
			if( ptrITU == NULL )
			{
				strCountry = strCodeITU;
			}
			else
			{
				strCountry = ptrITU->countryName();
			}
			// LOG_INFO("CodeITU=%s Country=%s", strCodeITU.c_str(), strCountry.c_str() );
			description += "c=" + strCountry;

			rec.m_name = newLine.substr( 34, 14 );
			strtrim( rec.m_name );

			// no, de , TS, A1B, Ger ...
			std::string strLanguage;
			{
				std::string strISO639 = uppercase( newLine.substr( 59, 3 ) );
				strtrim( strISO639 );

				const RecordISO639 * ptrISO639 = NULL;
			
				if( false == strISO639.empty() )
				{
					ptrISO639 = CatalogISO639::Find_2_B( strISO639 );
					if( ptrISO639 == NULL )
					{
						ptrISO639 = CatalogISO639::Find_1( strISO639 );
					}
				}
				if( ptrISO639 == NULL )
				{
					strLanguage = strISO639;
				}
				else
				{
					strLanguage = ptrISO639->languageName();
				}
				// LOG_INFO("ISO639=%s Language=%s", strISO639.c_str(), strLanguage.c_str() );
			}
			description += " v=" + strLanguage;

			/// The location can also be the target. Sometimes identical to ITU.
			std::string strLocation = newLine.substr( 63, 11 );
			strtrim(strLocation);
			if( strLocation != strCodeITU )
			{
				description += " l=" + strLocation;
			}

			/// This can be the callsign, a location, a week schedule.
			std::string strLast = newLine.substr( 74 );
			strtrim( strLast );
			// There might be a ctrl-M at the end.
			if( strLast[ strLast.size() - 1 ] == 'M' - 'A' + 1 )
				strLast.resize( strLast.size() - 1 );

			description += " z=" + strLast;

			/// Beneficial because of reference counting.
			static const std::string rmode_AM("AM");
			static const std::string rmode_FM("FM");

			if( ( rec.rfcarrier > 80000 * 1000 ) && ( rec.rfcarrier < 120000 * 1000 ) )
				rec.m_rmode = rmode_FM ;
			else
				rec.m_rmode = rmode_AM ;

			rec.carrier = 0 ;
			rec.mode = MODE_NULL;

			/// For now, just concatenate all available information.
			rec.m_description = description; // strjoin( strLanguage, strLocation, strLast );

// dxcc_open gives the coordinates of each country.


			return istrm ;
		} while(false);

		istrm.setstate(std::ios::badbit);
		return istrm ;
	}
}; // RecordPerseus

// ----------------------------------------------------------------------------

#define KRP_SUFFIX "krp"
#define TXT_SUFFIX "txt"

/// This will be set when we will be able to load the frequencies of an ADIF file.
#undef FREQ_LIST_WITH_ADIF

#ifdef FREQ_LIST_WITH_ADIF
/// Tells if a filename is ended by a given extension.
static bool file_has_extension( const char * filnam, const char * filext )
{
	int lennam = strlen(filnam);
	int lenext = strlen(filext);

	return ( 0 == strcmp( filnam + lennam - lenext, filext ) );
}
#endif // FREQ_LIST_WITH_ADIF

/// For a given type of frequency list element, does the loading from file, storage etc...
template< class RecordType >
struct FrequencySource
: public virtual FrequencyListBase
, public virtual RecordLoaderContainer
{
	/// Directly loaded to the main frequency list.
	virtual bool ReadRecord( std::istream & istrm ) {
		RecordType tmp ;
		istrm >> tmp ;
		if( istrm || istrm.eof() ) {
			tmp.m_data_source = frequency_source_id();
			freqlist.push_back( tmp );
			return true ;
		}
		return false ;
	}

	/// Before re-loading from file, cleanup the frequency list of stations of this source.
	virtual void Clear() {
		/// Erases from the frequencies list, those from a given data source: EIBI, MWList etc...
		FreqListType::iterator en = std::remove_if(
			freqlist.begin(),
			freqlist.end(),
			std::bind2nd( std::mem_fun_ref( & qrg_mode_extended_t::is_source ), frequency_source_id() ) );
		LOG_DEBUG("Removing %d elements, from %d", (int)std::distance( en, freqlist.end() ), (int)freqlist.size() );
		freqlist.erase( en, freqlist.end() );
	}
};

/// For a given type of frequency list element, does the loading from file, storage etc...
template< class RecordType, class LoaderType >
struct LoadableFrequencyList
: public virtual FrequencyListBase
, public RecordLoader< LoaderType >
{
	/// Adds a new frequency source only if the file is there.
	static void conditionalAddLine(void)
	{
		std::auto_ptr< LoaderType > ptr( new LoaderType );
		std::string filnam = ptr->storage_filename().first;

		struct stat st;
		if(stat(filnam.c_str(), &st) >= 0 )
		{
			g_SourcesBrowser->add_line( LoaderType::shortTitle(), ptr.release(), false );
		}
	}

	/// Updates the frequencies which are visible or not, depending on the flag.
	int loadFrequencyFile()
	{
		LoaderType & myLoader = LoaderType::InstCatalog();
		int nbRec = myLoader.LoadAndRegister();
		if(nbRec < 0) {
			LOG_WARN("Error loading %s", myLoader.Url() );
			return -1 ;
		}
		return nbRec;
	}
};

/// This loads the EIBI stations file, about 11000 records.
struct LoaderEIBI
: public LoadableFrequencyList< RecordEIBI, LoaderEIBI >
, public FrequencySource< RecordEIBI >
{
	/** This file is also installed in data directory. Name changes with the year.
	 * http://www.eibispace.de/dx/README.TXT
	 * Since B06, the fundamental database is a CSV semicolon-separated list
	 * that contains all the broadcasts relevant during the season ...
	 * sked-Xzz.csv for the CSV database, load this into EiBiView
	 * X=A: Summer season
	 * X=B: Winter season
	 * zz:  Two-character number for the year.
	 */
	const char * Url(void) const {
		return "http://www.eibispace.de/dx/sked-b12.csv";
	}

	const char * Description(void) const {
		return _("EiBi shortwaves");
	}

	qrg_mode_extended_t::FreqSourceId frequency_source_id() {
		return 2;
	}

	static const char * shortTitle() { return "EIBI"; }
};

/// This loads the MWList stations file, about 30000 records.
struct LoaderMWList
: public LoadableFrequencyList< RecordMWList, LoaderMWList >
, public FrequencySource< RecordMWList >
{
	/// Needed because we cannot built a nice filename from the URL.
	std::string base_filename() const {
		static const std::string name("mwlist.txt");
		return name;
	}

	/// This file is also installed in data directory. The file is zipped.
	const char * Url(void) const {
		return "http://www.mwlist.org/mw_server.php?task=015&token=fldigi1803";
	}

	const char * Description(void) const {
		return _("MWList radio db");
	}

	qrg_mode_extended_t::FreqSourceId frequency_source_id() {
		return 3;
	}

	static const char * shortTitle() { return "MWList"; }
};

/// This loads frequencies from a Perseus file.
class LoaderPerseus
: public FrequencySource< RecordPerseus >
{
	/// Unique internal id of this source of frequencies/
	qrg_mode_extended_t::FreqSourceId m_freq_source ;

	std::string m_filename ;

public:
	LoaderPerseus( const std::string & filnam  )
	: m_filename(filnam)
	{
		LOG_INFO("filename=%s", m_filename.c_str() );
		m_freq_source = visi_ctxt.allocate_source();
	}

	qrg_mode_extended_t::FreqSourceId frequency_source_id() {
		return m_freq_source;
	}

	int loadFrequencyFile()
	{
		LOG_INFO("filename=%s", m_filename.c_str() );
		int nbRec = LoadFromFile( m_filename );
		if(nbRec < 0) {
			LOG_WARN("Error loading %s", m_filename.c_str() );
			return -1;
		}
		return nbRec;
	}
};

/// Changes incrementically the frequency list font size.
static void cb_ZoomFreqListFont( Fl_Widget *, void *)
{
	int currTextSz = qso_opBrowser->textsize();
	if( currTextSz >= 14 )
		currTextSz = 8 ;
	else
		currTextSz += 2 ;
	qso_opBrowser->textsize(currTextSz);
	qso_opBrowser->redraw();
}

/// When clicking to load a frequency list file, or extract frequencies from an ADIF file.
static void cb_LoadFreqFile( Fl_Widget *, void *)
{
	static const char * filters =
		"Text\t*.txt\n"
		"Perseus\t*userlist*.*\n"
#ifdef FREQ_LIST_WITH_ADIF
		"ADIF\t*."   ADIF_SUFFIX "\n"
#endif // FREQ_LIST_WITH_ADIF
#ifdef __APPLE__
		"\n"
#endif
		;

	const char* filnam = FSEL::select( _("Open frequency list"), filters );
	if (filnam) {
#ifdef FREQ_LIST_WITH_ADIF
		if( file_has_extension(filnam, ADIF_SUFFIX) ) {
			cAdifIO tmpFile;
			cQsoDb tmpDb ;
			tmpFile.do_readfile(filnam, &tmpDb );
		} else
#endif // FREQ_LIST_WITH_ADIF
		if(
			( strstr(filnam, "userlist") )
		||	( strstr(filnam, "txt") ) ) {

			/// We trust the widget to delete the object...
			FrequencyListBase * ptrLoader = new LoaderPerseus( filnam );
	
			/// Just displays the basename of the file.
			const char * ptrBasename = strrchr( filnam, '/' );
			if( ptrBasename == NULL )
				ptrBasename = filnam ;
			else
				ptrBasename++ ;

			/// Checked as visible because we load the file and want to see it.
			g_SourcesBrowser->add_line(
					ptrBasename,
					ptrLoader,
					true );
		} else {
			LOG_ERROR("Unexpected file extension %s",filnam);
			return ;
		}

		/// Uses the same sort and deduplication for all sorts of new data.
		FreqListToBrowser();

		LOG_INFO("Loaded %s", filnam );
	}
}

/// The range of distances from the user to an emitter. Can hide stations which are too far.
static const struct {
	double       m_km ;
	const char * m_txt ;
} listDists[] = {
	{    10.0,    "10 km"},
	{   100.0,   "100 km"},
	{  1000.0,  "1000 km"},
	{  5000.0,  "5000 km"},
	{ 40000.0, _("All")  }
};

static const size_t nbChoiceDistance = sizeof(listDists) / sizeof(*listDists);

/// Callback when choosing a maximum station distance.
static void cb_browserDistances( Fl_Widget * widget, void *)
{
	Fl_Choice * ptrChoice = (Fl_Choice *)widget ;
	int idx = ptrChoice->value();

	visi_ctxt.set_max_distance( listDists[ idx ].m_km );
	visi_ctxt.updateVisibility();
}

/// Fills the combo box with all possible distances.
static void fillChoiceDistances( Fl_Choice * choiceDistances )
{
	for( size_t i = 0; i < nbChoiceDistance ; ++ i )
	{
		choiceDistances->add( listDists[i].m_txt );
	}
	choiceDistances->tooltip(_("Select distance"));
	choiceDistances->callback( cb_browserDistances, 0);
	int choice = nbChoiceDistance ;
	choiceDistances->value( choice - 1 ); // Select last index, all frequencies visible.
	visi_ctxt.set_max_distance( listDists[ choice - 1 ].m_km );
}

// ----------------------------------------------------------------------------

/// Fills the main window.
void qso_createFreqList(int frqHorPos, int frqVrtPos, int fullWidth, int totalHeight, int pad, int Hentry )
{
	int widthList = fullWidth - 70 ;

	g_SourcesBrowser = new Sources_Check_Browser( frqHorPos, frqVrtPos, widthList, totalHeight );
	g_SourcesBrowser->tooltip(_("Display other frequencies lists"));

	LoaderEIBI::conditionalAddLine();
	LoaderMWList::conditionalAddLine();

	/// Stations displayed based on the distance to the users locator.
	int widthButtons = fullWidth - widthList - pad ;

	int horPosButtons = frqHorPos + widthList + pad;
	Fl_Choice * choiceDistances
		= new Fl_Choice( horPosButtons, frqVrtPos, widthButtons, Hentry );
	fillChoiceDistances(choiceDistances);

	/// For loading Perseus files.
	Fl_Button * btnLoadFreqList
		= new Fl_Button( horPosButtons, frqVrtPos + Hentry + pad, widthButtons, Hentry, _("Perseus..."));
	btnLoadFreqList->callback(cb_LoadFreqFile, 0);
	btnLoadFreqList->tooltip(_("Load Perseus frequency lists"));

	/// For zooming the font
	Fl_Button * btnZoomFreqListFont
		= new Fl_Button( horPosButtons, frqVrtPos + 2 * ( Hentry + pad ), Hentry, Hentry);
	btnZoomFreqListFont->image(new Fl_Pixmap(text_icon));
	btnZoomFreqListFont->callback(cb_ZoomFreqListFont, 0);
	btnZoomFreqListFont->tooltip(_("Change frequency list font size"));
}

int cb_qso_opBW()
{
	if (progdefaults.chkUSERIGCATis)
		rigCAT_setwidth(qso_opBW->value());
	return 0;
}

static void sendFreq(long long f)
{
#if USE_HAMLIB
	if (progdefaults.chkUSEHAMLIBis)
		hamlib_setfreq(f);
	else
#endif
		rigCAT_setfreq(f);
}

/// Callback of the frequency widget.
void qso_movFreq(Fl_Widget* w, void*)
{
	cFreqControl *fc = (cFreqControl *)w;
	long long f;
	f = fc->value();
	if (fc == qsoFreqDisp1) {
		qsoFreqDisp2->value(f);
		qsoFreqDisp3->value(f);
	} else if (fc == qsoFreqDisp2) {
		qsoFreqDisp1->value(f);
		qsoFreqDisp3->value(f);
	} else {
		qsoFreqDisp1->value(f);
		qsoFreqDisp2->value(f);
	}

	qso_displayFreq(f);

	sendFreq(f);
	return;
}

/// Adjusts the frequency list to the nearest frequency of the current one.
void qso_displayFreq( long long freq )
{
	LOG_INFO("freq=%" PRId64, freq);
	/// The others members are defaulted.
        qrg_mode_extended_t m(freq);

	int nbfreq = freqlist.size();
	if( nbfreq == 0 ) return ;

	/// The frequencies are sorted so no need to iterate.
        FreqListType::const_iterator pos = lower_bound(freqlist.begin(), freqlist.end(), m, qrg_mode_extended_t::cmp_rfcarrier_only );

	int idx_old = qso_opBrowser->value();

	/// Indices in Fl_Browse start at one. Returns zero if no row is selected.
	long long freq_old = idx_old == 0 ? 0 : freqlist[idx_old-1].rfcarrier ;

	/// Depending if the previous frequency if bigger or lower than the next one, adjust differently.
	if( pos != freqlist.end() )
	{
		if( freq_old > freq)
		{
			while( ( pos != freqlist.begin() ) && ( freq < pos->rfcarrier - pos->carrier ) )
			{
				--pos ;
			}
		}
		else if( freq_old < freq )
		{
			while( ( pos != freqlist.end() ) && ( freq > pos->rfcarrier ) )
			{
				++pos ;
			}
		}
		else
		{
			LOG_INFO("Early leave idx_old=%d nbfreq=%d", idx_old, nbfreq );
			return ;
		}
	}

	/// Indices start at 1.
	int idx_new = 0 ;

	qso_opBrowser->deselect();
        if (pos == freqlist.end())
	{
		idx_new = nbfreq ;
	}
	else
	{
		idx_new = 1 + pos - freqlist.begin();
		if(
			( freq >= pos->rfcarrier - pos->carrier )
		&&	( freq <= pos->rfcarrier - pos->carrier + IMAGE_WIDTH ) )
		{
			/// If we are right in the range.
			qso_opBrowser->select( idx_new );
		}
	}
	qso_opBrowser->middleline(idx_new);
	LOG_INFO("idx_new=%d nbfreq=%d", idx_new, nbfreq );
}

void qso_selectFreq()
{
	int n = qso_opBrowser->value();
	if (!n) return;

	n -= 1;

	const qrg_mode_extended_t & refFrq = freqlist[n];
// transceiver frequency
	if (freqlist[n].rfcarrier > 0) {
		qsoFreqDisp1->value(refFrq.rfcarrier);
		qsoFreqDisp2->value(refFrq.rfcarrier);
		qsoFreqDisp3->value(refFrq.rfcarrier);
		sendFreq(refFrq.rfcarrier);
	}
// transceiver mode
	if (freqlist[n].m_rmode != "NONE") {
		qso_opMODE->value(refFrq.m_rmode.c_str());
		cb_qso_opMODE();
	}
// modem type & audio sub carrier
	if (freqlist[n].mode != NUM_MODES) {
		if (refFrq.mode != active_modem->get_mode())
			init_modem_sync(refFrq.mode);
		if (refFrq.carrier > 0)
			active_modem->set_freq(refFrq.carrier);
	}

	/** Some details of the selected frequency are copied in the area used for logging.
	 * The difficulty is to make the operation reversible because each frequency in the list
	 * has less fields than a contact. So we do the inverse operation of adding a frequency to the list.
	 * We cannot garanty we reproduce the same data.
	 */
	std::vector< std::string > splitName;
	strsplit( splitName, refFrq.m_name, ',' );
	splitName.resize(2);
	inpCall1->value( splitName[0].c_str() );
	inpName1->value( splitName[1].c_str() );

	std::vector< std::string > splitDescription ;
	strsplit( splitDescription, refFrq.m_description, ',' );
	splitDescription.resize(3);
	inpQth->value(splitDescription[0].c_str());
	inpCountry->value(splitDescription[1].c_str());
	inpNotes->value(splitDescription[2].c_str());
}

/// Tune the rig frequency to the row currently pointed at in the frequency list.
void qso_setFreq()
{
	int n = qso_opBrowser->value();
	if (!n) return;

	n -= 1;
// transceiver frequency
	if (freqlist[n].rfcarrier > 0) {
		qsoFreqDisp->value(freqlist[n].rfcarrier);
		sendFreq(freqlist[n].rfcarrier);
	}
}

/// Removes the current frequency list row from the frequency list.
void qso_delFreq()
{
	int v = qso_opBrowser->value() - 1;

	/// Both containers must always have the same number of elements.
	if (v >= 0) {
		freqlist.erase(freqlist.begin() + v);
		qso_opBrowser->remove(v + 1);
	}
}

/// Adds the current frequency to the frequency list.
void qso_addFreq()
{
	long long freq = qsoFreqDisp->value();

	if(freq <= 0 ) return ;

	/// Some fields come from the ADIF editor. Default data_source is FREQS_NONE.
	std::string name = strjoin( inpCall1->value(), inpName1->value() );
	std::string description = strjoin( inpQth->value(), inpCountry->value(), inpNotes->value() );

	/// We use this dummy object so that members get the right default value.
	qrg_mode_extended_t m(
		freq,
                qso_opMODE->value(),
		active_modem ? active_modem->get_freq() : 0,
		active_modem ? active_modem->get_mode() : NUM_MODES,
		name,
		description );

	/// Removes blank chars from the station locator.
	const char * ptrCoo = inpLoc->value();
	while( *ptrCoo && strchr( " \t", *ptrCoo ) ) ++ptrCoo ;

	/// If the user locator is not given, no adjustment.
	if( *ptrCoo != '\0' ) try
	{
		m.m_coordinates = CoordinateT::Pair( ptrCoo );

		double dist_to_user = m.m_coordinates.distance( CoordinateT::Pair( progdefaults.myLocator ) );

		visi_ctxt.set_max_distance( dist_to_user + 10.0, true );
		visi_ctxt.updateVisibility();
	}
	catch( const std::exception & exc )
	{
		fl_alert2( "Incorrect station locator:%s", ptrCoo );
		return ;
	}

	/// Search the range of identical frequencies.
	std::pair< FreqListType::iterator, FreqListType::iterator > pairIts
		= equal_range(freqlist.begin(), freqlist.end(), m, qrg_mode_extended_t::cmp_rfcarrier_only );

	FreqListType::iterator itDest ;

	/// Searches for a frequency with an empty description and name, but same mode, so we can reuse it.
	for( itDest = pairIts.first; itDest != pairIts.second; ++itDest )
	{
		if(itDest->rfcarrier != m.rfcarrier) throw std::runtime_error("Inconsistent");

		if(
		(itDest->mode      == m.mode) &&
		(itDest->m_name.empty()        || (itDest->m_name == m.m_name) ) &&
		(itDest->m_description.empty() || (itDest->m_description == m.m_description) ) )
			break;
	}

	/// Now updates the frequencies browser.
	qrg_mode_extended_t::buffer_type qrg_buf ;
	m.str(qrg_buf);

	/// No need to sort the container, it is inserted at the right place.
        size_t pos = 1 + itDest - freqlist.begin();

	/// Do we replace an existing frequency or do we create a new one ?
	if(  itDest != pairIts.second ) {
		*itDest = m ;
		qso_opBrowser->text(pos, qrg_buf );
	}
	else {
		freqlist.insert( itDest, m );
		qso_opBrowser->insert(pos, qrg_buf );
	}
}

/// Changes the color of frequencies. Only black frequencies are saved to the frequency list.
void qso_updateFreqColor(int pos)
{
	qrg_mode_extended_t & refFreq = freqlist.at(pos-1);

	switch( refFreq.m_data_source ) {
		/// If the frequency is EIBI or MWList, moved to the user frequency clist (In black).
		default                        :
		/// If red color, then becomes black.
		case qrg_mode_extended_t::FREQS_UNKNOWN : refFreq.m_data_source = qrg_mode_extended_t::FREQS_NONE ; break;
		// If user frequency list, goes to red.
		case qrg_mode_extended_t::FREQS_NONE    : refFreq.m_data_source = qrg_mode_extended_t::FREQS_UNKNOWN ; break ;
	}

	/// To change the color, the text must be changed.
	qrg_mode_extended_t::buffer_type qrg_buf ;
	freqlist[pos].str(qrg_buf);
	qso_opBrowser->text(pos, qrg_buf );
}

void setTitle()
{
	if (windowTitle.length() > 0) {
		txtRigName->label(windowTitle.c_str());
		txtRigName->redraw_label();
	} else {
		txtRigName->label();
		txtRigName->redraw_label();
	}
}

bool init_Xml_RigDialog()
{
	LOG_DEBUG("xml rig");
	initOptionMenus();
	clearList();
	buildlist();
	windowTitle = xmlrig.rigTitle;
	setTitle();
	return true;
}

bool init_NoRig_RigDialog()
{
	LOG_DEBUG("no rig");
	qso_opBW->deactivate();
	qso_opMODE->clear();

	for (size_t i = 0; i < sizeof(modes)/sizeof(modes[0]); i++) {
		qso_opMODE->add(modes[i].name);
	}
	LSBmodes.clear();
	LSBmodes.push_back("LSB");
	LSBmodes.push_back("CWR");
	LSBmodes.push_back("RTTY");
	LSBmodes.push_back("PKTLSB");

	qso_opMODE->index(0);
	qso_opMODE->activate();

	clearList();
	buildlist();

	windowTitle = _("Enter Xcvr Freq");
	setTitle();

	return true;
}

#if USE_HAMLIB
bool init_Hamlib_RigDialog()
{
	LOG_DEBUG("hamlib");

	qso_opBW->deactivate();
	qso_opMODE->clear();

	for (size_t i = 0; i < sizeof(modes)/sizeof(modes[0]); i++) {
		mode_nums[modes[i].name] = modes[i].mode;
		mode_names[modes[i].mode] = modes[i].name;
		qso_opMODE->add(modes[i].name);
	}
	clearList();
	buildlist();

	windowTitle = "Hamlib ";
	windowTitle.append(xcvr->getName());

	setTitle();

	return true;
}
#endif
