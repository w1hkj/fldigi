// ----------------------------------------------------------------------------
// synop.h  --  SYNOP decoding
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

#ifndef _SYNOP_H
#define _SYNOP_H

// This tells how Synop data are serialized.
class synop_callback {
public:
	virtual ~synop_callback() {} ;

	// These methods could as well be pure virtual.
	virtual bool interleaved(void) const { return true; }
	virtual void print( const char * str, size_t nb, bool ) const  = 0;
	virtual bool log_adif(void) const = 0;
	virtual bool log_kml(void) const = 0 ;
};


// Implementation hidden in synop.cxx
class synop {
	// When set, the output does not contain Synop sentences but only
	// the name of the regular expression which matched. It helps
	// for debugging because the output is independent of the locale.
	static bool m_test_mode ;
public:

	static const synop_callback * ptr_callback ;

	template< class Callback >
	static void setup()
	{
		static const Callback cstCall = Callback();
		ptr_callback = &cstCall ;
	};

	static synop * instance();

	static void regex_usage(void);

	virtual ~synop() {};

	// It is used as a global object, the constructor does not do anything.
	virtual void init() = 0;

	virtual void cleanup() = 0;

	/// We should have a tempo as well.
	virtual void add(char c) = 0;

	// When Synop decoding is disabled.
	virtual void flush(bool finish_decoding) = 0;

	virtual bool enabled(void) const = 0;

	static bool GetTestMode(void) { return m_test_mode ; };
	static void SetTestMode(bool test_mode) { m_test_mode = test_mode ; };
};

// gathers the various data files used for Synop decoding.
struct SynopDB {
	// Loads the files from s given directory.
	static bool Init( const std::string & data_dir );

	// For testing purpose.
	static const std::string & IndicatorToName( int wmo_indicator );
	static const std::string IndicatorToCoordinates( int wmo_indicator );

	// To Test the reading of our weather stations data files.
	static const std::string & BuoyToName( const char * buoy_id );
	static const std::string & ShipToName( const char * ship_id );
	static const std::string & JCommToName( const char * ship_id );
};

// ----------------------------------------------------------------------------

#endif // _SYNOP_H
