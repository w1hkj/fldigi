// ----------------------------------------------------------------------------
//
// Date.cxx  date class for Fast Light Took Kit
//
// Copyright (C) 1998 David Freese
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

#include <iostream>
#include <cstring>
#include <ctime>
#include <cstdio>

#include "date.h"

using namespace std;

const int Date::mdays[] = 
  { 0, 31, 28, 31, 30,  31,  30,  31,  31,  30,  31,  30, 31 };

const int Date::jdays[2][13] = {
  { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
  { 0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
};

const char *Date::month_name[] =
{
  "January",
  "Febuary",
  "March",
  "April",
  "May",
  "June",
  "July",
  "August",
  "September",
  "October",
  "November",
  "December"
};

void Date::today()
{
  time_t t;
  struct tm *now;
  time( &t );
  now = localtime( &t );
  year = now->tm_year + 1900;
  month = now->tm_mon + 1;
  day = now->tm_mday;
}

Date::Date()
{
  today();
  fmt = 0;
}

Date::Date( int m, int d, int y )
{
  setDate( m, d, y );
  fmt = 1;
}

void Date::setDate( int mm, int dd, int yy )
{
  if( isvalid( mm, dd, yy ) ) {
    year = yy; month = mm; day = dd;
  } else
    today();
}

void Date::setDate( Date &dt )
{
  year = dt.year;
  month = dt.month;
  day = dt.day;
}

void Date::setFormat( int iFmt )
{
  fmt = iFmt;
}

void Date::Year( int y )
{
  year = y;
}

int Date::Year()
{
  return year;
}

void Date::Month( int m )
{
  month = m;
}

int Date::Month()
{
  return month;
}

void Date::Day( int d )
{
  day = d;
}

int Date::Day()
{
  return day;
}

bool Date::leapYear( int y )
{
  if( y % 400 == 0 || ( y % 100 != 0 && y % 4 == 0 ) )
    return true;
  return false;
}


bool Date::isvalid( int m, int d, int y )
{
  if( y > 2035 ) return false;
  if( m < 1 || m > 12 ) return false;
  if( d < 1 ) return false;
  if( leapYear( y ) ){
    if( m == 2 && d > 29 )
      return false;
    else
      return true;
  }
  if( d > mdays[m] ) return false;
  return true;
}

int Date::daysinmonth (int month, int leap)
{
  /* Validate the month. */
  if (month < JANUARY || month > DECEMBER)
    return -1;
  
  /* Return 28, 29, 30, or 31 based on month/leap. */
  switch (month) {
    case FEBRUARY:
      return leap ? 29 : 28;
    default:
      return mdays[month];
  }
}


int Date::dayofyear (int year, int mon, int mday)
{
  /* Return day of year. */
  return mday + jdays[isleapyear (year) ? 1 : 0][mon];
}

int Date::dayofepoch (int year, int mon, int mday)
{
  int  doe;
  int  era, cent, quad, rest;
  
  /* break down the year into 400, 100, 4, and 1 year multiples */
  rest = year - 1;
  quad = rest / 4;        rest %= 4;
  cent = quad / 25;       quad %= 25;
  era = cent / 4;         cent %= 4;
  
  /* set up doe */
  doe = dayofyear (year, mon, mday);
  doe += era * (400 * 365 + 97);
  doe += cent * (100 * 365 + 24);
  doe += quad * (4 * 365 + 1);
  doe += rest * 365;
  
  return doe;
}

int Date::dayofweek (int year, int mon, int mday)
{
  return dayofepoch (year, mon, mday) % 7;
}

void Date::previous_month ()
{
  if (month == 1)
    {
      month = 12;
      year--;
    }
  else
    month--;

  while ((day >= 1) && (!datevalid ()))
    day--;
}

void Date::next_month ()
{
  if (month == 12)
    {
      month = 1;
      year++;
    }
  else
    month++;

  while ((day >= 1) && (!datevalid ()))
    day--;
}

void Date::previous_year ()
{
  if (month == 2 && day == 29)
    day = 28;
  year--;
}

void Date::next_year ()
{
  if (month == 2 && day == 29)
    day = 28;
  year++;
}


char *Date::szDate (int fmt)
{
  static char temp[20];
  char        szMonth[10];
  switch (fmt) {
    case 1 :
      snprintf (temp, sizeof(temp), "%02d/%02d/%02d",
      month, 
      day, 
      year > 1999 ? year - 2000 : year - 1900);
      break;
    case 2 :
      snprintf (temp, sizeof(temp), "%4d%02d%02d", year, month, day);
      break;
    case 3 :  
      snprintf (temp, sizeof(temp), "%s %2d, %4d",
      month_name[month - 1], 
      day, 
      year);
      break;
    case 4 :
      strcpy (szMonth, month_name [month - 1]);
      szMonth[3] = 0; 
      snprintf (temp, sizeof(temp), "%s %2d, %4d", szMonth, day, year);
      break;
    case 5 :
      strcpy (szMonth, month_name [month - 1]);
      szMonth[3] = 0; 
      snprintf (temp, sizeof(temp), "%02d %s %4d", day, szMonth, year);
      break;
    case 0 :
    default :
      snprintf (temp, sizeof(temp), "%02d/%02d/%04d",
      month, 
      day,
      year); 
      break;
  }      
  return temp;
}

char *Date::szDate ()
{
  return szDate (fmt);
}

// operator functions

ostream &operator<<( ostream &output, Date &d )
{
  output << d.szDate ();
  return output;
}

bool Date::endOfMonth( int d )
{
  if( month == 2 && leapYear( year ) )
    return (d == 29 );  // last day of Feb in leap year
  else
    return (d == mdays[ month ] );
}

void Date::helpIncrement()
{
  if( endOfMonth( day ) && month == 12 ) {  // end year
    day = 1;
    month = 1;
    ++year;
  } else if ( endOfMonth( day ) ) {
    day = 1;
    ++month;
  } else
    ++day;
}

Date &Date::operator++()
{
  helpIncrement();
  return *this;     // reference return to create an lvalue
}

Date Date::operator++( int )
{
  Date temp = *this;
  helpIncrement();
  return temp;     // return non-increment, saved temporary object
}

const Date &Date::operator+=( int ndays )
{
  for( int i = 0; i < ndays; i++ )
    helpIncrement();
  return *this;    // enables cascading
}


bool Date::operator==( const Date &d )
{
  if( this->year != d.year ) return false;
  if( this->month != d.month ) return false;
  if( this->day != d.day ) return false;
  return true;
}

bool Date::operator!=( const Date &d )
{
  return ( !( *this == d ) );
}

bool Date::operator<( const Date &d )
{
  if( this->year < d.year ) return true;
  if( this->year > d.year ) return false;
  if( this->month < d.month ) return true;
  if( this->month > d.month ) return false;
  if( this->day < d.day ) return true;
  return false;
}

bool Date::operator>( const Date &d )
{
  if( *this < d ) return false;
  if( *this == d ) return false;
  return true;
}

void Date::operator=( const Date &d )
{
  this->year = d.year;
  this->month = d.month;
  this->day = d.day;
}

double Date::dJulian ()
{
  int DaysInYear = 365;
  if (leapYear ()) DaysInYear++;
  return ( year + 1.0 * (dayofyear (year, month, day) - 1) / DaysInYear);
}
