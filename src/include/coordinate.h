// ----------------------------------------------------------------------------
// coordinate.h  --  Handling of longitude and latitude.
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

#ifndef _COORDINATE_H
#define _COORDINATE_H

#include <iostream>

// Models a longitude or latitude.
class CoordinateT
{
	// Precision is: 360 * 3600 = 1296000, 21 bits. A float might be enough.
	double m_angle ;  // In decimal degrees, between -180.0 and 180.0.
	bool   m_is_lon ; // Longitude or latitude.
	// TODO: Consider adding a big offset to m_angle, instead of an extra flag.

	void Check(void) const ;

	void Init( char direction, double degrees );
public:
	CoordinateT(bool ll=true)
	: m_angle(0.0), m_is_lon(ll) {};

	CoordinateT( double degrees, bool is_lon );

	CoordinateT( char direction, double degrees );

	CoordinateT( char direction, int degree, int minute, int second );

	double angle(void) const { return m_angle ; }
	bool is_lon(void) const { return m_is_lon; }

	// Specific for reading from the file of navtex or wmo stations.
	// Navtex: "57 06 N"
	// Wmo   : "69-36N", "013-27E", "009-25E"
	friend std::istream & operator>>( std::istream & istrm, CoordinateT & ref );

	friend std::ostream & operator<<( std::ostream & ostrm, const CoordinateT & ref );

	class Pair ;
}; // CoordinateT

// Longitude , latitude.
class CoordinateT::Pair
{
	CoordinateT m_lon, m_lat ;
public:
	Pair() {}

	Pair( const CoordinateT & coo1, const CoordinateT & coo2 );

	Pair( double lon, double lat );

	Pair( const std::string & locator );

	CoordinateT longitude() const { return m_lon ; }

	CoordinateT latitude() const { return m_lat ; }

	CoordinateT & longitude() { return m_lon ; }

	CoordinateT & latitude() { return m_lat ; }

	double distance( const Pair & a ) const;

	std::string locator() const ;

	friend std::istream & operator>>( std::istream & istrm, Pair & ref );

	friend std::ostream & operator<<( std::ostream & ostrm, const Pair & ref );
}; // CoordinateT::Pair


#endif // _COORDINATE_H
