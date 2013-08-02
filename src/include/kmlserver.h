// ----------------------------------------------------------------------------
// kmlserver.h  --  KML Server
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

#ifndef _KMLSERVER_H
#define _KMLSERVER_H

#include <limits.h>

#include <string>
#include <vector>
#include <sstream>

#include <coordinate.h>

/// Keyhole Markup Language: This publishes a complete message, localisation+time+weather etc...
class KmlServer {
protected:
	/// Counts the number of complete messages written.
	int m_nb_broadcasts ;
	int exit_kml_server;
	int request_broadcast;
public:
	/// List of key-value pairs displayed for example in Google Earth balloons.
	struct CustomDataT : public std::vector< std::pair< std::string, std::string > > {
		/// Also used when reloading a KML file.
		void Push( const char * k, const std::string & v );

		/// TODO: Most of keys are duplicated. Should store them in a hash to reduce memory footprint.
		void Push( const char * k, const char * v ) {
			Push( k, std::string(v) );
		}
		/// This helper makes insertions simpler.
		template< class Type >
		void Push( const char * k, const Type & v ) {
			std::stringstream strm ;
			strm << v ;
			Push( k, strm.str() );
		}
	};

	KmlServer() : m_nb_broadcasts(0), exit_kml_server(0), request_broadcast(0) {}

	virtual ~KmlServer() {}

	/// BEWARE: Will work until 2038.
	static const time_t UniqueEvent = INT_MAX ;

	/// This can for example go to a NMEA client.
	virtual void Broadcast(
		const std::string       & category,
		time_t                    evtTim,
		const CoordinateT::Pair & refCoo,
		double                    altitude,
		const std::string       & kml_name,
		const std::string       & styleNam,
		const std::string       & descrTxt,
		const CustomDataT       & custDat ) = 0;

	/// Singleton.
	static KmlServer * GetInstance(void);

	/// Number of calls to Broadcast(). Debugging purpose only.
	int NbBroadcasts(void) const { return m_nb_broadcasts; }

	/// Debugging only, just to check what happens inside.
	void ResetCounter() { m_nb_broadcasts = 0;}

	virtual void Reset() = 0;

	/// TODO: Maybe have one display style per category, instead of the same for all.
	virtual void InitParams(
			const std::string & kml_command,
			const std::string & kml_dir,
			double              kml_merge = 10000,
			int                 kml_retention = 0, // Keep all data.
			int                 kml_refresh = 120,
			int                 kml_balloon_style = 0) = 0;

	virtual void ReloadKmlFiles() = 0;

	/// Creates the process for the command to be run when KML data is saved.
	static void SpawnProcess();

	/// Stops the sub-thread, flush KML data to the files.
	static void Exit();

	static std::string Tm2Time( time_t tim );
	/// No value means, now.
	static std::string Tm2Time(void);
};

#endif // _KMLSERVER_H
