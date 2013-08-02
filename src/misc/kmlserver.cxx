// ----------------------------------------------------------------------------
// kmlserver.cxx  --  KML Server
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <sys/stat.h>

#include <string>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <list>
#include <stdexcept>
#include <fstream>
#include <sstream>

#include "config.h"
#include "kmlserver.h"
#include "gettext.h"
#include "debug.h"
#include "threads.h"
#include "strutil.h"
#include "configuration.h"

#include "irrXML.h"

#include "timeops.h"

/** Some platforms have problems with condition variables apparently.
 * When cancelling a thread which waits in pthread_cond_timedwait,
 * the thread is stuck.
 * We replace it by an unconditional wait, and a test on a boolean
 * which indicates if data was saved in the internal buffers.
 * The consequence is that data are not immediately saved in KML files,
 * and the user has to wait until the end of the delay.
 */

#if !defined(__APPLE__)
#	define FLDIGI_KML_CONDITION_VARIABLE 1
#endif

// ----------------------------------------------------------------------------

static const char * KmlSrvUnique = "Permanent";

/// This must follow a specific ISO format so it can be serialized in KML files.
static void KmlTimestamp( std::ostream &ostrm, time_t tim ) {
	if(tim == KmlServer::UniqueEvent) {
		ostrm << KmlSrvUnique;
		return;
	}
	tm objTm = *gmtime(&tim);
	char bufTm[24];
	// See http://www.w3.org/TR/xmlschema-2/#isoformats
	sprintf( bufTm, "%4d-%02d-%02dT%02d:%02dZ",
			objTm.tm_year + 1900,
			objTm.tm_mon + 1,
			objTm.tm_mday,
			objTm.tm_hour,
			objTm.tm_min );
	ostrm << bufTm ;
}

/// For debugging purpose.
static std::string KmlTimestamp( time_t tim ) {
	std::stringstream strm ;
	KmlTimestamp( strm, tim );
	return strm.str();
}

/// Deserialize a timestamp, inverse of KmlTimestamp.
static time_t KmlFromTimestamp( const char * ts ) {
	if(ts == NULL ) throw std::runtime_error("Null timestamp");

	if( 0 == strcmp( ts, KmlSrvUnique ) ) return KmlServer::UniqueEvent ;

	/// So all fields are initialised with correct default values.
	time_t timNow = time(NULL);
	tm objTm = *gmtime( &timNow ); 

	int r = sscanf( ts, "%4d-%02d-%02dT%02d:%02dZ",
			&objTm.tm_year,
			&objTm.tm_mon,
			&objTm.tm_mday,
			&objTm.tm_hour,
			&objTm.tm_min );
	if( r != 5 ) throw std::runtime_error("Cannot read timestamp from " + std::string(ts) );
	objTm.tm_year -= 1900;
	objTm.tm_mon -= 1;
	objTm.tm_sec = 0;

	time_t res = mktime( &objTm );
	if( res < 0 ) throw std::runtime_error("Cannot make timestamp from " + std::string(ts) );
	return res;
}

// ----------------------------------------------------------------------------

/// Some chars are forbidden in HTML documents. This replaces them by HTML entities.
// We do not need to create a temporary copy of the transformed string.
// See Html entities here: http://www.w3schools.com/tags/ref_entities.asp
static void StripHtmlTags( std::ostream & ostrm, const char * beg, bool newLines = false )
{
	const char * ptr = NULL;
	// TODO: Consider &nbsp; &cent; &pound; &yen; &euro; &sect; &copy; &reg; &trade;
	for( const char * it = beg ; ; ++it )
	{
		/** Other characters are filtered:
		*  U+0009, U+000A, U+000D: these are the only C0 controls accepted in XML 1.0;
		*  U+0020–U+D7FF, U+E000–U+FFFD: */
		char ch = *it ;
		switch( ch )
		{
			case 0x01 ... 0x08 :
			// case 0x09  :
			// case 0x0A  :
			case 0x0B ... 0x0C :
			// case 0x0D  :
			case 0x0E ... 0x0F :
				     ptr = " "; break;
			case '"'   : ptr = "&quot;"; break;
			case '\''  : ptr = "&apos;"; break;
			case '&'   : ptr = "&amp;" ; break;
			case '<'   : ptr = "&lt;"  ; break;
			case '>'   : ptr = "&gt;"  ; break;
			case '\0'  : break ;
			case '\n'  : if(newLines) // Should we replace new lines by "<BR>" ?
						{
							ptr = "<BR>" ;
							break;
						}
				     // Otherwise we print the newline char like the other chars.
			default    : continue ;
		}
		if( it != beg ) {
			ostrm.write( beg, it - beg );
		}

		if( ch == '\0' ) break ;
		assert(ptr);
		ostrm << ptr ;
		beg = it + 1 ;
	}
}

/// Some values such as Navtex messages may contain newline chars.
static void StripHtmlTagsNl( std::ostream & ostrm, const std::string & beg ) {
	StripHtmlTags( ostrm, beg.c_str(), true );
}

static void StripHtmlTags( std::ostream & ostrm, const std::string & beg ) {
	StripHtmlTags( ostrm, beg.c_str() );
}

// ----------------------------------------------------------------------------

/// Also used when reloading a KML file.
void KmlServer::CustomDataT::Push( const char * k, const std::string & v ) {
	push_back( value_type( k, v ) );
}

// ----------------------------------------------------------------------------

/// Different sorts of KML datas. This list is hardcoded but they are processed identically.
static const char * categories[] = {
	"User",
	"Synop",
	"Navtex"
};
static const size_t nb_categories = sizeof(categories) / sizeof(*categories);

/// Written at the beginning of each KML document.
static void KmlHeader( std::ostream & ostrm, const std::string & title ) {
	ostrm <<
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<kml xmlns=\"http://earth.google.com/kml/2.1\">\n"
		"<Document>\n"
		"<name>" << title << "</name>\n" ;
}

/// Appended at the end of each KML document.
static const char KmlFooter[] = 
	"</Document>\n"
	"</kml>\n" ;

/// Contains for example GIF images, and all the styles. Can be customised by the user.
static const std::string namStyles = "styles.kml";

/** Used to code the data for reloading. The description tag is too complicated
*  to parse and its serialization might not be reversible.
*  We use preprocessor constants instead of char arrays so they can be concatenated at compile-time.
*  The tags must be as short as possible but the values easy to deserialize.
*  */
#define FLDIGI_TAG_ITM   "fldigi:itm"
#define FLDIGI_TAG_EVT   "fldigi:evt"
#define FLDIGI_TAG_KEY   "k"
#define FLDIGI_TAG_VAL   "v"
#define FLDIGI_TIMESTAMP "ts"

/// Global singleton of all KML-related things.
class  KmlSrvImpl : public KmlServer {
	volatile bool m_loaded ;          /// Set when ready for operation.
	std::string   m_kml_dir;          /// Where kml files are saved.
	std::string   m_command;          /// Started each time KML files are saved.
	int           m_pid_command;      /// Process id of the running commend for KML files.
	double        m_merge_dist;       /// Below this, placemark with same name are merged.
	int           m_retention_delay;  /// Purge old data.
	int           m_refresh_interval; /// In seconds, written in KML files.
	int           m_balloon_style;    /// Display style in KML balloons: Lines, matrices, plain text.
	pthread_t     m_writer_thread ;   /// Periodically woken up to write things to the KML file.

	/// Models a KML placemark. It contains events indexed my a timestamp.
	/// We need an ordered container in order to remove old data in one erase.
	class PlacemarkT : public std::multimap< time_t, CustomDataT > {
		CoordinateT::Pair m_coord;
		double            m_altitude;
		std::string       m_styleNam; // The icon.
		std::string       m_kmlId ;   // Unique KML id for the placemark.
		std::string       m_descrTxt ;// KML snippet.

		/// Serialize the internal data to XML so they can be easily read.
		void SerializeForReading( std::ostream & ostrm ) const {

			/// Custom data elements for reading the content when restarting.
			ostrm << "<ExtendedData xmlns:fldigi=\"http://www.w1hkj.com\">\n";

			/// Print from the most recent event, which is at the end.
			for( const_iterator itEvt = begin(), enEvt = end(); itEvt != enEvt; ++itEvt )
			{
				ostrm << "<" FLDIGI_TAG_EVT " " FLDIGI_TIMESTAMP "=\"";
				KmlTimestamp(ostrm,itEvt->first);
				ostrm << "\">\n" ;
				const CustomDataT & refCust = itEvt->second;
				for( CustomDataT::const_iterator it = refCust.begin(), en = refCust.end(); it != en; ++it )
				{
					ostrm << "<" FLDIGI_TAG_ITM " " FLDIGI_TAG_KEY "=\"" << it->first
						<< "\" " FLDIGI_TAG_VAL "=\"";
					StripHtmlTags(ostrm,it->second);
					ostrm << "\" />"; 
				}
				ostrm << "</" FLDIGI_TAG_EVT ">\n" ;
			}
			/// Dumps all events in natural order
			ostrm << "</ExtendedData>\n";
		}
	public:
		/// Constructor called by "Broadcast", not from reading a KML file.
		PlacemarkT(
			const CoordinateT::Pair & refCoo,
			double                    altitude,
			const std::string       & styleNam,
			const std::string       & kmlNam )
		: m_coord( refCoo )
		, m_altitude( altitude )
		, m_styleNam( styleNam ) {
			/// The unique key is indeed the name and the time,
			/// because an object such as a ship might move and come back to the same place.
			/// We add a counter because during tests, the timestamps are too close.
			static int dummyCnt = 0 ;
			std::stringstream strm ;
			/// No need to store the kml name because it is the multimap key.
			StripHtmlTags(strm,kmlNam);
			strm << ':' << Tm2Time() << ':' << ++dummyCnt ;
			m_kmlId = strm.str();
		}

		/// Constructor for deserialization. Strings comes from the KML file.
		PlacemarkT() : m_altitude(0.0) {}

		void Clear() {
			m_styleNam.clear();
			m_kmlId.clear();
			m_descrTxt.clear();
			clear();
		}

		/// Used when reading a KML file. Read coordinates and altitude from a string when reloading a KML file.
		void SetCoordinates( const char * str ) {
			double lon, lat ;
			if( str == NULL ) {
				throw std::runtime_error("Null coordinates text");
			}
			int r = sscanf( str, "%lf,%lf,%lf", &lon, &lat, &m_altitude );
			if( r != 3 ) {
				static const std::string msg("Cannot read coordinates and altitude:");
				throw std::runtime_error(msg+str);
			}
			m_coord = CoordinateT::Pair( lon, lat );
		}

		/// Used when reading a KML file. HTML entities are already removed. "+1" is for the "#".
		void SetStyle( const char * str ) {
			if(str == NULL ) throw std::runtime_error("Null input style");
			size_t sz = namStyles.size();
			// If the strings are equal, then strlen(str) >= sz, and str[sz] == '\0' if equality.
			if( ( 0 == strncmp( str, namStyles.c_str(), sz ) ) && ( str[sz] == '#' ) ) {
				m_styleNam = str + sz + 1;
			} else {
				LOG_INFO("Inconsistent URL style:%s",str );
				m_styleNam = str ;
			}
		}

		/// Used when reading a KML file.
		void SetKmlId( const char * str ) {
			std::stringstream strm ;
			// The KML deserializer irrXML transforms the HTML entities
			// into normal chars. We must do the reverse transformation.
			StripHtmlTags(strm,str);
			m_kmlId = strm.str();
		}

		/// Just add the events without suppressing duplicate information.
		void AppendEvent(
			time_t              evtTim,
			const std::string & descrTxt,
			const CustomDataT & custDat )
		{
			if( m_descrTxt.empty() )
				m_descrTxt = descrTxt ;
			else
				if( ! descrTxt.empty() )
				{
					// We want to ensure that it is not a miscommunication.
					if( NULL == strstr( m_descrTxt.c_str(), descrTxt.c_str() ) ) {
						m_descrTxt += "," + descrTxt ;
					}
				}

			/// Default time is now. evtTim might have another special value.
			if(evtTim == 0) {
				evtTim = time(NULL);
			}
			insert( value_type( evtTim, custDat ) );
		}

		const CoordinateT::Pair & coordinates() const { return m_coord;}
		double altitude(void) const { return m_altitude;}
		const std::string & style(void) const { return m_styleNam; }
		const std::string & KmlId(void) const { return m_kmlId; }

		/// When writing the style to the KML file.
		void styleToKml(std::ostream & ostrm) const {
			ostrm << namStyles << '#';
			StripHtmlTags(ostrm,m_styleNam);
		}

		/// Used when several PlacemarkT with the same kmlNam but different styles. Keep the first only.
		void style(const std::string & styl) { m_styleNam = styl; }

		/// This is NOT the Euclidian distance but tries to reflect a position change on a 3D map.
		double distance_to( const PlacemarkT & refOther ) const {
			double delta_altitude_km = fabs( m_altitude - refOther.m_altitude ) * 0.001 ;
			// Big coefficient for the altitude, to show on the map what happens.
			double horiz_dist = m_coord.distance( refOther.m_coord );
			return horiz_dist + 10 * delta_altitude_km ;
		}

		/// Adds the events of another placemark. Manages events enforced to be unique.
		void concatenate( const PlacemarkT & refOther ) {
			if( refOther.empty() ) return ;

			time_t firstTm = refOther.begin()->first ;
			if( firstTm == KmlServer::UniqueEvent ) {
				clear();
				/// We keep this special time_t value to enforce unicity of CustomDataT.
				insert( *refOther.begin() );
			} else {
				insert( refOther.begin(), refOther.end() );
			}
		} // PlacemarkT::concatenate

		/// This transforms our coordinates into KML ones.
		void WriteCooSub( std::ostream & ostrm ) const
		{
			ostrm	<< m_coord.longitude().angle() << ','
				<< m_coord.latitude().angle() << ','
				<< m_altitude ;
		}

		/// Writes the placemark to a KML stream.
		void Serialize( std::ostream & ostrm, const std::string & kmlNam, int balloon_style ) const
		{
			// Range of events which occured at this place.
			const_reverse_iterator beEvt = rbegin(), enEvt = rend();

			// There should be at least one event.
			if( beEvt == enEvt ) {
				LOG_WARN("Inconsistency: No event kmlId=%s",m_kmlId.c_str() );
				return ;
			}

			// The unique key is indeed the name and the time,
			// because an object such as a ship might move and come back to the same place.
			// We add a counter because during tests, the timestamps are too close.
			ostrm << "<Placemark id=\"" << m_kmlId << "\">\n";

			// Beware of the sign of longitude.
			ostrm << "<Point><coordinates>" ;
			WriteCooSub( ostrm );
			ostrm << "</coordinates></Point>\n";

			if( kmlNam.empty() )
				ostrm << "<name>No name</name>\n";
			else {
				// Looks like there is a bug in KML when displaying a placemark ID containing an hyphen.
				ostrm << "<name>";
				StripHtmlTags(ostrm,kmlNam);
			       	ostrm << "</name>\n";
			}

			ostrm << "<styleUrl>";
			styleToKml(ostrm);
			ostrm << "</styleUrl>\n";

			// More information here: http://freegeographytools.com/2007/putting-time-data-into-a-kml-file
			// 1944-06-06T06:00:00  See http://www.w3.org/TR/xmlschema-2/#isoformats
			// We do not add the timestamps because it is not very ergonomic. Should be added to the linestrings too.
			static const bool withTimestamp = false ;
			if( withTimestamp ) {
				// Last update time is last argument.
				ostrm << "<Timestamp><when>";
				KmlTimestamp(ostrm,enEvt->first);
				ostrm << "</when></Timestamp>\n";
			}

			/// Whats is displayed on the margin. Must be short.
			ostrm << "<Snippet maxLines=\"1\">";
			StripHtmlTags(ostrm,m_descrTxt);
		       	ostrm << "</Snippet>\n";


			/**
		 	* Unfortunately it is not possible to use CSS in Google-maps, due to "content scrubbing":
		 	* http://stackoverflow.com/questions/8421260/styling-kml-with-css-in-google-maps-v3
		 	* Scrubbing the contents is a security measure to prevent malicious code from executing.
		 	* It removes JavaScript, CSS, iframe, embed and object tags.
		 	*/
			static const char * colorTime = "#0099CC";
			static const char * colorKey = "#FF9933";

			ostrm << "<description>";

			/// Data can be displayed into one big matrix, or several tables, one per event.
			switch( balloon_style )
			{
			case 0: /// Plain text, for example for GPX conversion.
				for( const_reverse_iterator itEvt = beEvt; itEvt != enEvt; ++itEvt )
				{
					ostrm << "Timestamp:" << Tm2Time(itEvt->first) << "\n" ;
					const CustomDataT & refCust = itEvt->second;
					for( CustomDataT::const_iterator it = refCust.begin(), en = refCust.end(); it != en; ++it )
					{
						StripHtmlTags(ostrm,it->first);
					       	ostrm << ":";
						/// Do not insert <br> tags.
						StripHtmlTags(ostrm,it->second);
					       	ostrm << "\n" ;
					}
					ostrm << "\n" ;
				}
				break;
			case 1: // One distinct HTML table per event.
				ostrm << "<![CDATA[<table border=\"1\">";
				// Print from the most recent event, which is at the end.
				for( const_reverse_iterator itEvt = beEvt; itEvt != enEvt; ++itEvt )
				{
					ostrm << "<tr>";
					ostrm << "<td bgcolor=" << colorTime << " colspan=\"2\">" << Tm2Time(itEvt->first) << "</td>"
						"</tr>" ;
					const CustomDataT & refCust = itEvt->second;
					for( CustomDataT::const_iterator it = refCust.begin(), en = refCust.end(); it != en; ++it )
					{
						ostrm << "<tr>" "<td>";
						StripHtmlTags(ostrm,it->first);
					       	ostrm << "</td>" "<td>";
						StripHtmlTagsNl(ostrm,it->second);
					       	ostrm << "</td>" "</tr>" ;
					}
				}
				ostrm << "</table>]]>\n";
				break;
		case 2 :
				{
				/// Transposition of the html matrix.
				typedef std::vector<time_t> TitleT ;
				TitleT titles ;
				typedef std::vector< std::string > RowT ;

				const_iterator beCol = begin(), enCol = end();
				for( const_iterator itCol = beCol; itCol != enCol; ++itCol )
				{
					titles.push_back( itCol->first );
				}
				size_t nbCols = titles.size();

				typedef std::map< std::string, RowT > MtxT;
				MtxT mtx ;

				size_t iCols = 0;
				for( const_iterator itCol = beCol; itCol != enCol; ++itCol, ++iCols )
				{
					const CustomDataT & refCust = itCol->second;
					for( CustomDataT::const_iterator it = refCust.begin(), en = refCust.end(); it != en; ++it )
					{
						MtxT::iterator itMtx = mtx.find( it->first );
						if( itMtx == mtx.end() ) {
							itMtx = mtx.insert( mtx.end(), MtxT::value_type( it->first, RowT() ) );
							itMtx->second.resize(nbCols);
						}
						itMtx->second[iCols] = it->second ;
					}
				}

				ostrm << "<![CDATA[<table border=\"1\">";
				ostrm << "<tr><td></td>";
				for( size_t iCols = 0; iCols < nbCols; ++iCols ) {
					ostrm << "<td bgcolor=" << colorTime << ">" << Tm2Time(titles[iCols]) << "</td>";
				}
				ostrm << "</tr>" ;

				for( MtxT::const_iterator itMtx = mtx.begin(), enMtx = mtx.end(); itMtx != enMtx; ++itMtx )
				{
					ostrm << "<tr>";
					ostrm << "<td bgcolor=" << colorKey << ">";
					StripHtmlTags(ostrm,itMtx->first);
					ostrm << "</td>";

					// TODO: Do not write twice the same value if it is not numeric (Starting with a digit)
					// or longer than N characters.
					for(
						RowT::const_iterator itRow = itMtx->second.begin(), enRow = itMtx->second.end();
						itRow != enRow;
						++itRow )
					{
						ostrm << "<td>";
						StripHtmlTags(ostrm,*itRow);
						ostrm << "</td>";
					}
					ostrm << "</tr>" ;
				}
				ostrm << "</table>]]>\n";
				break;
				}
			}
			ostrm << "</description>\n";
			// TODO: Other dsplay style: All elements on a one single table. Removal of duplicate text values etc...


			SerializeForReading( ostrm );

			ostrm << "</Placemark>\n";
		} // PlacemarkT::Serialize
	}; // PlacemarkT

	/** The placemark name is unique wrt to the application.
	It might map to several PlaceMark, each having distinct coordinates.
	It is not really possible to sort the different locations by coordinates,
	because it is a list created by the object trajectory.
	class PlacesMapT : public std::multimap< std::string, PlacemarkT >
	*/
	class PlacesMapT : public std::multimap< std::string, PlacemarkT >
	{
		/// Written to by the main thread with lock protection, read (and emptied)
		/// by the sub thread which is later in charge of writing things to disk.
		typedef std::list< value_type > PlacemarkListT ;

		/// A separate queue helps for performance because the insertion in the main container
		/// might take time and the main thread may lose data.
		PlacemarkListT m_queue_to_insert ;

		/// This is not set when inserting at load time, but when emptying
		/// the queue, and when data are ready for writing to disk.
		mutable bool m_must_save ;

	public:
		PlacesMapT() : m_must_save(false) {}

		/// Finds an object with the same name and close enough.
		/// If an object with the same name exists but is distant, creates a new one,
		/// plus a path between the two.
		/// Called by the main thread at startup when loading the previous KML files. Then later
		/// called by the subthread in charge of flushing PlacemarkT to the KML file.
		void DirectInsert( const value_type & refVL, double merge_dist )
		{
			/// Not needed to use equal_range because we need the last placemark matching
			/// this key, and this iterator is forward_iterator only.
			iterator it = find( refVL.first ), en = end() ;
			if( it == en ) {
				// LOG_INFO("Cannot find '%s'", refVL.first.c_str() );
				it = insert( end(), refVL );
				return;
			}

			/// Searches for the last element with the same key.
			iterator last = it, next = it ;
			++next ;
			while( next != en && next->first == refVL.first ) {
				last = next;
				++next ;
			}

			double dist = last->second.distance_to( refVL.second );

			/// We can reuse the last element because it is not too far from our coordinates.
			if( 1000 * dist < merge_dist ) {
				/// LOG_INFO("Reusing '%s' merge_dist=%lf", refVL.first.c_str(), dist );

				/** There will be one event only if adding a new received event,
				otherwise several if reloading from a file.
				The new events will be inserted based on their timestamp.
				*/
				// last->second.insert( refVL.second.begin(), refVL.second.end() );
				last->second.concatenate( refVL.second );
				return ;
			}

			// LOG_INFO("Inserted '%s' merge_dist=%lf", refVL.first.c_str(), dist );

			/// The object is inserted at the end of all elements with the same key.
			iterator ret = insert( next, refVL );

			/// Runtime check of an assumption.
			{
				iterator tst = last ;
				++tst ;
				if( tst != ret ) {
					LOG_WARN("Iterators assumption is wrong (1): %s", refVL.first.c_str() );
				}
				++tst ;
				if( tst != next ) {
					LOG_WARN("Iterators assumption is wrong (2): %s", refVL.first.c_str() );
				}
			}

			/// They must have the same style otherwise they will be in different folders.
			if( refVL.second.style() != last->second.style() ) {
				LOG_WARN("Correcting style discrepancy %s: %s != %s",
						refVL.first.c_str(),
						refVL.second.style().c_str(),
						last->second.style().c_str() );
				ret->second.style( last->second.style() );
			}
		} // DirectInsert

		/// Enqueues a new placemark for insertion by the subthread. Called by the  main thread
		/// each time a Broadcast of a new PlacemarkT is done.
		void Enqueue( const std::string & kmlNam, const PlacemarkT & refPM ) {
			/// So we will save to a file, because something changed.
			m_queue_to_insert.push_back( value_type( kmlNam, refPM ) );
		}

		/// Called by the subthread. It can merge data of placemarks with the same name
		/// and different positions due to a move. This has to be very fast because under lock protection.
		void FlushQueue(double merge_dist) {
			// LOG_INFO("FlushQueue nbelts %d sz=%d", m_queue_to_insert.size(), size() );

			if( m_queue_to_insert.empty() ) return ;

			for( PlacemarkListT::iterator itPL = m_queue_to_insert.begin(), enPL = m_queue_to_insert.end(); itPL != enPL; ++ itPL )
			{
				DirectInsert( *itPL, merge_dist );
			}
			// LOG_INFO("Flushed into sz=%d", size() );

			// TODO: If lock contention problems, we might swap this list with another one owned by this
			// objet. This would later be merged into the container before saving data to disk.
			m_queue_to_insert.clear();
			m_must_save = true ;
		}

		/// Removes obsolete data for one category only.
		void PruneKmlFile( int retention_delay )
		{
			/// By convention, it means keeping all data.
			if( retention_delay <= 0 ) return ;

			/// Called only once per hour, instead of at every call. Saves CPU.
			static time_t prev_call_tm = 0 ;
			time_t now = time(NULL);

			static const int seconds_per_hour = 60 * 60 ;

			/// First call of this function, always do the processing.
			if( prev_call_tm != 0 ) {
				/// If this was called for less than one hour, then return.
				if( prev_call_tm > now - seconds_per_hour ) return ;
			}
			prev_call_tm = now ;

			/// Cleanup all data older than this.
			time_t limit_time = now - retention_delay * seconds_per_hour ;

			LOG_INFO("sz=%d retention=%d hours now=%s limit=%s",
				(int)size(), retention_delay, KmlTimestamp(now).c_str(), KmlTimestamp(limit_time).c_str() );

			size_t nbFullErased = 0 ;
			size_t nbPartErased = 0 ;
			for( iterator itMap = begin(), nxtMap = itMap, enMap = end() ; itMap != enMap; itMap = nxtMap ) {
				PlacemarkT & refP = itMap->second ;
				++nxtMap ;

				/// Erases all elements older than the limit, or the whole element.
				PlacemarkT::iterator itP = refP.upper_bound( limit_time );
				if( itP == refP.end() ) {
					erase( itMap );
					++nbFullErased ;
				} else if( itP != refP.begin() ) {
					refP.erase( refP.begin(), itP );
					++nbPartErased ;
				}
			}

			// Maybe the container only lost data because of data expiration, so it must be saved.
			bool must_save_now = m_must_save || ( nbFullErased > 0 ) || ( nbPartErased > 0 ) ;
			LOG_INFO("Sz=%d FullyErased=%d PartialErased=%d m_must_save=%d must_save_now=%d",
					(int)size(), (int)nbFullErased, (int)nbPartErased, m_must_save, must_save_now );
			m_must_save = must_save_now ;
		}

		/// This is not efficient because we reopen the file at each access, but it ensures
		/// that the file is consistent and accessible at any moment.
		bool RewriteKmlFileOneCategory(
				const std::string & category,
				const std::string & kmlFilNam,
				int balloon_style ) const {
			// Normally, it is stable when we insert an element with a duplicate key.
			typedef std::multiset< PlacesMapT::const_iterator, PlacesMapIterSortT > PlacesMapItrSetT ;

			PlacesMapItrSetT plcMapIterSet ;

			/// If there is nothing to save, not needed to create a file.
			if( false == m_must_save ) return false ;

			/// For safety purpose, we do not empty the file. It might be an error.
			if( empty() ) {
				LOG_INFO("Should empty KML file %s. Grace period.", kmlFilNam.c_str() );
				return false ;
			}

			m_must_save = false ;

			for( const_iterator itPlcMap = begin(), en = end(); itPlcMap != en ; ++itPlcMap )
			{
				plcMapIterSet.insert( itPlcMap );
			}
			int nbPlacemarks = plcMapIterSet.size();

			// This file must be atomic because it is periodically read.
			// TODO: Checks if another process has locked the ".tmp" file.
			// If so , we should reread the KML file and merge our data with it.
			// This would allow to have several fldigi sessions simultaneously running
			// on the same KML files. On the other hand, it should already work if these
			// processes are updating different categories, which is more probable.
			AtomicRenamer ar( kmlFilNam );

			KmlHeader(ar, category) ;

			// This will easily allow hierarchical folders.
			std::string lastStyle ;
			for( PlacesMapItrSetT::const_iterator itStyl = plcMapIterSet.begin(), enStyl = plcMapIterSet.end(); itStyl != enStyl; )
			{
				ar << "<Folder>";

				// Hyphen: No idea why, but the string "10-meter discus buoy:W GULF 207 NM " just displays  as "10-"
				ar << "<name>";
				StripHtmlTags( ar,(*itStyl)->second.style() );
			       	ar << "</name>";

				// Several placemarks with the same name: A single object has moved.
				PlacesMapItrSetT::const_iterator itStylNext = itStyl ;
				for(;;) {
					// TODO: Objects with the same name and different coordinates must be signaled so.
					(*itStylNext)->second.Serialize( ar, (*itStylNext)->first, balloon_style);
					++itStylNext ;
					if( itStylNext == enStyl ) break ;
					if( (*itStyl)->second.style() != (*itStylNext)->second.style() ) break ;
				}

				// Now, in the same loop, we draw the polylines between placemarks with the same name.
				for( PlacesMapItrSetT::const_iterator itNamBeg = itStyl, itNamLast = itNamBeg; itNamLast != itStylNext ; )
				{
					PlacesMapItrSetT::const_iterator itNamNxt = itNamLast;
					++itNamNxt ;
					if( ( itNamNxt == itStylNext ) || ( (*itNamNxt)->first != (*itNamLast)->first ) ) {
						// No point tracing a line with one point only.
						if( *itNamBeg != *itNamLast ) {
							DrawPolyline( ar, category, *itNamBeg, *itNamLast );
						}
						itNamBeg = itNamNxt ;
					}
					itNamLast = itNamNxt ;
				}

				itStyl = itStylNext ;
				ar << "</Folder>";
			}

			ar << KmlFooter ;

			LOG_INFO("Saved %s: %d placemarks to %s", category.c_str(), nbPlacemarks, kmlFilNam.c_str() );
			return true ;
		} // KmlSrvImpl::PlacesMapT::RewriteKmlFileOneCategory

	}; // KmlSrvImpl::PlacesMapT

	/// There is a very small number of categories: Synop, Navtex etc...
	struct PlacemarksCacheT : public std::map< std::string, PlacesMapT > {
		const PlacesMapT * FindCategory( const std::string & category ) const {
			const_iterator it = find( category );
			if( it == end() ) return NULL ;
			return &it->second;
		}
		PlacesMapT * FindCategory( const std::string & category ) {
			iterator it = find( category );
			if( it == end() )
				it = insert( end(), value_type( category, PlacesMapT() ) );
			return &it->second ;
		}
	};

	/// At startup, should be reloaded from the various KML files.
	PlacemarksCacheT m_placemarks ;

	/// Used for signaling errors.
	void close_throw( FILE * ofil, const std::string & msg ) const {
		if( ofil ) fclose( ofil );
		throw std::runtime_error( strformat( "%s:%s", msg.c_str(), strerror(errno) ) );
	}

	/// This points to the specific KML file of the category.
	void CategoryNetworkLink( std::ostream & strm, const std::string & category ) const
	{
		strm << 
			"<NetworkLink>\n"
			"	<name>" << category << "</name>\n"
			"	<Link>\n"
			"		<href>" << category << ".kml</href>\n"
			"		<refreshMode>onInterval</refreshMode>\n"
			"		<refreshInterval>" << m_refresh_interval << "</refreshInterval>\n"
			"	</Link>\n"
			"</NetworkLink>\n";
	} // KmlSrvImpl::CategoryNetworkLink

	/// This file copy does not need to be atomic because it happens once only.
	void CopyStyleFileIfNotExists(void) {
		/// Where the installed file is stored and never moved from. Used as default.
		std::string namSrc = PKGDATADIR "/kml/" + namStyles ;

		/// The use might customize its styles file: It will not be altered.
		std::string namDst = m_kml_dir + namStyles ;

		/// Used to copy the master file to the user copy if needed.
		FILE * filSrc = NULL;
		FILE * filDst = fopen( namDst.c_str(), "r" );

		/// If the file is there, leave as it is because it is maybe customize.
		if( filDst ) {
			LOG_INFO("Style file %s not altered", namDst.c_str() );
			goto close_and_quit ;
		}
		filDst = fopen( namDst.c_str(), "w" );
		if( filDst == NULL ) {
			LOG_INFO("Cannot open destination style file %s", namDst.c_str() );
			goto close_and_quit ;
		}
		filSrc = fopen( namSrc.c_str(), "r" );
		if( filSrc == NULL ) {
			LOG_INFO("Cannot open source style file %s", namSrc.c_str() );
			goto close_and_quit ;
		}

		/// Transient buffer to copy the file.
    		char buffer[BUFSIZ];
    		size_t n;

   		while ((n = fread(buffer, sizeof(char), sizeof(buffer), filSrc)) > 0)
   		{
       			if (fwrite(buffer, sizeof(char), n, filDst) != n) {
				LOG_WARN("Error %s copying style file %s to %s", strerror(errno), namSrc.c_str(), namDst.c_str() );
				goto close_and_quit ;
			}
    		}
		LOG_INFO("Style file %s copied to %s", namSrc.c_str(), namDst.c_str() );
	close_and_quit:
		if( filDst ) fclose(filDst);
		if( filSrc ) fclose(filSrc);
	}

	/// This creates a KML file with links to the categories such as Synop and Navtex.
	void CreateMainKmlFile(void) {
		// This is the file, that the user must click on.
		std::string baseFil = m_kml_dir + "fldigi.kml" ;

		LOG_INFO("Creating baseFil=%s", baseFil.c_str() );

		/// We do not need to make this file atomic because it is read once only.
		AtomicRenamer ar( baseFil );

		KmlHeader( ar, "Fldigi");

		for( size_t i = 0; i < nb_categories; ++i )
			CategoryNetworkLink( ar, categories[i] );

		ar << KmlFooter ;
	}

	// TODO: Consider hierarchical categories: "Synop/buoy/Inmarsat"

	/// A specific name is chosen so we can later update this line.
	static void DrawPolyline(
		std::ostream             & ostrm,
		const std::string        & category,
		PlacesMapT::const_iterator beg,
		PlacesMapT::const_iterator last ) {

		/// The polyline gets an id based on the beginning of the path, which will never change.
		ostrm
			<< "<Placemark id=\"" << beg->second.KmlId() << ":Path\">"
			"<name>" << beg->second.KmlId() << "</name>"
			"<LineString>"
			"<altitudeMode>clampToGround</altitudeMode><tessellate>1</tessellate>\n"
			"<coordinates>\n";
		double dist = 0.0 ;
		int nbStops = 0 ;
		// "135.2, 35.4, 0. 
		for(;;) {
			beg->second.WriteCooSub( ostrm );
			ostrm << "\n";
			if( beg == last ) break ;
			PlacesMapT::const_iterator next = beg ;
			++next ;
			++nbStops;
			dist += beg->second.coordinates().distance( next->second.coordinates() );
			beg = next ;
		};

		ostrm << "</coordinates>"
			"</LineString>"
			"<Snippet>" << dist << " " << _("kilometers") << " in " << nbStops << " " << _("stops") << "</Snippet>"
 			"<Style>"
  			"<LineStyle><color>#ff0000ff</color></LineStyle> "
 			"</Style>"
			"</Placemark>\n";
	} // DrawPolyline

	/// Similar to a std::ofstream but atomically updated by renaming a temp file.
	struct AtomicRenamer : public std::ofstream {
		/// Target file name.
		std::string   m_filnam ;
		/// Temporary file name. Renamed to the target when closed.
		std::string   m_filtmp ;
	public:
		/// This opens a temporary file when all the writing is done.
		AtomicRenamer( const std::string & filnam )
		: m_filnam( filnam )
		, m_filtmp( filnam + ".tmp" )
		{
			// LOG_INFO("AtomicRenamer opening tmp %s", filnam.c_str() );
			open( m_filtmp.c_str() );
			if( bad() ) {
				LOG_WARN("Cannot open %s", m_filtmp.c_str() );
			}
		}

		/// Atomic because rename is an atomic too, and very fast if in same directory.
		~AtomicRenamer() {
			close();
			/// This is needed on Windows.
			int ret_rm = remove( m_filnam.c_str() );
			if( ( ret_rm != 0 ) && ( errno != ENOENT ) ) {
				LOG_WARN("Cannot remove %s: %s", m_filnam.c_str(), strerror(errno) );
			}

			int ret_mv = rename( m_filtmp.c_str(), m_filnam.c_str() );
			if( ret_mv ) {
				LOG_WARN("Cannot rename %s to %s:%s", m_filtmp.c_str(), m_filnam.c_str(), strerror(errno) );
			}
		}
	};

	/// The KML filename associated to a category.
	std::string CategFile( const std::string & category ) const {
		return m_kml_dir + category + ".kml";
	}

	/// Resets the files. Called from the test program and the GUI.
	void CreateNewKmlFile( const std::string & category ) const {
		// This file must be atomic because it is periodically read.
		AtomicRenamer ar( CategFile( category ) );
		KmlHeader( ar, category);
		ar << KmlFooter ;
	}

	/// Template parameters should not be local types.
	struct PlacesMapIterSortT {
		// This sort iterators on placemarks, based on the style name then the placemark name.
		bool operator()( const PlacesMapT::const_iterator & it1, const PlacesMapT::const_iterator & it2 ) const {
			int res = it1->second.style().compare( it2->second.style() );
			if( res == 0 )
				res = it1->first.compare( it2->first );
			return res < 0 ;
		}
	};

	/// Various states of the KML reader.
	#define KMLRD_NONE                                                   1
	#define KMLRD_FOLDER                                                 2
	#define KMLRD_FOLDER_NAME                                            3
	#define KMLRD_FOLDER_PLACEMARK                                       4
	#define KMLRD_FOLDER_PLACEMARK_NAME                                  5
	#define KMLRD_FOLDER_PLACEMARK_POINT                                 6
	#define KMLRD_FOLDER_PLACEMARK_POINT_COORDINATES                     7
	#define KMLRD_FOLDER_PLACEMARK_STYLEURL                              8
	#define KMLRD_FOLDER_PLACEMARK_SNIPPET                               9
	#define KMLRD_FOLDER_PLACEMARK_EXTENDEDDATA                         10
	#define KMLRD_FOLDER_PLACEMARK_EXTENDEDDATA_FLDIGIEVENT             11
	#define KMLRD_FOLDER_PLACEMARK_EXTENDEDDATA_FLDIGIEVENT_FLDIGIITEM  12
	#define KMLRD_FOLDER_PLACEMARK_UNDEFINED                            13
	#define KMLRD_FOLDER_UNDEFINED                                      14

	/// Debugging purpose only.
	static const char * KmlRdToStr( int kmlRd ) {
		#define KMLRD_CASE(k) case k : return #k ;
		switch(kmlRd) {
			KMLRD_CASE(KMLRD_NONE)
			KMLRD_CASE(KMLRD_FOLDER)
			KMLRD_CASE(KMLRD_FOLDER_NAME)
			KMLRD_CASE(KMLRD_FOLDER_PLACEMARK)
			KMLRD_CASE(KMLRD_FOLDER_PLACEMARK_NAME)
			KMLRD_CASE(KMLRD_FOLDER_PLACEMARK_POINT)
			KMLRD_CASE(KMLRD_FOLDER_PLACEMARK_POINT_COORDINATES)
			KMLRD_CASE(KMLRD_FOLDER_PLACEMARK_STYLEURL)
			KMLRD_CASE(KMLRD_FOLDER_PLACEMARK_SNIPPET)
			KMLRD_CASE(KMLRD_FOLDER_PLACEMARK_EXTENDEDDATA)
			KMLRD_CASE(KMLRD_FOLDER_PLACEMARK_EXTENDEDDATA_FLDIGIEVENT)
			KMLRD_CASE(KMLRD_FOLDER_PLACEMARK_UNDEFINED)
			KMLRD_CASE(KMLRD_FOLDER_UNDEFINED)
			default : return "Unknown KMLRD code";
		}
		#undef KMLRD_CASE
	}

	/// Loads a file of a previous session. The mutex should be locked at this moment.
	void ReloadSingleKmlFile( const std::string & category ) {
		std::string kmlFilNam = CategFile( category );

		LOG_INFO("kmlFilNam=%s m_merge_dist=%lf", kmlFilNam.c_str(), m_merge_dist );

		PlacesMapT *ptrMap = m_placemarks.FindCategory( category );

		FILE * filKml = fopen( kmlFilNam.c_str(), "r" );
		if( filKml == NULL ) {
			LOG_ERROR("Could not open %s. Creating one.", kmlFilNam.c_str() );
			CreateNewKmlFile( category );
			return ;
		}
		/// The destructor ensures the file will be closed if an exception is thrown.
		struct FilCloserT {
			FILE * m_file ;
			~FilCloserT() { fclose(m_file); }
		} Closer = { filKml };

		std::auto_ptr< irr::io::IrrXMLReader > xml( irr::io::createIrrXMLReader( Closer.m_file ) );
		if( xml.get() == NULL ) {
			LOG_ERROR("Could not parse %s", kmlFilNam.c_str() );
			return ;
		}

		using namespace irr::io ;

		int currState = KMLRD_NONE ;

		std::string currFolderName ;
		std::string currPlcmrkName ;
		time_t      currTimestamp = 0;
		std::string currPlacemarkDescr ;
		CustomDataT currCustData ;
		PlacemarkT  currPM;
		bool        currIsPoint = false ;
		std::string avoidNode ;

		/// Stores the unique nodes which are misplaced.
		typedef std::set< std::string > UnexpectedNodesT ;

		UnexpectedNodesT unexpectedNodes ;

		// <Folder><name>ship</name><Placemark id="Ship:AUP06:2012-10-11 04:28:9">
		// <Point><coordinates>146.8,-19.2,0</coordinates></Point>
		// <name>Ship:AUP06</name>
		// <styleUrl>styles.kml#ship</styleUrl>
		// <description><![CDATA[<table border="1"><tr><td bgcolor=#00FF00 colspan="2">2012-09-24 12:00</td></tr><tr
		while(xml->read())
		{
			switch(xml->getNodeType())
			{
			case EXN_TEXT: {
				if( ! avoidNode.empty() ) break ;

				const char * msgTxt = xml->getNodeData();
				LOG_DEBUG( "getNodeData=%s currState=%s", msgTxt, KmlRdToStr(currState) );
				switch(currState) {
					case KMLRD_FOLDER_NAME : 
					       currFolderName = msgTxt ? msgTxt : "NullFolder";
					       break;
					case KMLRD_FOLDER_PLACEMARK_POINT_COORDINATES : 
					       currPM.SetCoordinates(msgTxt);
					       break;
					case KMLRD_FOLDER_PLACEMARK_NAME : 
					       currPlcmrkName = msgTxt ? msgTxt : "NullPlacemarkName";
					       break;
					case KMLRD_FOLDER_PLACEMARK_SNIPPET : 
					       currPlacemarkDescr = msgTxt ? msgTxt : "NullSnippet";
					       break;
					case KMLRD_FOLDER_PLACEMARK_STYLEURL : 
					       currPM.SetStyle(msgTxt);
					       break;
					default:
					       break;
				}
				break;
			}
			case EXN_ELEMENT: {
				if( ! avoidNode.empty() ) break ;

				const char *nodeName = xml->getNodeName();
				// LOG_INFO( "getNodeName=%s currState=%s", nodeName, KmlRdToStr(currState) );
				// TODO: Have a hashmap for each case.
				switch(currState) {
					case KMLRD_NONE :
						if (!strcmp("Folder", nodeName)) {
							currState = KMLRD_FOLDER ;
						} else {
							/// These tags are not meaningful for us.
							if(	strcmp( "kml", nodeName ) &&
								strcmp( "Document", nodeName ) &&
								strcmp( "name", nodeName ) )
							{
							LOG_INFO("Unexpected %s in document %s. currState=%s",
								nodeName, category.c_str(), KmlRdToStr(currState) );
							}
						}
						break;
					case KMLRD_FOLDER : 
						if (!strcmp("name", nodeName)) {
							currState = KMLRD_FOLDER_NAME ;
						}
						else if (!strcmp("Placemark", nodeName)) {
							currState = KMLRD_FOLDER_PLACEMARK ;
							currPM.Clear();
							const char * strId = xml->getAttributeValue("id");
							currPM.SetKmlId( strId );
						}
						else {
							avoidNode = nodeName ;
							currState = KMLRD_FOLDER_UNDEFINED ;
							LOG_INFO("Unexpected %s in folder %s. currState=%s",
								nodeName, currFolderName.c_str(), KmlRdToStr(currState) );
						}
						break;
					case KMLRD_FOLDER_PLACEMARK : 
						// There are different sorts of Placemark such as Point or LineString.
						if (!strcmp("Point", nodeName)) {
							currState = KMLRD_FOLDER_PLACEMARK_POINT ;
							currIsPoint = true ;
						}
						else if (!strcmp("name", nodeName)) {
							currState = KMLRD_FOLDER_PLACEMARK_NAME ;
						}
						else if (!strcmp("Snippet", nodeName)) {
							currState = KMLRD_FOLDER_PLACEMARK_SNIPPET ;
						}
						else if (!strcmp("styleUrl", nodeName)) {
							currState = KMLRD_FOLDER_PLACEMARK_STYLEURL ;
						}
						else if (!strcmp("ExtendedData", nodeName)) {
							currState = KMLRD_FOLDER_PLACEMARK_EXTENDEDDATA ;
						}
						else if (!strcmp("description", nodeName)) {
							// We do not care about this tag, but it is always here.
							currState = KMLRD_FOLDER_PLACEMARK_UNDEFINED ;
						}
						else {
							avoidNode = nodeName ;
							currState = KMLRD_FOLDER_PLACEMARK_UNDEFINED ;

							/// At the same time, it is detected and inserted.
							std::pair< UnexpectedNodesT::iterator, bool >
								pr = unexpectedNodes.insert( nodeName );

							// Other occurences of the same nodes will not be signaled.
							if( pr.second ) {
								LOG_INFO("Unexpected %s in placemark id=%s name=%s. currState=%s",
									nodeName, currPM.KmlId().c_str(),
									currPlcmrkName.c_str(), KmlRdToStr(currState) );
							}
						}
						break;
					case KMLRD_FOLDER_PLACEMARK_POINT : 
						if (!strcmp("coordinates", nodeName)) {
							currState = KMLRD_FOLDER_PLACEMARK_POINT_COORDINATES ;
						}
						else {
							LOG_INFO("Unexpected %s in coordinates. currState=%s",
								nodeName, KmlRdToStr(currState) );
						}
						break;
					case KMLRD_FOLDER_PLACEMARK_EXTENDEDDATA:
						if (!strcmp(FLDIGI_TAG_EVT, nodeName)) {
							currState = KMLRD_FOLDER_PLACEMARK_EXTENDEDDATA_FLDIGIEVENT ;
							currTimestamp = KmlFromTimestamp( xml->getAttributeValue(FLDIGI_TIMESTAMP) );
							currCustData.clear();
						}
						else LOG_INFO("Unexpected %s in extended data. currState=%s", nodeName, KmlRdToStr(currState) );
						break;
					case KMLRD_FOLDER_PLACEMARK_EXTENDEDDATA_FLDIGIEVENT:
						// http://irrlicht.sourceforge.net/forum/viewtopic.php?t=10532
						// The Parser irrXml will not call "EXN_ELEMENT_END" because the tag <fldigi:item/>
						// has a trailing slash. Therefore we stay in the same state.
						// We could call irr::io::IIrrXMLReader<...>::isEmptyElement() to check that.
						if (!strcmp(FLDIGI_TAG_ITM, nodeName)) {
							assert( xml->isEmptyElement() );
							const char * strKey = xml->getAttributeValue(FLDIGI_TAG_KEY);
							if(strKey == NULL ) {
								LOG_INFO("Null item key");
								break ;
							}
							const char * strVal = xml->getAttributeValue(FLDIGI_TAG_VAL);
							if(strVal == NULL ) {
								LOG_INFO("Null item value");
								break ;
							}
							currCustData.Push( strKey, strVal );
						}
						else LOG_INFO("Unexpected %s in event. currState=%s", nodeName, KmlRdToStr(currState) );
						break;
					default:
						break;
				}
				break;
			}
			case EXN_ELEMENT_END: {
				if( ! avoidNode.empty() ) {
					const char * msgTxt = xml->getNodeData();
					if( avoidNode == msgTxt ) {
						// LOG_INFO("Leaving forbidden element %s. currState=%s", avoidNode.c_str(), KmlRdToStr(currState) );
						// We can leave the quarantine.
						avoidNode.clear();
					} else {
						// LOG_INFO("Still in forbidden element %s, leaving %s. currState=%s",
						// 		avoidNode.c_str(), msgTxt, KmlRdToStr(currState) );
						break ;
					}
				}

				// We should check that this string matches wuth the state expects, but this is much
				// faster to use only integers.
				// LOG_INFO("End of %s currState=%s", xml->getNodeData(), KmlRdToStr(currState) );
				switch(currState) {
					case KMLRD_FOLDER :
						currState = KMLRD_NONE ;
						break;
					case KMLRD_FOLDER_PLACEMARK :
						// Loads only "Point" placemarks. Do not load "Linestring".
						if( ( ! currPlcmrkName.empty() ) && currIsPoint ) {
							ptrMap->DirectInsert( PlacesMapT::value_type( currPlcmrkName, currPM ), m_merge_dist );
						}

						currTimestamp = -1 ;
						currPlacemarkDescr.clear();
						currPlcmrkName.clear();
						currIsPoint = false ;
						currState = KMLRD_FOLDER ;
						break;
					case KMLRD_FOLDER_NAME :
					case KMLRD_FOLDER_UNDEFINED :
						currState = KMLRD_FOLDER ;
						break;
					case KMLRD_FOLDER_PLACEMARK_NAME :
					case KMLRD_FOLDER_PLACEMARK_POINT :
					case KMLRD_FOLDER_PLACEMARK_STYLEURL :
					case KMLRD_FOLDER_PLACEMARK_SNIPPET :
					case KMLRD_FOLDER_PLACEMARK_EXTENDEDDATA :
					case KMLRD_FOLDER_PLACEMARK_UNDEFINED :
						currState = KMLRD_FOLDER_PLACEMARK ;
						break;
					case KMLRD_FOLDER_PLACEMARK_POINT_COORDINATES :
						currState = KMLRD_FOLDER_PLACEMARK_POINT ;
						break;
					case KMLRD_FOLDER_PLACEMARK_EXTENDEDDATA_FLDIGIEVENT :
						currState = KMLRD_FOLDER_PLACEMARK_EXTENDEDDATA ;
						// TODO: As soon as possible, use std::move for performance.
						currPM.AppendEvent( currTimestamp, currPlacemarkDescr, currCustData );
						currCustData.clear();
						break;
					case KMLRD_NONE:
						break;
					default:
						LOG_ERROR("Should not happen %s", KmlRdToStr(currState));
						break;
					}
				// LOG_INFO("currState=%s", KmlRdToStr(currState) );
				break;
			}
			case EXN_NONE:
			case EXN_COMMENT:
			case EXN_CDATA:
			case EXN_UNKNOWN:
				break;
			default:
				// LOG_INFO( "Default NodeType=%d", xml->getNodeType());
				break;
			}
		}

		LOG_INFO("kmlFilNam=%s loaded sz=%d", kmlFilNam.c_str(), (int)ptrMap->size() );
	} // KmlSrvImpl::ReloadSingleKmlFile

	/// Rewrites only the categories which have changed.
	bool RewriteKmlFileFull(void) {
		bool wasSaved = false ;
		// LOG_INFO("nb_categories=%d", nb_categories );
		for( size_t i = 0; i < nb_categories; ++i ) {
			const char * category = categories[i];
			PlacesMapT *ptrMap = m_placemarks.FindCategory( category );
			if( ptrMap == NULL ) {
				LOG_INFO("Category %s undefined", category );
				continue;
			}
			ptrMap->PruneKmlFile( m_retention_delay );
			wasSaved |= ptrMap->RewriteKmlFileOneCategory( category, CategFile( category ), m_balloon_style );
		}
		return wasSaved ;
	} // KmlSrvImpl::RewriteKmlFileFull

#ifdef FLDIGI_KML_CONDITION_VARIABLE
	/// This is signaled when geographic data is broadcasted.
	pthread_cond_t  m_cond_queue ;
#else
	/// This is set to true when geographic data is broadcasted.
	bool            m_bool_queue ;

	/// This tells that the subthread must leave at the first opportunity.
	bool            m_kml_must_leave;
#endif
	pthread_mutex_t m_mutex_write ;

	typedef std::list< PlacemarkT > PlacemarkListT ;

	PlacemarkListT m_queues[ nb_categories ];

	/// Called in a subthread. Woken up by a timed condition to save content to a KML file.
	void * ThreadFunc(void) {
		MilliSleep(2000); // Give time enough to load data.
		int r ;

		// This is normally the default behaviour.
		r = pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
		if( r != 0 ) {
			LOG_ERROR("pthread_setcancelstate %s", strerror(errno) );
			return NULL;
		}
		r = pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, NULL );
		if( r != 0 ) {
			LOG_ERROR("pthread_setcanceltype %s", strerror(errno) );
			return NULL;
		}
		int refresh_delay = m_refresh_interval ;

		// Endless loop until end of program, which cancels this subthread.
		for(;;)
		{
#ifdef FLDIGI_KML_CONDITION_VARIABLE
			struct timespec tmp_tim;
			{
				guard_lock myGuard( &m_mutex_write );

				// It does not need to be very accurate.
				tmp_tim.tv_sec = time(NULL) + refresh_delay;
				tmp_tim.tv_nsec = 0 ;

				// LOG_INFO("About to wait %d seconds", refresh );
				r = pthread_cond_timedwait( &m_cond_queue, &m_mutex_write, &tmp_tim );
				if( ( r != ETIMEDOUT ) && ( r != 0 ) ) {
					LOG_ERROR("pthread_cond_timedwait %s d=%d", strerror(errno), m_refresh_interval );
					return (void *)"Error in pthread_cond_timed_wait";
				}
#else
			/// On the platforms where pthread_cond_timedwait has problems, everything behaves
			// as if there was a timeout when data is saved. refresh_delay is never changed.
			for( int i = 0; i < refresh_delay; ++i )
			{
				MilliSleep( 1000 );
				if( m_kml_must_leave )
				{
					LOG_INFO("Exit flag detected. Leaving");
					return (void *)"Exit flag detected";
				}
			}
			{
				guard_lock myGuard( &m_mutex_write );
				r = m_bool_queue ? ETIMEDOUT : 0 ;
				m_bool_queue = false;
#endif

				// Except if extremely slow init, the object should be ready now: Files loaded etc...
				if( ! m_loaded ) {
					static int nb_retries = 3 ;
					if( nb_retries == 0 ) {
						static const char * error_message = "KML server could not start. Leaving";
						LOG_ERROR("%s", error_message );
						return (void *)error_message ;
					}
					--nb_retries ;
					LOG_INFO("KML server not ready yet. Cnt=%d. Restarting.",nb_retries);
					MilliSleep(1000); // Give time to load data.
					continue ;
				}

				// We might have missed the condition signal so a quick check will not do any harm.
				for( size_t i = 0; i < nb_categories; ++i )
				{
					PlacesMapT *ptrMap = m_placemarks.FindCategory( categories[i] );
					if( ptrMap == NULL ) {
						LOG_INFO("Category %s undefined", categories[i] );
						continue;
					}
					// TODO: If there are contention problems, internally swap the queue
					// with a fresh empty one.
					ptrMap->FlushQueue( m_merge_dist );
				}
				// LOG_INFO("Releasing lock" );
			}
			if( r == ETIMEDOUT )
			{
				// LOG_INFO("Saving after wait=%d", refresh );
				bool wasSaved = RewriteKmlFileFull();

				// Maybe a user process must be created to process these KML files.
				if(wasSaved) {
					SpawnCommand();
				}

				// Reset the interval to the initial value.
				refresh_delay = m_refresh_interval ;
#ifdef FLDIGI_KML_CONDITION_VARIABLE
			} else {
				refresh_delay = tmp_tim.tv_sec - time(NULL);
				if( refresh_delay <= 0 ) refresh_delay = 1 ;
				// LOG_INFO("Interrupted when waiting. Restart with wait=%d", refresh );
#endif
			}
		} // Endless loop.
		return NULL ;
	} // ThreadFunc

	/// The C-style function called by pthread.
	static void * ThreadFunc( void * ptr ) {
		return static_cast< KmlSrvImpl *>(ptr)->ThreadFunc();
	};

public:
	/// When setting or changing core parameters.
	void InitParams(
		const std::string & kml_command,
		const std::string & kml_dir,
		double              kml_merge_distance,
		int                 kml_retention_delay,
		int                 kml_refresh_interval,
		int                 kml_balloon_style)
	try
	{
		/// The thread should NOT access anything at this moment.
		guard_lock myGuard( &m_mutex_write );

		/// If the string is empty, no command is executed.
		m_command = kml_command;
		strtrim( m_command );

		m_merge_dist = kml_merge_distance ;
		m_retention_delay = kml_retention_delay ;

		static const int min_refresh = 10 ;
		if( kml_refresh_interval < min_refresh ) {
			LOG_WARN("Refresh interval too small %d minimum is %d", kml_refresh_interval, min_refresh );
			kml_refresh_interval = min_refresh ;
		}
		m_refresh_interval = kml_refresh_interval ;
		m_balloon_style = kml_balloon_style ;

		LOG_INFO("dir=%s merge_distance=%lf retention_delay=%d refresh_interval=%d balloon_style=%d",
			kml_dir.c_str(), kml_merge_distance, kml_retention_delay, kml_refresh_interval, kml_balloon_style );

		m_kml_dir = kml_dir ;

		/// This enforces that the directory name always ends with a slash.
		if( m_kml_dir[ m_kml_dir.size() - 1 ] != '/' ) m_kml_dir += '/' ;

		const char * resdir = create_directory( m_kml_dir.c_str() );
		if ( resdir ) {
			throw std::runtime_error( strformat( "Cannot create %s:%s", m_kml_dir.c_str(), resdir ) );
		}

		CreateMainKmlFile();
		CopyStyleFileIfNotExists();
	} catch (const std::exception & exc) {
		// We assume that the calling program has no try/catch handler.
		LOG_ERROR("Caught exception:%s", exc.what() );
	} catch (...) {
		LOG_ERROR("Caught unknown exception");
	}

	virtual void ReloadKmlFiles()
	{
		/// The thread should NOT access anything at this moment.
		guard_lock myGuard( &m_mutex_write );

		// This loads placemarks without the subthread, because it is simpler.
		for( size_t i = 0; i < nb_categories; ++i ) {
			try {
				ReloadSingleKmlFile( categories[i] );
			} catch( const std::exception & exc ) {
				LOG_INFO("Category %s. Caught %s", categories[i], exc.what() );
			}
		}

		// Now the object is usable. Theoretically should be protected by a mutex.
		LOG_DEBUG("Object ready");

		/// Even if an exception was thrown when loading the previous file, it does not 
		/// prevent to overwrite the old files with new and clean ones.
		m_loaded = true ;
	}

	/// Invalid values everywhere, intentionnaly.
	KmlSrvImpl()
	: m_loaded(false)
	, m_pid_command(-1)
	, m_merge_dist(-1.0)
	, m_retention_delay(-1)
	, m_refresh_interval(-1)
	, m_balloon_style(0)
	{
		LOG_DEBUG("Creation");
#ifdef FLDIGI_KML_CONDITION_VARIABLE
		pthread_cond_init( &m_cond_queue, NULL );
#else
		m_bool_queue = false;
		m_kml_must_leave = false;
#endif
		pthread_mutex_init( &m_mutex_write, NULL );

		/// TODO: Add this thread to the other fldigi threads stored in cbq[].
		if( pthread_create( &m_writer_thread, NULL, ThreadFunc, this ) ) {
			/// It is not urgent because this does not interact with the main thread.
			LOG_ERROR("pthread_create %s", strerror(errno) );
		}
	}

	/** We cannot use KML updates with a local file:
	 * "I want to know if it exists any solution to put relative paths in a
	 * targetHref to load upload files in a directory in my computer simply
	 * without any server running."
	 * "Unfortunately, no, the security restrictions around <Update> prevent
	 * this explicitly."
	 */

	/// iconNam: wmo automated fixed other dart buoy oilrig tao
	void Broadcast(
		const std::string & category,
		time_t evtTim,
		const CoordinateT::Pair & refCoo,
		double altitude,
		const std::string & kmlNam,
		const std::string & styleNam,
		const std::string & descrTxt,
		const CustomDataT & custDat )
	{
		/// Hyphen: No idea why, but the string "10-meter discus buoy:W GULF 207 NM " just displays  as "10-"
		std::string tmpKmlNam = strreplace( kmlNam, "-", " ");
		PlacemarkT currPM( refCoo, altitude, styleNam, tmpKmlNam );
		currPM.AppendEvent( evtTim, descrTxt, custDat );

		guard_lock myGuard( &m_mutex_write );

		++KmlServer::m_nb_broadcasts;
		PlacesMapT *ptrMap = m_placemarks.FindCategory( category );
		if(ptrMap == NULL ) {
			LOG_ERROR("Category %s undefined", category.c_str());
		}
		ptrMap->Enqueue( tmpKmlNam, currPM );
#ifdef FLDIGI_KML_CONDITION_VARIABLE
		pthread_cond_signal( &m_cond_queue );
#else
		m_bool_queue = true;
#endif
		LOG_INFO("'%s' sz=%d time=%s nb_broad=%d m_merge_dist=%lf",
			descrTxt.c_str(), (int)ptrMap->size(),
			KmlTimestamp(evtTim).c_str(),
			KmlServer::m_nb_broadcasts,m_merge_dist );
	}

	/// It flushes the content to disk.
	~KmlSrvImpl() {
		{
			/// This will not be killed in the middle of the writing.
			LOG_INFO("Cancelling writer thread");
			guard_lock myGuard( &m_mutex_write );

#ifdef FLDIGI_KML_CONDITION_VARIABLE
			LOG_INFO("Cancelling subthread");
			int r = pthread_cancel( m_writer_thread );
			if( r ) {
				LOG_ERROR("pthread_cancel %s", strerror(errno) );
				return ;
			}
#else
			LOG_INFO("Setting exit flag.");
			m_kml_must_leave = true ;
#endif
		}
		// LOG_INFO("Joining subthread");
		void * retPtr;
		int r = pthread_join( m_writer_thread, &retPtr );
		if( r ) {
			LOG_ERROR("pthread_join %s", strerror(errno) );
			return ;
		}
		const char * msg =
			(retPtr == NULL)
				? "Null"
				: (retPtr == PTHREAD_CANCELED)
					? "Canceled thread"
					: static_cast<const char *>(retPtr);
		LOG_INFO("Thread stopped. Message:%s", msg );

		/// Here we are sure that the subthread is stopped. The subprocess is not called.
		RewriteKmlFileFull();

#ifdef FLDIGI_KML_CONDITION_VARIABLE
		pthread_cond_destroy( &m_cond_queue );
#endif
		pthread_mutex_destroy( &m_mutex_write );
	}

	/// Empties the generated files.
	void Reset(void) {
		for( size_t i = 0; i < nb_categories; ++i ) {
			CreateNewKmlFile(categories[i]);
		}
		ResetCounter();
	}

	/// This is called when KML files are saved, or on demand from the configuration tab.
	void SpawnCommand() {
		if( m_command.empty() ) return ;

		/** This stores the process id so the command is not restarted if still running.
		This allows to start for example Google Earth only once. But this allows
		also to run GPS_Babel for each new set of files, as soon as they are created.
		*/
		int is_proc_still_running = test_process( m_pid_command );
		if( is_proc_still_running == -1 ) return ;
		if( ( m_pid_command <= 0 ) || ( is_proc_still_running == 0 ) ) {
			m_pid_command = fork_process( m_command.c_str() );
			LOG_INFO("%s: Pid=%d Command=%s", __FUNCTION__, m_pid_command, m_command.c_str() );
		}
	}

}; // KmlSrvImpl

/// Singleton. It must be destroyed at the end.
static KmlServer * g_inst = NULL;

static KmlSrvImpl * Pointer() {
	if( NULL == g_inst ) {
		g_inst = new KmlSrvImpl();
	}
	KmlSrvImpl * p = dynamic_cast< KmlSrvImpl * >( g_inst );
	if( p == NULL ) {
		LOG_ERROR("Null pointer");
		throw std::runtime_error("KmlServer not initialised");
	}
	return p ;
}

std::string KmlServer::Tm2Time( time_t tim ) {
	char bufTm[24];
	tm tmpTm;
	gmtime_r( &tim, & tmpTm );

	sprintf( bufTm, "%4d-%02d-%02d %02d:%02d",
			tmpTm.tm_year + 1900,
			tmpTm.tm_mon + 1,
			tmpTm.tm_mday,
			tmpTm.tm_hour,
			tmpTm.tm_min );
	return bufTm;
}

/// Returns current time.
std::string KmlServer::Tm2Time( ) {
	return Tm2Time( time(NULL) );
}

/// One singleton for everyone.
KmlServer * KmlServer::GetInstance(void)
{
	return Pointer();
}

/// This creates a process running the user command.
void KmlServer::SpawnProcess() {
	Pointer()->SpawnCommand();
}

/// Called by thr main program, clean exit.
void KmlServer::Exit(void) {
	// We assume that the calling program has no try/catch handler.
	// LOG_INFO("Exiting");
	try {
		KmlServer * pKml = Pointer();
		if( pKml ) {
			delete pKml;
		}
	} catch (const std::exception & exc) {
		LOG_ERROR("Caught exception:%s",exc.what() );
	} catch (...) {
		LOG_ERROR("Caught unknown exception");
	}
}
