/* -*-C++-*- 

   "$Id: Fl_Date.H,v 1.4 2000/03/30 04:43:56 davefreese Exp $"
   
   Copyright 1999-2000 by the Dave Freese & the Flek development team.
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.
   
   Please report all bugs and problems to "flek-devel@sourceforge.net".

*/
#ifndef DATE_H
#define DATE_H

#include <iosfwd>

typedef enum {
  SUNDAY,
  MONDAY,
  TUESDAY,
  WEDNESDAY,
  THURSDAY,
  FRIDAY,
  SATURDAY
} weekday_t;

typedef enum {
  JANUARY = 1,
  FEBRUARY,
  MARCH,
  APRIL,
  MAY,
  JUNE,
  JULY,
  AUGUST,
  SEPTEMBER,
  OCTOBER,
  NOVEMBER,
  DECEMBER
} month_t;

class Date {
  friend std::ostream &operator<<(std::ostream &, Date &);

protected:
  int  year;
  int  month;
  int  day;
  int  fmt;
  static const int mdays[];
  static const int jdays[][13];
  static const char *month_name[];
  void helpIncrement();

public:
  Date();
  Date( int m, int d, int y );

  void setDate( int, int, int );      // set the date
  void setDate( Date & );
  void setFormat (int);
  void today();                       // set date to the present day
  void Year( int );
  int Year();
  void Month( int );
  int Month();
  void Day( int );
  int Day();
  
  void previous_month ();
  void next_month ();
  void previous_year ();
  void next_year ();

  bool endOfMonth( int );
  
  bool leapYear (int);
  bool leapYear () { return leapYear (year); }
  bool isleapyear (int year) { return leapYear (year); }
  bool isleapyear () { return isleapyear (year); }
    
  int daysinmonth (int, int);
  int daysinmonth () 
    { return daysinmonth (month, isleapyear (year)); }

  bool isvalid (int, int, int);
  bool datevalid (int year, int mon, int day)
    { return isvalid (mon, day, year); }
  bool datevalid () 
    { return isvalid (month, day, year); }
  
  int dayofyear (int, int, int);
  int dayofyear () 
    { return dayofyear (year, month, day); }
  
  int dayofepoch (int, int, int);
  int dayofepoch () 
    { return dayofepoch (year, month, day); }
  
  int dayofweek (int, int, int);
  int dayofweek () 
    { return dayofweek (year, month, day); }
    
  char *szDate (int);
  char *szDate ();
  double dJulian ();
  
  bool operator==( const Date & );
  bool operator!=( const Date & );
  bool operator<( const Date & );
  bool operator>( const Date & );
  void operator=( const Date & );
  const Date &operator+=( int);        // add days, modify object
  Date &operator++();                  // pre-increment operator
  Date operator++( int );              // post-increment operator
  
};

#endif
