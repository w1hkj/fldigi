// ----------------------------------------------------------------------------
// coordinate.cxx  --  Handling of longitude and latitude.
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

#include <math.h>
#include <string.h>
#include <stdexcept>
#include <iomanip>
#include <sstream>

#include "config.h"
#include "coordinate.h"
#include "locator.h"

void CoordinateT::Check(void) const
{
	if( m_is_lon ) {
		if( ( m_angle >= -180.0 ) && ( m_angle <= 180.0 ) ) return ;
	} else {
		if( ( m_angle >=  -90.0 ) && ( m_angle <=  90.0 ) ) return ;
	}
	std::stringstream strm ;
	strm << "Invalid m_angle=" << m_angle << " m_is_lon=" << m_is_lon ;
	throw std::runtime_error(strm.str());
}

CoordinateT::CoordinateT( double degrees, bool is_lon )
: m_angle( fmod(degrees, 360.0 ) ), m_is_lon(is_lon)
{
	if( m_angle > 180.0 ) m_angle -= 360.0 ;
	Check();
};

// Longitude East and Latitude North are positive.
void CoordinateT::Init( char direction, double angle_degrees )
{
	m_angle = angle_degrees ;
	switch( direction )
	{
		case 'W':
		case 'w':
			m_angle = -m_angle ;
		case 'E':
		case 'e':
			if( ( angle_degrees < -180 ) || ( angle_degrees > 180 ) )
				throw std::runtime_error("Invalid longitude degree");
			m_is_lon = true ;
			break ;
		case 'S':
		case 's':
			m_angle = -m_angle ;
		case 'N':
		case 'n':
			if( ( angle_degrees < -90 ) || ( angle_degrees > 90 ) )
				throw std::runtime_error("Invalid latitude degree");
			m_is_lon = false ;
			break ;
		default:
			throw std::runtime_error("Invalid direction");
	}
	Check();
}

CoordinateT::CoordinateT( char direction, double angle_degrees ) {
	Init( direction, angle_degrees );
}

CoordinateT::CoordinateT( char direction, int degree, int minute, int second )
{
	// std::cout << "ctor d=" << direction << " " << degree << " " << minute << " " << second << "\n";
	if( ( degree < 0 ) || ( degree > 180 ) )
		throw std::runtime_error("Invalid degree");

	if( ( minute < 0 ) || ( minute >= 60 ) )
		throw std::runtime_error("Invalid minute");

	if( ( second < 0 ) || ( second >= 60 ) )
		throw std::runtime_error("Invalid second");

	double angle_degrees = (double)degree + (double)minute / 60.0 + (double)second / 3600.0 ;
	Init( direction, angle_degrees );
}

// Specific for reading from the file of navtex or wmo stations.
// Navtex: "57 06 N"
// Wmo   : "69-36N", "013-27E", "009-25E" ou floating-point degrees: "12.34 E".
// Station Latitude or Latitude :DD-MM-SSH where DD is degrees, MM is minutes, SS is seconds 
// and H is N for northern hemisphere or S for southern hemisphere or
// E for eastern hemisphere or W for western hemisphere.
// The seconds value is omitted for those stations where the seconds value is unknown.
std::istream & operator>>( std::istream & istrm, CoordinateT & ref )
{
	if( ! istrm ) return istrm ;

	std::stringstream sstrm ;

	char direction ;
	while( true ) {
		// istrm >> direction ;
		direction = (char)istrm.get();
		if( ! istrm ) return istrm ;
		switch( direction ) {
			case 'e':
			case 'E':
			case 'w':
			case 'W':
			case 's':
			case 'S':
			case 'n':
			case 'N':
				break;
			case '0' ... '9' :
			case '.' :
			case '-' :
			case '+' :
			case ' ' :
			case '\t' :
				sstrm << direction ;
				continue;
			default:
				istrm.setstate(std::ios::eofbit);
				return istrm ;
		}
		break;
	}
	// TODO: Check that the direction is what we expect.

	std::string tmpstr = sstrm.str();
	// std::cout << "READ:" << tmpstr << ":" << direction << "\n";

	const char * tmpPtr = tmpstr.c_str();
	int i_degree, i_minute, i_second ;
	if( ( 3 == sscanf( tmpPtr, "%d-%d-%d", &i_degree, &i_minute, &i_second ) )
	||  ( 3 == sscanf( tmpPtr, "%d %d %d", &i_degree, &i_minute, &i_second ) ) ) {
		ref = CoordinateT( direction, i_degree, i_minute, i_second );
		return istrm;
	} 

	if( ( 2 == sscanf( tmpPtr, "%d-%d", &i_degree, &i_minute ) )
	||  ( 2 == sscanf( tmpPtr, "%d %d", &i_degree, &i_minute ) ) ) {
		ref = CoordinateT( direction, i_degree, i_minute, 0 );
		return istrm;
	} 
	
	double d_degree ;
	if( 1 == sscanf( tmpPtr, "%lf", &d_degree ) ) {
		ref = CoordinateT( direction, d_degree );
		return istrm;
	}

	istrm.setstate(std::ios::eofbit);
	return istrm ;
}

std::ostream & operator<<( std::ostream & ostrm, const CoordinateT & ref )
{
	bool sign = ref.m_angle > 0 ;
	double ang = sign ? ref.m_angle : -ref.m_angle;

	ostrm << std::setfill('0') << std::setw( ref.m_is_lon ? 3 : 2 ) << (int)ang << "°"
		<< std::setfill('0') << std::setw(2) << ( (int)( 0.5 + ang * 60.0 ) % 60 ) << "'"
		<< std::setfill('0') << std::setw(2) << (int)fmod( ang * 3600.0, 60 ) << "''"
		<< " ";
	ostrm << ( ref.m_is_lon ? sign ? 'E' : 'W' : sign ? 'N' : 'S' );
	return ostrm;
}

CoordinateT::Pair::Pair( const CoordinateT & coo1, const CoordinateT & coo2 )
: m_lon( coo1.is_lon() ? coo1 : coo2 )
, m_lat( coo2.is_lon() ? coo1 : coo2 )
{
	if( ! ( coo1.is_lon() ^ coo2.is_lon() ) )
	{
		throw std::runtime_error("Internal inconsistency");
	}
}

CoordinateT::Pair::Pair( double lon, double lat )
: m_lon( CoordinateT( lon, true  ) )
, m_lat( CoordinateT( lat, false ) ) {}

CoordinateT::Pair::Pair( const std::string & locator )
{
	double lon, lat ;
	int res = locator2longlat( &lon, &lat, locator.c_str() );
	if( res != RIG_OK ) {
		throw std::runtime_error("Cannot decode Maidenhead locator:" + locator );
	};
	m_lon = CoordinateT( lon, true  );
	m_lat = CoordinateT( lat, false );
}

double CoordinateT::Pair::distance( const Pair & a ) const
{
	double dist, azimuth ;
	int res = qrb(
		longitude().angle(), latitude().angle(),
		a.longitude().angle(), a.latitude().angle(),
		&dist, &azimuth );
	if( res != RIG_OK) {
		std::stringstream sstrm ;
		sstrm << "Bad qrb result:" << *this << " <-> " << a ;
		throw std::runtime_error(sstrm.str());
	}
	return dist ;
}

std::string CoordinateT::Pair::locator(void) const
{
	char buf[64];
	int ret = longlat2locator(
			longitude().angle(),
			latitude().angle(),
			buf,
			3 );

	if( ret == RIG_OK ) {
		return buf ;
	}
	return std::string();
}

std::ostream & operator<<( std::ostream & ostrm, const CoordinateT::Pair & ref )
{
	ostrm << ref.latitude() << "/" << ref.longitude();
	return ostrm;
}

std::istream & operator>>( std::istream & istrm, CoordinateT::Pair & ref )
{
	istrm >> ref.latitude() >> ref.longitude();
	return istrm;
}


