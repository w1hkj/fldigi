// ----------------------------------------------------------------------------
// synop.cxx  --  SYNOP decoding
//
// Copyright (C) 2012
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <time.h>

#include "re.h"

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <stdexcept>
#include <typeinfo>
#include <memory>

#include "config.h"
#include "synop.h"
#include "kmlserver.h"
#include "configuration.h"
#include "fl_digi.h"
#include "gettext.h"
#include "debug.h"
#include "field_def.h"

#include "record_loader.h"
#include "coordinate.h"
#include "strutil.h"

#include "FL/fl_ask.H"

// ----------------------------------------------------------------------------
// RTTY is 850 Hz shift, 75 baud, ITA2 Baudot code.
// 3231.0  KAWN  RTTY (Tune in LSB)
// 7784.0  KAWN  RTTY
// 11120.0  KAWN  RTTY (Tune in LSB)
// 13530.0  KAWN  RTTY
// 19324.5  KAWN  RTTY
// 19530.0  KAWN  RTTY (Usually "fox" marker)
// ----------------------------------------------------------------------------

/// Writes a string range to the current print callback.
static void disp_range( std::string::const_iterator b, std::string::const_iterator e, bool bold = false) {
	const char * first = &( *b );
	const char * last = &( *e );
	assert( first <= last );
	size_t nb = last - first ;
	synop::ptr_callback->print( first, nb, bold );
};

// ----------------------------------------------------------------------------

/// Builds a time with Synop messages which just have event's day/hour/min.
static time_t DayHourMin2Tm( int day, int hour, int min ) {
	// TODO: Check that time() is UTC too.
	time_t tmpTime = time(NULL);
	tm aTm = *gmtime(&tmpTime);

	// Maybe observation from previous month.
	if( day > aTm.tm_mday ) {
		aTm.tm_mon-- ;
		if( aTm.tm_mon < 0 ) {
			aTm.tm_mon = 11 ;
			aTm.tm_year-- ;
		}
	}
	aTm.tm_mday = day ;
	aTm.tm_hour = hour ;
	aTm.tm_min = min ;

	time_t my_time = mktime( & aTm );
	if( my_time <= 0 ) throw std::runtime_error("Invalid time");
	return my_time ;
}

/// Absolute time difference in number of days.
static int diffTm( time_t tim1, time_t tim2 )
{
	if( (tim1 <= 0 ) || (tim2 <= 0 ) ) throw std::runtime_error("Invalid times");

	double nbSecs = difftime( tim1, tim2 );
	return abs( 0.5 + nbSecs * ( 1.0 / ( 24 * 3600 ) ) );
}

/// We could use any other time as long as it is clear.
static std::string Tm2SynopTime( time_t tim ) {
	return KmlServer::Tm2Time( tim );
}
	
// ----------------------------------------------------------------------------

/// Base class for displaying the key-value pairs read from Synop broadcast.
class Serializer {
	/// It returns the previous value which can therefore be restored.
	Serializer * m_prevSerial ;

	/// This happens when switching temporarily to this serializer.
	static Serializer * m_srl ;
public:
	/// It registers itself as a serializer.
	Serializer() {
		m_prevSerial = m_srl ;
		m_srl = this ;
	}
	/// It restores the previous serializer.
	virtual ~Serializer() {
		m_srl = m_prevSerial ;
	}

	virtual void StartSection( const std::string & section_name ) = 0 ;
	virtual void AddItem( const char * key, const char * value, const char * unit ) = 0;

	static Serializer * Instance(void) {
		if( m_srl == NULL ) throw std::runtime_error("Null m_srl");
		return m_srl;
	}
};

// Current serializer, that is, the object which prints Synop attributes.
Serializer * Serializer::m_srl = NULL;

// ----------------------------------------------------------------------------

static const char * Unit_hPa           = "hPa";
static const char * Unit_degrees       = "degrees";
static const char * Unit_hours         = "hours";
static const char * Unit_minutes       = "mn";
static const char * Unit_seconds       = "seconds";
static const char * Unit_Celsius       = "°C";
static const char * Unit_knots         = "knots";
static const char * Unit_feet          = "feet";
static const char * Unit_km            = "km";
static const char * Unit_meters        = "meters";
static const char * Unit_centimeters   = "cm";
static const char * Unit_mm            = "mm";
static const char * Unit_meters_second = "m/s";

// ----------------------------------------------------------------------------

/// Compile-time number of elements of a static array.
#ifndef G_N_ELEMENTS
#define G_N_ELEMENTS(arr) ((sizeof(arr))/(sizeof(arr[0])))
#endif

/// For documentation only. Tells the WMO code table of an information.
#define FM12CodeTable( arr, num, txt )

// ----------------------------------------------------------------------------

/// Used to select from lookup tables.
template< class Key > struct choice {
	const Key    m_key;
	const char * m_val;
};

// Simple lookup.
template< class Key, class Dflt >
const char * choice_map( const choice< Key > * choices, size_t nb_choices, const Key & key, const Dflt & dflt )
{
	for( ; nb_choices; --nb_choices, ++choices )
	{
		if( key == choices->m_key ) return choices->m_val ;
	}
	return dflt ;
}

// ----------------------------------------------------------------------------


// There are about 11000 records, so memory usage is an issue.
class RecordWmoStation
{
	/// Block Number :2 digits representing the WMO-assigned block.
	int               m_block ;

	/// Station Number :3 digits representing the WMO-assigned station.
	int               m_station ;

	/// ICAO Location Indicator :4 alphanumeric characters,
	/// not all stations in this file have an assigned location indicator.
	/// The value "----" is used for stations that do not have an assigned location indicator.
	char              m_icao_indicator[5]; // TODO: Not used at the moment.

	/// Place Name :Common name of station location.
	std::string       m_name ;

	// State :2 character abbreviation (included for stations located in the United States only).

	// Country Name :Country name is ISO short English form.
	// TODO: Consider replacing it with an ISO integer key.
	std::string       m_country;

	// WMO Region :digits 1 through 6 representing the corresponding WMO region, 7 stands for the WMO Antarctic region.

	// Station Latitude or Latitude :DD-MM-SSH where DD is degrees, MM is minutes, SS is seconds 
	// and H is N for northern hemisphere or S for southern hemisphere or
	// E for eastern hemisphere or W for western hemisphere.
	// The seconds value is omitted for those stations where the seconds value is unknown.
	CoordinateT::Pair m_station_coordinates ;

	/// Upper Air Latitude :DD-MM-SSH.

	/// Station Elevation (Ha) :The station elevation in meters. Value is omitted if unknown.
	int               m_station_elevation ;

	/// Upper Air Elevation (Hp) :The upper air elevation in meters. Value is omitted if unknown.

	/// RBSN indicator :P if station is defined by the WMO as belonging to the Regional Basic Synoptic Network, omitted otherwise.

	/// Delimiter between values in a record.
	static const char m_delim = ';';
public:
	/// More information here: http://weather.noaa.gov/tg/site.shtml
	RecordWmoStation()
	: m_block(0)
	, m_station(0)
	{}

	int wmo_indicator() const { return m_block * 1000 + m_station; }

	const CoordinateT::Pair & station_coordinates() const { return m_station_coordinates; }
	int station_elevation(void) const { return m_station_elevation; }

	const std::string & country() const { return m_country; }

	/// This garantees an unique name.
	const std::string & station_name() const { return m_name; }
	void rename_station( const std::string & nam ) {
		// LOG_INFO("Renaming %s to %s", m_name.c_str(), nam.c_str() );
		m_name = nam;
	}

	// http://weather.noaa.gov/data/nsd_bbsss.txt
	// 01;023;ENDU;Bardufoss;;Norway;6;69-04N;018-32E;;;7;79;P
	// Wrong record:
	// 71;113;CWZA;Agassiz Automated Reporting Station ;;Canada;;49-15N;121-46W;;;15;;
	friend std::istream & operator>>( std::istream & istrm, RecordWmoStation & rec )
	{
		if( read_until_delim( m_delim, istrm, rec.m_block                                                 )
		&&  read_until_delim( m_delim, istrm, rec.m_station                                               )
		&&  read_until_delim( m_delim, istrm, rec.m_icao_indicator                                        )
		&&  read_until_delim( m_delim, istrm, rec.m_name                                                  )
		&&  read_until_delim( m_delim, istrm  /* State */                                                 )
		&&  read_until_delim( m_delim, istrm, rec.m_country                                               )
		&&  read_until_delim( m_delim, istrm  /* WMO region */                                            )
		&&  read_until_delim( m_delim, istrm, rec.m_station_coordinates.latitude()                        )
		&&  read_until_delim( m_delim, istrm, rec.m_station_coordinates.longitude()                       )
		&&  read_until_delim( m_delim, istrm  /* Upper air latitude */                                    )
		&&  read_until_delim( m_delim, istrm  /* Upper air longitude */                                   )
		&&  read_until_delim( m_delim, istrm, rec.m_station_elevation,                 0                  )
		&&  read_until_delim( m_delim, istrm  /* Upper air elevation */                                   )
		&&  read_until_delim( m_delim, istrm  /* Rsbn indicator */                                        )

		&& ( rec.m_station_coordinates.latitude().is_lon() == false    )
		&& ( rec.m_station_coordinates.longitude().is_lon() == true    )
		) 
		{
			strtrim( rec.m_name );
			return istrm ;
		}

		istrm.setstate(std::ios::badbit);
		return istrm ;
	}

	friend std::ostream & operator<< ( std::ostream & ostrm, const RecordWmoStation & rec )
	{
		ostrm << rec.m_name ;
		return ostrm;
	}
}; // RecordWmoStation

// ----------------------------------------------------------------------------
// srst2;29.683 N;94.033 W;0.7;Sabine Pass, TX;NDBC;NDBC Meteorological/Ocean;fixed
// ssbn7;33.842 N;78.476 W;;Sunset Beach Nearshore Waves;CORMP;IOOS Partners;other
// Adapted from http://www.ndbc.noaa.gov/ndbcmapstations.json
// ftp://tgftp.nws.noaa.gov/data/observations/marine/stations/station_table.txt
// # STATION_ID | OWNER | TTYPE | HULL | NAME | PAYLOAD | LOCATION | TIMEZONE | FORECAST | NOTE
// #
// 0y2w3|CG|Weather Station||Sturgeon Bay CG Station, WS||44.794 N 87.313 W (44&#176;47'40" N 87&#176;18'47" W)|C| |
// 13001|PR|Atlas Buoy|PM-595|NE Extension||12.000 N 23.000 W (12&#176;0'0" N 23&#176;0'0" W)|| |

/// This allows to find a buoy characteristics.
class RecordBuoy {
	std::string       m_id ;
	std::string       m_owner ;    /// "UW" : University of Washington etc...
	std::string       m_type ;     /// "Offshore Buoy", "6-meter NOMAD buoy" etc...
	std::string       m_hull;      /// "PM-599", "3D37"
	std::string       m_name ;     /// "Ecuador INOCAR", "150 NM East of Cape HATTERAS"
	std::string       m_payload;   /// "AMPS payload"
	CoordinateT::Pair m_location ; /// "32.501 N 79.099 W (32&#176;30'2" N 79&#176;5'58" W)"
	std::string       m_timezone ; /// "E"
	std::string       m_forecast;  /// "FZUS52.KCHS FZNT22.KWBC"
	std::string       m_note;      /// "This buoy was removed ..."

	/// From the CVS file.
	static const char m_delim = '|';
public:
	/// Unique identifier in a catalog.
	const std::string & id() const { return m_id; }

	const CoordinateT::Pair & station_coordinates() const { return m_location; }

	/// The name coming from the file might not be unique.
	const std::string & buoy_name(void) const { return m_name;}

	/// Needed because buoy names are not unique
	void rename_buoy( const std::string & new_buoy_name ) { m_name = new_buoy_name ; }

	const std::string & owner(void) const { return m_owner;}
	const std::string & payload(void) const { return m_payload;}
	const std::string & note(void) const { return m_note;}

	/// TODO: Speedup by doing that once only.
	std::string title(void) const {
		std::string title = m_type ;
		if( ! m_name.empty() ) title += ":" + m_name ;
		if( ! m_payload.empty() ) title += ":" + m_payload ;
		if( ! m_owner.empty() ) title += ":" + m_owner ;
		return title ;
	}

	/// fixed, other, dart, buoy, oilrig, tao
	const std::string & type(void) const { return m_type;}

	friend std::istream & operator>>( std::istream & istrm, RecordBuoy & rec )
	{
		if( read_until_delim( m_delim, istrm, rec.m_id        )
		&&  read_until_delim( m_delim, istrm, rec.m_owner     )
		&&  read_until_delim( m_delim, istrm, rec.m_type      )
		&&  read_until_delim( m_delim, istrm, rec.m_hull      )
		&&  read_until_delim( m_delim, istrm, rec.m_name      )
		&&  read_until_delim( m_delim, istrm, rec.m_payload   )
		&&  read_until_delim( m_delim, istrm, rec.m_location  )
		&&  read_until_delim( m_delim, istrm, rec.m_timezone  )
		&&  read_until_delim( m_delim, istrm, rec.m_forecast  )
		&&  read_until_delim( m_delim, istrm, rec.m_note      )
		) 
		{
			// std::cout << "id=" << rec.m_id << " name=" << rec.m_name << "\n";
			return istrm ;
		}

		istrm.setstate(std::ios::badbit);
		return istrm ;
	}
}; // RecordBuoy

// ----------------------------------------------------------------------------
// http://www.metoffice.gov.uk/media/csv/e/7/ToR-Stats-SHIP.csv
// June 2012,,,,,,,,
// CTRY,CALLSIGN,NAME,Observations,N<30,N<60,N<120,N>360,Average (R-O) (mins)
//     , B2M1297, ,1,1,1,1,0,30
//     , B2M1303, ,17,16,17,17,0,17.9
//     , BATEU00, ,366,366,366,366,0,5

class RecordShip {
	std::string m_callsign;
	std::string m_country;
	std::string m_name ;

	static const char m_delim = ',';
public:
	const std::string & callsign(void) const { return m_callsign; }
	const std::string & country(void) const { return m_country; }
	const std::string & name(void) const { return m_name; }

	friend std::istream & operator>>( std::istream & istrm, RecordShip & rec )
	{
		if( read_until_delim( m_delim, istrm, rec.m_country   )
		&&  read_until_delim( m_delim, istrm, rec.m_callsign  )
		&&  read_until_delim( m_delim, istrm, rec.m_name      )
		&&  read_until_delim( m_delim, istrm                  )
		&&  read_until_delim( m_delim, istrm                  )
		&&  read_until_delim( m_delim, istrm                  )
		&&  read_until_delim( m_delim, istrm                  )
		&&  read_until_delim( m_delim, istrm                  )
		&&  read_until_delim( m_delim, istrm                  )
		) 
		{
			strtrim( rec.m_country );
			strtrim( rec.m_callsign );
			strtrim( rec.m_name );
			// std::cout << "id=" << rec.m_callsign << " name=" << rec.m_name << "\n";
			strcapitalize( rec.m_name );
			// std::cout << "id=" << rec.m_callsign << " name=" << rec.m_name << "\n";
			return istrm ;
		}

		istrm.setstate(std::ios::badbit);
		return istrm ;
	}
}; // RecordShip

// ----------------------------------------------------------------------------

/**
Huge file, but contains many information.
There are many duplicates for each WMO indicator, it does not matter for us.
ftp://ftp.jcommops.org/JCOMMOPS/GTS/wmo/wmo_list.txt

GTS stands for Global Telecommunications System

JCOMMOPS WMO-PLATFORM cross reference list as of 2012-09-08 for WMO number still allocated in the last 6 months (with WMO Ids that are numerical)
WMO;TELECOM ID;TELECOM SYSTEM;PTFM NAME;PTFM FAMILY;PTFM TYPE;CONTACT NAME;EMAIL;PROGRAM;ROLE;AGENCY;COUNTRY;ALLOC_DATE;DEALLOC_DATE;ARGOS_PROG
10044;BSH:10044;METEOSAT;MB:10044;FIXED;METSTATION;;;GERMANY MB;Program Manager;KIEL UNIVERSITY;DEU;2001-01-01;3000-01-01;
10044;BSH:10044;METEOSAT;MB:10044;FIXED;METSTATION;;;GERMANY MB;Programme GTS Coordinator;KIEL UNIVERSITY;DEU;2001-01-01;3000-01-01;
11908;88671;ARGOS;ARGOS:88671;DB;SVPBD2;;;AOML-GDP;Programme GTS Coordinator;NOAA/AOML;USA;2011-07-19;3000-01-01;

Apparently, special codes here:
http://www.meds-sdmm.dfo-mpo.gc.ca/isdm-gdsi/international-internationale/j-comm/CODES/wmotable_e.htm
*/

/// JComm stands for Joint Commission on Oceanography and Marine Meteorology (J-COMM)
class RecordJComm {
	std::string m_wmo ; // TODO: This could be an integer.
	std::string m_telecom_system ;
	std::string m_ptfm_name ;
	std::string m_ptfm_family ;
	std::string m_ptfm_type ;
	std::string m_program ;
	std::string m_agency ;
	std::string m_country ;

	static const char m_delim = ';';
public:
	/// TODO: This could be an integer.
	const std::string & wmo()            const { return m_wmo; }
	const std::string & telecom_system() const { return m_telecom_system; }
	const std::string & ptfm_name()      const { return m_ptfm_name; }
	const std::string & ptfm_family()    const { return m_ptfm_family; }
	const std::string & ptfm_type()      const { return m_ptfm_type; }
	const std::string & program()        const { return m_program; }
	const std::string & agency()         const { return m_agency; }
	const std::string & country()        const { return m_country; }

	friend std::istream & operator>>( std::istream & istrm, RecordJComm & rec )
	{
		if( read_until_delim( m_delim, istrm, rec.m_wmo            )
		&&  read_until_delim( m_delim, istrm  /* Telecom Id */     )
		&&  read_until_delim( m_delim, istrm, rec.m_telecom_system )
		&&  read_until_delim( m_delim, istrm, rec.m_ptfm_name      )
		&&  read_until_delim( m_delim, istrm, rec.m_ptfm_family    )
		&&  read_until_delim( m_delim, istrm, rec.m_ptfm_type      )
		&&  read_until_delim( m_delim, istrm  /* Contact Name */   )
		&&  read_until_delim( m_delim, istrm  /* Email */          )
		&&  read_until_delim( m_delim, istrm, rec.m_program        )
		&&  read_until_delim( m_delim, istrm  /* Role */           )
		&&  read_until_delim( m_delim, istrm, rec.m_agency         )
		&&  read_until_delim( m_delim, istrm, rec.m_country        )
		&&  read_until_delim( m_delim, istrm  /* Alloc Date */     )
		&&  read_until_delim( m_delim, istrm  /* Dealloc Date */   )
		&&  read_until_delim( m_delim, istrm  /* Argos Prog */     )
		) 
		{
			return istrm ;
		}

		istrm.setstate(std::ios::badbit);
		return istrm ;
	}

	// Unfortunately the physical type of the station is embedded in the ptfm_name.
	// So we rearrange data, to have a decent icon name. Handles cases such as:
	// 42365;Ursa809;IRIDIUM;PLATFORM:42365;FIXED;FIXED
	// 62130;IRID:62130;IRIDIUM;OILPLAT:62130;MB;MB;
	// Very rough solution, quite OK for the moment.
	void SetJCommFields( std::string & kmlNam, std::string & iconNam ) const {
		if(
			( strstr( m_ptfm_name.c_str(), "PLATFORM" ) )
		||	( strstr( m_ptfm_name.c_str(), "OILPLAT" ) )
		) {
			kmlNam = agency() + ":(" + m_wmo + ")";
			iconNam = "Oil Platform";
		} else if(
			( strstr( m_ptfm_name.c_str(), "BUOY" ) )
		) {
			kmlNam = agency() + ":(" + m_wmo + ")";
			iconNam = "Buoy";
		} else {
			kmlNam = ptfm_name() + "," + agency();
			iconNam = telecom_system();
		}
		strcapitalize(kmlNam);
	}
}; // RecordJComm

// ----------------------------------------------------------------------------

/// This wraps a record type and allows to load a cvs file and access it using a key.
template< class Key, class Record, Key (Record::*Method)(void) const, class Terminal >
class Catalog : public RecordLoader< Terminal >
{
	/// The keying method might return a reference instead of a value.
	template< class Type > struct deref { typedef Type type ; };
	template< class Type > struct deref< Type & > { typedef Type type ; };
	/// If the return value of the indexing function is for example a reference
	/// to a const string, then KeyType is a string.
	template< class Type > struct deref< const Type & > { typedef Type type ; };
protected:
	typedef typename deref< Key >::type KeyType ;
	typedef std::map< KeyType, Record > CatalogType ;
	typedef typename CatalogType::iterator IteratorType ;

	CatalogType m_catalog ;

	bool FillAndTest() {
		int nbRec = this->LoadAndRegister();
		if( nbRec < 0 ) {
			return false ;
		}
		else {
			LOG_INFO("record=%s nb_recs=%d", typeid(Record).name(), nbRec );
			return true ;
		}
	}
public:
	void Clear() {
		m_catalog.clear();
	}
	bool ReadRecord( std::istream & istrm ) {
		Record tmp ;
		istrm >> tmp ;
		if( istrm || istrm.eof() ) {
			m_catalog[ (tmp.*Method)() ] = tmp ;
			return true ;
		}
		return false;
	}

	/// Returns a station with wmo_indicator to zero if cannot find the right one.
	static const Record * FindFromKey( Key key )
	{
		CatalogType & refCat = RecordLoader< Terminal >::InstCatalog().m_catalog;

		typename CatalogType::const_iterator it = refCat.find( key );
		return ( it == refCat.end() ) ? NULL : &it->second ;
	}

	bool Fill() {
		return FillAndTest();
	}

}; // Catalog

// ----------------------------------------------------------------------------

/// This contains all WMO stations records read from the file.
class CatalogWmoStations : public Catalog< int, RecordWmoStation, &RecordWmoStation::wmo_indicator, CatalogWmoStations >
{
public:
	// After data loading, there is an extra step to ensure that names are unique.
	// 85;196;SLCP;Concepcion;;Bolivia;3;16-09S;062-01W;;;497;;
	// 85;682;SCIE;Concepcion;;Chile;3;36-46S;073-03W;;;12;;P
	// 86;134;SGCO;Concepcion;;Paraguay;3;23-25S;057-18W;;;74;74;
	bool Fill()
	{
		if( ! FillAndTest() ) return false ;

		typedef std::multimap< std::string, IteratorType > HashT ;
		HashT allNames ;

		// First take the names 
		// LOG_INFO("Eliminating duplicates out of %d elements",m_catalog.size());
		for( IteratorType it = m_catalog.begin(), en = m_catalog.end(); it != en; ++it )
		{
			RecordWmoStation & refWmo = it->second ;
			allNames.insert( allNames.end(), HashT::value_type( refWmo.station_name(), it ) );
		}

		size_t nbDupl = 0 ;

		// Iterates on all names, take only the duplicates.
		for( HashT::iterator itH = allNames.begin(), itNextH = itH, enH = allNames.end(); itH != enH; itH = itNextH )
		{
			// LOG_INFO("Name=%s", itH->first.c_str() );
			size_t nbKeys = 1 ;
			for(;;) {
				++itNextH;
				if( itNextH == enH ) break ;
				if( itNextH->first != itH->first ) break ;
				++nbKeys;
			}
			// LOG_INFO("Name=%s nb=%d", itH->first.c_str(), nbKeys );

			// If no duplicates, then try next one.
			if( nbKeys == 1 ) continue ;

			++nbDupl ;
			// LOG_INFO("%d: Name %s %d occurrences", nbDupl, itH->first.c_str(), nbKeys );

			// There should not be many elements, two or three duplicates, maximum five apparently.
			typedef std::set< std::string > DiffNamesT ;
			DiffNamesT differentNames ;
			// Check that all countries are different.
			for( HashT::iterator itSubH = itH; itSubH != itNextH; ++itSubH ) {
				RecordWmoStation & refWmo = itSubH->second->second ;
				// LOG_INFO("Trying %s", refWmo.station_name().c_str() );
				// Appends the country.
				refWmo.rename_station( refWmo.station_name() + "," + refWmo.country() );
				std::pair< DiffNamesT::iterator, bool > tmpPair = differentNames.insert( refWmo.station_name() );
				if( tmpPair.second ) continue ;
				// Appends the WMO
				refWmo.rename_station( strformat( "%s,%05d", refWmo.station_name().c_str(), refWmo.wmo_indicator() ) );
				tmpPair = differentNames.insert( refWmo.station_name() );
				if( tmpPair.second ) continue ;
				LOG_ERROR("This should never happen because WMO indicator is unique");
				return false ;
			}
		}

		if(nbDupl) {
			LOG_INFO("Eliminated %d duplicates out of %d elements", (int)nbDupl, (int)m_catalog.size());
		}
		return true ;
	}

	/// A static file with the same name is also installed in directory data.
	const char * Url(void) const {
		return "http://weather.noaa.gov/data/nsd_bbsss.txt";
	}

	const char * Description() const {
		return _("WMO stations");
	}
}; // CatalogWmoStations

// ----------------------------------------------------------------------------

/// This contains all WMO stations records read from the file.

/// Derived class necessary because names are not unique and must be revised.
class CatalogBuoy : public Catalog< const std::string &, RecordBuoy, &RecordBuoy::id, CatalogBuoy >
{
public:
	/// After data loading, there is an extra step to ensure that names are unique.
	bool Fill()
	{
		if( ! FillAndTest() ) return false ;

		typedef std::multimap< std::string, IteratorType > HashT ;
		HashT allNames ;

		// LOG_INFO("Eliminating duplicates out of %d elements",m_catalog.size());

		/// First take the names 
		for( IteratorType it = m_catalog.begin(), en = m_catalog.end(); it != en; ++it )
		{
			RecordBuoy & refWmo = it->second ;
			allNames.insert( allNames.end(), HashT::value_type( refWmo.buoy_name(), it ) );
		}

		size_t nbDupl = 0 ;

		/// Iterates on all names, take only the duplicates.
		for( HashT::iterator itH = allNames.begin(), itNextH = itH, enH = allNames.end(); itH != enH; itH = itNextH )
		{
			// LOG_INFO("Name=%s", itH->first.c_str() );
			size_t nbKeys = 1 ;
			for(;;) {
				++itNextH;
				if( itNextH == enH ) break ;
				if( itNextH->first != itH->first ) break ;
				++nbKeys;
			}
			// LOG_INFO("Name=%s nb=%d", itH->first.c_str(), nbKeys );

			// If no duplicates, then try next one.
			if( nbKeys == 1 ) continue ;

			++nbDupl ;
			// LOG_INFO("%d: Buoy name %s %d occurrences", nbDupl, itH->first.c_str(), nbKeys );

			// There should not be many elements, two or three duplicates, maximum five apparently.
			typedef std::set< std::string > DiffNamesT ;
			DiffNamesT differentNames ;
			// Check that all countries are different.
			for( HashT::iterator itSubH = itH; itSubH != itNextH; ++itSubH ) {
				RecordBuoy & refBuoy = itSubH->second->second ;
				// Appends the id
				if( refBuoy.buoy_name().empty() )
					refBuoy.rename_buoy( refBuoy.id().c_str() );
				else
					refBuoy.rename_buoy( strformat( "%s-%s", refBuoy.buoy_name().c_str(), refBuoy.id().c_str() ) );
				std::pair< DiffNamesT::iterator, bool > tmpPair = differentNames.insert( refBuoy.buoy_name() );
				// LOG_INFO("Buoy set to %s", refBuoy.buoy_name().c_str() );
				if( tmpPair.second ) continue ;
				LOG_ERROR("This should never happen because buoy id is unique");
				return false ;
			}
		}

		if(nbDupl) {
			LOG_INFO("Eliminated %d duplicates out of %d elements", (int)nbDupl, (int)m_catalog.size());
		}
		return true ;
	}

	/// A static file with the same name is also installed in directory data.
	const char * Url(void) const {
		return "ftp://tgftp.nws.noaa.gov/data/observations/marine/stations/station_table.txt";
	}
	const char * Description() const {
		return _("Weather buoys");
	}
}; // CatalogBuoy

// ----------------------------------------------------------------------------

/// Known list of VOS weather ships, Volunteer Observing Ships.
struct CatalogShip : public Catalog< const std::string &, RecordShip, &RecordShip::callsign, CatalogShip >
{
	/// A static file with the same name is also installed in directory data.
	const char * Url(void) const {
		return "http://www.metoffice.gov.uk/media/csv/e/7/ToR-Stats-SHIP.csv";
	}
	const char * Description() const {
		return _("Weather ships");
	}
};

/// Another public-domain file, returns information given a WMO-like key.
struct CatalogJComm : public Catalog< const std::string &, RecordJComm, &RecordJComm::wmo, CatalogJComm >
{
	/// A static file with the same name is also installed in directory data.
	const char * Url(void) const {
		return "ftp://ftp.jcommops.org/JCOMMOPS/GTS/wmo/wmo_list.txt";
	}
	const char * Description() const {
		return _("Argos & Iridium");
	}
};

// ----------------------------------------------------------------------------

// Returns true if properly initialised.
// TODO: Have all the derived class link to the base class so the initialisation
// can be done without having to enumerate the sub-classes.
bool SynopDB::Init( const std::string & data_dir )
{
	try
	{
		return	CatalogWmoStations::InstCatalog().Fill()
		&&	CatalogBuoy::InstCatalog().Fill       ()
		&&	CatalogShip::InstCatalog().Fill       ()
		&&	CatalogJComm::InstCatalog().Fill      ();
	}
	catch( const std::exception & exc )
	{
		fl_alert("Could not load SYNOP data files: Exception=%s", exc.what() );
		return false ;
	}
	return true;
}

/// For testing purpose only in stand-alone program flsynop.
const std::string & SynopDB::IndicatorToName( int wmo_indicator )
{
	const RecordWmoStation * ptrWmo = CatalogWmoStations::FindFromKey( wmo_indicator );
	static const std::string empty_str ;
	return ( ptrWmo == NULL ) ? empty_str : ptrWmo->station_name();
}

/// For testing purpose only in stand-alone program flsynop.
const std::string SynopDB::IndicatorToCoordinates( int wmo_indicator )
{
	const RecordWmoStation * ptrWmo = CatalogWmoStations::FindFromKey( wmo_indicator );
	if ( ptrWmo == NULL ) return "No coordinates";
	std::stringstream strm ;
	strm << ptrWmo->station_coordinates() << " " ;
	strm << ptrWmo->station_coordinates().longitude().angle() << " " ;
	strm << ptrWmo->station_coordinates().latitude().angle() << " " ;
	return strm.str();
}

/// For testing purpose only in stand-alone program flsynop.
const std::string & SynopDB::BuoyToName( const char * buoy_id )
{
	const RecordBuoy * ptrBuoy = CatalogBuoy::FindFromKey( buoy_id );
	static const std::string empty_str ;
	return ( ptrBuoy == NULL ) ? empty_str : ptrBuoy->buoy_name();
}

/// For testing purpose only in stand-alone program flsynop.
const std::string & SynopDB::ShipToName( const char * ship_id )
{
	const RecordShip * ptrShip = CatalogShip::FindFromKey( ship_id );
	static const std::string empty_str ;
	return ( ptrShip == NULL ) ? empty_str : ptrShip->name();
}

/// For testing purpose only in stand-alone program flsynop.
const std::string & SynopDB::JCommToName( const char * wmo )
{
	const RecordJComm * ptrJComm = CatalogJComm::FindFromKey( wmo );
	static const std::string empty_str ;
	return ( ptrJComm == NULL ) ? empty_str : ptrJComm->ptfm_name();
}

// ----------------------------------------------------------------------------


/// Intrusive reference counter associated with RefCntPtr.
template<class Obj>
class WithRefCnt {
	size_t m_ref_cnt ;
public:
	WithRefCnt() : m_ref_cnt(0) {};

	template<class Obj2>
	friend class RefCntPtr;
};

/// Intrusive smart pointer with reference counting.
template< class Obj >
class RefCntPtr {
	/// This must derive from the class WithRefCnt.
	Obj * m_ptr ;
public:
	RefCntPtr() : m_ptr(NULL) {}
	/// The reference counter is incremented.
	RefCntPtr( Obj * ptr ) : m_ptr(ptr) {
		if( ptr ) ++ptr->m_ref_cnt ;
	}
	/// Deletes the pointed object if reference count reaches zero.
	~RefCntPtr() {
		if( m_ptr ) {
			--m_ptr->m_ref_cnt ;
			if( m_ptr->m_ref_cnt == 0 ) delete m_ptr ;
		}
	}
	/// The reference counter is incremented.
	RefCntPtr( const RefCntPtr & ptr ) : m_ptr( ptr.m_ptr ) {
		if( m_ptr ) ++m_ptr->m_ref_cnt ;
	}
	/// The reference counter is incremented.
	RefCntPtr & operator=( const RefCntPtr & ptr ) {
		if( this != &ptr ) {
			m_ptr = ptr.m_ptr ;
			if( m_ptr ) ++m_ptr->m_ref_cnt ;
		}
		return *this;
	}
	const Obj * operator->() const { return m_ptr;}
	Obj * operator->() { return m_ptr;}
	operator bool() const { return m_ptr; }
};

// ----------------------------------------------------------------------------

/// Synop section number.
enum section_t {
	SECTION_ZCZC_DLM=0,
	SECTION_HEAD_GRP,
	SECTION_IDENTLOC,   // 000
	SECTION_LAND_OBS,   // 111
	SECTION_SEA_SURF,   // 222
	SECTION_CLIM_DAT,   // 333
	SECTION_NATCLOUD,   // 444
	SECTION_NAT_CODE,   // 555
	SECTION_AUTO_DAT,
	SECTION_NNNN_DLM,
	SECTION_SECT_NBR
};

static const char * SectionToString( section_t group_number ) {
	switch(group_number) {
		case SECTION_ZCZC_DLM : return _("Bulletin start");
		case SECTION_HEAD_GRP : return _("Header");
		case SECTION_IDENTLOC : return _("Identification and location");
		case SECTION_LAND_OBS : return _("Land observations");
		case SECTION_SEA_SURF : return _("Sea surface observations");
		case SECTION_CLIM_DAT : return _("Climatological data");
		case SECTION_NATCLOUD : return _("National data, clouds");
		case SECTION_NAT_CODE : return _("National data");
		case SECTION_AUTO_DAT : return _("Automatisch erzeugte Daten");
		case SECTION_NNNN_DLM : return _("Bulletin end");
 		default               : return _("Unknown Synop group");
	}
}

/// [nextSec][predSec] : Indicates that section Next can be preceded by section Pred.
static const char sectionTransitions[SECTION_SECT_NBR][SECTION_SECT_NBR] = {
	// ZCZC_DLM HEAD_GRP IDENTLOC LAND_OBS SEA_SURF CLIM_DAT NATCLOUD NAT_CODE AUTO_DAT NNNN_DLM
	{  0,       0,       0,       0,       0,       0,       0,       0,       0,       0 }, // ZCZC_DLM
	{  1,       0,       0,       0,       0,       0,       0,       0,       0,       0 }, // HEAD_GRP
	{  0,       1,       0,       0,       0,       0,       0,       0,       0,       0 }, // IDENTLOC
	{  0,       1,       1,       0,       0,       0,       0,       0,       0,       0 }, // LAND_OBS
	{  0,       0,       0,       1,       0,       0,       0,       0,       0,       0 }, // SEA_SURF
	{  0,       0,       0,       1,       1,       0,       0,       0,       0,       0 }, // CLIM_DAT
	{  0,       0,       0,       1,       1,       1,       0,       0,       0,       0 }, // NATCLOUD
	{  0,       0,       0,       1,       1,       1,       1,       0,       0,       0 }, // NAT_CODE
	{  0,       0,       0,       1,       1,       1,       1,       1,       0,       0 }, // AUTO_DAT
	{  1,       1,       1,       1,       1,       1,       1,       1,       1,       0 }  // NNNN_DLM
};

// ----------------------------------------------------------------------------

/// Priority helps to classify regular expressions matching the same work.
typedef double priority_t ;

/// Minimum priorities sum to be accepted as a valid chain of tokens.
static const priority_t MIN_PRIO = 2.0 ;

/// This models a Synop token, that is, a group of (Most of time) five digits.
class TokenProxy : public std::string, public WithRefCnt< TokenProxy >
{
public:
	typedef RefCntPtr< TokenProxy > Ptr ;

	/// List of pointers to TokenProxy, each of them associated to a token and its regex.
	class TokVec : public std::vector< Ptr > , public WithRefCnt< TokVec >
	{
		TokVec( const TokVec & );
		TokVec & operator=(const TokVec &);

		/// For debugging, writes the regular expression.
		void DumpTotal(section_t group_number, bool kml_mode) const {
			// This information is not needed at the moment,
			// but it is not possible to discard it if the section is empty, because
			// we do not know that yet.
			Serializer * ptrSerial = Serializer::Instance();
			if(kml_mode) {
				ptrSerial->StartSection( SectionToString(group_number) );
			}

			for( size_t i = 0; i < size(); ++ i ) {
				TokenProxy & prox = *const_cast< TokenProxy *>( at(i).operator->() );
				/// If set, this adds in the output which regular expression was detected.
				static const bool addRegexToOutput = false ;
				if( addRegexToOutput && ! kml_mode ) {
					ptrSerial->AddItem( "Regex", prox.RegexName(), prox.c_str() );
				}
				if(kml_mode) {
					prox.DrawKml();
				} else {
					prox.Print();
				}
			}
		}

	public:
		typedef RefCntPtr< TokVec > Ptr ;
	private:
		/// Chains (sections) are linked so that any section can search the previous ones.
		TokVec::Ptr m_previous_container ;
	public:
		TokVec() {};

		/// Used to find a preceding token containing a specific information in a previous tokens chain.
		TokVec::Ptr previous() const { return m_previous_container; }

		void previous(TokVec::Ptr prev) { m_previous_container = prev; }

		/// Debugging purpose: It outputs the concatenation of all regular expressions names.
		std::string MiniDump(section_t group_number,bool disp_all = false) const {
			std::stringstream strm ;
			for( size_t i = 0; i < size(); ++ i ) {
				TokenProxy & prox = *const_cast< TokenProxy *>( at(i).operator->() );
				strm << prox.RegexName() ;
				if( disp_all ) {
					strm << "#" << static_cast<const std::string &>(prox);
				}
				strm << '+';
			}
			return strm.str();
		}

		void DumpTokVec(bool test_mode, section_t group_number, bool kml_mode) const {
			if( test_mode )
			{
				// Just displays the regular expression name, for debugging.
				std::string str( MiniDump(group_number) );
				disp_range( str.begin(), str.end() );
			}
			else
			{
				DumpTotal(group_number, kml_mode);
			}
		}
	};
private:
	/// The index of the regular expression that this token uses.
	int                m_regex_idx;

	/// Pointer to the chain of TokenProxy.
	TokVec::Ptr        m_container ;

	/// Offset in the input buffer which indicates the end of what is parsed.
	size_t             m_offset_end ;

	virtual bool Parse( const char * str ) = 0 ;

	/* TODO: Output style.
	10035 [Germany, 54ø32'N 009ø33'E SCHLESWIG]
	11575 [manned] [cloud height:600-1000m] [visibility:25km]
	42905 [cloud cover:4/8] [wind dir:290 deg, speed:5]
	10066 [air temp:+6.6]
	21022 [dew-point temp:-2.2]
	30131 [pressure at station level:1013.1hPa]
	40190 [pressure at sea level:1019.0hPa]
	53014 [pressure:increasing rapidly] [change in 3h:1.4hPa]
	69922 [precipitation:0.2mm during last 12 hours]
	70181 [past wx: shower(s), cloud cover 1/2 of sky]
   	      [wx now: Clouds generally dissolving or becoming less developed]
	84801 [cloud info]
  	333 [section 3]
	10094 [maximum temp:+9.4]
	20029 [minimum temp:+2.9]
	55307 81823 [clouds:1/8 or less, cumulus, 690m]
	83633 [clouds:3/8, stratocumulus, 990m]

	The method Append() will print the five-digits group.
	Later on, duplicate and consecutive groups will be suppressed.
	*/
	void ItemAdd( const char * key, const char * value, const char * unit ) const {
		Serializer::Instance()->AddItem( key, value, unit );
	}

	virtual void Print() const = 0 ;
	virtual void DrawKml() const { Print() ; }

	TokenProxy & operator=(const TokenProxy & );
protected:
	void disp_arr(const char ** array,size_t arrsz,int idx,int offset,const char * title) const
	{
		/// It is a Synop convention that slashes mean no information.
		if( idx == '/' ) return ;
		size_t idx_array = idx - offset;
		char buffer[64];
		const char * val;
		if( idx_array < arrsz )
			val = array[idx_array];
		else {
			sprintf( buffer, _("Unknown index %d/%d/%d"), (int)idx, (int)idx_array, (int)arrsz );
			val = buffer ;
		}
		Append( title, val );
	}

	TokenProxy() : m_regex_idx(-1), m_offset_end(0) {}

public:
	virtual ~TokenProxy() {}

	/// The boolean flag allows to search only in the current chain but also
	/// in the previous ones backward. This could be faster by restricting
	/// only to given section numbers.
	template< class TokenDerived >
	const TokenDerived * get_ptr(bool previous_chains) const ;

	template< class TokenDerived >
	static Ptr Generator( int rgx_idx, const std::string & wrd, size_t off ) {
		TokenProxy * ptr = new TokenDerived ;
		ptr->m_regex_idx = rgx_idx ;
		static_cast< std::string & >(*ptr) = wrd ;
		ptr->m_offset_end = off ;
		return Ptr(ptr);
	};

	void Section( const char * key ) const {
		Serializer::Instance()->StartSection( key );
	}

	static const bool tstDisp = false ;

	template< class Val >
	void Append( const char * key, const Val & val, const char * unit = NULL ) const
	{
		std::stringstream strm ;
		strm << val ;
		ItemAdd( key, strm.str().c_str(), unit );
	}
 
	void Append( const char * key, const std::string & val, const char * unit = NULL ) const
	{
		ItemAdd( key, val.c_str(), unit );
	}

	void Append( const char * key, const char * val, const char * unit = NULL ) const
	{
		ItemAdd( key, val, unit );
	}
 
	void Append( const char * key, double val, const char * unit = NULL ) const
	{
		char buf[20];
		sprintf( buf, "%.1lf", val );
		ItemAdd( key, buf, unit );
	}
 
	void Append( const char * key, int val, const char * unit = NULL ) const
	{
		char buf[12];
		sprintf( buf, "%d", val );
		ItemAdd( key, buf, unit );
	}

	/// This ensures that each token points to the chain containing it.
	friend void PushItself( Ptr mySelf, TokVec::Ptr ptr ) {
		mySelf->m_container = ptr ;
		ptr->push_back( mySelf );
	};

	size_t offset_begin(void) const {
		assert( m_offset_end >= size() );
		return m_offset_end - size();
	}
	size_t offset_end(void) const { return m_offset_end; }

	const char * RegexName(void) const ;

	bool ParseItself(void) {
		return Parse( c_str() );
	}

	/// It is dependent on the previous token.
	// TODO: This should be explicitely dependent on a preceding regex.
	virtual bool CanComeFirst(void) const { return true; }
}; // TokenProxy

typedef TokenProxy::Ptr (*ProxyGen)( int reg_idx, const std::string & wrd, size_t txt_offset);

/// Stores the regular expression associated to a Synop code,
/// plus a factory to create an object modelizing this code.
class RegexT : public WithRefCnt< RegexT >
{
	const char * m_str ;
	const char * m_name ;
	regex_t      m_regex ;
	ProxyGen     m_generator ;
	priority_t   m_priority ;

	void treat_error(int stat) const {
		char errbuf[512];
		size_t lenbuf = sprintf( errbuf, "%s:", m_str );
		regerror(stat, &m_regex, errbuf + lenbuf, sizeof(errbuf) - lenbuf );
		throw std::runtime_error(errbuf);
	}
	RegexT();
	RegexT(const RegexT &);

	/// Token now just need an integer to point to their regular expression.
	/// It is therefore very fast to create/copy a token.
	typedef std::vector< const RegexT * > StorageT ;

	/// Where we store all created and compiled regexes, indexed with an integer.
	static StorageT & storage(void) {
		static StorageT s_storage ;
		return s_storage ;
	}

public:
	RegexT( const char * reg, const char * name, ProxyGen gener, priority_t priority )
	: m_str(reg)
	, m_name(name)
	, m_generator(gener)
	, m_priority(priority) {
		char tmpbuf[ 3 + strlen(m_str) ];
		sprintf( tmpbuf, "^%s$", m_str );
		int stat = regcomp( &m_regex, tmpbuf, REG_EXTENDED|REG_NOSUB);
		if(stat) treat_error(stat);
	}
	~RegexT() {
		regfree( &m_regex );
	}

	/// Returns the regular expression given its index.
	static const RegexT * Find(size_t idx ) { 
		assert( idx < Nb() );
		return storage()[ idx ];
	}

	static size_t Nb(void) {
		return RegexT::storage().size();
	}

	/// Given a regular expression patterns, creates and compile the regular expression and stores it with an integer.
	static int CreateRgx( const char * reg, const char * name, ProxyGen gener, priority_t priority )
	{
		typedef std::map< const char *, int > Name2idxT ;
		static Name2idxT name2idx ;

		Name2idxT::iterator it = name2idx.find(name);
		if( it != name2idx.end() ) return it->second ;

		int new_idx = storage().size();

		storage().push_back( new RegexT( reg, name, gener, priority ) );

		// The name is later used for easier retrieval from a Synop token.
		name2idx[ name ] = new_idx ;
		return new_idx ;
	};

	static TokenProxy::Ptr CreateTokenProxy( int reg_idx, const std::string & wrd, size_t txt_offset ) {
		return Find(reg_idx)->m_generator( reg_idx, wrd, txt_offset );
	}

	const char * Name(void) const { return m_name; }

	static const char * Name(size_t idx) { return Find(idx)->m_name; }

	// If two different tokens, with different regular expressions, match for the
	// same word, the priority tells which one to take. Default value is one.
	static priority_t Priority( size_t idx ) { 
		assert( idx < Nb() );
		return storage()[ idx ]->m_priority;
	}

	bool Match( const std::string & str ) const {
		int stat = regexec( &m_regex, str.c_str(), 0, 0, 0 );
		switch(stat) {
			case 0:
				//std::cout << "DEBUG: Matched [" << str << "] with [" << m_str << "] (" << m_name << ")\n";
				return true ;
			case REG_NOMATCH:
				// std::cout << "DEBUG: NoMatch [" << str << "] with [" << m_str << "]\n";
				return false ;
			default: break ;
		}
		treat_error(stat);
		return 0 ; // Will never be reached.
	}

	/// Stores the match result for each regular expression.
	class Context {

		// First bit to store whether we tried to match. Next bit for the result.
		std::vector<bool> m_flags ;

		/// TODO: Consider a compile-time size because we know the number of regular expressions.
		static size_t NbElts() { return RegexT::Nb(); };
	public:
		Context() : m_flags( NbElts() * 2, false ) {}
		virtual ~Context() {}

		// This helps performance because the same regex appears in several chains.
		virtual bool Mtch( size_t reg_idx, const std::string & str ) {
			assert( m_flags.size() == NbElts() * 2 );
			assert( m_flags.size() > 2 * reg_idx + 1 );
			if( m_flags.at( 2 * reg_idx ) ) return m_flags[ 2 * reg_idx + 1 ];
			m_flags[ 2 * reg_idx ] = true ;
			bool res = RegexT::Find(reg_idx)->Match( str );
			m_flags[ 2 * reg_idx + 1 ] = res;
			return res;
		}
	};

}; // RegexT

/// Loops in the tokens held by the container, for a precise type.
template< class TokenDerived >
const TokenDerived * TokenProxy::get_ptr(bool previous_chains) const  
{
	for(
		TokVec::Ptr curr_container = m_container ;
		curr_container ;
		curr_container = curr_container->previous() )
	{
		for( TokVec::const_iterator it = curr_container->begin(), en = curr_container->end();
			it != en ;
			++it )
		{
			const  TokenDerived * ptr = dynamic_cast< const TokenDerived * >( it->operator->() );
			if( ptr != NULL ) return ptr ;
		}

		if( ! previous_chains ) break ;
	}
	return NULL ;
}

const char * TokenProxy::RegexName(void) const { return RegexT::Name(m_regex_idx); };

/// Transforms a token nickname into a classname, intentionaly small to reduce symbol table size.
#define CLASSTK(a) Tk_##a

/// Each Synop group (aka Token° is associated to a regular expression, and a priority.
#define GENTK_PRIORITY( TokDrv, Rgx, prio ) \
static int MakeRegex() { return RegexT::CreateRgx( Rgx, #TokDrv, TokenProxy::Generator< CLASSTK(TokDrv) >, prio ); }

/// TODO: It is not necessary to add "=" or ";" at the end of the regular expressions.
#define GENTK( TokDrv, Rgx ) GENTK_PRIORITY( TokDrv, Rgx, 1.0 )

/// Definition of a derived class associated to a five-digits group (Most of times) and a regular expression.
/// Virtual inheritance because it might derive from another TokenProxy derived class.
#define HEADTK(a) class CLASSTK(a) : virtual public TokenProxy

/// For defining chains of tokens. Some tokens might be repeated, hence TKn.
#define TKx(a) CLASSTK(a)::MakeRegex()

/// If this token appears once and once only.
#define TK1(a) {TKx(a),false}

/// If this token can be repeated.
#define TKn(a) {TKx(a),true}

/// One element in a chain of token which forms a Synop line.
struct TOKGEN {
	int  m_rgx_idx; // Index to a regular expression.
	bool m_many;    // If this token can be repeated or appears once only.
};


// ----------------------------------------------------------------------------

/// Matches the usual message begin.
HEADTK(ZCZC) {
public:
	GENTK_PRIORITY( ZCZC, "ZCZC", MIN_PRIO )

	bool Parse( const char * str )
	{
		return 0 == strcmp( str, "ZCZC" );
	}

	void Print() const {
		Section( "Bulletin preamble" );
	}
};

/// The id which might come after ZCZC.
HEADTK(ZCZC_id) {
	int m_id ;
public:
	GENTK( ZCZC_id, "[0-9]{2,4}" )

	bool Parse( const char * str )
	{
		return 1 == sscanf( str, "%d", &m_id );
	}

	void Print() const {
		Append( _("Report number"), m_id );
	}

	/// Can come only after ZCZC
	bool CanComeFirst(void) const { return false ; }
};

/// Usual end of weather message.
HEADTK(NNNN) {
public:
	GENTK_PRIORITY( NNNN, "NNNN", MIN_PRIO )

	bool Parse( const char * str )
	{
		return 0 == strcmp( str, "NNNN" );
	}

	void Print() const {
		Section( "Bulletin end" );
	}
};

/// Any coding will work if we store two chars in an integer.
#define CHR2(x1,x2) (unsigned char)x1 * 256 + (unsigned char)x2

static const char * wx_code_to_txt( const char * strwx )
{
	int two_chars = CHR2(strwx[0],strwx[1]);
	switch( two_chars ) {
		case CHR2('S','H'): return _("Synoptic ship reports");
		case CHR2('S','I'): return _("Intermediate synoptic reports");
		case CHR2('S','M'): return _("Synoptic observations");
		case CHR2('S','N'): return _("Non-standard synoptic hour");
		default      : return NULL;
	}
}

#undef CHR2

HEADTK(TTAAii) {
	char m_datatype[3];
	char m_geographical[3] ;
	int  m_number ;
public:
	/**
	SH:Synoptic ship reports
	SI:Intermediate synoptic reports
	SM:Synoptic observations
 	*/
	GENTK( TTAAii, "S[HIMN][A-Z0-9]{2}[0-9]{2}" )

	bool Parse( const char * str )
	{
		return 3 == sscanf( str, "%2s%2s%2d", m_datatype, m_geographical, &m_number );
	}

	void Print() const {
		const char * dt_type = wx_code_to_txt(m_datatype);
		if(dt_type) {
			Append(_("Data type"), dt_type );
		} else {
			std::stringstream strm ;
			strm << _("Unknown") << ':' << m_datatype << '.';
			Append(_("WX data type"), strm.str() );
		}
		Append(_("Geographical"), m_geographical );
		const char * num = NULL ;
		switch( m_number ) {
			case  1 ... 19 : num = _("Inclusive for global distribution"); break;
			case 20 ... 39 : num = _("Inclusive for regional and interregional distribution"); break;
			case 40 ... 89 : num = _("Inclusive for national and bilaterally agreed distribution"); break;
			default        : num = _("Reserved"); break;
		}
		Append(_("Number"), num );
	}
};

/// ICAO location. Found in document 7910.
/// Or here: http://www.wmo.int/pages/prog/www/ois/Operational_Information/VolumeC1/CCCC_en.html
/// http://weather.rap.ucar.edu/surface/stations.txt
/// ftp://www.wmo.ch/wmo-ddbs/OperationalInfo/VolumeC1/To_WMO/
HEADTK(CCCC) {
	char m_icao[5];
public:
	GENTK( CCCC, "[A-Z]{4}" )

	/// All these comparisons, because there might be confusion with other four-letters groups.
	bool Parse( const char * str )
	{
		return 1 == sscanf( str, "%4s", m_icao )
		&& strcmp( m_icao, "AAXX" )
		&& strcmp( m_icao, "BBXX" )
		&& strcmp( m_icao, "OOXX" )
		&& strcmp( m_icao, "ZCZC" )
		&& strcmp( m_icao, "NNNN" );
	}

	/// TODO: Lookup in wmo file.
	void Print() const {
		Append( _("ICAO indicator"), m_icao );
	}
};

static std::string hour_min( int hour, int min ) {
	char buf[10];
	sprintf( buf, "%02d:%02d", hour, min );
	return buf ;
}

HEADTK(YYGGgg) {
	/// Default date is today.
	time_t m_time;
public:
	GENTK( YYGGgg, "[0-3][0-9][0-2][0-9][0-5][0-9]" )

	bool Parse( const char * str )
	{
		int day_of_month ;
		int UTC_observation_hour ;
		int UTC_observation_minute ;

		bool time_ok = ( 3 == sscanf( str, "%02d%02d%02d", &day_of_month, &UTC_observation_hour, &UTC_observation_minute ) );

		time_ok = time_ok && ( day_of_month <= 31 ) && ( UTC_observation_hour <= 24 ) && ( UTC_observation_minute <= 59 );

		if( ! time_ok ) return false ;

		m_time = DayHourMin2Tm( day_of_month, UTC_observation_hour, UTC_observation_minute );

		return true ;
	}

	void Print() const {
		Append( _("UTC observation time"), Tm2SynopTime( m_time ) );
	}

	/// Does not display anything because all the information is in the UTC time stamp.
	void DrawKml() const {}

	time_t ObservationTimeUTC() const { return m_time ; }
};


/** BBB Forms
The four forms of the BBB indicator group are:
     * RRx - Delayed (Retard)
     * CCx - Correction
     * AAx - Amendment
     * Pxx - Segment number
     *
*/
HEADTK(Numbered) {
	const char * m_format ;
	char m_number[3];
public:
	CLASSTK(Numbered)( const char * fmt ) : m_format(fmt) {};

	void Print() const {
		Append( _("Segment number"), m_number );
	}

	bool Parse( const char * str )
	{
		return 1 == sscanf( str, m_format, m_number );
	}

	const char * Number() const { return m_number ; }
};

HEADTK(RRx), virtual CLASSTK(Numbered) {
public:
	/** http://www.wmo.int/pages/prog/www/ois/Operational_Information/Publications/WMO_386/AHLsymbols/bbb_en.html
	"x=A for the first bulletin after the issuance of the initial bulletin;
	B, if another bulletin needs to be issued;
	and so on up to and including x = X; "
	This seems to be wrong, actually messages up to Y and Z are seen.
	GENTK( RRx, "RR[A-X]" )
	*/
	GENTK( RRx, "RR[A-Z]" )

	CLASSTK(RRx)() : CLASSTK(Numbered)( "RR%1s" ) {}
};

HEADTK(CCx), virtual CLASSTK(Numbered) {
public:
	GENTK( CCx, "CC[A-X]" )

	CLASSTK(CCx)() : CLASSTK(Numbered)( "CC%1s" ) {}
};

HEADTK(AAx), virtual CLASSTK(Numbered) {
public:
	GENTK( AAx, "AA[A-X]" )

	CLASSTK(AAx)() : CLASSTK(Numbered)( "AA%1s" ) {}
};

/// Pxx is the segmentation BBB group as defined in the WMO document Guidelines For The Use Of The Indicator BBB
HEADTK(Pxx), virtual CLASSTK(Numbered) {
public:
	/// Details here: http://www.nws.noaa.gov/tg/bbb.php
	GENTK( Pxx, "P[A-Z]{2}" )

	CLASSTK(Pxx)() : CLASSTK(Numbered)( "P%2s" ) {}
};

/// ISMCS WMO STATION NUMBER LIST
/// http://www.ncdc.noaa.gov/oa/climate/rcsg/cdrom/ismcs/alphanum.html
/// http://www.weather.unisys.com/wxp/Appendices
/// http://weather.unisys.com/wxp/Appendices/Formats/SYNOP.html

/// 000 Group - Identification and Location
/// 
/// IIiii The WMO number of the station.
HEADTK(IIiii) {
	int          m_wmo_indicator ;
	int          m_region_number ;
	const char * m_region_name ;
public:
	GENTK( IIiii, "[0-9]{5}" )
	// GENTK( IIiii, "[0-9]{5}", MIN_PRIO )

	/**
	The IIiii Structure

	The II or block number is allocated to the services within each Region by regional agreement.

	Station numbers iii corresponding to a common block number (II)
	except 89 are usually distributed so that the zone covered by a
	block number is divided into horizontal strips; e.g. one of several
	degrees of latitude. Where possible, station numbers within each strip increase
	from west to east and the first figure of the 3-figure station number increases from north to south.

	Station index numbers for station in the Antarctic are allocated by the Secretary-General
	in accordance with the following scheme:

	Each station has an international number 89xxy, where xx indicated the nearest 10 degree
	meridian which is numerically lower than the station longitude. For east longitudes,
	50 is added; e.g. 89124 indicated a station between 120 degrees and 130 degrees west
	and 89654 indicates a station between longitudes 150 degrees and 160 degrees east.
	The figure "y" is allocated roughly according to the latitude of the station
	with "y" increasing towards the south.
	For station for which an international numbers are no longer available within the
	above scheme, the algorithm will be expanded by adding 20 to xx for west longitudes
	(range of index numbers 200-380) and 70 for east longitudes (range of index numbers 700-880)
	to provide new index numbers.
	Antarctic station which held numbers before the introduction of this scheme in 1957
	retain their previously allocated index numbers.
	*/
	bool Parse( const char * str )
	{
		// std::cout << "wmo=" << str << "\n";

		/* Station index numbers consisting of one figure repeated five times, e.g. 55555, 77777, etc., 
		or ending with 0000 or 9999, or duplicating special code indicators, e.g. 10001, 77744, 19191,
		89998, etc., are not assigned to meteorological stations. We might check if the code exists or not. */
		if( 1 != sscanf( str, "%d", &m_wmo_indicator ) ) return false ;

		m_region_name = "";
		m_region_number = 0 ;

		// http://weather.noaa.gov/tg/site.shtml
		switch(m_wmo_indicator) {
			case 00000:
			case 11111:
			case 22222:
			case 33333:
			case 44444:
			case 55555:
			case 66666:
			case 77777:
			case 88888:
			case 99999:	m_region_name = _("Unassigned");
					return false ;
			default   :	break ;
		}

		/// More special codes.
		switch(m_wmo_indicator%10000) {
			case 0000:
			case 9999: return false ;
			default  : break ;
		}

		switch(m_wmo_indicator) {
			case 60000 ... 69999:
				m_region_name = _("Africa");
				m_region_number = 1 ;
				break;
			case 20000 ... 20099:
			case 20200 ... 21999:
			case 23000 ... 25999:
			case 28000 ... 32999:
			case 35000 ... 36999:
			case 38000 ... 39999:
			case 40350 ... 48599:
			case 48800 ... 49999:
			case 50000 ... 59999:
				m_region_name = _("Asia");
				m_region_number = 2 ;
				break;
			case 80000 ... 88999:
				m_region_name = _("South America");
				m_region_number = 3 ;
				break;
			case 70000 ... 79999:
				m_region_name = _("North and Central America");
				m_region_number = 4 ;
				break;
			case 48600 ... 48799:
			case 90000 ... 98999:
				m_region_name = _("South-West Pacific");
				m_region_number = 5 ;
				break;
			case 00000 ... 19999:
			case 20100 ... 20199:
			case 22000 ... 22999:
			case 26000 ... 27999:
			case 33000 ... 34999:
			case 37000 ... 37999:
			case 40000 ... 40349:
				m_region_name = _("Europe");
				m_region_number = 6 ;
				break;
			case 89000 ... 89999:
				m_region_name = _("Antarctic");
				m_region_number = 9 ;
				break;
		}

		/* NOT SURE THIS IS REALLY USEFUL   <<<<<<<<<<<================== */
		// std::cout << "region=" << m_region_name << "\n";
		return true ;
	}

	/// Huge list: http://www.ncdc.noaa.gov/oa/climate/rcsg/cdrom/ismcs/alphanum.html
	int WmoIndicator(void) const { return m_wmo_indicator; }

	/// Official file: http://weather.noaa.gov/data/nsd_bbsss.txt
	void Print() const {
		std::stringstream strm ;
		strm << std::setfill('0') << std::setw(5) << m_wmo_indicator ;
		std::string wmo_str = strm.str();
		Append( _("WMO Station"), wmo_str );

		const RecordWmoStation * ptrWmo = CatalogWmoStations::FindFromKey( m_wmo_indicator );
		if( ptrWmo )
			Append( _("WMO station"), *ptrWmo );
		else {
			Append( _("WMO station"), "WMO_" + wmo_str );
		}
	}

	void DrawKml() const {}
};

HEADTK(AAXX) {
public:
	GENTK( AAXX, "AAXX" )

	bool Parse( const char * str )
	{
		return 0 == strcmp( str, "AAXX" );
	}

	void Print() const {
		Section( _("Land station observation") );
	}
};

/// SHIP report.
HEADTK(BBXX) {
public:
	GENTK( BBXX, "BBXX" )

	bool Parse( const char * str )
	{
		return 0 == strcmp( str, "BBXX" );
	}

	void Print() const {
		Section( _("Ship observation") );
	}
};

/// SYNOP MOBILE.
HEADTK(OOXX) {
public:
	GENTK( OOXX, "OOXX" )

	bool Parse( const char * str )
	{
		return 0 == strcmp( str, "OOXX" );
	}

	void Print() const {
		Section( _("Mobile observation") );
	}
};

/// http://metaf2xml.sourceforge.net/parser.pm.html#trends
HEADTK(MMMULaULo) {
	/// See decoding here: http://icoads.noaa.gov/Release_1/suppG.html
	int m_Marsden_square ;
	double m_latitude;
	double m_longitude;
public:
	/// Lower priority because it should not be selected against any other chain beginning.
	GENTK_PRIORITY( MMMULaULo, "[0-9/]{5}", 0.5 )

	/// TODO: Marsden square was not tested enough.
	bool Parse( const char * str )
	{
		m_Marsden_square = -1;
		m_latitude = 0.0 ;
		m_longitude = 0.0 ;
		if( 0 == strncmp( str, "/////", 5 ) ) return true ;

		// Only slashes or no slash at all.
		if ( NULL != strchr( str, '/' ) ) return false ;

		char char_lat ;
		char char_lon ;
		if ( 3 != sscanf( str, "%3d%c%c", &m_Marsden_square, &char_lat, &char_lon ) ) return false;

		m_latitude = 90.0 - ( m_Marsden_square / 36.0 ) ;
		m_longitude = ( m_Marsden_square % 36 ) - 30.0 ;
		m_latitude +=  char_lat - '0' ;
		m_longitude += char_lon - '0' ;

		/// East longitude is positive, West is negative.
		return true;
	}

	void Print() const {
		/// Very approximate.
		if( m_Marsden_square > 0 ) {
			Append( _("Marsden latitude"), m_latitude );
			Append( _("Marsden longitude"), m_longitude );
		} else {
			Append( _("Coordinates"), _("Marsden square not defined") );
		}
	}

	bool MarsdenValid(void) const { return m_Marsden_square >= 0; }
	double Latitude(void)  const {
		assert( MarsdenValid() );
		return m_latitude;
	}
	double Longitude(void) const {
		assert( MarsdenValid() );
		return m_longitude;
	}

	bool CanComeFirst(void) const { return false ; }
};

/// Position of the station (Marsden square, height)
/// http://metaf2xml.sourceforge.net/parser.pm.html#trends
/// h0h0h0h0im   elevation of mobile land station, units of elevation, and elevation accuracy
///   h0hoh0h0 elevation if meters or feet as indicated by im
///   im indicator for units of elevation and confidence factor of accuracy
HEADTK(h0h0h0h0im) {
	/// See decoding here: http://icoads.noaa.gov/Release_1/suppG.html
	int m_height ;
	char m_indicator ;
public:
	GENTK( h0h0h0h0im, "[0-9/]{4}[1-8]" )

	bool Parse( const char * str )
	{
		m_height = 0;
		m_indicator = '0';
		return ( 2 == sscanf( str, "%4d%c", &m_height, &m_indicator ) );
	}

	void Print() const {
		const char * unit = "";
		switch(m_indicator) {
			case '1' ... '4' : unit = Unit_meters; break;
			case '5' ... '8' : unit = Unit_feet; break;
			default: ;
		}
		Append( _("Height"), m_height, unit );
		const char * indicators[] = {
			_("Not Used"),
			_("Excellent (within 3 meters)"),
			_("Good (within 10 meters)"),
			_("Fair   (within 20 meters)"),
			_("Poor (more than 20 meters)"),
			_("Excellent (within 10 feet)"),
			_("Good (within 30 feet)"),
			_("Fair   (within 60 feet)"),
			_("Poor (more than 60 feet)") };
		disp_arr(indicators,G_N_ELEMENTS(indicators),m_indicator,'0',_("Precision"));
	}

	/// It is dependent on the previous token, MMMUL.
	bool CanComeFirst(void) const { return false ; }
};


/// NIL
HEADTK(NIL) {
public:
	GENTK( NIL, "NIL[;=]?" )

	bool Parse( const char * str )
	{
		return 0 == strncmp( str, "NIL", 3 );
	}

	void Print() const {
		Append( _("Token"), _("End of section") );
	}

	void DrawKml() const {}
};

/// Ship or Buoy Observations: IIIII The ship or buoy identifier
HEADTK(IIIII) {
	static const size_t maxsz = 10 ;
	char m_ship_buoy_identifier[maxsz] ;
public:
	GENTK( IIIII, "[A-Z0-9]{3,9}" )

	/// We filter some identifiers.
	bool Parse( const char * str )
	{
		// If this is a five-digits string, it cannot reasonably be an identifier.
		if( ( 5 == strlen( str ) ) &&
		    ( 5 == strspn( str, "0123456789" ) ) ) {
			return false ;
		}

		/// Due to parsing error, we might take the next group header (333 or 555) as a ship name.
		bool resu = 
			( 1 == sscanf( str, "%9s", m_ship_buoy_identifier ) )
		&&	( strcmp( m_ship_buoy_identifier, "333" ) )
		&&	( strcmp( m_ship_buoy_identifier, "555" ) );

		if( resu ) {
			// Some ships are apparently anonymous, we give them an unique name.
			if( 0 == strcmp( "SHIP", m_ship_buoy_identifier ) ) {
				static int ship_counter = 0 ;
				++ship_counter ;
				sprintf( m_ship_buoy_identifier, "SHIP_%d", ship_counter );
			}
		} else {
			m_ship_buoy_identifier[0] = '\0';
		}
		return resu;
	}

	void Print() const {
		Append( _("Ship/Buoy identifier"), m_ship_buoy_identifier );
	}

	void DrawKml() const {}

	/// Information about buoys:
	/// http://www.ndbc.noaa.gov/marine_notice.shtml
	/// http://www.ndbc.noaa.gov/stndesc.shtml
	/// http://www.hpc.ncep.noaa.gov/html/stationplot_buoy.shtml
	const char * ShipIdentifier(void) const { return m_ship_buoy_identifier; }
};

/// ff -- wind speed in units determined by wind type indicator (see above) 
static const choice< char > wind_speed_units[] = {
	{ '0',_("m/s (Estimated)") },
	{ '1',_("m/s (Anemometer)") },
	{ '3',_("knots (Estimated)") },
	{ '4',_("knots (Anemometer)") } };
FM12CodeTable( wind_speed_units, 1855, _("Indicator for source and units of wind speed") );

/// Apparently one can also find YYGGggi:
/// YY -- Monatstag (UTC)
/// GG -- Beobachtungszeit (UTC) in vollen Stunden
/// gg -- Beobachtungszeit in Minuten (wird nur bei Halbstundenterminen benutzt: gg = 30)
/// iw -- Indikator für Windangaben:

/// YYGGi
/// YY -- The day of the month
/// GG  -- The hour of the observation (UTC)
/// iw -- Wind type indicator
/// For more safety, we could search today's date only, but testing is more difficult.
HEADTK(YYGGi) {
	time_t m_time;
	char m_wind_type_indicator ;
public:
	GENTK( YYGGi, "[0-3][0-9][0-2][0-9][0134]" )

	bool Parse( const char * str )
	{
		int day_of_month;
		int UTC_observation_hour;

		if ( 3 != sscanf( str, "%02d%02d%c",
			&day_of_month,
			&UTC_observation_hour,
			&m_wind_type_indicator ) ) return false ;

		if ( ( day_of_month > 31 )
		||   ( day_of_month < 1 )
		||   ( UTC_observation_hour > 24 )
		||   ( NULL == strchr( "0134", m_wind_type_indicator ) ) ) return false;


		// std::cout << __FUNCTION__ << ":" << UTC_observation_hour << " " << day_of_month << "\n";
		m_time = DayHourMin2Tm( day_of_month, UTC_observation_hour, 0 );
		return true;
	}

	void Print() const {
		Append( _("UTC observation time"), Tm2SynopTime( m_time ) );

		// No need to display it twice because it will appear after the speed value.
		if( tstDisp ) {
			Append( _("Wind type indicator"), 
				choice_map( wind_speed_units, G_N_ELEMENTS( wind_speed_units ),
					m_wind_type_indicator, _("Unknown speed unit type") ) );
		}
	}

	/// Does not display anything because all the information is displayed later.
	void DrawKml() const {}

	time_t ObservationTimeUTC() const { return m_time ; }

	friend class CLASSTK(Nddff);
};

/// 99LLL QLLLL
/// LLL -- Latitude of observation to .1 degrees
/// Q -- Quadrant of observation
HEADTK(99LLL) {
	int m_latit_10deg ;
public:
	GENTK( 99LLL, "99[0-9]{3}" )

	bool Parse( const char * str )
	{
		// The latitude starts at the third char, after "99".
		if ( 1 != sscanf( str + 2, "%d", &m_latit_10deg ) ) return false;

		// Latitudes are between 0 and 90 degrees.
		return ( m_latit_10deg <= 900 );
	}

	void Print() const {
		Append( _("QLLLL token"), _("Present") );
	}

	/// Does not display anything because all the information is displayed later as coordinates.
	void DrawKml() const {}

	/// It is dependent on the previous token.
	bool CanComeFirst(void) const { return false ; }

	friend class CLASSTK(QLLLL);
};

/// LLLL -- Longitude of  observation to .1 degrees 
HEADTK(QLLLL) {
	char           m_quadrant ;
	int            m_longit_10deg ;
	mutable bool   m_calc_done ;
	mutable bool   m_coordinates_ok ;
	mutable double m_Longitude ;
	mutable double m_Latitude ;

	/// Can be called only once this token is inserted in a group.
	void Calc(void) const {
		if( m_calc_done ) return ;
		m_calc_done = true ;
		const CLASSTK(99LLL) * ptr_99LLL = TokenProxy::get_ptr< CLASSTK(99LLL) >(false);
		if( ptr_99LLL ) {
			m_Longitude = 0.0;
			m_Latitude = 0.0;
			m_coordinates_ok = false ;
		}
		double lat = ptr_99LLL ? (double)ptr_99LLL->m_latit_10deg * 0.1 : 0.0 ;
		double lon = (double)m_longit_10deg * 0.1 ;
		m_coordinates_ok = true ;
		// East longitude and north latitude are positive.
		switch( m_quadrant ) {
			case '1':                          break ; // North-East
			case '3': lat = -lat;              break ; // South-East
			case '5': lon = -lon; lat = -lat;  break ; // South-West
			case '7': lon = -lon;              break ; // North-West
			default : m_coordinates_ok = false; break;
		}
		m_Longitude = lon;
		m_Latitude  = lat;
	}
public:
	CLASSTK(QLLLL) () : m_calc_done(false), m_coordinates_ok(false) {}

	GENTK( QLLLL, "[1357][0-9]{4}" )

	bool Parse( const char * str )
	{
		// Check that longitude  is smaller than 180 in tenth of degrees.
		return ( 2 == sscanf( str, "%c%d", &m_quadrant, &m_longit_10deg ) )
		&&     ( m_longit_10deg <= 1800 );
	}

	void Print() const {
		Calc();
		if(m_coordinates_ok) {
			Append( _("Longitude"), m_Longitude );
			Append( _("Latitude"), m_Latitude );
		} else {
			Append( _("Coordinates"), _("Wrong coordinates format") );
		}
	}

	/// Does not display anything because all the information is displayed as coordinates.
	void DrawKml() const {}

	bool CanComeFirst(void) const { return false ; }

	const bool CoordinatesOK() const { return m_coordinates_ok; }
	double Longitude() const {
		Calc();
		assert( m_coordinates_ok );
		return m_Longitude;
	}
	double Latitude () const {
		Calc();
		assert( m_coordinates_ok );
		return m_Latitude ;
	}
};

static const char * cloud_bases[]= {
	_("0 to 50 m"),
	_("50 to 100 m"),
	_("100 to 200 m"),
	_("200 to 300 m"),
	_("300 to 600 m"),
	_("600 to 1000 m"),
	_("1000 to 1500 m"),
	_("1500 to 2000 m"),
	_("2000 to 2500 m"),
	_("above 2500 m"),
	_("unknown") };
FM12CodeTable( cloud_bases, 1600, _("Height above surface of the base of the lowest cloud seen") );

/// 111 Group - Land Observations
/// Apparently there are differences between FM12 and FM13X which is more recent.
/// iihVV
HEADTK(iihVV) {
	char m_precipitation ;
	char m_station_type ;
	char m_cloud_base ;
	int m_visibility ;
public:
	GENTK( iihVV, "[0-4][1-7][0-9/][0-9/]{2}" )

	bool Parse( const char * str )
	{
		// TODO; Frequently mismatched with "222//" . Should add a special case.
		int nbMtch = sscanf( str, "%c%c%c%d", &m_precipitation, &m_station_type, &m_cloud_base, &m_visibility );
		// "46///" is a valid string.
		// std::cout << __FUNCTION__ << ":" << str << " " << nbMtch << "\n";
		switch(nbMtch) {
			case 4 : return true ;
			case 3 : m_visibility = -1; // i.e. "missing"
				 return 0 == strcmp( str + 3, "//" );
			default: return false ;
		}
	}

	bool isAutomated(void) const {
		switch( m_station_type ) {
			default : return false ;
			case '4':
			case '5':
			case '6':
			case '7': return true;
		}
	}

	void Print() const {
		/// iR -- Precipitation indicator
		static const char * precipitations[] = {
			_("In groups 1 and 3"),
			_("In group 1 only"),
			_("In group 3 only"),
			_("Omitted, no precipitation"),
			_("Omitted, no observation") };
		FM12CodeTable( precipitations, 1819, _("Indicator for inclusion or mossion of precipitation data") );
		disp_arr(precipitations,G_N_ELEMENTS(precipitations),m_precipitation,'0',_("Precipitations"));

		/// ix -- Station type and present and past weather indicator. Tells if the group 7WW is included.
		static const char * station_types[] = {
			_("Manned station (With 7WW)"),
			_("Manned station. Not significant (No 7WW)"),
			_("Manned station. No observation (No 7WW)"),
			_("Automated station (With 7WW)"),
			_("Automated station. Not significant (No 7WW)"),
			_("Automated station. No observation (No 7WW)"),
			_("Automated station (With 7WW)") };
		FM12CodeTable( stations_types, 1860, _("Indicator for type of station operation and for present and past weather data") );
		disp_arr(station_types,G_N_ELEMENTS(station_types),m_station_type,'1',_("Station type"));

		/// h -- Cloud base of lowest cloud seen (meters above ground)
		disp_arr(cloud_bases,G_N_ELEMENTS(cloud_bases),m_cloud_base,'0',_("Cloud base"));

		/// VV -- Visibility
		const char *vis = _("Visibility");
		switch( m_visibility ) {
			case  0       : Append( vis, _("Less than 0.1"),              Unit_km ); break ;
			case  1 ...50 : Append( vis,  m_visibility / 10,              Unit_km ); break ;
			case 51 ...79 : Append( vis,  m_visibility - 50,              Unit_km ); break ;
			case 80 ...88 : Append( vis,  30 + 5 * ( m_visibility - 80 ), Unit_km ); break ;
			case 89       : Append( vis, _("Greater than 70"),            Unit_km ); break;
			case 90       : Append( vis, _("Less than 0.05"),             Unit_km ); break;
			case 91       : Append( vis, 0.05,                            Unit_km ); break;
			case 92       : Append( vis, 0.2,                             Unit_km ); break;
			case 93       : Append( vis, 0.5,                             Unit_km ); break;
			case 94       : Append( vis, 1,                               Unit_km ); break;
			case 95       : Append( vis, 2,                               Unit_km ); break;
			case 96       : Append( vis, 4,                               Unit_km ); break;
			case 97       : Append( vis, 10,                              Unit_km ); break;
			case 98       : Append( vis, 20,                              Unit_km ); break;
			case 99       : Append( vis, _("Greater than 50"),            Unit_km ); break;
			default       : Append( vis, _("Missing") );                        break;
		}
	}
};

/// TODO: Use these graphic symbols: http://www.hpc.ncep.noaa.gov/html/stationplot_buoy.shtml
static const char * cloud_covers[]= {
	_("0 eighths (clear)"),
	_("1/8"),
	_("2/8"),
	_("3/8"),
	_("4/8"),
	_("5/8"),
	_("6/8"),
	_("7/8"),
	_("8/8 (overcast)"),
	_("Sky obscured"),
	_("No observation") };
FM12CodeTable( cloud_covers, 2700, _("Amount of cloud cover") );

/// Nddff
HEADTK(Nddff) {
	char m_cloud_cover ;
	/// In ten of degrees.
	int m_wind_direction ;
	int m_wind_speed ;
public:
	GENTK( Nddff, "[0-9/][0-9/]{2}[0-9/]{2}" )

	bool Parse( const char * str )
	{
		m_cloud_cover = '/';
		m_wind_direction = 0;
		m_wind_speed = 0 ;
		// Can be "/////" or "8////" plus trailing characters.

		if ( 0 == strncmp( str + 1, "////", 4) ) return true ;
		return ( 3 == sscanf( str, "%c%2d%2d", &m_cloud_cover, &m_wind_direction, &m_wind_speed ) )
		&&     ( ( m_wind_direction <= 36 ) || ( m_wind_direction == 99 ) );
	}

	void Print() const {
		// N -- Total cloud cover
		disp_arr(cloud_covers,G_N_ELEMENTS(cloud_covers),m_cloud_cover,'0',_("Cloud cover"));

		// dd -- wind direction in 10s of degrees
		if( m_wind_direction == 99 )
			Append( _("Wind direction"), _("Variable, all directions, confused, indeterminate direction") );
		else if( m_wind_direction == 0 )
			Append( _("Wind direction"), _("No motion or no waves") );
		else
			Append( _("Wind direction"), m_wind_direction * 10 - 5 , Unit_degrees );
		FM12CodeTable( xxx, 0877, _("True direction in tenth of degrees") );

		// We search for this token not only in this section but in the previous ones.
		const CLASSTK(YYGGi) * ptr_YYGGi = TokenProxy::get_ptr< CLASSTK(YYGGi) >(true);

		const char * wind_speed_title = _("Wind speed");
		if( ptr_YYGGi ) {
			Append( wind_speed_title, m_wind_speed, 
				choice_map( wind_speed_units, G_N_ELEMENTS( wind_speed_units ),
					ptr_YYGGi->m_wind_type_indicator, _("Unknown speed unit type") ) );
		} else {
			Append( wind_speed_title, m_wind_speed, _("No unit (YYGGi missing)") ); 
		}
	       
	}

	bool CanComeFirst(void) const { return false ; }
};

/// 00fff (optional)
HEADTK(00fff) {
	int m_wind_speed ;
public:
	/// Lower priority than Nddff
	GENTK_PRIORITY( 00fff, "00[0-9]{3}", 0.5 )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "00%3d", &m_wind_speed ) );
	}

	void Print() const {
		// fff -- wind speed if value greater than 100
		Append( _("Wind speed"), m_wind_speed ); 
	}
};

/// Returns true if this temperature makes sense.
static bool CheckCelsius( char sign, int temperature_tenth )
{
	switch( sign ) {
		case '1' : return temperature_tenth < 700 ;
		case '0' : return temperature_tenth < 600 ;
		case '/' : return true ;
		default  : return false ;
	}
}

/// Prints the temperature or relative humidity.
static void AppCelsius( const TokenProxy * ptrTok, const char * title, char sign, int temperature_tenth )
{
	if( title == NULL ) title = "No title";

	switch( sign ) {
		case '1' : temperature_tenth = -temperature_tenth ; 
		case '0' : ptrTok->Append( title, temperature_tenth * 0.10, Unit_Celsius ); 
			   break;
		case '9' : ptrTok->Append( _("Relative humidity"), (double)temperature_tenth * 0.10 , "%" );
			   break;
		case '/' : ptrTok->Append( title, _("Undefined") ); 
			   break;
		default  : ptrTok->Append( title, _("Unexpected case") ); 
			   break;
	}
}

/// 1sTTT_air -- Temperature
HEADTK(1sTTT_air) {
	char m_temperature_sign ;
	int m_temperature ;
public:
	GENTK( 1sTTT_air, "1[01/][0-9/]{3}[;=]?" )

	bool Parse( const char * str )
	{
		m_temperature_sign = '/';
		m_temperature = 0;
		if( 0 == strncmp( str, "1////", 5 ) ) return true ;
		return ( 2 == sscanf( str, "1%c%3d", &m_temperature_sign, &m_temperature) )
			&& ( strchr( "01", m_temperature_sign ) != NULL )
			&& CheckCelsius( m_temperature_sign, m_temperature );
	}

	/// s: sign of temperature (0=positive, 1=negative). TTT: Temperature in .1 C
	void Print() const {
		AppCelsius( this, _("Temperature"), m_temperature_sign, m_temperature );
	}
};

/// 2sTTT_dew -- Dewpoint
HEADTK(2sTTT_dew) {
	char m_temperature_sign ;
	int m_temperature ;
public:
	GENTK( 2sTTT_dew, "2[019/][0-9/]{3}[;=]?" )

	bool Parse( const char * str )
	{
		m_temperature = 0;
		int nbMtch = sscanf( str, "2%c%3d", &m_temperature_sign, &m_temperature);
		switch( nbMtch) {
			case 0 : return false;
			case 1 : return 0 == strcmp( str + 1, "////" );
				 // Temperature check does not apply if humidity.
			case 2 : return ( m_temperature_sign == '9' ) || CheckCelsius( m_temperature_sign, m_temperature );
			default: return false;
		}
	}

	/// TTT -- Dewpoint temperature in .1 C (if sign is 9, TTT is relative humidity)
	void Print() const {
		// s -- sign of temperature (0=positive, 1=negative, 9 = Relative humidity)
		AppCelsius( this, _("Dewpoint temperature"), m_temperature_sign, m_temperature );
	}
};

/// Station pressure in 0.1 mb (thousandths digit omitted, last digit can be slash, then pressure in full mb)
static void thousands_omitted( const TokenProxy * ptrTok, const char * pressure, const char * title )
{
	char buf[7];
	int idx = 0 ;
	if( pressure[0] == '0' ) buf[idx++] = '1' ;
	strcpy( buf + idx, pressure );
	char *slashpos = strchr( buf, '/' );
	if( slashpos != NULL ) *slashpos = '0' ;
	double tmpPres ;
	// Checks reasonable values for a pressure.
	if( ( 1 != sscanf( buf, "%lf", &tmpPres ) )
		|| ( tmpPres > 12000 ) 
		|| ( tmpPres <  7000 ) ) {
		ptrTok->Append( title, _("Inconsistent:") + std::string(pressure) );
	} else {
		ptrTok->Append( title, (int)( tmpPres / 10.0 ), Unit_hPa );
	}
};

/// 3PPPP -- Station pressure in 0.1 mb (thousandths digit omitted, last digit can be slash, then pressure in full mb)
HEADTK(3PPPP) {
	char m_station_pressure[5];
public:
	GENTK( 3PPPP, "3[0789/][0-9/]{3}" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "3%s", m_station_pressure ) );
	}

	void Print() const {
		thousands_omitted( this, m_station_pressure, _("Station pressure") );
	}
};

/** 4PPPP -- Sea level pressure in 0.1 mb (thousandths digit omitted, last digit can be slash, then pressure in full mb)
Can be as well 4ahhh http://metaf2xml.sourceforge.net/parser.pm.html : 
a3 Isobaric surface (CT 0264), hhh Geopotential of isobaric surface
*/
HEADTK(4PPPP) {
	bool m_4PPPP ;
	char m_sea_level_pressure[5];
	char m_isobaric_surface ;
	int  m_geopotential ;
public:
	GENTK( 4PPPP, "4[0912378/][0-9/]{3}" )

	bool Parse( const char * str )
	{
		m_4PPPP = false;
		m_sea_level_pressure[0] = '\0';
		m_isobaric_surface = '/';
		m_geopotential = -1;
		switch( str[1] ) {
			case '0':
			case '9': m_4PPPP = true ;
				  return ( 1 == sscanf( str, "4%s", m_sea_level_pressure ) );
			case '1':
			case '2':
			case '3':
			case '7':
			case '8': m_4PPPP = false ;
				  return ( 2 == sscanf( str, "4%c%3d", &m_isobaric_surface, &m_geopotential ) );
			case '/': m_4PPPP = true ;
				  return ( 0 == strcmp( str, "4////" ) );
			default : return false ;
		}
	}

	void Print() const {
		if(m_4PPPP) {
			thousands_omitted( this, m_sea_level_pressure, _("Sea level pressure") );
		} else {
			int iso_surf = 0 ;
			switch( m_isobaric_surface ) {
				case '1': iso_surf = 1000; break; // 100 meters.
				case '2': iso_surf =  925; break; //  800 meters.
				case '3': iso_surf =  500; break; // 5000 meters
				case '7': iso_surf =  700; break; // 3000 meters
				case '8': iso_surf =  850; break; // 1500 meters.
				default : break ;
			}
			FM12CodeTable( xxx, 0264, _("Standard Isobaric surface for which the geopotential is reported") );
			Append( _("Isobaric surface"), iso_surf, Unit_hPa );
		}
	}
};

/// 4ahhh -- Geopotential of nearest mandatory pressure level
HEADTK(4ahhh) {
	char m_mandatory_pressure_level ;
	char m_geopotential_height[4] ;
public:
	GENTK( 4ahhh, "4[12579][0-9]{3}" )

	bool Parse( const char * str )
	{
		return ( 2 == sscanf( str, "4%c%s", &m_mandatory_pressure_level, m_geopotential_height ) );
	}

	void Print() const {
		/// a3 -- mandatory pressure level
		static const choice< char > mandatory_pressure_levels[] = {
			{ '1', "1000" },
			{ '2', "925" },
			{ '5', "500" },
			{ '7', "700" },
			{ '8', "850" } };

		Append( _("Mandatory pressure level"),
			choice_map( mandatory_pressure_levels, G_N_ELEMENTS( mandatory_pressure_levels ),
				m_mandatory_pressure_level, _("Unknown mandatory pressure level") ),
			_("millibar") );
		/// hhh -- geopotential height omitting thousandths digit
		thousands_omitted( this, m_geopotential_height, _("Geopotential height") );
	}
};

/// 5appp -- Pressure tendency over 3 hours
HEADTK(5appp) {
	char m_pressure_tendency ;
	int m_pressure_change ;
public:
	GENTK( 5appp, "5[0-8/][0-9/]{3}[;=]?" )

	bool Parse( const char * str )
	{
		// std::cout << "str=" << str << "\n";
		int nbMtch = sscanf( str, "5%c%d", &m_pressure_tendency, &m_pressure_change );
		switch( nbMtch ) {
			case 0 : return false;
			case 1 : return 0 == strcmp( str, "5////");
			case 2 : return true;
			default: return false;
		}
	}

	/// Symbols associated to each value: http://www.hpc.ncep.noaa.gov/html/stationplot_buoy.shtml
	void Print() const {
		/// a -- characteristics of pressure tendency
		static const char * pressure_tendencies[] = {
			_("Increasing, then decreasing. Same or higher"),
			_("Increasing, then steady. Raises"),
			_("Increasing steadily. Raises"),
			_("Decreasing or steady, then increasing. Raises"),
			_("Steady. Resultant same"),
			_("Decreasing, then increasing. Lowers"),
			_("Decreasing, then steady. Lowers"),
			_("Decreasing steadily. Lowers"),
			_("Increasing or steady, then decreasing. Lowers") };
		FM12CodeTable( pressure_tendencies, 0200, _("Characteristics of pressure tendency during three last hours") );
		disp_arr(pressure_tendencies,G_N_ELEMENTS(pressure_tendencies),m_pressure_tendency,'0',_("Pressure tendency"));

		/// ppp -- 3 hour pressure change in 0.1 mb
	       	if( m_pressure_tendency == '/' ) {
			Append( _("Pressure change"), _("Not specified") );
		} else {
			Append( _("Pressure change"), m_pressure_change / 10.0, Unit_hPa );
		}
	}
};

static void display_precipitation( const TokenProxy * ptrTok, char precipitation_duration )
{
	int nb_hours ;
	/// This could be replaced by a table like the other codes tables.
	switch( precipitation_duration ) {
		case '1': nb_hours = 6; break;
		case '2': nb_hours = 12; break;
		case '3': nb_hours = 18; break;
		case '4': nb_hours = 24; break;
		case '5': nb_hours = 1; break;
		case '6': nb_hours = 2; break;
		case '7': nb_hours = 3; break;
		case '8': nb_hours = 9; break;
		case '9': nb_hours = 15; break;
		case '/': nb_hours = 24; break;
		default : nb_hours = -1; break;
	}
	FM12CodeTable( xxx, 4019, _("Duration of period of precipitation") );
	if( nb_hours < 0 )
		ptrTok->Append( _("Precipitation duration"), _("Undetermined") );
	else
		ptrTok->Append( _("Precipitation duration"), nb_hours, Unit_hours );
}

/// 6RRRt -- Liquid precipitation
HEADTK(6RRRt) {
	int m_precipitation_amount ;
	char m_precipitation_duration ;
public:
	GENTK( 6RRRt, "6[0-9/]{3}[0-9/][;=]?" )

	bool Parse( const char * str )
	{
		m_precipitation_amount = -1 ;
		m_precipitation_duration = '0' ;
		return ( 2 == sscanf( str, "6%3d%c", &m_precipitation_amount, &m_precipitation_duration ) )
		||     ( 1 == sscanf( str, "6///%c", &m_precipitation_duration ) )
		||     ( 0 == strncmp( str, "60000", 5 ) );
	}

	void Print() const {
		/// RRR -- Precipitation amount in mm
		const char * precip = _("Precipitation amount");
		switch( m_precipitation_amount ) {
		case   0 ... 988 : Append( precip, m_precipitation_amount, Unit_mm );break;
		case         989 : Append( precip, _("989 mm or more") );break;
		case         990 : Append( precip, _("Traces") );break;
		case 991 ... 999 : Append( precip, ( m_precipitation_amount - 990 ) / 10.0, Unit_mm );break;
		default          : Append( precip, m_precipitation_amount, _("Inconsistent") ); break;
		}
		FM12CodeTable( xxx, 3590, _("Amount of precipitation") );

		/// t -- Duration over which precipitation amount measured
		display_precipitation( this, m_precipitation_duration );
	}
};

/// 7wwWW -- Present and past weather
HEADTK(7wwWW) {
	int m_present_weather ;
	char m_past_weather_1 ;
	char m_past_weather_2 ;
	mutable bool m_calc_done ;
	mutable bool m_automated ;

	/// Can be called only once this token is inserted in a group.
	void Calc(void) const {
		if( m_calc_done ) return ;
		m_calc_done = true ;
		const CLASSTK(iihVV) * ptr_iihVV = TokenProxy::get_ptr< CLASSTK(iihVV) >(false);
		if( ptr_iihVV ) {
			m_automated = ptr_iihVV->isAutomated();
		} else m_automated = false ;
	}

	void PrintPastWeather( char weather, const char * title ) const {
		static const char * past_weathers_manned[] = {
		_("Cloud covering less than half of sky"),
		_("Cloud covering more than half of sky during part of period and more than half during part of period"),
		_("Cloud covering more than half of sky"),
		_("Sandstorm, duststorm or blowing snow"),
		_("Fog, or thick haze"),
		_("Drizzle"),
		_("Rain"),
		_("Snow or mixed rain and snow"),
		_("Showers"),
		_("Thunderstorms") };
		FM12CodeTable( past_weathers_manned, 4561, _("Past weather reported from a manned weather station") );

		static const char * past_weathers_automated[] = {
		_("No significant weather"),
		_("Visibility reduced"),
		_("Blowing phenomena, visibility reduced"),
		_("Fog"),
		_("Precipitation"),
		_("Drizzle"),
		_("Rain"),
		_("Snow, or Ice pellets"),
		_("Showers or intermittent precipitation"),
		_("Thunderstorm") };
		FM12CodeTable( past_weathers_automated, 4532, _("Past weather reported from an automated weather station") );

		// http://fr.scribd.com/doc/86346935/Land-Synoptic-Code
		// http://atmo.tamu.edu/class/atmo251/LandSynopticCode.pdf "Land Synoptic Code"
		if( m_automated ) {
			disp_arr(past_weathers_automated,G_N_ELEMENTS(past_weathers_automated),weather,'0',title);
		} else {
			disp_arr(past_weathers_manned,G_N_ELEMENTS(past_weathers_manned),weather,'0',title);
		}
	}
public:
	CLASSTK(7wwWW) () : m_calc_done(false), m_automated(false) {}

	GENTK( 7wwWW, "7[0-9/]{2}[0-9/][0-9/]" )

	bool Parse( const char * str )
	{
		m_present_weather = '/' ;
		m_past_weather_1 = m_past_weather_2 = '/';
		return 	( 0 == strcmp( "7////", str ) ) ||
			( 2 == sscanf( str, "7//%c%c", &m_past_weather_1, &m_past_weather_2 ) ) ||
			( 3 == sscanf( str, "7%2d%c%c", &m_present_weather, &m_past_weather_1, &m_past_weather_2 ) );
	}

	void Print() const {
		Calc();

		/// ww -- Present weather
		static const char * present_weathers_manned[] = {
		_("Clear skies"),
		_("Clouds dissolving"),
		_("State of sky unchanged"),
		_("Clouds developing"),
		// Haze, smoke, dust or sand
		_("Visibility reduced by smoke"),
		_("Haze"),
		_("Widespread dust in suspension not raised by wind"),
		_("Dust or sand raised by wind"),
		_("Well developed dust or sand whirls"),
		_("Dust or sand storm within sight but not at station"),
		// Non-precipitation events
		_("Mist"),
		_("Patches of shallow fog"),
		_("Continuous shallow fog"),
		_("Lightning visible, no thunder heard"),
		_("Precipitation within sight but not hitting ground"),
		_("Distant precipitation but not falling at station"),
		_("Nearby precipitation but not falling at station"),
		_("Thunderstorm but no precipitation falling at station"),
		_("Squalls within sight but no precipitation falling at station"),
		_("Funnel clouds within sight"),
		// Precipitation within past hour but not at observation time
		_("Drizzle"),
		_("Rain"),
		_("Snow"),
		_("Rain and snow"),
		_("Freezing rain"),
		_("Rain showers"),
		_("Snow showers"),
		_("Hail showers"),
		_("Fog"),
		_("Thunderstorms"),
		// Duststorm, sandstorm, drifting or blowing snow
		_("Slight to moderate duststorm, decreasing in intensity"),
		_("Slight to moderate duststorm, no change"),
		_("Slight to moderate duststorm, increasing in intensity"),
		_("Severe duststorm, decreasing in intensity"),
		_("Severe duststorm, no change"),
		_("Severe duststorm, increasing in intensity"),
		_("Slight to moderate drifting snow, below eye level"),
		_("Heavy drifting snow, below eye level"),
		_("Slight to moderate drifting snow, above eye level"),
		_("Heavy drifting snow, above eye level"),
		// Fog or ice fog
		_("Fog at a distance"),
		_("Patches of fog"),
		_("Fog, sky visible, thinning"),
		_("Fog, sky not visible, thinning"),
		_("Fog, sky visible, no change"),
		_("Fog, sky not visible, no change"),
		_("Fog, sky visible, becoming thicker"),
		_("Fog, sky not visible, becoming thicker"),
		_("Fog, depositing rime, sky visible"),
		_("Fog, depositing rime, sky not visible"),
		// Drizzle
		_("Intermittent light drizzle"),
		_("Continuous light drizzle"),
		_("Intermittent moderate drizzle"),
		_("Continuous moderate drizzle"),
		_("Intermittent heavy drizzle"),
		_("Continuous heavy drizzle"),
		_("Light freezing drizzle"),
		_("Moderate to heavy freezing drizzle"),
		_("Light drizzle and rain"),
		_("Moderate to heavy drizzle and rain"),
		// Rain
		_("Intermittent light rain"),
		_("Continuous light rain"),
		_("Intermittent moderate rain"),
		_("Continuous moderate rain"),
		_("Intermittent heavy rain"),
		_("Continuous heavy rain"),
		_("Light freezing rain"),
		_("Moderate to heavy freezing rain"),
		_("Light rain and snow"),
		_("Moderate to heavy rain and snow"),
		// Snow
		_("Intermittent light snow"),
		_("Continuous light snow"),
		_("Intermittent moderate snow"),
		_("Continuous moderate snow"),
		_("Intermittent heavy snow"),
		_("Continuous heavy snow"),
		_("Diamond dust"),
		_("Snow grains"),
		_("Snow crystals"),
		_("Ice pellets"),
		// Showers
		_("Light rain showers"),
		_("Moderate to heavy rain showers"),
		_("Violent rain showers"),
		_("Light rain and snow showers"),
		_("Moderate to heavy rain and snow showers"),
		_("Light snow showers"),
		_("Moderate to heavy snow showers"),
		_("Light snow/ice pellet showers"),
		_("Moderate to heavy snow/ice pellet showers"),
		_("Light hail showers"),
		_("Moderate to heavy hail showers"),
		// Thunderstorms
		_("Thunderstorm in past hour, currently only light rain"),
		_("Thunderstorm in past hour, currently only moderate to heavy rain"),
		_("Thunderstorm in past hour, currently only light snow or rain/snow mix"),
		_("Thunderstorm in past hour, currently only moderate to heavy snow or rain/snow mix"),
		_("Light to moderate thunderstorm"),
		_("Light to moderate thunderstorm with hail"),
		_("Heavy thunderstorm"),
		_("Heavy thunderstorm with duststorm"),
		_("Heavy thunderstorm with hail") };
		FM12CodeTable( present_weathers_manned, 4677, _("Present weather reported from a manned weather station") );

		/// http://near-goos1.jodc.go.jp/rdmdb/format/JMA/wawa.html
		static const char * present_weathers_automated[] = {
		_("No significant weather observed"),
		_("Clouds generally dissolving or becoming less developed during the past hour"),
		_("State of sky on the whole unchanged during the past hour"),
		_("Clouds generally forming or developing during the past hour"),
		_("Haze or smoke, or dust in suspension in the air, visibility equal to, or greater than, 1 km"),
		_("Haze or smoke, or dust in suspension in the air, visibility less than 1 km"),
		_("Reserved"),
		_("Reserved"),
		_("Reserved"),
		_("Reserved"),
		_("Mist"),
		_("Diamond dust"),
		_("Distant lightning"),
		_("Reserved"),
		_("Reserved"),
		_("Reserved"),
		_("Reserved"),
		_("Squalls"),
		_("Reserved"),

		// Code figures 20–26 are used to report precipitation, fog (or ice fog)
		// or thunderstorm at the station during the preceding hour but not at the time of observation.
		_("Fog"),
		_("PRECIPITATION"),
		_("Drizzle (not freezing) or snow grains"),
		_("Rain (not freezing)"),
		_("Snow"),
		_("Freezing drizzle or freezing rain"),
		_("Thunderstorm (with or without precipitation)"),
		_("BLOWING OR DRIFTING SNOW OR SAND"),
		_("Blowing or drifting snow or sand, visibility equal to, or greater than, 1 km"),
		_("Blowing or drifting snow or sand, visibility less than 1 km"),
		_("FOG"),
		_("Fog or ice fog in patches"),
		_("Fog or ice fog, has become thinner during the past hour"),
		_("Fog or ice fog, no appreciable change during the past hour"),
		_("Fog or ice fog, has begun or become thicker during the past hour"),
		_("Fog, depositing rime"),
		_("Reserved"),
		_("Reserved"),
		_("Reserved"),
		_("Reserved"),
		_("PRECIPITATION"),
		_("Precipitation, slight or moderate"),
		_("Precipitation, heavy"),
		_("Liquid precipitation, slight or moderate"),
		_("Liquid precipitation, heavy"),
		_("Solid precipitation, slight or moderate"),
		_("Solid precipitation, heavy"),
		_("Freezing precipitation, slight or moderate"),
		_("Freezing precipitation, heavy"),
		_("Reserved"),
		_("DRIZZLE"),
		_("Drizzle, not freezing, slight"),
		_("Drizzle, not freezing, moderate"),
		_("Drizzle, not freezing, heavy"),
		_("Drizzle, freezing, slight"),
		_("Drizzle, freezing, moderate"),
		_("Drizzle, freezing, heavy"),
		_("Drizzle and rain, slight"),
		_("Drizzle and rain, moderate or heavy"),
		_("Reserved"),
		_("RAIN"),
		_("Rain, not freezing, slight"),
		_("Rain, not freezing, moderate"),
		_("Rain, not freezing, heavy"),
		_("Rain, freezing, slight"),
		_("Rain, freezing, moderate"),
		_("Rain, freezing, heavy"),
		_("Rain (or drizzle) and snow, slight"),
		_("Rain (or drizzle) and snow, moderate or heavy"),
		_("Reserved"),
		_("SNOW"),
		_("Snow, slight"),
		_("Snow, moderate"),
		_("Snow, heavy"),
		_("Ice pellets, slight"),
		_("Ice pellets, moderate"),
		_("Ice pellets, heavy"),
		_("Snow grains"),
		_("Ice crystals"),
		_("Reserved"),
		_("SHOWER(S) or INTERMITTENT PRECIPITATION"),
		_("Rain shower(s) or intermittent rain, slight"),
		_("Rain shower(s) or intermittent rain, moderate"),
		_("Rain shower(s) or intermittent rain, heavy"),
		_("Rain shower(s) or intermittent rain, violent"),
		_("Snow shower(s) or intermittent snow, slight"),
		_("Snow shower(s) or intermittent snow, moderate"),
		_("Snow shower(s) or intermittent snow, heavy"),
		_("Reserved"),
		_("Hail"),
		_("THUNDERSTORM"),
		_("Thunderstorm, slight or moderate, with no precipitation"),
		_("Thunderstorm, slight or moderate, with rain showers and/or snow showers"),
		_("Thunderstorm, slight or moderate, with hail"),
		_("Thunderstorm, heavy, with no precipitation"),
		_("Thunderstorm, heavy, with rain showers and/or snow showers"),
		_("Thunderstorm, heavy, with hail"),
		_("Reserved"),
		_("Reserved"),
		_("Tornado") };
		FM12CodeTable( present_weathers_automated, 4680, _("Present weather reported from an automated weather station") );

		// http://fr.scribd.com/doc/86346935/Land-Synoptic-Code

		if( m_automated ) {
			disp_arr(present_weathers_manned,
				G_N_ELEMENTS(present_weathers_manned),
				m_present_weather,0,_("Present weather - Manned"));
			PrintPastWeather( m_past_weather_1, _("Past weather type 1 - Manned"));
			PrintPastWeather( m_past_weather_2, _("Past weather type 2 - Automated"));
		} else {
			disp_arr(present_weathers_automated,
				G_N_ELEMENTS(present_weathers_automated),
				m_present_weather,0,_("Present weather - Automated"));
			PrintPastWeather( m_past_weather_1, _("Past weather type 1 - Automated"));
			PrintPastWeather( m_past_weather_2, _("Past weather type 2 - Automated"));
		}
	}
};

/// 8NCCC -- Cloud type information
HEADTK(8NCCC) {
	char m_low_clouds_amount ;
	char m_low_cloud_type ;
	char m_mid_cloud_type ;
	char m_high_cloud_type ;
public:
	GENTK( 8NCCC, "8[0-9/][0-9/]{3}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 4 == sscanf( str, "8%c%c%c%c",
			&m_low_clouds_amount,
			&m_low_cloud_type,
			&m_mid_cloud_type,
			&m_high_cloud_type ) );
	}

	void Print() const {
		/// N -- Amount of low clouds covering sky, if no low clouds, the amount of the middle clouds
		const char * low_cloud_title = m_low_cloud_type != '0' ? _("Amount of low clouds") : _("Amount of middle clouds");
		if( m_low_clouds_amount == '/' )
			Append( low_cloud_title, _("Unspecified") );
		else
			Append( low_cloud_title, m_low_clouds_amount - '0' );

		/// CL -- Low cloud type
		static const choice< char > arr_low_clouds[] = {
			{ '0', _("No low clouds") },
			{ '1', _("Cumulus humulis or fractus (no vertical development)") },
			{ '2', _("Cumulus mediocris or congestus (moderate vertical development)") },
			{ '3', _("Cumulonimbus calvus (no outlines nor anvil)") },
			{ '4', _("Stratocumulus cumulogenitus (formed by spreading of cumulus)") },
			{ '5', _("Stratocumulus") },
			{ '6', _("Stratus nebulosus (continuous sheet)") },
			{ '7', _("Stratus or cumulus fractus (bad weather)") },
			{ '8', _("Cumulus and stratocumulus (multilevel)") },
			{ '9', _("Cumulonimbus with anvil") },
			{ '/', _("Low clouds unobserved due to darkness or obscuration") }
		};
		Append( _("Low clouds type"),
			choice_map( arr_low_clouds, G_N_ELEMENTS( arr_low_clouds ), m_low_cloud_type, _("unknown low clouds type") ) );
		FM12CodeTable( arr_low_clouds, 0513, _("Clouds of the genera stratocumulus, stratus,cumulus, and cumulonimbus") );

		/// CM -- Middle cloud type
		static const choice< char > arr_mid_clouds[] = {
			{ '0', _("No middle clouds") },
			{ '1', _("Altostratus translucidous (mostly transparent)") },
			{ '2', _("Altostratus opacus or nimbostratus") },
			{ '3', _("Altocumulus translucidous (mostly transparent)") },
			{ '4', _("Patches of altocumulus (irregular, lenticular)") },
			{ '5', _("Bands of altocumulus") },
			{ '6', _("Altocumulus cumulogenitus (formed by spreading of cumulus)") },
			{ '7', _("Altocumulus (multilayers)") },
			{ '8', _("Altocumulus castellanus (having cumuliform tufts)") },
			{ '9', _("Altocumulus of a chaotic sky") },
			{ '/', _("Middle clouds unobserved due to darkness or obscuration ") },
		};
		Append( _("Middle clouds type"),
			choice_map( arr_mid_clouds, G_N_ELEMENTS( arr_mid_clouds ), m_mid_cloud_type, _("unknown middle clouds type") ) );
		FM12CodeTable( arr_mid_clouds, 0515, _("Clouds of the genera altocumulus,altostratus, and nimbostratus") );

		/// CH -- High cloud type
		static const choice< char > arr_high_clouds[] = {
			{ '0', _("No high clouds") },
			{ '1', _("Cirrus fibratus (wispy)") },
			{ '2', _("Cirrus spissatus (dense in patches)") },
			{ '3', _("Cirrus spissatus cumulogenitus (formed out of anvil)") },
			{ '4', _("Cirrus unicus or fibratus (progressively invading sky)") },
			{ '5', _("Bands of cirrus or cirrostratus invading sky (less than 45 degree above horizon)") },
			{ '6', _("Bands of cirrus or cirrostratus invading sky (more than 45 degree above horizon)") },
			{ '7', _("Cirrostratus covering whole sky") },
			{ '8', _("Cirrostratus not covering sky but not invading") },
			{ '9', _("Cirrocumulus") },
			{ '/', _("High clouds unobserved due to darkness or obscuration") }
		};
		Append( _("High clouds type"),
			choice_map( arr_high_clouds, G_N_ELEMENTS( arr_high_clouds ), m_high_cloud_type, _("unknown high clouds type") ) );
		FM12CodeTable( arr_mid_clouds, 0509, _("Clouds of the genera cirrus,cirrocumulus, and cirrostratus") );
	}
};

/// 9GGgg -- Time of observation in hours and minutes 
HEADTK(9GGgg) {
	int m_hours;
	int m_minutes;
public:
	GENTK( 9GGgg, "9[0-2][0-9][0-5][0-9][;=]?" )

	bool Parse( const char * str )
	{
		return ( 2 == sscanf( str, "9%2d%2d", &m_hours, &m_minutes ) );
	}

	void Print() const {
		Append( _("Observation time"), hour_min( m_hours, m_minutes ) );
	}
};

/// 222 Group - Sea Surface Observations
/// 
///       222Dv
HEADTK(222Dv) {
	char m_ship_direction;
	char m_ship_average_speed;
public:
	/// Not only it is selected before similar tokens, but also it can be kept at the end like two tokens together.
	GENTK_PRIORITY( 222Dv, "222[0-9/][0-9/][;=]?", MIN_PRIO )
	// GENTK( 222Dv, "222[0-9/][0-9/][;=]?" )

	bool Parse( const char * str )
	{
		return ( 2 == sscanf( str, "222%c%c", &m_ship_direction, &m_ship_average_speed ) );
	}

	void Print() const {
		/// D -- direction of ship movement
		const char * ship_directions[] = {
			_("Calm"),
			_("North-East"),
			_("East"),
			_("South-East"),
			_("South"),
			_("South-West"),
			_("West"),
			_("North-West"),
			_("North"),
			_("Unknown") };
		disp_arr(ship_directions,G_N_ELEMENTS(ship_directions),m_ship_direction,'0',_("Ship direction") );

		/// * v -- ship's average speed
		const char * ship_average_speeds[] = {
			_("0 knots"),
			_("1 to 5 knots"),
			_("6 to 10 knots"),
			_("11 to 15 knots"),
			_("16 to 20 knots"),
			_("21 to 25 knots"),
			_("26 to 30 knots"),
			_("31 to 35 knots"),
			_("36 to 40 knots"),
			_("over 40 knots ") };
		disp_arr(ship_average_speeds,G_N_ELEMENTS(ship_average_speeds),m_ship_average_speed,'0',_("Ship average speed") );
	}
};

/// 0sTTT -- Sea surface temperature
HEADTK(0sTTT) {
	/// s -- sign of temperature (0=positive, 1=negative)
	char m_temperature_sign ;
	/// TTT -- Temperature in .1 C
	int  m_temperature ;
	const char * m_temperature_type ;

	bool CheckParams() {
		switch(m_temperature_sign) {
			case '0':
			case '1': m_temperature_type = _("Intake measurement"); break;
			case '2':
			case '3': m_temperature_type = _("Bucket measurement"); break;
			case '4':
			case '5': m_temperature_type = _("Hull contact sensor"); break;
			case '6':
			case '7': m_temperature_type = _("Other"); break;
			default : m_temperature_type = _("Inconsistent"); break ;
		}
		switch(m_temperature_sign) {
			case '1':
			case '3':
			case '5':
			case '7': m_temperature_sign = '1' ;
				  break ;
			case '0':
			case '2':
			case '4':
			case '6': m_temperature_sign = '0' ;
				  break ;
			default : return false ;
		}
		return true ;
	}
public:
	/// TODO: Group terminator (=;) is detected before and could be removed from regular expressions.
	GENTK( 0sTTT, "0[01234567/][0-9/]{3}[;=]?" )

	bool Parse( const char * str )
	{
		m_temperature_sign = '/';
		m_temperature = 999;
		m_temperature_type = NULL;

		if( 0 == strncmp( str, "0////", 5 ) ) return true ;
		return ( 2 == sscanf( str, "0%c%3d", &m_temperature_sign, &m_temperature ) )
		&&     CheckParams()
		&&     CheckCelsius( m_temperature_sign, m_temperature );
	}

	/// TTT -- Temperature in .1 C. s -- sign of temperature (0=positive, 1=negative)
	void Print() const {
		if( m_temperature_sign != '/' )
			AppCelsius( this, _("Sea surface temperature"), m_temperature_sign, m_temperature );
		else
			Append( _("Sea surface temperature"), _("Unspecified") );

		if( m_temperature_type == NULL )
			Append( _("Temperature type"), _("Unspecified") ); 
		else
			Append( _("Temperature type"), m_temperature_type ); 
	}
};

/// 1PPHH -- Wave heights in 0.5 m increments
HEADTK(1PPHH) {
	/// PP -- Period of waves in seconds
	int m_waves_period;
	/// HH -- Height of waves in 0.5 m increments
	int m_waves_height;
public:
	GENTK( 1PPHH, "1[0-9/]{2}[0-9/]{2}" )

	bool Parse( const char * str )
	{
		m_waves_period = 0;
		m_waves_height = 0;
		return ( 0 == strncmp( str, "1////", 5 ) )
		||     ( 2 == sscanf( str, "1%2d%2d", &m_waves_period, &m_waves_height ) );
	}

	void Print() const {
		Append( _("Waves period"), m_waves_period, Unit_seconds );
		Append( _("Waves height"), 0.5 * m_waves_height, Unit_meters );
	}
};

/// http://www.top-wetter.de/themen/synopschluessel.htm says that 1PPHH is instrumented,
/// that 2PPHH is not and 70HHH is not instrumented either. Figures should be close however.
HEADTK(2PPHH) {
	int m_waves_period;
	int m_waves_height;
public:
	/// 2PPHH -- Wave period and heights (instrumented)
	GENTK( 2PPHH, "2[0-9/]{2}[0-9/]{2}" )

	bool Parse( const char * str )
	{
		m_waves_period = -1;
		m_waves_height = -1;
		if( 0 == strcmp( str, "2////") ) return true;
		int nbMtch = sscanf( str, "2%2d%2d", &m_waves_period, &m_waves_height );
		if(nbMtch != 2) return false ;
		/// Wave heights must be realistic, forty meters is too high..
		if( m_waves_height > 80 ) return false ;
		return true;
	}

	void Print() const {
		if( m_waves_period >= 0 )
			Append( _("Instrumented waves period"), m_waves_period, Unit_seconds );
		if( m_waves_height >= 0 )
			Append( _("Instrumented waves height"), 0.5 * m_waves_height, Unit_meters );
	}
};

/** Generally speaking:
Any element not reported are normally reported with a slash.
If an entire group of elements is not reported,
skip the group completely (Do not report a group as ///// )
*/


/// 3dddd -- Direction of swells (up to 2 swells)
HEADTK(3dddd) {
	int m_wind_direction1 ;
	int m_wind_direction2 ;

	/// TODO: The direction should be smaller than 36.
	void wind_dir( const char * title, int dir ) const {
		if( dir >= 0 )
			Append( title, dir * 10, Unit_degrees );
	}
public:
	GENTK( 3dddd, "3[0-9/]{4}" )

	bool Parse( const char * str )
	{
		m_wind_direction1 = -1;
		m_wind_direction2 = -1;
		if( 0 == strcmp( str, "3////") ) return true;
		return ( 2 == sscanf( str, "3%2d%2d", &m_wind_direction1, &m_wind_direction2 ) )
		    || ( 1 == sscanf( str, "3//%2d", &m_wind_direction2 ) )
		    || ( 1 == sscanf( str, "3%2d//", &m_wind_direction1 ) );
	}

	void Print() const {
		wind_dir( _("Direction of primary swell waves"), m_wind_direction1 );
		wind_dir( _("Direction of secondary swell waves"), m_wind_direction2 );
	}
};

/// 4PPHH -- Period and direction of first set of swells
HEADTK(4PPHH) {
	int m_swell_waves_period ;
	int m_swell_waves_height ;
public:
	GENTK( 4PPHH, "4[0-9/]{4}" )

	bool Parse( const char * str )
	{
		m_swell_waves_period = -1;
		m_swell_waves_height = -1;
		if( 0 == strcmp( str, "4////") ) return true;
		return ( 2 == sscanf( str, "4%2d%2d", &m_swell_waves_period, &m_swell_waves_height ) ) 
		||     ( 1 == sscanf( str, "4%2d//", &m_swell_waves_period ) );
	}

	void Print() const {
		if( m_swell_waves_period >= 0 )
			Append( _("Primary swell waves period"), m_swell_waves_period, Unit_seconds );
		if( m_swell_waves_height >= 0 )
			Append( _("Primary swell waves height"), 0.5 * m_swell_waves_height, Unit_meters );
	}
};

/// 5PPHH -- Period and direction of second set of swells
HEADTK(5PPHH) {
	int m_swell_waves_period ;
	int m_swell_waves_height ;
public:
	GENTK( 5PPHH, "5[0-9/]{4}" )

	bool Parse( const char * str )
	{
		m_swell_waves_period = -1;
		m_swell_waves_height = -1;
		if( 0 == strcmp( str, "5////") ) return true;

		int nbMtch = sscanf( str, "5%2d%2d", &m_swell_waves_period, &m_swell_waves_height );
		if( nbMtch != 2 ) return false ;

		/// Realistic values only. Waves are not 40 meters high.
		if( m_swell_waves_height > 80 ) return false ;
		return true ;
	}

	void Print() const {
		if( m_swell_waves_period >= 0 )
			Append( _("Secondary swell waves period"), m_swell_waves_period, Unit_seconds );
		if( m_swell_waves_height >= 0 )
			Append( _("Secondary swell waves height"), 0.5 * m_swell_waves_height, Unit_meters );
	}
};

/// 6IEER -- Ice accretion on ships
HEADTK(6IEER) {
	char m_ice_accretion_code;
	int m_ice_accretion_thickness;
	char m_ice_accretion_rate;
public:
	GENTK( 6IEER, "6[0-5/][0-9/]{2}[0-4/]" )

	bool Parse( const char * str )
	{
		m_ice_accretion_code = '_';
		m_ice_accretion_thickness = -1;
		m_ice_accretion_rate = '_';
		if( 0 == strcmp( str, "6////" ) ) return true;
		return ( 3 == sscanf( str, "6%c%2d%c", &m_ice_accretion_code, &m_ice_accretion_thickness, &m_ice_accretion_rate ) );
	}

	void Print() const {
		static const char * ice_accretion_codes[] = {
			_("Not relevant"), // Should not happen, but it does.
			_("Icing from ocean spray"),
			_("Icing from fog"),
			_("Icing from spray and fog"),
			_("Icing from rain"),
			_("Icing from spray and rain") };
		disp_arr(ice_accretion_codes,G_N_ELEMENTS(ice_accretion_codes),m_ice_accretion_code,'0',_("Ice accretion code") );

		if( m_ice_accretion_thickness >= 0 )
			Append( _("Ice accretion thickness"), m_ice_accretion_thickness, Unit_centimeters );
		else
			Append( _("Ice accretion thickness"), _("Not relevant") );

		static const char * ice_accretion_rates[] = {
			_("Ice not building up"),
			_("Ice building up slowly"),
			_("Ice building up rapidly"),
			_("Ice melting or breaking up slowly"),
			_("Ice melting or breaking up rapidly") };
		disp_arr(ice_accretion_rates,G_N_ELEMENTS(ice_accretion_rates),m_ice_accretion_rate,'0',_("Ice accretion rate") );
	}
};

/// 70HHH -- Wave heights to 0.1 m (instrumented)
HEADTK(70HHH) {
	int m_wave_height ;
public:
	/// This group does not appear in NWSOH document.
	GENTK( 70HHH, "70[0-9/]{3}[;=]?" )

	bool Parse( const char * str )
	{
		m_wave_height = -1;
		return ( 0 == strncmp( str, "70///", 5 ) )
		||     ( 1 == sscanf( str, "70%3d", &m_wave_height ) );
	}

	void Print() const {
		const char * txt = _("Wave height");
		if( m_wave_height >= 0 )
			Append( txt, m_wave_height * 0.1, Unit_meters );
		else
			Append( txt, _("Undetermined"), Unit_meters );
	}
};

/// 8aTTT -- Wet bulb temperature 
HEADTK(8aTTT) {
	char         m_wet_bulb_sign_type ;
	char         m_temperature_sign ;
	int          m_wet_bulb_temperature ;
	const char * m_title ;

	/// Called once we managed to extract the arguments.
	bool CheckParams() {
		switch( m_wet_bulb_sign_type ) {
			case '0':
				m_temperature_sign = '0';
				m_title = _("Positive or zero measured");
				break;
			case '1':
				m_temperature_sign = '1';
				m_title = _("Negative measured");
				break;
			case '2':
				m_temperature_sign = '1';
				m_title = _("Iced bulb measured");
				break;
			case '5':
				m_temperature_sign = '0';
				m_title = _("Positive or zero computed");
				break;
			case '6':
				m_temperature_sign = '1';
				m_title = _("Negative computed");
				break;
			case '7':
				m_temperature_sign = '1';
				m_title = _("Iced bulb computed");
				break;
			default :
				m_temperature_sign = '/';
				m_title = _("Inconsistent");
				return false;
			};
		return true;
	}
public:
	GENTK( 8aTTT, "8[0-9/]{4}[;=]?" )

	bool Parse( const char * str )
	{
		m_wet_bulb_sign_type = '/';
		m_temperature_sign = '/';
		m_wet_bulb_temperature = 0 ;
		m_title = "Not specified";
		if( 0 == strncmp( str, "8////", 5 ) ) return true;
		return ( 2 == sscanf( str, "8%c%3d", &m_wet_bulb_sign_type, &m_wet_bulb_temperature ) )
			&& CheckParams();
	}

	void Print() const {
		static const std::string sep(":");
		std::string title = _("Web bulb temperature") + sep + m_title ;
		AppCelsius( this, title.c_str(), m_temperature_sign, m_wet_bulb_temperature );
	}
};

/// Separator for ice detection. 
HEADTK(ICE) {
public:
	// TODO: It is possible to have free text after the string ICE.
	GENTK( ICE, "ICE" )

	bool Parse( const char * str )
	{
		return 0 == strcmp( str, "ICE" );
	}

	void Print() const {
		Append( _("Type"), "{ICE}" );
	}
	bool CanComeFirst(void) const { return false ; }
};

/// Appears only in NWSOH document.
/// http://www.ndbc.noaa.gov/ice/sea_ice.shtml
/// Data reported in Ships Synoptic Code, Group 2 ICE section (ciSibiDizi).
HEADTK(cSbDz) {
	char m_sea_ice_arrangement;
	char m_sea_ice_development_stage;
	char m_ice_of_land_origin;
	char m_bearing_of_principle_ice_edge;
	char m_sea_ice_situation ;
public:
	/// There might be four chars only ??
	GENTK( cSbDz, "[0-9/]{4}[0-9/]?[;=]?" )

	bool Parse( const char * str )
	{
		return ( 5 == sscanf( str, "%c%c%c%c%c", 
			&m_sea_ice_arrangement,
			&m_sea_ice_development_stage,
			&m_ice_of_land_origin,
			&m_bearing_of_principle_ice_edge,
			&m_sea_ice_situation ) );
	}

	void Print() const {
		/// ci = Concentration or Arrangement of Sea Ice
		static const char * sea_ice_arrangements[] = {
			_("No sea ice in sight"),
			_("Ship in open lead more than 1 nautical mile wide, or ship in fast ice with no boundary beyond limit of visibility"),
			_("Sea ice present in concentrations less than 3/10 (3/8); open water or very open pack ice"),
			_("4/10 to 6/10 (3/8 to less than 6/8); open pack ice"),
			_("7/10 to 8/10 (6/8 to less than 7/8); close pack ice"),
			_("9/10 or more, but not 10/10 (7/8 to less than 8/8); very close pack ice"),
			_("Strips and patches of pack ice with open water between"),
			_("Strips and patches of close or very close pack ice with areas of lesser concentration between"),
			_("Fast ice with open water, very open or open pack ice to seaward of the ice boundary"),
			_("Fast ice with close or very close pack ice to seaward of the ice boundary") };
		disp_arr(sea_ice_arrangements,G_N_ELEMENTS(sea_ice_arrangements),m_sea_ice_arrangement,'0',
			_("Concentration or arrangement of Sea Ice") );

		/// Si = Sea Ice Stage of Development
		static const char * sea_ice_development_stages[] = { 
			_("New ice only (frail ice, grease ice, slush ice, shuga)"),
			_("Nilas or ice rind, less than 10 cm thick"),
			_("Young ice (grey ice, grey-white ice), 10-30 cm thick"),
			_("Predominantly new and/or young ice with some first year ice"),
			_("Predominantly thin first-year ice with some new and/or young ice"),
			_("All thin first-year ice (30-70 cm thick)"),
			_("Predominantly medium first-year ice (70-120 cm thick) and thick first-year ice (more than 120 cm thick) with some thinner (younger) first-year ice"),
			_("All medium and first-year ice"),
			_("Predominantly medium and thick first-year ice with some old ice (usually more than 2 meters thick)"),
			_("Predominantly old ice") };
		disp_arr(sea_ice_development_stages,G_N_ELEMENTS(sea_ice_development_stages),m_sea_ice_development_stage,'0',
			_("Sea Ice Stage of Development") );

		/// bi = Ice of Land Origin
		static const char * ice_of_land_origins[] = {
			_("No ice of land origin"),
			_("1-5 icebergs, no growlers or bergy bits"),
			_("6-10 icebergs, no growlers or bergy bits"),
			_("11-20 icebergs, no growlers or bergy bits"),
			_("Up to and including 10 growlers and bergy bits - no icebergs"),
			_("More than 10 growlers and bergy bits - no icebergs"),
			_("1-5 icebergs with growlers and bergy bits"),
			_("6-10 icebergs with growlers and bergy bits"),
			_("11-20 icebergs with growlers and bergy bits"),
			_("More than 20 icebergs with growlers and bergy bits - a major hazard to navigation") };
		disp_arr(ice_of_land_origins,G_N_ELEMENTS(ice_of_land_origins),m_ice_of_land_origin,'0',
			_("Ice of Land Origin") );

		/// Di = Bearing of Principle Ice Edge
		static const char * bearing_of_principle_ice_edges[] = {
			_("Ship in shore or flaw lead"),
			_("Principle ice edge towards NE"),
			_("Principle ice edge towards E"),
			_("Principle ice edge towards SE"),
			_("Principle ice edge towards S"),
			_("Principle ice edge towards SW"),
			_("Principle ice edge towards W"),
			_("Principle ice edge towards NW"),
			_("Principle ice edge towards N"),
			_("Not determined (ship in ice)") };
		disp_arr(bearing_of_principle_ice_edges,G_N_ELEMENTS(bearing_of_principle_ice_edges),m_bearing_of_principle_ice_edge,'0',
			_("Bearing of Principle Ice Edge") );

		/// zi = Present Sea Ice Situation and Three Hour Trend
		static const char * sea_ice_situations[] = {
   			_("Ship in open water with floating ice in sight"),
   			_("Ship in easily penetrable ice; conditions improving"),
   			_("Ship in easily penetrable ice; conditions not changing"),
   			_("Ship in easily penetrable ice; conditions worsening"),
   			_("Ship in ice difficult to penetrate; conditions improving"),
   			_("Ship in ice difficult to penetrate; conditions not changing"),
   			_("Ice forming and floes freezing together"),
   			_("Ice under slight pressure"),
   			_("Ice under moderate or severe pressure"),
   			_("Ship beset") };
		disp_arr(sea_ice_situations,G_N_ELEMENTS(sea_ice_situations),m_sea_ice_situation,'0',
			_("Present Sea Ice Situation and Three Hour Trend") );
	}

	/// In fact, completely dependent on the previous token. THIS SHOULD BE ENFORCED.
	bool CanComeFirst(void) const { return false ; }
};

/// 333 Group - Special / Climatological Data
HEADTK(333) {
public:
	GENTK( 333, "333" )

	bool Parse( const char * str )
	{
		return 0 == strcmp( str, "333" );
	}

	void Print() const {
		Section( _("Special or climatological data") );
	}
};

/// 0.... -- Regionally developed data
HEADTK(0____) {
	char m_not_decoded_yet[5] ;
public:
	GENTK( 0____, "0[0-9A-Z/]{2}[0-9A-Z]{2}" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "0%4s", m_not_decoded_yet ) );
	}

	void Print() const {
		Append( _("Regionally developed data"), m_not_decoded_yet );
	}
};

/// 1sTTT_max -- Maximum temperature over previous 24 hours
HEADTK(1sTTT_max) {
	/// s -- sign of temperature (0=positive, 1=negative)
	char m_temperature_sign ;
	/// TTT -- Temperature in .1 C
	int m_temperature ;
public:
	GENTK( 1sTTT_max, "1[01/][0-9/]{3}[;=]?" )

	bool Parse( const char * str )
	{
		m_temperature_sign = '/';
		m_temperature = 0;
		if( 0 == strncmp( str, "1////", 5 ) ) return true ;
		return ( 2 == sscanf( str, "1%c%3d", &m_temperature_sign, &m_temperature) )
		&&     ( strchr( "01", m_temperature_sign ) != NULL )
		&&     CheckCelsius( m_temperature_sign, m_temperature );
	}

	/// s -- sign of temperature (0=positive, 1=negative). TTT -- Temperature in .1 C
	void Print() const {
		AppCelsius( this, _("Maximum 24 hours temperature"), m_temperature_sign, m_temperature );
	}
};


/// 2sTTT_min -- Minimum temperature over previous 24 hours
HEADTK(2sTTT_min) {
	/// s -- sign of temperature (0=positive, 1=negative)
	char m_temperature_sign ;
	/// TTT -- Temperature in .1 C
	int m_temperature ;
public:
	/// 12- bzw. 15-stündige Minimumtemperatur (wird nur um 06, 09 und 18 UTC gemeldet)
	GENTK( 2sTTT_min, "2[01/][0-9/]{3}[;=]?" )

	bool Parse( const char * str )
	{
		int nbMtch = sscanf( str, "2%c%3d", &m_temperature_sign, &m_temperature);
		switch( nbMtch) {
			case 0 : return false;
			case 1 : return 0 == strcmp( str + 1, "////" );
			case 2 : return CheckCelsius( m_temperature_sign, m_temperature );
			default: return false;
		}
	}

	/// s -- sign of temperature (0=positive, 1=negative)
	/// TTT -- Minimum temperature over previous 24 hours in .1 C (if sign is 9, TTT is relative humidity)
	void Print() const {
		AppCelsius( this, _("Minimum temperature over previous 24 hours"), m_temperature_sign, m_temperature );
	}
};

/// Not sure of how it should be decoded.
#ifdef DECODED_3Ejjj
/// Can be 3EssTgTg : http://www.ogimet.com/docs/WMO306vol-II.pdf
/// TgTg Ground (grass) minimum temperature of the preceding night, in whole degrees Celsius, its sign being given by sn.
/// (3-group in Section 3 of FM 12)
HEADTK(3Ejjj) {
	char m_ground_state ;
	int  m_temperature ;
	bool m_valid_temperature ;
public:
	GENTK( 3Ejjj, "3[0-9/][/01][/0-9]{2}[;=]?" )

	// No regional decision has been made for the use of these letters so they will be encoded as solidi (///).
	bool Parse( const char * str )
	{
		char char_sign, d1, d2 ;
		int r = sscanf( str, "3%c%c%c%c", &m_ground_state, &char_sign, &d1, &d2 );
		if( r != 4 ) return false ;

		if( ( d1 == '/' ) ^ ( d2 == '/' ) ) return false ;
		if( ( d1 == '/' ) == ( d2 == '/' ) ) {
			m_valid_temperature = false ;
			return true ;
		}
		m_valid_temperature = true ;
		m_temperature = 10 * ( d1 - '0' ) + ( d2 - '0' );
		if( char_sign == '1' ) m_temperature = -m_temperature ;
		return true ;
	}

	void Print() const {
		/// Code table 0901 E: State of the ground without snow or measurable ice cover
		static const char * ground_without_snow[] = {
			_("Surface of ground dry (without cracks and no appreciable amount of dust or loose sand)"),
			_("Surface of ground moist"),
			_("Surface of ground wet (standing water in small or large pools on surface)"),
			_("Flooded"),
			_("Surface of ground frozen"),
			_("Glaze on ground"),
			_("Loose dry dust or sand not covering ground completely"),
			_("Thin cover of loose dry dust or sand covering ground completely"),
			_("Moderate or thick cover of loose dry dust or sand covering ground completely"),
			_("Extremely dry with cracks") };
		FM12CodeTable( ground_without_snow, 0901, _("State of the ground without snow or measurable ice cover") );
		disp_arr(ground_without_snow,G_N_ELEMENTS(ground_without_snow),m_ground_state,'0',_("State of the ground without snow or measurable ice cover") );

		if( m_valid_temperature ) {
			Append( _("Ground grass minimum temperature of preceding night"), m_temperature, Unit_Celsius );
		}
	}
};
#else
/// 3Ejjj -- Regionally developed data
HEADTK(3Ejjj) {
	char m_not_decoded_yet[5] ;
public:
	/// TODO: See here: http://www.met.fu-berlin.de/~stefan/fm12.html#32
	GENTK( 3Ejjj, "3[0-9A-Z/]{4}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "3%4s", m_not_decoded_yet ) );
	}

	void Print() const {
		Append( _("Regionally developed data"), m_not_decoded_yet );
	}
};
#endif

/// 4Esss -- Snow depth
HEADTK(4Esss) {
	char m_snow_cover ;
	int m_snow_depth ;
public:
	GENTK( 4Esss, "4[0-9/][0-9]{3}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 2 == sscanf( str, "4%c%3d", &m_snow_cover, &m_snow_depth ) );
	}

	// http://fr.scribd.com/doc/86346935/Land-Synoptic-Code
	void Print() const {
		/// E-prime -- State of ground with snow cover
		static const char *snow_covers[] = {
			_("Predominantly covered with ice"),
			_("Compact or wet snow covering less than half of ground"),
			_("Compact or wet snow covering more than half of ground but not completely covered"),
			_("Even layer of compact or wet snow covering entire ground"),
			_("Uneven layer of compact or wet snow covering entire ground"),
			_("Loose dry snow covering less than half of ground"),
			_("Loose dry snow covering more than half of ground but not completely covered"),
			_("Even layer of loose dry snow covering entire ground"),
			_("Uneven layer of loose dry snow covering entire ground"),
			_("Snow covering ground completely with deep drifts ") };
		disp_arr(snow_covers,G_N_ELEMENTS(snow_covers),m_snow_cover,'0',_("State of ground with snow cover") );
		FM12CodeTable( snow_covers, 0975, _("State of the ground with snow cover or measurable ice cover") );

		/// sss -- snow depth in cm: Code table 3889 sss : Total depth of snow
		switch( m_snow_depth ) {
			case 0 : 
				Append( _("Snow depth"), _("Not used") );
				break;
			default :
				Append( _("Snow depth"), m_snow_depth, Unit_centimeters );
				break;
			case 997 : 
				Append( _("Snow depth"), _("Less than 0.5 cm") );
				break;
			case 998 : 
				Append( _("Snow depth"), _("Snow cover, not continuous") );
				break;
			case 999 : 
				Append( _("Snow depth"), _("Measurement impossible or inaccurate") );
				break;
		}
		FM12CodeTable( , 3889, _("Total depth of snow") );
	}
};

/// 5jjjj -- Additional information
HEADTK(5jjjj) {
	char m_not_decoded_yet[5] ;
public:
	GENTK( 5jjjj, "5[012346789/][0-9/]{3}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "5%4s", m_not_decoded_yet ) )
			|| ( 0 == strncmp( str, "5////", 5 ) );
	}

	void Print() const {
		Append( _("Additional information"), m_not_decoded_yet );
	}
};

/// 553SS -- Sonnenscheindauer der letzten ganzen bzw. halben* Stunde in 1/10 Stunden
HEADTK(553SS) {
	int m_Sonnenscheindauer;
public:
	GENTK( 553SS, "553[0-9/]{2}[;=]?" )

	bool Parse( const char * str )
	{
		m_Sonnenscheindauer = -1;
		return ( 0 == strncmp( str, "553//", 5 ) )
		||     ( 1 == sscanf( str, "553%2d", &m_Sonnenscheindauer ) );
	}

	void Print() const {
		const char * txt = _("Sonnenscheindauer der letzten ganzen");
		if( m_Sonnenscheindauer < 0 )
			Append( txt, _("Undetermined"), Unit_hours );
		else
			Append( txt, m_Sonnenscheindauer / 10.0, Unit_hours );
	}
};

/* Not sure: http://www.met.fu-berlin.de/~stefan/fm12.html#32
 * 55SSS -- Sonnenscheindauer des Vortags in 1/10 Stunden (wird nur um 06 UTC gemeldet)
 * 553SS -- Sonnenscheindauer der letzten ganzen bzw. halben* Stunde in 1/10 Stunden
 */

/// 55jjj jjjjj -- Additional information (can be multiple groups)
HEADTK(55jjj) {
	char m_not_decoded_yet[5] ;
public:
	// Not sure it can really have four chars.
	GENTK( 55jjj, "55[012456789/][0-9/]{2}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "55%3s", m_not_decoded_yet ) );
	}

	void Print() const {
		Append( _("Undecoded extra information"), m_not_decoded_yet );
	}
};

/// Can be added several times after 55jjj
HEADTK(jjjjj) {
	char m_not_decoded_yet[5] ;
public:
	GENTK( jjjjj, "[0-9/]{5}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "%5s", m_not_decoded_yet ) );
	}

	void Print() const {
		Append( _("Undecoded extra information"), m_not_decoded_yet );
	}

	/// It is dependent on the previous token.
	bool CanComeFirst(void) const { return false ; }
};

/// 2FFFF -- Summe der Globalstrahlung des Vortags in J/cm2
HEADTK(2FFFF) {
	int m_global_strahlung;
public:
	GENTK( 2FFFF, "2[0-9/]{4}[;=]?" )

	bool Parse( const char * str )
	{
		m_global_strahlung = -1;
		return ( 0 == strncmp( str, "2////", 5 ) )
		||     ( 1 == sscanf( str, "2%4d", &m_global_strahlung ) );
	}

	void Print() const {
		/// Global Strahlung
		const char * txt = _("Global radiation");
		if( m_global_strahlung >= 0 )
			Append( txt, m_global_strahlung, "J/cm2" );
		else
			Append( txt, _("Undetermined") );
	}
};

/// 3FFFF -- Diffuse Himmelsstrahlung der letzten ganzen bzw. halben* Stunde in kJ/m2 = 1/10 J/cm2
HEADTK(3FFFF) {
	int m_himmel_strahlung;
public:
	GENTK( 3FFFF, "3[0-9/]{4}[;=]?" )

	bool Parse( const char * str )
	{
		m_himmel_strahlung = -1;
		return ( 0 == strncmp( str, "3////", 5 ) )
		||     ( 1 == sscanf( str, "3%4d", &m_himmel_strahlung ) );
	}

	/// Diffuse Himmelsstrahlung der letzten ganzen bzw. halben* Stunde
	void Print() const {
		Append( _("Diffuse sky radiation of last half hour"),
			m_himmel_strahlung, "kJ/m2" );
	}
};

/// 4FFFF -- Atmosphärische Wärmestrahlung der letzten ganzen bzw. halben* Stunde
HEADTK(4FFFF) {
	int m_waerme_strahlung;
public:
	GENTK( 4FFFF, "4[0-9/]{4}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "4%4d", &m_waerme_strahlung ) );
	}

	/// Atmosphärische Wärmestrahlung der letzten ganzen bzw. halben* Stunde
	void Print() const {
		Append( _("Atmospheric thermal radiation of last half hour"),
			m_waerme_strahlung, "kJ/m2" );
	}
};

/// 6RRRtb -- Liquid precipitation
typedef CLASSTK(6RRRt) CLASSTK(6RRRtb);

/// 7RRRR -- 24 hour precipitation in mm
HEADTK(7RRRR) {
	int m_24h_precipitations_mm ;
public:
	GENTK( 7RRRR, "7[0-9]{4}=?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "7%4d", &m_24h_precipitations_mm ) );
	}

	void Print() const {
		const char * precip = _("24 hours precipitations");
		if( m_24h_precipitations_mm == 9999 )
			Append( precip, _("None") );
		else
			Append( precip, m_24h_precipitations_mm, Unit_mm );
	}
};

static const char * cloud_genuses[] = {
	_("Cirrus (Ci)"),
	_("Cirrocumulus (Cc)"),
	_("Cirrostratus (Cs)"),
	_("Altocumulus (Ac)"),
	_("Altostratus (As)"),
	_("Nimbostratus (Ns)"),
	_("Stratocumulus (Sc)"),
	_("Stratus (St)"),
	_("Cumulus (Cu)"),
	_("Cumulonimbus (Cb)") };

/// 8NChh -- Cloud layer data
HEADTK(8NChh) {
	char m_cloud_cover ;
	char m_cloud_genus ;
	int m_cloud_base_height ;
public:
	GENTK( 8NChh, "8[0-9/][0-9/][0-9/]{2}[;=]?" )

	bool Parse( const char * str )
	{
		m_cloud_cover = '0';
		m_cloud_genus = '/';
		m_cloud_base_height = 0 ;
		return ( 3 == sscanf( str, "8%c%c%2d", &m_cloud_cover, &m_cloud_genus, &m_cloud_base_height ) )
		||     ( 2 == sscanf( str, "8%c%c//", &m_cloud_cover, &m_cloud_genus ) )
		|| ( 0 == strncmp( str, "80///", 5 ) );	
	}

	void Print() const {

		// N -- cloud cover
		disp_arr(cloud_covers,G_N_ELEMENTS(cloud_covers),m_cloud_cover,'0',_("Cloud cover"));

		// C -- genus of cloud
		disp_arr(cloud_genuses,G_N_ELEMENTS(cloud_genuses),m_cloud_genus,'0',_("Cloud genus") );

		// * hh -- height of cloud base
		const char * title = _("Cloud base height");
		switch( m_cloud_base_height ) {
			case  0        : Append( title, _("Less than 30 meters") ); break;
			case  1 ... 50 : Append( title, m_cloud_base_height * 30, Unit_meters ); break;
			case 51 ... 56 : Append( title, 1500 + ( m_cloud_base_height - 50 ) *   50, Unit_meters ); break; 
			case 57 ... 80 : Append( title, 1800 + ( m_cloud_base_height - 56 ) *  300, Unit_meters ); break; 
			case 81 ... 88 : Append( title, 9000 + ( m_cloud_base_height - 80 ) * 1500, Unit_meters ); break; 
			case 89        : Append( title, _("Greater than 21000 m") ); break;
			case 90 ... 99 : disp_arr(cloud_bases,G_N_ELEMENTS(cloud_bases),m_cloud_base_height,90,title );
					break;
			default        : break;
		}
	}
};

/// 9SSss -- Supplementary information 
/// http://www.met.fu-berlin.de/~stefan/fm12.html#32
/// 9SPSPspsp -- Besondere Wettererscheinungen und zusätzliche Informationen (Gruppe kann mehrmals verschlüsselt werden)
HEADTK(9SSss) {
	int m_figure ;
	int m_value ;
public:
	GENTK( 9SSss, "9[0-9]{2}[0-9/]{2}[;=]?" )

	bool Parse( const char * str )
	{
		m_value = -1 ;
		return ( 2 == sscanf( str, "9%2d%2d", &m_figure, &m_value ) )
		   ||  ( 1 == sscanf( str, "9%2d//", &m_figure ) );
	}

	// TODO: Description of decoding alone takes about 100 lines.
	// http://www.met.fu-berlin.de/~stefan/fm12.html#32
	void Print() const {
		Append( _("Figure"), m_figure );
		Append( _("Value"), m_value );
	}
};

/// 444 Group - National data, clouds.
HEADTK(444) {
public:
	GENTK( 444, "444" )

	bool Parse( const char * str )
	{
		return 0 == strcmp( str, "444" );
	}

	void Print() const {
		Section( "National data, clouds" );
	}
};

/// NCHHC
/// The coding can be more complicated, with several groups.
/// http://www.top-wetter.de/themen/synopschluessel.htm#444
HEADTK(NCHHC) {
	char m_cloud_cover ;
	char m_cloud_genus ;
	int m_cloud_top_height ;
	char m_cloud_characteristics ;
public:
	GENTK( NCHHC, "[0-9][0-9][0-9]{2}[0-9][;=]?" )

	bool Parse( const char * str )
	{
		m_cloud_cover = '0';
		m_cloud_genus = '/';
		m_cloud_top_height = 0 ;
		m_cloud_characteristics = '/';
		return ( 4 == sscanf( str, "%c%c%2d%c", &m_cloud_cover, &m_cloud_genus, &m_cloud_top_height, &m_cloud_characteristics ) );
	}

	void Print() const {
		// N -- cloud cover
		disp_arr(cloud_covers,G_N_ELEMENTS(cloud_covers),m_cloud_cover,'0',_("Cloud cover"));

		// C -- genus of cloud
		disp_arr(cloud_genuses,G_N_ELEMENTS(cloud_genuses),m_cloud_genus,'0',_("Cloud genus") );

		// * hh -- height of cloud top
		Append( _("Cloud top height"), m_cloud_top_height * 100, Unit_meters );

		static const char * cloud_characteristics[] = {
			_("Scattered clouds"), // Vereinzelte Wolken
			_("Flat, closed cloud cover"), // Flache, geschlossene Wolkendecke
			_("Shallow clouds with small apertures"), // Flache Wolkendecke mit kleinen Durchbrüchen
			_("Shallow clouds with large openings"), // Flache Wolkendecke mit großen Durchbrüchen
			_("Corrugated, solid cloud cover"), // Gewellte, geschlossene Wolkendecke
			_("Undulating clouds with small apertures"), // Gewellte Wolkendecke mit kleinen Durchbrüchen
			_("Undulating clouds with large openings"), // Gewellte Wolkendecke mit großen Durchbrüchen
			_("Closed billows clouds"), // Geschlossene Wogenwolkendecke
			_("Groups of waves of clouds"), // Gruppen von Wogenwolken
			_("Several layers of clouds at different altitudes") // Mehrere Wolkenschichten in verschiedenen Höhen
		};
		disp_arr(cloud_characteristics,G_N_ELEMENTS(cloud_characteristics),m_cloud_characteristics,'0',_("Cloud characteristics") );
	}
};


/// 555 Group - National code group
HEADTK(555) {
public:
	GENTK( 555, "555" )

	bool Parse( const char * str )
	{
		return 0 == strcmp( str, "555" );
	}

	void Print() const {
		Section( _("National code group") );
	}
};

/// 0sTTT_land -- Land surface temperature
/// s -- sign of temperature (0=positive, 1=negative)
/// TTT -- Temperature in .1 C
HEADTK(0sTTT_land) {
	char m_temperature_sign ;
	int  m_temperature ;
public:
	// TODO: Do not know why, string finished by a semicolon.
	GENTK( 0sTTT_land, "0[01][0-9/]{3}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 2 == sscanf( str, "0%c%3d", &m_temperature_sign, &m_temperature ) )
		&&     ( strchr( "01", m_temperature_sign ) != NULL )
		&&     CheckCelsius( m_temperature_sign, m_temperature );
	}

	void Print() const {
		// s -- sign of temperature (0=positive, 1=negative)
		// TTT -- Temperature in .1 C
		AppCelsius( this, _("Land temperature 5 cm over surface"), m_temperature_sign, m_temperature );
	}
};

/// 1RRRr -- Niederschlagsmenge in der letzten ganzen bzw. halben* Stunde
HEADTK(1RRRr) {
	int m_precipitation_amount ;
	char m_precipitation_duration ;
public:
	GENTK( 1RRRr, "1[0-9/]{3}[0-9/][;=]?" )

	bool Parse( const char * str )
	{
		m_precipitation_amount = -1 ;
		m_precipitation_duration = '0' ;
		return ( 2 == sscanf( str, "1%3d%c", &m_precipitation_amount, &m_precipitation_duration ) );
	}

	void Print() const {
		// RRR -- Precipitation amount in mm : Niederschlagsmenge
		const char * precip_amount = _("Precipitation amount");
		switch( m_precipitation_amount ) {
		default  : Append( precip_amount, m_precipitation_amount / 10.0, Unit_mm );break;
		case 999 : Append( precip_amount, _("Not measurable") );break;
		}

		const char * precip_dur = _("Precipitation duration");
		// t -- Duration over which precipitation amount measured
		switch( m_precipitation_duration ) {
		case '0' ... '9' : Append( precip_dur, 6 * ( m_precipitation_duration - '0' ), Unit_minutes ); break;
		default          : Append( precip_dur, _("Undetermined") ); break;
		}
	}
};

/// 2sTTT_avg -- Tagesmittel der Lufttemperatur des Vortages
HEADTK(2sTTT_avg) {
	char m_temperature_sign ;
	int m_temperature ;
public:
	GENTK( 2sTTT_avg, "2[01][0-9]{3}[;=]?" )

	bool Parse( const char * str )
	{
		int nbMtch = sscanf( str, "2%c%3d", &m_temperature_sign, &m_temperature);
		switch( nbMtch) {
			case 0 : return false;
			case 1 : return 0 == strcmp( str + 1, "////" );
			case 2 : return CheckCelsius( m_temperature_sign, m_temperature );
			default: return false;
		}
	}

	/// s: sign of temperature (0=positive, 1=negative, 9 = RH). TTT: Temperature in .1 C (if sign is 9, TTT is relative humidity)
	void Print() const {
		AppCelsius( this, _("Daily mean air temperature of the previous day"), m_temperature_sign, m_temperature );
	}
};

HEADTK(22fff) {
	int m_wind_speed_meter_second;
public:
	GENTK( 22fff, "22[0-9]{3}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "22%3d", &m_wind_speed_meter_second ) );
	}

	void Print() const {
		Append( _("Wind speed 10 minutes average"), m_wind_speed_meter_second / 10.0, Unit_meters_second );
	}
};

HEADTK(23SS) {
	int m_sun_shine_duration;
public:
	GENTK( 23SS, "23[0-9]{2}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "23%2d", &m_sun_shine_duration ) );
	}

	void Print() const {
		Append( _("Total hours of sunshine duration"), m_sun_shine_duration, Unit_minutes );
	}
};

HEADTK(24Wt) {
	char m_precipitation_indicator;
	char m_precipitation_duration;
public:
	GENTK( 24Wt, "24[01236789]{2}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "24%c%c", &m_precipitation_indicator, &m_precipitation_duration ) );
	}

	void Print() const {
		// WR -- Indikator zur Niederschlagsgruppe und Kennzeichnung der Niederschlagsform
		static const char * Niederschlagsgruppen[] = {
			_("Kein Niederschlag"),
			_("Nur abgesetzte Niederschläge"),
			_("Nur flüssige abgesetzte Niederschläge"),
			_("Nur feste abgesetzte Niederschläge"),
			_("Undefined"),
			_("Undefined"),
			_("Niederschlag in flüssiger Form"),
			_("Niederschlag in fester Form"),
			_("Niederschlag in flüssiger und fester Form"),
			_("Niederschlagsmessung ausgefallen") };
		// TODO: TRANSLATION
		disp_arr(Niederschlagsgruppen,G_N_ELEMENTS(Niederschlagsgruppen),m_precipitation_indicator,'0',_("Precipitations group") );
		// t -- Duration over which precipitation amount measured
		display_precipitation( this, m_precipitation_duration );
	}
};

/// 25wzwz -- zusätzliche Wettererscheinung  (Gruppe kann bis zu 4-mal verschlüsselt werden):
HEADTK(25ww) {
	int m_wetter_erscheinung;
public:
	GENTK( 25ww, "25[0-9]{2}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "25%2d", &m_wetter_erscheinung ) );
	}

	void Print() const {

		static const choice< int > wettererscheinungen[] = {
    			// Kondensstreifen
			{ 1, _("Sich schnell auflösende Kondensstreifen (Lebensdauer < 1 Minute)") },
			{ 2, _("Sich langsam auflösende Kondensstreifen (Lebensdauer 1 bis 14 Minuten)") },
			{ 3, _("Beständige Kondensstreifen (Lebensdauer > 15 Minuten)") },
			{ 4, _("Aufgelöste, vollständig in Cirrusbewölkung übergegangene Kondensstreifen") },
    			// Reif
			{ 5, _("Strahlungsreif") },
			{ 6, _("Advektionsreif") },
			{ 7, _("Rauhreif") },
			{ 8, _("Rauheis") },
			{ 9, _("Klareis") },
    			// Decken aus festen Niederschlägen (> 50 % des Erdbodens bedeckend)
			{15, _("Grieseldecke") },
			{16, _("Eiskörnerdecke") },
			{17, _("Graupeldecke") },
			{18, _("Hageldecke") },
    			// Wettererscheinungen
			{23, _("Eiskörner in der letzten Stunde") },
			{24, _("Glatteisbildung in der letzten Stunde") },
			{31, _("Sandfegen (unter Augenhöhe)") },
			{32, _("Sandtreiben (über Augenhöhe)") },
			{33, _("Sandverwehungen > 5 cm") },
			{36, _("Schneeverwehungen > 20 cm") },
			{45, _("nässender Nebel") },
    			// Glätteerscheinungen
			{71, _("Reifglätte") },
			{75, _("Schneeglätte") },
			{76, _("Eisglätte (überfrierende Nässe)") },
			{77, _("Glatteis (gefrierender Regen/Sprühregen)") },
			{81, _("Gefrierender Schauerniederschlag") },
    			// sonstige Erscheinungen
			{99, _("Böenwalze") }
		};
		Append( _("Zusätzliche Wettererscheinung"),
			choice_map( wettererscheinungen, G_N_ELEMENTS(wettererscheinungen), m_wetter_erscheinung, _("Undefined") ) );
		// TODO: TRANSLATION
	}
};

/// 26fff -- mittlere Windgeschwindigkeit der letzten Stunde in 1/10 m/s
HEADTK(26fff) {
	int m_wind_speed_meter_second;
public:
	GENTK( 26fff, "22[0-9]{3}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "26%3d", &m_wind_speed_meter_second ) );
	}

	void Print() const {
		Append( _("Last hour average wind speed"),
			m_wind_speed_meter_second / 10.0, Unit_meters_second );
	}
};

/// 3LGLGLsLs -- Anzahl der registrierten Blitze in den letzten 30 Minuten
/// LG LG -- Gesamtzahl der Blitze
/// Ls Ls -- Anzahl der Blitze starker Intensität
HEADTK(3LLLL) {
	int m_Gesamtzahl_der_Blitze;
	int m_Anzahl_der_Blitze_starker_Intensitaet;
public:
	GENTK( 3LLLL, "3[0-9]{4}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 2 == sscanf( str, "2%2d%2d", &m_Gesamtzahl_der_Blitze, &m_Anzahl_der_Blitze_starker_Intensitaet ) );
	}

	void Print() const {
		Append( _("Number of flashes"), m_Gesamtzahl_der_Blitze );
		Append( _("Number of high-intensity flashes"), m_Anzahl_der_Blitze_starker_Intensitaet );
	}
};

/*
    * 4RwRwwzwz -- Wasseräquivalent der Schneedecke und zusätzliche Wettererscheinungen
      Rw Rw -- Spezifisches Wasseräquivalent der Schneedecke in 1/10 mm/cm
      wz wz -- zusätzliche Wettererscheinung:

          Kondensstreifen
          01 -- sich schnell auflösende Kondensstreifen
          02 -- sich langsam auflösende Kondensstreifen
          03 -- beständige Kondensstreifen
          04 -- aufgelöste, in Cirrusbewölkung übergegangene Kondensstreifen
          Reif
          05 -- Strahlungsreif
          06 -- Advektionsreif
          07 -- Rauhreif
          08 -- Rauheis
          09 -- Klareis
          Decken aus festen Niederschlägen (> 50 % des Erdbodens bedeckend)
          15 -- Grieseldecke
          16 -- Eiskörnerdecke
          17 -- Graupeldecke
          18 -- Hageldecke
          Wettererscheinungen
          23 -- Eiskörner in der letzten Stunde
          24 -- Glatteisbildung in der letzten Stunde
          31 -- Sandfegen, unter Augenhöhe
          32 -- Sandtreiben, über Augenhöhe
          33 -- Sandverwehungen > 5 cm
          36 -- Schneeverwehungen > 20 cm
          Glätteerscheinungen
          71 -- Reifglätte
          75 -- Schneeglätte
          76 -- Eisglätte (überfrierende Nässe)
          77 -- Glatteis (gefrierender Regen/Sprühregen)
          81 -- gefrierender Schauerniederschlag
          sonstige Erscheinungen
          99 -- Böenwalze

    * 5s's's'tR -- Neuschneehöhe

      Hinweis: Mit dieser Gruppe wird in der Regel um 06 UTC die 24-stündige und um 18 UTC die 12-stündige Neuschneehöhe gemeldet.

      s's's' -- Neuschneehöhe in cm   (997 = < 0.5 cm, 998 = Flecken oder Reste, 999 = Angabe nicht möglich)

      tR -- Bezugszeitraum, über welche die Neuschneehöhe gemessen wurde:

          0 -- nicht aufgeführter oder vor dem Termin endender Zeitraum
          1 -- 6 Stunden
          2 -- 12 Stunden
          3 -- 18 Stunden
          4 -- 24 Stunden
          5 -- 1 Stunde bzw. 30 Minuten (bei Halbstundenterminen)
          6 -- 2 Stunden
          7 -- 3 Stunden
          8 -- 9 Stunden
          9 -- 15 Stunden
          / -- Sondermessung wegen Überschreitung des Schwellenwerts (> 5 cm bzw. 10 cm in < 12 Stunden)

    * 7h'h'ZD' -- Dunst, Talnebel und Wolken unterhalb des Stationsniveaus
      h'h' -- Höhe der Obergrenze der Erscheinung über NN
      Z -- Entwicklung von Dunst, Talnebel und Wolken unterhalb des Stationsniveaus
      D' -- Sektor des Hauptanteils der Bedeckung mit der Erscheinung
    * 8Ns/hshs -- automatisch ermittelte Höhe von Wolkenschichten (Gruppe kann mehrmals verschlüsselt werden)
      Ns -- Bedeckungsgrad der Wolkenschicht in Achteln
      hs hs -- Höhe der Wolkenuntergrenze:

          00 -- < 30 m (< 100 ft)
          01 -- 30 m (100 ft)
          02 -- 60 m (200 ft)
          03 -- 90 m (300 ft)
          ...
          50 -- 1500 m (5000 ft)
          ======================
          56 -- 1800 m (6000 ft)
          57 -- 2100 m (7000 ft)
          ...
          80 -- 9000 m (30000 ft)
          =======================
          81 -- 10500 m (35000 ft)
          82 -- 12000 m (40000 ft)
          ...
          88 -- 21000 m (70000 ft)
          89 -- höher als 21000 m (> 70000 ft)
          ====================================
          90 -- 0 bis 49 m (0 - 166 ft)
          91 -- 50 bis 99 m (167 - 333 ft)
          92 -- 100 bis 199 m (334 - 666 ft)
          93 -- 200 bis 299 m (667 - 999 ft)
          94 -- 300 bis 599 m (1000 - 1999 ft)
          95 -- 600 bis 999 m (2000 - 3333 ft)
          96 -- 1000 bis 1499 m (3334 - 4999 ft)
          97 -- 1500 bis 1999 m (5000 - 6666 ft)
          98 -- 2000 bis 2499 m (6667 - 8333 ft)
          99 -- 2500 m oder höher (> 8334 ft)
    * 910ff -- Höchste Windspitze in den letzten 10 Minuten (wird immer gemeldet!)
*/

/// 911ff -- Höchste Windspitze in der letzten Stunde (wird immer gemeldet!)
HEADTK(911ff) {
	int m_hoechste_windspitze_letzten_stunde;
public:
	GENTK( 911ff, "911[0-9]{2}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "911%2d", &m_hoechste_windspitze_letzten_stunde ) );
	}

	void Print() const {
		Append( _("Last hour maximum wind speed"),
			m_hoechste_windspitze_letzten_stunde, Unit_meters_second );
	}
};

/// 912ff -- Höchstes 10-Minuten-Mittel der Windgeschwindigkeit in der letzten Stunde (wird immer gemeldet!)
HEADTK(912ff) {
	int m_hoechste_10mn_wind_geschwindigkeit_letzten_stunde;
public:
	GENTK( 912ff, "912[0-9]{2}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "912%2d", &m_hoechste_10mn_wind_geschwindigkeit_letzten_stunde ) );
	}

	void Print() const {
		Append( _("Last hour ten minutes average wind speed"),
			m_hoechste_10mn_wind_geschwindigkeit_letzten_stunde, Unit_meters_second );
	}
};



/*
    * PIC INp -- Wolkenbedeckung von Bergen (Gruppe kann bis zu 2-mal verschlüsselt werden)
      I -- Kennziffer und Richtung des Berges
      Np -- Bedeckungsgrad des Berges mit Wolken in Achteln
    * BOT hesnTTT -- Temperaturen im Erdboden (werden immer gemeldet!)
      he -- Meßtiefe:

          0 -- 0 cm
          1 -- 5 cm
          2 -- 10 cm
          3 -- 20 cm
          4 -- 50 cm
          5 -- 100 cm
          6 -- 200 cm

      sn -- Vorzeichen der Erdbodentemperatur (0 = positiv, 1 = negativ)
      TTT -- Erdbodentemperatur der angegebenen Meßtiefe in 1/10 Grad Celsius

    * 80000 -- Kenngruppe für ergänzende klimatologische Meß- und Beobachtungsdaten für die Klimaroutinen

    * 1RRRRWR -- 6-stündiger Niederschlag

      Hinweis: Diese Gruppe wird zu den Hauptterminen (00, 06, 12, 18 UTC) gemeldet.

      RRRR -- Niederschlagsmenge in 1/10 mm   (0000 = trocken, 9999 = < 0.05 mm)
      WR -- Indikator zur Niederschlagsgruppe und Kennzeichnung der Niederschlagsform:

          0 -- kein Niederschlag
          1 -- nur abgesetzte Niederschläge (flüssig und fest)
          2 -- nur flüssige abgesetzte Niederschläge
          3 -- nur feste abgesetzte Niederschläge
          6 -- gefallener Niederschlag in flüssiger Form
          7 -- gefallener Niederschlag in fester Form
          8 -- gefallener Niederschlag in flüssiger und fester Form
          9 -- Niederschlagsmessung ausgefallen

    * 2SSSS -- Tagessumme der Sonnenscheindauer des Vortags in Minuten (wird um 06 UTC gemeldet)
    * 3fkfkfk< -- Höchste Windspitze des Vortags (00 - 24 UTC) in 1/10 m/s (wird um 06 UTC gemeldet)
    * 4fxkfxkfxk -- Höchstes 10-Minuten-Mittel der Windgeschwindigkeit des Vortags (00 - 24 UTC) in 1/10 m/s (wird um 06 UTC gemeldet)
    * 5RwRw -- Spezifisches Wasseräquivalent der Schneedecke in 1/10 mm/cm (wird nur montags, mittwochs und freitags um 06 UTC gemeldet, wenn die Schneehöhe mindestens 5 cm beträgt)
    * 6VAVAVBVBV CVC --
      Niederschläge und Wettererscheinungen des Vortags (werden um 06 UTC gemeldet)
      VAVA -- Gefallener Niederschlag des Vortags (00 - 24 UTC):

          000 -- kein gefallener Niederschlag
          001 -- Regen (Sprüh-/Nieselregen, Regen, Regentropfen)
          002 -- gefrierender Regen (gefrierender Sprüh-/Nieselregen, gefrierender Regen)
          004 -- Schnee (Schnee, Schneekristalle, Schneeflocken)
          008 -- Graupel (Schneegriesel, Reifgraupel, Frostgraupel, Eiskörner)
          016 -- Hagel

          Hinweis: Bei gleichzeitigem Auftreten mehrerer Niederschlagsarten werden die jeweiligen Schlüsselzahlen addiert, z. B.: Regen und Schnee und Graupel = 001 + 004 + 008 = 013

      VBVB -- Abgesetzter oder abgelagerter Niederschlag des Vortags (00 - 24 UTC):

          000 -- kein abgesetzter oder abgelagerter Niederschlag
          001 -- Tau (Strahlungstau, Advektionstau, weißer Tau)
          002 -- Reif (Strahlungsreif, Advektionsreif)
          004 -- Rauhreif/Rauhfrost
          008 -- Rauheis/Klareis
          016 -- Glatteis/Eisglätte
          032 -- Decke aus festen Niederschlägen (mindestens 50 % des Bodens bedeckend)

          Hinweis: Bei gleichzeitigem Auftreten mehrerer abgesetzter Niederschläge werden die jeweiligen Schlüsselzahlen addiert, z. B.: Tau und Reif sowie Schneedecke = 001 + 002 + 032 = 035

      VCVC -- Sonstige Wettererscheinungen des Vortags (00 - 24 UTC):

          000 -- keine sonstigen Wettererscheinungen
          001 -- Nebel (Nebel, Nebeltreiben)
          002 -- Gewitter (Nahgewitter, Ferngewitter)
          004 -- starker Wind (Windstärke 6 und 7 im 10-Min.-Mittel)
          008 -- stürmischer Wind (Windstärke > 8 im 10-Min.-Mittel)

          Hinweis: Bei gleichzeitigem Auftreten mehrerer Wettererscheinungen werden die jeweiligen Schlüsselzahlen addiert, z. B.: Gewitter und stürmischer Wind = 002 + 008 = 010

    * 7snTxkTxkTxk -- Maximumtemperatur des Vortags (00 - 24 UTC) (wird um 06 UTC gemeldet)
      sn -- Vorzeichen der Temperatur (0 = positiv, 1 = negativ)
      Txk Txk Txk -- Maximumtemperatur in 1/10 Grad Celsius
    * 8snTnkTnkTnk -- Minimumtemperatur des Vortags (00 - 24 UTC) (wird um 06 UTC gemeldet)
      sn -- Vorzeichen der Temperatur (0 = positiv, 1 = negativ)
      Tnk Tnk Tnk -- Minimumtemperatur in 1/10 Grad Celsius
    * 9snTgTgTgsTg -- Minimumtemperatur des Vortags (00 - 24 UTC) 5 cm über dem Erdboden bzw. der Schneedecke (wird um 06 UTC gemeldet)
      sn -- Vorzeichen der Temperatur (0 = positiv, 1 = negativ)
      Tg Tg Tg -- Erdbodenminimumtemperatur in 1/10 Grad Celsius
      sTg -- Bedeckung des Temperaturmeßfühlers 5 cm über dem Erdboden mit Schnee oder Eis am Vortag (0 = nein, 1 = ja, / = Angabe nicht möglich) 
*/

/// Abschnitt 6 - Automatisch erzeugte Daten
HEADTK(666) {
public:
	GENTK( 666, "666" )

	bool Parse( const char * str )
	{
		return 0 == strcmp( str, "666" );
	}

	void Print() const {
		Section( _("Automatisch erzeugte Daten") );
	}
};


/// 1snTxTxTx -- Maximumtemperatur der letzten Stunde
/// sn -- Vorzeichen der Temperatur (0 = positiv, 1 = negativ)
/// Tx Tx Tx -- Maximumtemperatur in 1/10 Grad Celsius
HEADTK(1snTxTxTx) {
	char m_temperature_sign ;
	int m_temperature;
public:
	GENTK( 1snTxTxTx, "1[01][0-9]{3}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 2 == sscanf( str, "1%c%3d", &m_temperature_sign, &m_temperature) )
		&&     CheckCelsius( m_temperature_sign, m_temperature );
	}

	void Print() const {
		AppCelsius( this, _("Last hour maximum temperature"), m_temperature_sign, m_temperature );
	}
};

/// 2snTnTnTn -- Minimumtemperatur der letzten Stunde
/// sn -- Vorzeichen der Temperatur (0 = positiv, 1 = negativ)
/// Tn Tn Tn -- Minimumtemperatur in 1/10 Grad Celsius
HEADTK(2snTxTxTx) {
	char m_temperature_sign ;
	int m_temperature;
public:
	GENTK( 2snTxTxTx, "2[01][0-9]{3}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 2 == sscanf( str, "2%c%3d", &m_temperature_sign, &m_temperature) )
		&&     CheckCelsius( m_temperature_sign, m_temperature );
	}

	void Print() const {
		AppCelsius( this, _("Last hour minimum temperature"), m_temperature_sign, m_temperature );
	}
};


/// 3snTnTnTn -- Minimumtemperatur 5 cm über dem Erdboden bzw. der Schneedecke in der letzten Stunde
/// sn -- Vorzeichen der Temperatur (0 = positiv, 1 = negativ)
/// Tn Tn Tn -- Erdbodenminimumtemperatur in 1/10 Grad Celsius

/// 6VMxVMxVMxVMx/ VMnVMnVMnVMn> -- automatisch gemessene maximale/minimale meteorologische Sichtweite (MOR) der letzten ganzen bzw. halben Stunde (nur bei Halbstundenterminen) in m

/// 7VMVMVMVM -- automatisch gemessene meteorologische Sichtweite (MOR) in m
HEADTK(7VVVV) {
	int m_MOR;
public:
	GENTK( 7VVVV, "7[0-9/]{4}[;=]?" )

	bool Parse( const char * str )
	{
		m_MOR = -1;
		return ( 1 == sscanf( str, "7%4d", &m_MOR ) )
		||     ( 0 == strncmp( str, "7////", 5 ) );
	}

	void Print() const {
		if(m_MOR >= 0)
			Append( _("MOR"), m_MOR, Unit_meters );
		else
			Append( _("MOR"), _("Undetermined") );
	}
};

/*
    * 80000 -- Kenngruppe für automatisch erzeugte Niederschlagsdaten

    * 0RRRrx 1RRRrx 2RRRrx 3RRRrx 4RRRrx 5RRRrx
      0, 1, 2, 3, 4, 5 (Gruppenkennziffer) -- Kennung der Zeitabschnitte der letzten Stunde:

          0 -- 0. bis 9. Minute
          1 -- 10. bis 19. Minute
          2 -- 20. bis 29. Minute
          3 -- 30. bis 39. Minute
          4 -- 40. bis 49. Minute
          5 -- 50. bis 59. Minute

      RRR -- 10-minütige Niederschlagshöhe in 1/10 mm

      rx -- Niederschlagsdauer im jeweiligen Zeitabschnitt in Minuten (0 = 10 Minuten bzw. kein Niederschlag gemessen)
*/

HEADTK(1VVff) {
	int m_visibility_metres;
	int m_gust_speed_knots;
public:
	GENTK( 1VVff, "1[0-9/][0-9/]{3}" )

	bool Parse( const char * str )
	{
		m_visibility_metres = 0;
		m_gust_speed_knots = 0;
		if( 0 == strcmp( str, "1////" ) ) return true ;

		if( str[1] == '/' ) {
			return ( 1 == sscanf( str, "1/%3d", &m_gust_speed_knots ) );
		} else {
			return ( 2 == sscanf( str, "1%2d%2d", &m_visibility_metres, &m_gust_speed_knots ) );
		}
	}

	void Print() const {
		Append( _("Visibility"), m_visibility_metres, Unit_meters );
		Append( _("Maximum gust speed in past 24 hours"), m_gust_speed_knots, Unit_knots );
	}
};

HEADTK(110ff) {
	int m_wind_speed_knots;
public:
	GENTK( 110ff, "110[0-9]{2}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "110%2d", &m_wind_speed_knots ) );
	}

	void Print() const {
		Append( _("Wind speed at 10 meters"), m_wind_speed_knots, Unit_knots );
	}
};

HEADTK(220ff) {
	int m_wind_speed_knots;
public:
	GENTK( 220ff, "220[0-9]{2}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 1 == sscanf( str, "220%2d", &m_wind_speed_knots ) );
	}

	void Print() const {
		Append( _("Wind speed at 20 meters"), m_wind_speed_knots, Unit_knots );
	}
};

HEADTK(3GGmm) {
	int m_end_measure_hour;
	int m_end_measure_minute;
public:
	GENTK( 3GGmm, "3[0-2][0-9][0-5][0-9][;=]?" )

	bool Parse( const char * str )
	{
		return ( 2 == sscanf( str, "3%2d%2d", &m_end_measure_hour, &m_end_measure_minute ) )
			&& ( m_end_measure_hour < 25 );
	}

	void Print() const {
		Append( _("Time of peak wind since last observation"), hour_min( m_end_measure_hour, m_end_measure_minute ) );
	}
};

HEADTK(4ddff) {
	int m_peak_direction_degrees;
	int m_peak_speed_knots;
public:
	GENTK( 4ddff, "4[0-9]{4}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 2 == sscanf( str, "4%3d%3d", &m_peak_direction_degrees, &m_peak_speed_knots ) );
	}

	void Print() const {
		Append( _("Direction"), 10.0 * m_peak_direction_degrees, Unit_degrees );
		Append( _("Speed"), m_peak_speed_knots, Unit_knots );
	}
};

/// End time in hours and minutes of the latest 10-minute continuous wind measurements.
HEADTK(6GGmm) {
	int m_end_measure_hour;
	int m_end_measure_minute;
public:
	/// More details: http://atmo.tamu.edu/class/atmo251/BuoyCode.pdf
	GENTK( 6GGmm, "6[0-2][0-9][0-5][0-9][;=]?" )

	bool Parse( const char * str )
	{
		return ( 2 == sscanf( str, "6%2d%2d", &m_end_measure_hour, &m_end_measure_minute ) )
			&& ( m_end_measure_hour < 25 );
	}

	void Print() const {
		Append( _("End time of latest 10-minute continuous wind measurement"), hour_min( m_end_measure_hour, m_end_measure_minute ) );
	}
};

/// 6 10-minute continuous wind measurements
HEADTK(dddfff) {
	int m_direction_degrees;
	int m_speed_knots;
public:
	GENTK( dddfff, "[0-9]{6}[;=]?" )

	bool Parse( const char * str )
	{
		return ( 2 == sscanf( str, "%3d%3d", &m_direction_degrees, &m_speed_knots ) );
	}

	void Print() const {
		Append( _("Direction"), m_direction_degrees, _("degrees") );
		Append( _("Speed"), m_speed_knots, _("knots") );
	}
};

// ----------------------------------------------------------------------------
/// Contains a Synop group and its length.
struct synop_group {
	size_t         m_usage_counter; // How many times did it match an input section. For debugging.
	section_t      m_section ;
	size_t         m_nb_toks ;
	const TOKGEN * m_tks ;
	synop_group( section_t g, size_t n, const TOKGEN * t )
	: m_usage_counter(0), m_section(g), m_nb_toks(n), m_tks(t) {}

	friend std::ostream & operator<<( std::ostream & ostrm, const synop_group & syn ) {
		for( size_t i =0 ; i < syn.m_nb_toks; ++i ) {
			ostrm << RegexT::Name( syn.m_tks[i].m_rgx_idx ) << "-";
		}
		return ostrm;
	}
};

/// Contains all patterns of Synop sections. We hold a buffer of the last words and try to match
/// it against all known patterns.
static std::vector< synop_group > arrSynopGroups ;

/// This defines a pattern which might mpatch a list of Synop tokens, i.e. a Synop section.
#define TKLST(g, ... ) \
{ \
	static const TOKGEN hiddenTokGen[] = { __VA_ARGS__ }; \
	arrSynopGroups.push_back( synop_group( g, G_N_ELEMENTS(hiddenTokGen), hiddenTokGen ) ); \
}

static const void init_patterns(void)
{
	static bool init = false ;
	if( init ) return ;
	init = true ;

	TKLST( SECTION_ZCZC_DLM,TK1(ZCZC),TK1(ZCZC_id) );
	TKLST( SECTION_NNNN_DLM,TK1(NNNN) );

	// http://www.nws.noaa.gov/tg/segment.html
	TKLST( SECTION_HEAD_GRP,TK1(TTAAii),TK1(CCCC),TK1(YYGGgg),TK1(RRx) );
	TKLST( SECTION_HEAD_GRP,TK1(TTAAii),TK1(CCCC),TK1(YYGGgg),TK1(CCx) );
	TKLST( SECTION_HEAD_GRP,TK1(TTAAii),TK1(CCCC),TK1(YYGGgg),TK1(AAx) );
	TKLST( SECTION_HEAD_GRP,TK1(TTAAii),TK1(CCCC),TK1(YYGGgg),TK1(Pxx) );

	TKLST( SECTION_IDENTLOC,TK1(IIiii),TK1(YYGGi),TK1(99LLL),TK1(QLLLL) );

	// http://atmo.tamu.edu/class/atmo251/LandSynopticCode.pdf
	// TODO: NOT SURE IIiii should be added at the end.
	TKLST( SECTION_IDENTLOC,TK1(AAXX),TK1(YYGGi) );

	// http://www.ominous-valve.com/wx_codes.txt

	// Can be followed by the callsign or the station id.
	TKLST( SECTION_IDENTLOC,TK1(BBXX),TK1(IIIII),TK1(YYGGi),TK1(99LLL),TK1(QLLLL) );

	// IIIII is the callsign.
	// OOXX D....D              YYGGiw 99LaLaLa QcLoLoLoLo MMMULaULo h0h0h0h0im
	TKLST( SECTION_IDENTLOC,TK1(OOXX),TK1(IIIII),TK1(YYGGi),TK1(99LLL),TK1(QLLLL),TK1(MMMULaULo),TK1(h0h0h0h0im) );

	// According to http://atmo.tamu.edu/class/atmo251/LandSynopticCode.pdf
	// - 00fff is optional.
	// - 2sTTT_dew can be replaced by 29UUU
	// - 4PPPP can be replaced by 4a3hhh
	// - 7WWWW can be replaced by 7wwWW
	// - 9GGgg is optional
	TKLST( SECTION_LAND_OBS,TK1(iihVV),TK1(Nddff),TK1(00fff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(4PPPP),TK1(5appp),TK1(6RRRt),TK1(7wwWW),TK1(8NCCC),TK1(9GGgg) );

	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(NIL) ); // 10

	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(4PPPP),TK1(5appp),TK1(6RRRt),TK1(7wwWW),TK1(8NCCC),TK1(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(4PPPP),TK1(5appp),TK1(6RRRt),TK1(8NCCC),TKn(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(4PPPP),TK1(5appp),TK1(7wwWW),TK1(8NCCC),TK1(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(4PPPP),TK1(5appp),TK1(7wwWW),TK1(9GGgg) ); // 61
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(4PPPP),TK1(5appp),TK1(6RRRt),TK1(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(4PPPP),TK1(5appp),TK1(8NCCC),TK1(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(4PPPP),TK1(5appp),TK1(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(4PPPP),TK1(6RRRt),TK1(7wwWW),TK1(8NCCC),TK1(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(4PPPP),TK1(7wwWW),TK1(8NCCC),TK1(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(4PPPP),TK1(8NCCC),TK1(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(5appp) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(3PPPP),TK1(9GGgg) );

	// http://allaboutweather.tripod.com/synopcode.htm
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(4PPPP),TK1(5appp),TK1(6RRRt),TK1(7wwWW),TK1(8NCCC) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(4PPPP),TK1(5appp),TK1(6RRRt),TK1(8NCCC) );

	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(4PPPP),TK1(5appp),TK1(7wwWW),TK1(8NCCC),TK1(9GGgg) );

	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(4PPPP),TK1(5appp),TK1(8NCCC) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(4PPPP),TK1(5appp),TK1(9GGgg) );
	// 01078 11470 30310 11045 21077 5//// 69941 70182 82360 333 91114=
	// A: Publish: Error, no coordinates. kmlNam= descrTxt= m_nb_tokens=5:Land observations=6RRRt#69941+7wwWW#70182+8NCCC#82360+;Climatological data=333#333+9SSss#91114+;


	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(4PPPP),TK1(7wwWW),TK1(8NCCC) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(4PPPP),TK1(8NCCC) );

	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(4PPPP),TK1(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(5appp),TK1(6RRRt),TK1(7wwWW),TK1(8NCCC) ); // 21
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(6RRRt),TK1(7wwWW),TK1(8NCCC) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(2sTTT_dew),TK1(6RRRt),TK1(8NCCC) );

	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(3PPPP),TK1(4PPPP),TK1(5appp),TK1(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(4PPPP),TK1(5appp),TK1(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(1sTTT_air),TK1(9GGgg) );
	TKLST( SECTION_LAND_OBS,TK1(IIiii),TK1(iihVV),TK1(Nddff),TK1(4PPPP),TK1(5appp),TK1(9GGgg) );


	// According to http://atmo.tamu.edu/class/atmo251/LandSynopticCode.pdf
	// - 0sTTT is optional.
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(0sTTT),TK1(1PPHH),TK1(2PPHH),TK1(3dddd),TK1(4PPHH),TK1(5PPHH),TK1(6IEER),TK1(70HHH),TK1(8aTTT) );
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(0sTTT),TK1(1PPHH),TK1(2PPHH),TK1(3dddd),TK1(4PPHH),TK1(5PPHH),TK1(6IEER),TK1(8aTTT),TK1(ICE),TK1(cSbDz) );
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(0sTTT),TK1(1PPHH),TK1(2PPHH),TK1(3dddd),TK1(4PPHH),TK1(70HHH),TK1(8aTTT) );
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(0sTTT),TK1(1PPHH),TK1(70HHH),TK1(8aTTT) );
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(0sTTT),TK1(2PPHH),TK1(3dddd),TK1(4PPHH),TK1(5PPHH),TK1(6IEER),TK1(8aTTT),TK1(ICE),TK1(cSbDz) );
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(0sTTT),TK1(2PPHH),TK1(3dddd),TK1(4PPHH),TK1(5PPHH),TK1(8aTTT),TK1(ICE),TK1(cSbDz) );

	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(0sTTT),TK1(2PPHH),TK1(3dddd),TK1(4PPHH),TK1(8aTTT),TK1(ICE),TK1(cSbDz) );

	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(0sTTT),TK1(2PPHH),TK1(3dddd),TK1(8aTTT) );
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(0sTTT),TK1(2PPHH),TK1(70HHH) );
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(0sTTT),TK1(2PPHH),TK1(8aTTT) );
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(0sTTT),TK1(3dddd),TK1(4PPHH) ); // 66

	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(0sTTT),TK1(8aTTT),TK1(ICE),TK1(cSbDz) );
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(1PPHH),TK1(2PPHH),TK1(3dddd),TK1(4PPHH),TK1(5PPHH),TK1(6IEER),TK1(70HHH),TK1(8aTTT) );
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(1PPHH),TK1(70HHH) );
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(2PPHH),TK1(3dddd),TK1(4PPHH),TK1(8aTTT) );
	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(2PPHH),TK1(70HHH) );

	TKLST( SECTION_SEA_SURF,TK1(222Dv),TK1(ICE),TK1(cSbDz) );

	// http://allaboutweather.tripod.com/synopcode.htm
	// 333 	1snTxTxTx (at 1800 UTC) 2snTnTnTn (at 0600 UTC)  3EsnTgTg (at 0600 UTC)  4E'sss (at 0600 UTC) 8NsChshs  9SpSpspsp
	// - SECTION 5
	// 555 	1V'f'/V'f''f'' 	2snTwTwTw 	iiirrr

	/// BEWARE: jjjjj COULD BE REPEATED ! Or only with 55jjj ??
	// According to http://atmo.tamu.edu/class/atmo251/LandSynopticCode.pdf
	// - 0____ is optional.
	// - jjjjj is optional.
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(0____),TKn(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(0____),TK1(1sTTT_max),TK1(2sTTT_min),TK1(3Ejjj),TK1(4Esss),TK1(55jjj),TK1(jjjjj),TK1(6RRRtb),TK1(7RRRR),TK1(8NChh),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(0____),TK1(1sTTT_max),TK1(2sTTT_min),TK1(3Ejjj),TK1(4Esss),TK1(5jjjj),TK1(6RRRtb),TK1(7RRRR),TK1(8NChh),TK1(9SSss) );

	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(0____),TK1(55jjj),TK1(jjjjj),TK1(8NChh),TK1(9SSss) );

	// http://www.top-wetter.de/themen/synopschluessel.htm
	// 333 0.... 1sTTT 2sTTT 3EsTT 4E'sss 55SSS 2FFFF 3FFFF 4FFFF 553SS 2FFFF 3FFFF 4FFFF 6RRRt 7RRRR 8NChh 9SSss
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(3Ejjj),TK1(553SS),TK1(2FFFF),TK1(3FFFF),TK1(4FFFF),TKn(8NChh) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(3Ejjj),TK1(553SS),TK1(2FFFF),TK1(3FFFF),TK1(6RRRt),TKn(8NChh) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(3Ejjj),TK1(553SS),TK1(2FFFF),TK1(7RRRR),TKn(9SSss) );

	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(3Ejjj),TK1(55jjj),TK1(jjjjj),TK1(6RRRtb),TK1(7RRRR),TK1(8NChh),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(3Ejjj),TK1(55jjj),TK1(jjjjj),TK1(7RRRR),TK1(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(3Ejjj),TK1(6RRRtb),TK1(7RRRR),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(3Ejjj),TK1(6RRRtb),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(4Esss),TK1(7RRRR) ); // 51
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(553SS),TK1(2FFFF),TK1(3FFFF),TK1(6RRRt),TK1(8NChh) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(553SS),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(6RRRtb),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(55jjj),TK1(jjjjj),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(6RRRtb),TK1(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(7RRRR),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(2sTTT_min),TK1(8NChh) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(3Ejjj),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(553SS),TK1(2FFFF),TK1(8NChh) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(55jjj),TK1(jjjjj),TK1(8NChh),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TKn(5jjjj),TKn(8NChh),TK1(9SSss) ); // 61
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TKn(5jjjj),TK1(6RRRtb),TKn(8NChh),TK1(9SSss) ); // 63
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(6RRRtb) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TKn(8NChh),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(1sTTT_max),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(2sTTT_min),TK1(3Ejjj),TK1(4Esss),TK1(5jjjj),TK1(8NChh),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(2sTTT_min),TK1(3Ejjj),TK1(55jjj),TK1(jjjjj),TK1(7RRRR) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(2sTTT_min),TK1(3Ejjj),TK1(5jjjj),TK1(55jjj),TK1(jjjjj),TK1(7RRRR),TK1(8NChh),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(2sTTT_min),TK1(3Ejjj),TK1(5jjjj),TK1(6RRRtb),TK1(7RRRR),TK1(8NChh) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(2sTTT_min),TK1(4Esss),TK1(7RRRR),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(2sTTT_min),TK1(5jjjj),TK1(8NChh),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(2sTTT_min),TK1(55jjj),TK1(jjjjj),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(2sTTT_min),TK1(553SS),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(2sTTT_min),TK1(8NChh),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(2sTTT_min),TK1(9SSss) );

	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(3Ejjj),TK1(4Esss),TK1(55jjj),TK1(jjjjj),TK1(3FFFF),TK1(553SS),TK1(2FFFF),TK1(3FFFF),TK1(6RRRt) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(3Ejjj),TK1(4Esss),TK1(553SS),TK1(6RRRt),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(3Ejjj),TK1(553SS),TK1(6RRRtb),TKn(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(3Ejjj),TK1(553SS),TKn(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(3Ejjj),TK1(55jjj),TK1(jjjjj),TK1(3FFFF),TK1(553SS),TK1(2FFFF),TK1(3FFFF),TK1(6RRRt),TKn(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(3Ejjj),TK1(55jjj),TK1(jjjjj),TK1(553SS),TK1(2FFFF),TK1(6RRRt),TKn(8NChh),TKn(9SSss) );

	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(3Ejjj),TK1(55jjj),TK1(jjjjj),TK1(7RRRR),TK1(8NChh),TK1(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(3Ejjj),TK1(55jjj),TK1(jjjjj),TK1(553SS),TK1(6RRRtb),TKn(8NChh),TKn(9SSss) );

	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(4Esss),TK1(553SS),TK1(6RRRtb),TKn(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(4Esss),TK1(55jjj),TK1(jjjjj),TK1(553SS),TK1(6RRRtb),TKn(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(4Esss),TK1(6RRRtb),TKn(9SSss) );

	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(553SS),TK1(2FFFF),TK1(3FFFF),TK1(6RRRt),TKn(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(553SS),TK1(2FFFF),TK1(3FFFF),TKn(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(553SS),TK1(2FFFF),TK1(6RRRt),TKn(8NChh) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(553SS),TK1(2FFFF),TKn(8NChh) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(553SS),TKn(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(553SS),TKn(9SSss) );

	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(5jjjj),TK1(6RRRtb) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TKn(5jjjj),TKn(8NChh),TKn(9SSss) );

	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(55jjj),TK1(jjjjj),TK1(2sTTT_min),TK1(3Ejjj),TK1(6RRRtb),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(55jjj),TK1(jjjjj),TK1(3Ejjj),TK1(6RRRtb),TKn(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(55jjj),TK1(jjjjj),TK1(3Ejjj),TKn(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(55jjj),TK1(jjjjj),TKn(8NChh),TKn(9SSss) );

	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(6RRRtb),TKn(8NChh),TKn(9SSss) );
	TKLST( SECTION_CLIM_DAT,TK1(333),TK1(6RRRtb),TKn(9SSss) );

	TKLST( SECTION_CLIM_DAT,TK1(333),TKn(8NChh),TKn(9SSss) );

	TKLST( SECTION_CLIM_DAT,TK1(333),TKn(9SSss) );



	// http://www.top-wetter.de/themen/synopschluessel.htm#444
	TKLST( SECTION_NATCLOUD,TK1(444),TK1(NCHHC) );


	// NOT SURE AT ALL.
	// http://allaboutweather.tripod.com/synopcode.htm
	TKLST( SECTION_NAT_CODE,TK1(555),TK1(1VVff),TK1(2sTTT_avg),TK1(3Ejjj),TK1(4Esss),TKn(5jjjj),TK1(8NChh),TK1(9SSss) );
	TKLST( SECTION_NAT_CODE,TK1(555),TK1(1VVff),TK1(2sTTT_avg),TK1(3Ejjj),TKn(5jjjj),TK1(8NChh),TK1(9SSss) );
	TKLST( SECTION_NAT_CODE,TK1(555),TK1(1VVff),TKn(8NChh),TK1(9SSss) );
	TKLST( SECTION_NAT_CODE,TK1(555),TK1(2sTTT_avg),TK1(5jjjj),TK1(8NChh),TK1(9SSss) );
	TKLST( SECTION_NAT_CODE,TK1(555),TK1(3Ejjj),TK1(4Esss),TK1(55jjj),TK1(jjjjj),TK1(8NChh),TK1(9SSss) );
	TKLST( SECTION_NAT_CODE,TK1(555),TK1(3Ejjj),TKn(8NChh),TK1(9SSss) );
	TKLST( SECTION_NAT_CODE,TK1(555),TK1(9SSss) );

	// http://metaf2xml.sourceforge.net/parser.pm.html
	// AT: 1snTxTxTx 6RRR/
	//    1snTxTxTx : maximum temperature on the previous day from 06:00 to 18:00 UTC 
	//    6RRR/ : amount of precipitation on the previous day from 06:00 to 18:00 UTC 
 	// BE: 1snTxTxTx 2snTnTnTn
	//    1snTxTxTx : maximum temperature on the next day from 00:00 to 24:00 UTC 
	//    2snTnTnTn : minimum temperature on the next day from 00:00 to 24:00 UTC 
 	// CA: 1ssss 2swswswsw 3dmdmfmfm 4fhftftfi
	//    1ssss : amount of snowfall, in tenths of a centimeter, for the 24-hour period ending at 06:00 UTC 
	//    2swswswsw : amount of water equivalent, in tenths of a millimeter, for the 24-hour snowfall ending at 06:00 UTC 
	//    3dmdmfmfm : maximum (mean or gust) wind speed, in knots, for the 24-hour period ending at 06:00 UTC and its direction 
	//    4fhftftfi : together with the previous group, the hundreds digit of the maximum wind speed (in knots),
	//                the time of occurrence of the maximum wind speed, and the speed range
	//                of the maximum two-minute mean wind speed, for the 24-hour period ending at 06:00 UTC and its direction 
 	// US land: RECORD* 0ittDtDtD 1snTT snTxTxsnTnTn RECORD* 2R24R24R24R24 44snTwTw 9YYGG
	//    RECORD : indicator for temperature record(s) 
	//    0ittDtDtD : tide data 
	//    1snTT snTxTxsnTnTn RECORD* 2R24R24R24R24 : city data: temperature, maximum and minimum temperature,
	//                indicator for temperature record(s), precipitation last 24 hours 
	//    44snTwTw : water temperature 
	//    9YYGG : additional day and hour of observation (repeated from Section 0) 
 	// US sea: 11fff 22fff 3GGgg 4ddfmfm 6GGgg dddfff dddfff dddfff dddfff dddfff dddfff 8ddfmfm 9GGgg
	//    11fff 22fff : equivalent wind speeds at 10 and 20 meters 
	//    3GGgg 4ddfmfm : maximum wind speed since the last observation and the time when it occurred 
	//    6GGgg : end time of the latest 10-minute continuous wind measurements 
	//    6 x dddfff : 6 10-minute continuous wind measurements 
	//    8ddfmfm 9GGgg : highest 1-minute wind speed and the time when it occurred 
 	// CZ: 1dsdsfsfs 2fsmfsmfsxfsx 3UU// 5snT5T5T5 6snT10T10T10 7snT20T20T20 8snT50T50T50 9snT100T100T100
	//    1dsdsfsfs : wind direction and speed from tower measurement 
	//    2fsmfsmfsxfsx : maximum wind gust speed over 10 minute period and the period W1W2 
	//    3UU// : relative humidity 
	//    5snT5T5T5 6snT10T10T10 7snT20T20T20 8snT50T50T50 9snT100T100T100 : soil temperature at the depths of 5, 10, 20, 50, and 100 cm 
 	// RU: 1EsnT'gT'g 2snTnTnTn 3EsnTgTg 4E'sss 52snT2T2 6RRRtR 7R24R24R24/ 88R24R24R24
	//    1EsnT'gT'g : state of the ground without snow or measurable ice cover, temperature of the ground surface 
	//    2snTnTnTn : minimum temperature last night 
	//    3EsnTgTg : state of the ground without snow or measurable ice cover, minimum temperature of the ground surface last night 
	//    4E'sss : state of the ground if covered with snow or ice, snow depth 
	//    6RRRtR : amount of precipitation for given period 
	//    7R24R24R24/ : amount of daily precipitation 
	//    88R24R24R24 : amount of daily precipitation if 30 mm or more


	// http://atmo.tamu.edu/class/atmo251/BuoyCode.pdf
	// 6GGgg is also called 6GGmm
	TKLST( SECTION_NAT_CODE,TK1(555),TK1(6GGmm) ); // Or 6snT10T10T10
	/// Applies to CMAN messages but apparently to others too.
	// 3GGgg 4ddfmfm: maximum wind speed since the last observation and the time when it occurred
	TKLST( SECTION_NAT_CODE,TK1(555),TK1(110ff),TK1(220ff),TK1(3GGmm),TK1(4ddff),TK1(6GGmm),TKn(dddfff) );


	// http://www.top-wetter.de/themen/synopschluessel.htm

#ifdef FULL_SYNOP_555
	TKLST( SECTION_NAT_CODE,TK1(555),TK1(0sTTT_land),TK1(1RRRr),TK1(2sTTT_avg),TK1(22fff),TK1(23SS),TK1(24Wt),TKn(25ww),TK1(26fff),TK1(3LLLL),TK1(5ssst),TK1(7hhZD),TK1(8N_hh),TK1(910ff),TK1(911ff),TK1(912ff),TK1(PIC),TK1(IN),TK1(BOT),TK1(hsTTT) );
#else
	TKLST( SECTION_NAT_CODE,TK1(555),TK1(0sTTT_land),TK1(1RRRr),TK1(2sTTT_avg),TK1(911ff),TK1(912ff) );
#endif

	// TODO: Consider optional tokens.
	// manfred@met.fu-berlin.de  Joerg Wichmann

	// http://www.top-wetter.de/themen/synopschluessel.htm
	// 666 1sTTT 2sTTT 3sTTT 6VVVV/VVVV 7VVVV
	TKLST( SECTION_AUTO_DAT,TK1(666),TK1(1snTxTxTx),TK1(2snTxTxTx),TK1(7VVVV) );

	// http://www.top-wetter.de/themen/synopschluessel.htm
	// 80000 0RRRr 1RRRr 2RRRr 3RRRr 4RRRr 5RRRr
};
#undef TK

/// Actual implementation of the synop objet. Details are hidden.
class synop_impl
: public synop
{
	/// All characters which are not flushed yet.
	std::string m_buf ;

	/// Words being read (Between spaces, tabs etc...)
	std::string m_current_word ;

	/// When disabled, chars pass through the object without change.
	bool m_enabled ;

	/// A chain of Synop tokens, which tries to match a group.
	class Chain {
		size_t                  m_idxGroup ; // Index of the Synop group.
		size_t                  m_idxTok ;   // Index of the first Synop token in the group.
		size_t                  m_nxtTok ;   // Index of the next Synop token in the group.
		priority_t              m_sum_prios ;
		TokenProxy::TokVec::Ptr m_tokens ;

		Chain();

		void PushElement(TokenProxy::Ptr tp) {
			if( ! m_tokens ) m_tokens = TokenProxy::TokVec::Ptr( new TokenProxy::TokVec );
			PushItself( tp, m_tokens );
		}
	public:
		/// TODO: Allocates m_tokens only if appending a first token is possible.
		Chain( int i, int j)
		: m_idxGroup(i), m_idxTok(j), m_nxtTok(j), m_sum_prios(0.0) {}

		const TokenProxy::TokVec::Ptr & Tokens() const {
			assert( m_tokens );
			return m_tokens;
		}

		section_t section(void) const {
			return arrSynopGroups[m_idxGroup].m_section;
		}

		/// Beware that some elements might have been repeated.
		bool IsFinished(void) const {
			// TODO: Our synop_group element should be copied here.
			const synop_group * ptrSynopGroup = &arrSynopGroups[m_idxGroup];
			if( m_nxtTok == m_idxTok ) return false ;
			size_t nbToks = ptrSynopGroup->m_nb_toks ;
			if( m_nxtTok > nbToks ) return true ;
			if( m_nxtTok < nbToks ) return false ;
			return ! ptrSynopGroup->m_tks[ m_nxtTok - 1 ].m_many;
		}

		/// Tries to add a new word at the end of a chain of tokens.
		TokenProxy::Ptr TryPush( RegexT::Context & rgxCtxt, const std::string & str, size_t txt_offset )
		{
			if( IsFinished() ) return TokenProxy::Ptr();

			const synop_group * ptrSynopGroup = &arrSynopGroups[m_idxGroup];
			assert( m_nxtTok <= ptrSynopGroup->m_nb_toks );

			if( m_nxtTok < ptrSynopGroup->m_nb_toks ) {
				const int reg_idx = ptrSynopGroup->m_tks[ m_nxtTok ].m_rgx_idx;
				if( rgxCtxt.Mtch( reg_idx, str ) ) {
					TokenProxy::Ptr tp = RegexT::CreateTokenProxy( reg_idx, str, txt_offset );

					/// Maybe a token which must not be at the beginning of a sequence.
					if( ( m_nxtTok == m_idxTok ) && ( false == tp->CanComeFirst() ) ) {
						return TokenProxy::Ptr() ;
					}

					/// It must parse the current word and the priority must be high enough.
					if( tp->ParseItself() ) {
						PushElement( tp );
						m_sum_prios += RegexT::Priority( reg_idx );
						++m_nxtTok ;
						/*
						std::cout << "DEBUG:" << __FUNCTION__
							<< " m_idxGroup=" <<  m_idxGroup << " m_idxTok=" << m_idxTok
							<< " reg=" << reg.Name() << " str=" << str
							<< " m_nxtTok=" << m_nxtTok 
							<< " priority=" << m_sum_prios << "\n" ;
						*/
						return tp ;
					}
				}
			}

			/// Maybe the previous pattern can be repeated if "m_many" flag ?
			if( m_nxtTok > m_idxTok ) {
				const TOKGEN & tokgen = ptrSynopGroup->m_tks[ m_nxtTok - 1 ];
				int reg_idx = tokgen.m_rgx_idx;
				if( tokgen.m_many && rgxCtxt.Mtch( reg_idx, str ) )
				{
					TokenProxy::Ptr tp = RegexT::CreateTokenProxy( reg_idx, str, txt_offset );
					/// It must parse the current word and the priority must be high enough.
					if( tp->ParseItself() ) {
						PushElement( tp );
						m_sum_prios += RegexT::Priority( reg_idx );
						/*
						std::cout << "DEBUG:" << __FUNCTION__
							<< " REPET m_idxGroup=" <<  m_idxGroup << " m_idxTok=" << m_idxTok
							<< " reg=" << reg.Name() << " str=" << str
							<< " m_nxtTok=" << m_nxtTok 
							<< " m_idxTok=" << m_idxTok 
							<< " maxTks=" << ptrSynopGroup->m_nb_toks
							<< " priority=" << m_sum_prios << "\n" ;
						*/
						return tp ;
					}
				}
			}

			/*
			 * MAYBE THE TOKEN IS OPTIONAL. IF YES, RETRY WITH NEXT TOKEN.
			*/

			// Now if the match is not empty, we might tolerate one dummy string.
			if( m_nxtTok > m_idxTok + 2 ) {
			}

			// If the previous match is a dummy string, maybe this is because two tokens coalesced,
			// therefore we try the next token.

			return TokenProxy::Ptr();
		} // Chain::TryPush

		// TODO: This is now simply the chain length, but we could refine the concept.
		priority_t sum_priorities(void) const { return m_sum_prios;}

		size_t text_begin() const {
			assert( ! m_tokens->empty() );
			return m_tokens->front()->offset_begin();
		}
		size_t text_end() const {
			assert( ! m_tokens->empty() );
			return m_tokens->back()->offset_end();
		}

		std::string test_display() const {
			return m_tokens->MiniDump(section(),true);
		}

		void display_chain( bool test_mode, bool kml_mode ) const {
			m_tokens->DumpTokVec( test_mode, section(), kml_mode );
		}

		// This helps to check if a pattern was used or not.
		void increment_usage_counter(void) const {
			arrSynopGroups[m_idxGroup].m_usage_counter++;
		}

		typedef std::list< Chain > List ;

		// Adds our current object at the end of a list. Fast copy thanks to smart pointers.
		void ConcatToList( List & refList ) const {
			refList.push_back( *this );
			List::iterator lastChain = refList.end();
			--lastChain ;
			if( lastChain != refList.begin() ) {
				List::iterator prevChain = lastChain ;
				--prevChain ;
				lastChain->m_tokens->previous( prevChain->m_tokens );
			}
		}
	}; // synop_impl::Chain

	Chain::List m_chains ;

	/// This represents the last successful chains which can be aggregated into a message.
	class Message : public Chain::List
	{
		// Debug purpose.
		std::string TstToStr(void) const {
			std::string res ;
			for( const_iterator it = begin(), en = end(); it != en; ++ it ) {
				res += SectionToString(it->section());
				res +="=" + it->test_display() + ";";
			}
			return res ;
		}

		/// Looks for a specific token in the tokens chain passed as an iterator.
		template< class TokClass >
		void SetIfNull( const TokClass * & refIter, const_iterator itChain ) const {
			const TokClass * tmpPtr = itChain->Tokens()->front()->template get_ptr< TokClass >(false);
			if( tmpPtr == NULL ) return ;
			if( refIter == NULL ) {
				refIter = tmpPtr ;
				return ;
			}
			/// In some circumstances, we might duplicate the header to split the message.
			LOG_WARN("Duplicate token=%s:%s, was:%s: m_nbTokens=%d:%s",
				typeid(TokClass).name(), tmpPtr->c_str(), refIter->c_str(),
				(int)m_nbTokens, TstToStr().c_str() );
			refIter = tmpPtr ; // Take the closest (and last).
		}

		// Total number of tokens in the list of token chains.
		size_t m_nbTokens;

		/// Called when the end of a message is detected.
		void Publish(void)
		{
			const_iterator it0 = begin();

			if(it0 == end() ) {
				LOG_DEBUG("No publish0 empty message");
				return;
			}
	
			// Quick check if the message is very short. Beware, it it the total
			// number of tokens, not the number of sections.
			if(m_nbTokens <= 2 ) {
				LOG_DEBUG("No publish1 m_nbTokens=%d:%s",
					static_cast<int>(m_nbTokens), TstToStr().c_str() );
				return ;
			}

			// We eliminate this kind of message which is not SYNOP although the beginning is similar.
			// Other simple combinations might be eliminated but they are rarer.
			// ZCZC 603     
			// WWXX60 EDZW 201700
			const_iterator it1 = it0;
			++it1 ;

			if( ( it0->section() == SECTION_HEAD_GRP )
			&&  ( it1            != end()            )
			&&  ( it1->section() == SECTION_IDENTLOC ) ) {
				if( m_nbTokens <= 3 ) {
					LOG_INFO("No publish3 %s", TstToStr().c_str() );
					return ;
				}
				// TODO: Store these for next run if their are missing.
			}

			if( it1 == end() ) {
				// For example, receiving only the following line makes no sense:
				// Climatological data=6RRRt#69907+8NChh#81822+9SSss#91113+9SSss#96480;+;
				if( it0->section() != SECTION_LAND_OBS ) {
					LOG_INFO("No publish2 %s", TstToStr().c_str() );
					return ;
				}
				// TODO: We should use the header SECTION_IDENTLOC of the previous message:
				// SMOS01 LOWM 190000
				// AAXX 19001
				// 11036 32565 73208 10000 21038 30065 40306 57008 8353/ 333 83629
				// 86360 91013 91113 91209=
				// 11010 35561 /2504 11031 21043 39946 40345 57009=
				// 11120 36/17 /9901 11111 21112 39620 40386 57005=
			}

			/// This gets some crucial information from the tokens.
			const CLASSTK(QLLLL)     * ptr_QLLLL     = NULL;
			const CLASSTK(IIIII)     * ptr_IIIII     = NULL;
			const CLASSTK(IIiii)     * ptr_IIiii     = NULL;
			const CLASSTK(iihVV)     * ptr_iihVV     = NULL;
			const CLASSTK(MMMULaULo) * ptr_MMMULaULo = NULL;
			const CLASSTK(YYGGi)     * ptr_YYGGi     = NULL;
			const CLASSTK(YYGGgg)    * ptr_YYGGgg    = NULL;
			const CLASSTK(Numbered)  * ptr_Numbered  = NULL;

			// In all the chains of tokens , we try to grap some specific tokens.
			for ( const_iterator itChain = begin(), enChain = end(); itChain != enChain; ++itChain ) {
				assert( itChain->Tokens() );
				SetIfNull< CLASSTK(QLLLL)     >( ptr_QLLLL    , itChain );
				SetIfNull< CLASSTK(IIIII)     >( ptr_IIIII    , itChain );
				SetIfNull< CLASSTK(IIiii)     >( ptr_IIiii    , itChain );
				SetIfNull< CLASSTK(iihVV)     >( ptr_iihVV    , itChain );
				SetIfNull< CLASSTK(MMMULaULo) >( ptr_MMMULaULo, itChain );
				SetIfNull< CLASSTK(YYGGi)     >( ptr_YYGGi    , itChain );
				SetIfNull< CLASSTK(YYGGgg)    >( ptr_YYGGgg   , itChain );
				SetIfNull< CLASSTK(Numbered)  >( ptr_Numbered , itChain );
			}

			bool foundCoo = false ;

			CoordinateT::Pair newCoo ;
			double altitudeStation = 0.0;
			if( ptr_QLLLL ) {
				if(ptr_QLLLL->CoordinatesOK() ) {
					newCoo = CoordinateT::Pair( ptr_QLLLL->Longitude(), ptr_QLLLL->Latitude() );
					foundCoo = true ;
				}
			}

			std::string kmlNam ;
			std::string iconNam ;
			std::string descrTxt ;
			std::string stationCountry ;

			bool foundIdentifier = false;
			/// It also indicates whether we could find the station name given the WMO indicator.
			if( ptr_IIiii ) {
				int wmoIndicInt = ptr_IIiii->WmoIndicator();
				std::stringstream strmWmo ;
				strmWmo << std::setfill('0') << std::setw(5) << wmoIndicInt;
				std::string wmoIndicStr = strmWmo.str();
				descrTxt = "WMO " + wmoIndicStr ;

				const RecordBuoy * ptrBuoy_Tok = CatalogBuoy::FindFromKey( wmoIndicStr );
				const RecordWmoStation * ptrWmo_Tok = CatalogWmoStations::FindFromKey( wmoIndicInt );

				/// We cannot rely on "isAutomated" to guess if it is a buoy or not.
				if( ptrWmo_Tok == NULL ) {
					if( ptrBuoy_Tok ) {
						foundIdentifier = true;
						CoordinateT::Pair tmpCoo = ptrBuoy_Tok->station_coordinates();
						if(foundCoo) {
							double dist = tmpCoo.distance( newCoo );
							if( dist > 100 ) {
								std::stringstream strm ;
								strm << "Coordinates accuracy issue with buoy: "
									<< ptrBuoy_Tok->buoy_name()
									<< " IIiii:" << newCoo
									<< " Against:" << tmpCoo
									<< " Dist:" << dist ;
								LOG_INFO("%s", strm.str().c_str() );
							}
						} else {
							foundCoo = true ;
						}
						// In both cases, we take the coordinates given by the WMO file.
						newCoo = tmpCoo ;
						altitudeStation = 0;
						kmlNam = ptrBuoy_Tok->title();
						iconNam = ptrBuoy_Tok->type();
					} else {
						const RecordJComm * ptrJComm_Tok = CatalogJComm::FindFromKey( wmoIndicStr );
						if( ptrJComm_Tok ) {
							ptrJComm_Tok->SetJCommFields( kmlNam, iconNam );
							if( stationCountry.empty() ) stationCountry = ptrJComm_Tok->country();
						} else {
							kmlNam = "Automated station:" + wmoIndicStr ;
							iconNam = "automated";
						}
					}
				}

				if( ptrBuoy_Tok == NULL ) {
					iconNam = "wmo";
					if( ptrWmo_Tok ) {
						foundIdentifier = true;
						CoordinateT::Pair tmpCoo = ptrWmo_Tok->station_coordinates();
						if( stationCountry.empty() ) stationCountry = ptrWmo_Tok->country();
						if(foundCoo) {
							double dist = tmpCoo.distance( newCoo );
							if( dist > 100 ) {
								std::stringstream strm ;
								strm << "Coordinates accuracy issue with WMO station: "
									<< ptrWmo_Tok->station_name()
									<< " IIiii:" << newCoo
									<< " Against:" << tmpCoo
									<< " Dist:" << dist ;
								LOG_INFO("%s", strm.str().c_str() );
							}
						} else {
							foundCoo = true ;
						}
						// In both cases, we take the coordinates given by the WMO file.
						newCoo = tmpCoo ;
						altitudeStation = ptrWmo_Tok->station_elevation();
						kmlNam = ptrWmo_Tok->station_name();
					} else {
						const RecordJComm * ptrJComm_Tok = CatalogJComm::FindFromKey( wmoIndicStr );
						if( ptrJComm_Tok ) {
							ptrJComm_Tok->SetJCommFields( kmlNam, iconNam );
							if( stationCountry.empty() ) stationCountry = ptrJComm_Tok->country();
						} else {
							LOG_INFO("Cannot find WMO station:%s", wmoIndicStr.c_str() );
							kmlNam = "WMO:" + wmoIndicStr ;
						}
					}
				}
				if( ( ptrWmo_Tok != NULL ) && ( ptrBuoy_Tok != NULL ) ) {
					LOG_WARN("Conflit buoy/WMO");
				}
			}

			std::string stationCallsign ;
			if( ptr_IIIII ) {
				std::string buoyNam ;
				const char * shipIdIIIII = ptr_IIIII->ShipIdentifier();

				if( ! descrTxt.empty() ) descrTxt += ",";
				descrTxt += _("Ship ");
				descrTxt += shipIdIIIII;

				const RecordShip * ptrShip_Tok = CatalogShip::FindFromKey( shipIdIIIII );
				if( ptrShip_Tok ) {
					stationCallsign = buoyNam = ptrShip_Tok->callsign();
					if( ! ptrShip_Tok->name().empty() )
						buoyNam += "," + ptrShip_Tok->name();
					if( iconNam.empty() ) iconNam = "ship"; else iconNam += " ship";
					if( stationCountry.empty() ) stationCountry = ptrShip_Tok->country();
				} else {
					const RecordBuoy * ptrBuoy_Tok = CatalogBuoy::FindFromKey( shipIdIIIII );

					if( ptrBuoy_Tok ) {
						buoyNam = ptrBuoy_Tok->title();
						if( iconNam.empty() ) iconNam = "other buoy"; else iconNam += " buoy";
					}
				}

				if( buoyNam.empty() ) {
					if( kmlNam.empty() ) {
						kmlNam = std::string("Ship:") + shipIdIIIII;
						if( iconNam.empty() ) iconNam = "ship"; else iconNam += " ship";
					} else {
						LOG_WARN("Conflict between station %s and ship/buoy identifier %s",
							kmlNam.c_str(), shipIdIIIII );
					}
				} else {
					if( kmlNam.empty() ) {
						kmlNam = buoyNam;
					} else {
						// Maybe the WMO station was there.
						if( foundIdentifier ) {
							LOG_WARN("Conflict between station %s and ship/buoy callsign %s identifier %s",
								kmlNam.c_str(), buoyNam.c_str(), shipIdIIIII );
						} else {
							// We concatenate the information. Maybe this is not a conflict after all.
							if( foundIdentifier )
								kmlNam += ",Ship:" + buoyNam ;
							else
								kmlNam = buoyNam ;
						}
					}
				}
			}

			if( ptr_MMMULaULo && ptr_MMMULaULo->MarsdenValid() ) {
				CoordinateT::Pair tmpCoo( ptr_MMMULaULo->Longitude(), ptr_MMMULaULo->Latitude() );
				if(foundCoo) {
					double dist = tmpCoo.distance( newCoo );
					if( dist > 100 ) {
						std::stringstream strm ;
						strm << "Coordinates accuracy issue with Marsden square. "
							<< " Coordinates:"<< newCoo
							<< " against:" << tmpCoo
							<< " Dist:" << dist;
						LOG_WARN("%s", strm.str().c_str() );
					}
				} else {
					foundCoo = true ;
				}
				newCoo = tmpCoo ;
			}

			// TODO: Adding time as fourth dimension:
			// http://earth.google.com/outreach/tutorial_time.html

			// TODO: Shame that we are losing this message because no coordinates.
			if( false == foundCoo ) {
				std::stringstream strm ;
				strm << "Error, no coordinates.";
				strm << " kmlNam=" << kmlNam ;
				strm << " descrTxt=" << descrTxt ,
				strm << " m_nb_tokens=" << m_nbTokens << ":";
				strm << TstToStr();
				LOG_WARN("%s", strm.str().c_str() );
				return ;
			}

			// If no time is defined, set current time.
			time_t tmObservation = 0 ;
			if( ptr_YYGGi ) {
				tmObservation = ptr_YYGGi->ObservationTimeUTC();
				if( ptr_YYGGgg ) {
					time_t obsTmGGgg = ptr_YYGGgg->ObservationTimeUTC();
					int days = diffTm( tmObservation, obsTmGGgg );
					if(days > 1) {
						LOG_WARN( _("Unreliable observation time: %s and %s"),
							Tm2SynopTime( tmObservation ).c_str(),
							Tm2SynopTime( obsTmGGgg ).c_str() );
					}
				}
			} else if( ptr_YYGGgg ) {
				tmObservation = ptr_YYGGgg->ObservationTimeUTC();
			}

			// The name must be unique.
			if( kmlNam.empty() ) {
				std::stringstream strm ;
				strm << _("Station") << " " << newCoo;
				kmlNam = strm.str();
			}
			if( iconNam.empty() ) iconNam = "Weather Station";
			if( descrTxt.empty() ) descrTxt = _("Undetermined station");

			if( synop::ptr_callback->log_adif() )
			{
				// This builds an ADIF message.
				struct AdifSerializer : public Serializer, public std::string {
					void StartSection( const std::string & section_name ) {
						static_cast<std::string &>(*this) += section_name + ADIF_EOL;
					}
					// Will add a new line to a text message in the ADIF record.
					void AddItem( const char * key, const char * value, const char * unit ) {
						std::string & refStr = *this ;
						refStr += key ;
						refStr += "=";
						refStr += value ;
						if(unit) {
							refStr += " " ;
							refStr += unit ;
						}
						refStr += ADIF_EOL;
					}
				}; // synop_impl::Message::KmlSerializer

				AdifSerializer adifSerial ;

				/// Concatenate information from each chain. This is where the serializer kmlSerial is called.
				for ( const_iterator itChain = begin(), enChain = end(); itChain != enChain; ++itChain ) {
					itChain->display_chain(false, true);
				}

				/// For updating the logbook with received messages.
				QsoHelper qso(MODE_RTTY) ;

				if( ! stationCallsign.empty() )
					qso.Push(CALL, stationCallsign );
				if( ! stationCountry.empty() )
					qso.Push(COUNTRY, stationCountry );
				{
					std::stringstream strm ;
					strm << newCoo ;
					qso.Push(QTH, strm.str() );
				}
				qso.Push(GRIDSQUARE, newCoo.locator() );
				qso.Push(NAME, kmlNam );
				/// If the header is clean, the message type is removed from the string.
				// In this context, this field cannot be used.
				qso.Push(XCHG1, iconNam );

				// AAx, RRx, CCx, Pxx
				if( ptr_Numbered ) {
					qso.Push(SRX, ptr_Numbered->Number() );
				}

				// Sequence of Chars and line-breaks, ASCII CR (code 13) + ASCII LF (code 10)
				qso.Push(NOTES, adifSerial );
			}

			if( synop::ptr_callback->log_kml() )
			{
				/// Writes messages to a list of key-value pairs, later displayed in KML.
				struct KmlSerializer : public Serializer, public KmlServer::CustomDataT {
					// TODO: Doubles the line width of the HTML table.
					void StartSection( const std::string & section_name ) {
						// m_freeText << section_name << "\n";
					}
					// Will add a new line to a HTML table.
					void AddItem( const char * key, const char * value, const char * unit ) {
						std::string val = unit ? value + std::string(" ") + unit : std::string(value);
						Push( key, val );
					}
				}; // synop_impl::Message::KmlSerializer

				KmlSerializer kmlSerial ;

				/// Concatenate information from each chain. This is where the serializer kmlSerial is called.
				for ( const_iterator itChain = begin(), enChain = end(); itChain != enChain; ++itChain ) {
					itChain->display_chain(false, true);
				}

				// Beware: Some WMO stations have the same name but are not mobile.
				KmlServer::GetInstance()->Broadcast(
					"Synop",
					tmObservation,
					newCoo,
					altitudeStation,
					kmlNam,
					iconNam,
					descrTxt,
					kmlSerial );
			}
		} // synop_impl::Message::Publish

	public:
		~Message() {
			Publish();
		}

		/// Inconditionnaly cleans the content because too many chars could not be read.
		void MsgFlush() {
			// Should be called also after a given timeout.
			Publish();
			Chain::List::clear();
			m_nbTokens = 0;
		}

		/// When getting rid of the current message.
		// We might keep it or flush everything etc...
		void MsgFlushAndMove( const Chain & refChain ) {
			// Depending on the section of the last chain of the message,
			// and the section from this brand new chain, this decides to aggregate
			// the new section at the end of the message, otherwise create a new one,
			// and broadcast the current message.
			if( ! empty() ) {
				section_t new_sec = refChain.section();
				section_t last_sec = Chain::List::back().section();

				// To build messages, checks if a section can follow another one.
				if( 0 == sectionTransitions[new_sec][last_sec] ) {
					MsgFlush();
				}
			}
			refChain.ConcatToList( *this );
			m_nbTokens += refChain.Tokens()->size();
		}
	}; // synop_impl::Message

	/// We add new chains of tokens, that is, sections, at the end of the message.
	Message     m_current_message ;

public:

	/// If margin is positive, this is the number of chars to be kept in the buffer for further decoding,
	/// because we are not sure at the moment of what they will be used for.
	/// If this is negative, this is the number of characters immediately after the decoded chain,
	/// which should be discarded: This is an end of section, '=' character.
	void decode_then_flush(int margin = 0 )
	{
		// The margin is here only for display purpose. If the chars are immediately
		// displayed, there should not be anything in the buffer.
		size_t len_to_keep = margin > 0 ? margin : 0 ;

		Chain::List::const_iterator beg = m_chains.begin(), en = m_chains.end();

		// Maybe we did not manage to decode anything.
		if( beg == en ) {
			// it might also be empty because we are in interleaved mode.
			if( m_buf.size() >= len_to_keep ) {
				disp_range( m_buf.begin(), m_buf.end() - len_to_keep );
				m_buf.erase(m_buf.begin(), m_buf.end() - len_to_keep);
			}
			return ;
		}

		Chain::List::const_iterator best = beg ;
		priority_t best_priority = best->sum_priorities() ;

		// TODO: We should count the solutions of the same priority (More or less the length),
		// with a different beginning as ours.
		for( Chain::List::const_iterator it = beg; ++it != en ; ) {
			assert( it->Tokens() );
			if( it->sum_priorities() > best_priority ) {
				best_priority = it->sum_priorities() ;
				best = it ;
			}
		};
		assert( best != en );

		m_current_message.MsgFlushAndMove(*best);

		// Maybe no chain is worth of interest.
		bool decoded_ok = best_priority >= MIN_PRIO ;

		// Counts the number of times this chain is selected.
		if( decoded_ok ) {
			best->increment_usage_counter();
		}

		// If interleaved mode, chars are immediately printed.
		if( m_buf.empty() ) {
			if( decoded_ok ) {
				// This just displays the single chain in the output buffer.
				best->display_chain( GetTestMode(), false );
			}
		}
		else {
			assert( len_to_keep <= m_buf.size() );
			if( decoded_ok ) {
				// Not sure why we remove the last char.
				size_t txt_beg = best->text_begin();
				size_t txt_end = best->text_end();

				// There might have been a synchronization problem when changing params.
				if( (txt_beg > 0) && ( txt_beg - 1 <= m_buf.size() ) ) {
					disp_range( m_buf.begin(), m_buf.begin() + txt_beg - 1 );
				} else {
					LOG_WARN("Bugcheck1: txt_beg=%d txt_end=%d margin=%d m_buf=%s",
							(int)txt_beg, (int)txt_end, (int)margin, m_buf.c_str() );
				}

				// This just displays the single chain in the output buffer.
				best->display_chain( GetTestMode(), false );

				if( (txt_beg > 0) && ( txt_beg - 1 + len_to_keep <= m_buf.size() ) ) {
					int to_discard = (margin < 0 ) ? - margin : 0 ;
					disp_range( m_buf.begin() + txt_end - 1 + to_discard , m_buf.end() - len_to_keep );
				} else {
					LOG_WARN("Bugcheck2: txt_beg=%d txt_end=%d margin=%d m_buf=%s",
							(int)txt_beg, (int)txt_end, (int)margin, m_buf.c_str() );
				}
			} else {
				disp_range( m_buf.begin(), m_buf.end() - len_to_keep );
			}
			m_buf.erase(m_buf.begin(), m_buf.end() - len_to_keep);
		}

		m_chains.clear();
	} // synop_impl::decode_then_flush
private:
	// TODO: When starting a section, we should take first tokens of the same priority, in order to finish.

	/// Adds a new current word. tries all possible chains to which this word matches one token, even in the middle.
	size_t AddTokInit()
	{
		RegexT::Context rgxCtxt;

		// std::cout << __FUNCTION__ << "\n";
		size_t nbInserts = 0 ;

		assert( m_chains.empty() );

		const size_t buf_sz = m_buf.size();

		// The same list can appear in several chains, this is intended.
		for( int i = 0, nbSynGrp = arrSynopGroups.size(); i < nbSynGrp; ++i )	
		{
			for( size_t j = 0, nbToks = arrSynopGroups[i].m_nb_toks; j < nbToks; ++j )
			{
				Chain tmpChain( i, j );

				if( tmpChain.TryPush( rgxCtxt, m_current_word, buf_sz ) )
				{
					m_chains.push_back( tmpChain );
					++nbInserts;
				}
			}
		}
		// std::cout << "DEBUG:" << __FUNCTION__ << ":" << m_current_word << ":at:" << m_buf.size() << " m_buf=" << m_buf << " nbIns=" << nbInserts << " nbChains=" << m_chains.size() << "\n";
		return nbInserts;
	} // synop_impl::AddTokInit

	/// Tries to add the last word to all possible token chains.
	size_t AddOtherTok()
	{
		class CtxtDerived : public RegexT::Context {
			struct BestStartTokenT {
				priority_t m_minPrio ; // Must be negative at startup.
				size_t     m_newReg ;
			public:
				BestStartTokenT() : m_minPrio(-1.0), m_newReg(~0) {}
				void TstSwapPrio( priority_t newPrio, size_t newReg ) {
					if( m_minPrio < newPrio ) {
						m_minPrio = newPrio;
						m_newReg = newReg;
					}
				}
			};

			/// This contains for each section the regex which matches the best
			// the current work, and starting a chain whose section immediately follow ours.
			BestStartTokenT m_bstStartToken[ SECTION_SECT_NBR ];

			section_t m_currSection ;
		public:
			CtxtDerived() : m_currSection( static_cast<section_t>(-1) ) {}

			void CmpSwapPriority( size_t newReg, section_t newSection ) {
				priority_t newPrio = RegexT::Priority( newReg );

				/// This iterates on all sections which might immediately precede this one.
				for( size_t precedSec = 0; precedSec < SECTION_SECT_NBR; ++precedSec ) {
					// Does it make sense to have one section type following another ?
					if( 0 == sectionTransitions[newSection][precedSec] ) continue ;
					m_bstStartToken[ precedSec ].TstSwapPrio( newPrio, newReg );
				}
			}

			/// Does the section comes just before ours ?
			void SetDistance( section_t predSection ) { m_currSection = predSection ; }

			// Called in two contexts:
			// When looking for a better beginning regex.
			// Or as a virtual in TryPush.
			virtual bool Mtch( size_t reg_idx, const std::string & str ) {
				if( m_bstStartToken[ m_currSection ].m_minPrio > RegexT::Priority( reg_idx ) )
				{
					/*
 					* We could add a condition on the length because of this:
					Expect [IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+
					Actual [IIIII+YYGGi+99LLL+QLLLL+ 41/96 222Dv+1PPHH+2PPHH+ 4PPPP+5appp+7wwWW+
 					*/
					/*
					std::cout << " prio=" << m_bstStartToken[ m_currSection ].m_minPrio << " str=" << str
						<< " m_newReg:" << m_bstStartToken[ m_currSection ].m_newReg
						<< " prioRgx=" << RegexT::Name(m_bstStartToken[ m_currSection ].m_newReg)
						<< " currSec=" << SectionToString(m_currSection)
						<< " against:" << RegexT::Name(reg_idx)
						<< " reg_idx:" << reg_idx
						<< " oldPrio:" << RegexT::Priority( reg_idx )
						<< "\n";
					*/
					return false ;
				}
				return RegexT::Context::Mtch( reg_idx, str );
			};
		}; // CtxtDerived

		CtxtDerived rgxCtxtDerived;

		const size_t buf_sz = m_buf.size();

		/// What happens if it matches the beginning if a chain
		// NOTE: We suppose that the beginning is never multiple ("Many").
		for( int idxGrp = 0, nbSynGrp = arrSynopGroups.size(); idxGrp < nbSynGrp; ++idxGrp )	
		{
			const synop_group * ptrSynopGroup = &arrSynopGroups[idxGrp];
			assert( ptrSynopGroup->m_nb_toks > 0 );
			assert( ptrSynopGroup->m_tks[ 0 ].m_many == false );
			const int reg_idx = ptrSynopGroup->m_tks[ 0 ].m_rgx_idx;
			if( rgxCtxtDerived.Mtch(reg_idx, m_current_word ) ) {
				TokenProxy::Ptr tp = RegexT::CreateTokenProxy( reg_idx, m_current_word, buf_sz );

				if( tp->ParseItself() ) {
					rgxCtxtDerived.CmpSwapPriority( reg_idx, ptrSynopGroup->m_section );
				}
			}
		}

		/// Contains the list of chains to which we could not add the current (and last) word.
		typedef std::list< Chain::List::iterator > ChainsNoInsrtsT ;
		ChainsNoInsrtsT chainsNoInsrts ;

		/// We try to add the new word to all potential solutions.
		size_t nbInserts = 0 ;
		for( Chain::List::iterator it = m_chains.begin(), en = m_chains.end(); it!= en; ++it )
		{
			/*
			Beware: Maybe the current word matches an intermediate token which is also the beginning of another chain.
			In this case, it does not count.
			It should not be a problem because the priorities are identical.
			*/
			rgxCtxtDerived.SetDistance( it->section() );
			if( it->TryPush( rgxCtxtDerived, m_current_word, buf_sz ) )
			{
				++nbInserts;
			} else {
				chainsNoInsrts.push_back( it );
			}
		}

		if( nbInserts != 0 ) {
			/// Removes the chains which cannot possibly match the current suite of tokens.
			for( ChainsNoInsrtsT::iterator it = chainsNoInsrts.begin(), en = chainsNoInsrts.end(); it != en; ++it ) 
			{
				m_chains.erase( *it );
			}
		}

		return nbInserts;
	} // synop_impl::AddOtherTok

public:
	synop_impl()
	{
		/// Display each decoded key/value/unit item, to a stream, for debug logging.
		struct SynopSerializer : public Serializer
		{
			void StartSection( const std::string & section_name ) {
				std::string str = section_name + "\n";
				disp_range( str.begin(), str.end(), true );
			}

			/// Print in bold chars or special color.
			void AddItem( const char * key, const char * value, const char * unit ) {
				std::stringstream strm ;
				strm << '\t' << key << '=' << value << ' ' << ( unit ? unit : "" ) << '\n';
				std::string str = strm.str();
				disp_range( str.begin(), str.end(), true );
			}
		}; // synop_impl::SynopSerializer

		/// It registers automatically as the lowest level serializer.
		static SynopSerializer fldigiSerial ;

		init_patterns();

		m_enabled = true ;
	}

	virtual ~synop_impl() {};

	/// Called as a virtual.
	void init() {
		cleanup();
	}

	void cleanup() {
		m_buf.clear();
		m_current_word.clear();
		m_chains.clear();
		m_enabled = false ;
	}

	/// Adds a received character to the Synop decoding buffer.
	void add( char c) {
		if( ! m_enabled ) {
			cleanup();
			m_enabled = true ;
		}

		static bool was_interleaved_before = false ;

		bool is_interleaved_now = synop::ptr_callback->interleaved();
		// I
		if( is_interleaved_now )
		{
			// Maybe has to display and flush the internal buffer.
			if( ! was_interleaved_before ) {
				// Of course it does not clears the current message.
				disp_range( m_buf.begin(), m_buf.begin() );
				m_buf.clear();
			}
		}
		was_interleaved_before = is_interleaved_now;

		bool noChains = m_chains.empty();

		// No chance to terminate the current message, too many chars not parsed.
		if( noChains && ( m_buf.size() > 20 ) ) {
			m_current_message.MsgFlush();
		}

		/// All delimiters are stored in the buffer, but the split loses them.

		if( ! is_interleaved_now ) {
			m_buf += c ;
		} else {
			std::string one_char( 1, c );
			disp_range( one_char.begin(), one_char.end(), false );
		}

		// Note that some chars are not part of Baudot (ITA2) charset and will never appear.
		switch(c) {
			case ';' : // Our RTTY decoder uses this for '='.
				c = '=' ;
			// TODO: Might as well remove '=' from all regular expressions.
			case '=' :
			case ' ' :
			case '\t' :
			// TODO: Frequently (But not always, this marks a section end).
			case '\n' :
			case '\r' :
				if( ! m_current_word.empty() ) {
					size_t nbInserts = 0 ;

					// The same list can appear in several chains, this is intended.
					if( noChains )
					{
						nbInserts = AddTokInit();
					} else {
						nbInserts = AddOtherTok();
					}

					if( c == '=' )
					{
						// -1, no display of '=', whose length is one.
						decode_then_flush(-1);
					} else if( nbInserts == 0 ) 
					{
						if( noChains ) {
							/// There was no chains before.
							decode_then_flush();
						} else {
							// If there was no insertion, because
							// the new word did not match any existing chain.
							// Then we will retry with this word
							// and update the buffer too.
							decode_then_flush( m_current_word.size() + 1 );

							// If cannot match anything at first stage,
							// then no need to keep the buffer.
							if( 0 == AddTokInit() ) {
								decode_then_flush();
							}
							// Otherwise we have started a new chain.
						}
					}

					m_current_word.clear();
				}
				break ;
			default   :
				if( m_current_word.size() > 10 )
				{
					decode_then_flush();
					m_current_word.clear();
				}
				else
				{
					m_current_word += c ;
				}
				break ;
		}
	}

	/// When Synop decoding is not needed.
	void flush(bool finish_decoding) {
		if( finish_decoding ) {
			decode_then_flush();
			flush(false);
		} else {
			disp_range(m_buf.begin(), m_buf.end() );
			m_current_message.MsgFlush();
			cleanup();
		}
	}

	bool enabled(void) const { return m_enabled; }
};

synop * synop::instance() {
	/// The destructor should not do any harm because we have no control on when it is called.
	static synop_impl g_synop ;

	return &g_synop ;
};

/// Prints the number of times each regular expression is used, so the unused ones can be removed.
void synop::regex_usage(void) {
	for( size_t idxGrp = 0; idxGrp < arrSynopGroups.size(); ++idxGrp )
	{
		const synop_group * ptrSynopGroup = &arrSynopGroups[idxGrp];
		if( ptrSynopGroup-> m_usage_counter > 0 ) continue ;

		std::cout << "DEBUG:" << "Unused:"
			<< " idxGrp=" <<  idxGrp 
			<< ' ' << *ptrSynopGroup
			<< '\n' ;
	}
}

/// This helps in debug mode: Only the regex name is displayed.
bool synop::m_test_mode = false ;

const synop_callback * synop::ptr_callback ;

/* Useful links about decoding:
http://www.kmlvalidator.com/home.htm
http://www.ndbc.noaa.gov/station_page.php?station=MBLA1
http://www.ncdc.noaa.gov/homr/
http://www.ncdc.noaa.gov/oa/climate/surfaceinventories.html
http://www.nodc.noaa.gov/BUOY/
http://www.nodc.noaa.gov/BUOY/all_buoy_info_latlon.txt
http://www.sailwx.info/shiptrack/researchships.phtml
http://www.meteo2.shom.fr/qctools/last-report-list_surfmar.htm
http://weather.noaa.gov/pub/logs/shipstats/shipstat.out.201206301330.csv
http://www.wmo.int/pages/prog/amp/mmop/buoy-ids.html
http://www.aoml.noaa.gov/hrd/format/tempdrop_format.html
http://www.ominous-valve.com/wx_codes.txt
http://www.vos.noaa.gov/ObsHB-508/ObservingHandbook1_2010_508_compliant.pdf
http://www.nws.noaa.gov/tg/head.html
http://weather.unisys.com/noaaport/text_summary.php
http://www.wmo.int/pages/prog/www/ois/Operational_Information/VolumeC1/CCCC_en.html
// http://www.wmo.int/pages/prog%2Fwww/WMOCodes/Manual/Volume-I-selection/Sel2.pdf

Other information:
See Klingenfuss
Thunder (corresponding in the SYNOP weather code to group 7, WW=17).
*/
