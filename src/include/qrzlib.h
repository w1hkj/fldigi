#ifndef QRZHEADER

// To include the email code remove the comment specifier from the following line
#define HAVE_EMAIL

// QRZ CDROM data structures

/*
**     Index Header Block Definition (Version 2)
**     (applies to all QRZ CDROMS from Version 2 onward) 
**
**     This block is located at the start of each index
*/

typedef struct {
  char  dataname[16];    /* Name of the data file            */
  char  bytesperkey[8];  /* Data Bytes per Index Item        */
  char  numkeys[8];      /* Number of items in this index    */
  char  keylen[8];       /* Length of each key item in bytes */
  char  version[8];      /* Database Version ID              */
} index_header;

/*

Index Usage

The name index is set to a maximum of 16 characters with longer names
being truncated.   Names are stored in last-first format with a space
between the names.  The city/state index uses 12 characters per entry,
the callsign index 6 characters and the zip code index 5 characters.

The data which follows the header is simply a long list of single field
records. The records are tightly packed on 'bytesperkey' boundaries
without separators.  There is no guarantee of a null terminator on any
index record entry.

When the program qrz.exe is run it first searches for a drive
containing the base directory \CALLBK .  Next, it loads all four index
files (callbkc.idx, callbkn.idx, callbks.idx and callbkz.idx) into
tables in memory.  These tables were kept small so as not to place an
undue RAM requirement on the user's system.

Next, when a user specifies a field and key to search, the program
searches the relevant index table and returns the closest match lower
(or equal to) the supplied key.  The table position of this key is then
taken and multiplied by the 'bytesperkey' value to arrive at a database
file offset.  This offset is then used to perform the first and only
seek into the database.  Once on position within the file, a sequential
search is performed to return the match.  The search terminates at the
next index key value if the field is not found.

The database files all have the same format.  The records each consist
of comma separated fields which end with a single newline '\n'  (ASCII
0xa) character.  Blank fields are simply stored as a comma.  Every
record has the same number of commas in it.  Actual comma's in the data
field are stored as a semi-colon ';' which should be replaced by a
comma in the user's output formatting routine.


Example:

AA7BQ ,LLOYD,,FRED L,,53340,90009,00009,8215 E WOOD DR,SCOTTSDALE,AZ,
85260,E,KJ6RK,A

The callsign database is sorted by SUFFIX, AREA, PREFIX.

For example, the following order would be observed:

QE24AA
...
ZZ99ZZ
...
A 0A  
...
AA0AAA
...
ZZ9ZZZ

This ordering also pertains to the index file since it is just a snaphot of
every nth record in the database.

*/

/*
**    Standard Record Field Offsets
*/
#define QRZLIB_CALL            0
#define QRZLIB_LNAME           1
#define QRZLIB_JR              2
#define QRZLIB_FNAME           3
#define QRZLIB_MI              4
#define QRZLIB_DOB             5
#define QRZLIB_EFDATE          6
#define QRZLIB_EXPDATE         7
#define QRZLIB_MAIL_STR        8
#define QRZLIB_MAIL_CITY       9
#define QRZLIB_MAIL_ST         10
#define QRZLIB_MAIL_ZIP        11
#define QRZLIB_CLASS           12
#define QRZLIB_P_CALL          13
#define QRZLIB_P_CLASS         14

/*

The fields JR and MI were obsoleted by the FCC in July 1994.

The callsign fields are arranged in a strict "ccdccc" columnar format
where 'c' represents a letter and 'd' a digit. Callsigns which do not
conform to the "ccdccc" format are space filled in the relevant
positions.  This field is rearranged to the proper layout by the user
program's output formatting routines.

All dates are stored in 5 character Julian format, e.g. 93003 equals
January 3, 1993.  Dates before 1900 or after year 2000 must be
determined by their context usage.  In other words, if the resultant
age does not make sense, then it's wrong.  For example, all licenses
expire in the future so 02 is 2002.  Birthdays are more difficult
but most can be determined to be greater than 10 years old.  This is
not a perfect method, but it does yield satisfactory results.

Some folks may notice that the database no longer contains station
location information.  This information is no longer supplied nor
available from the FCC since it is no longer a part of their record
keeping (See the May 1993 QST for more info).


Cross Reference Information

Callsigns in the database are now cross-referenced to both the current
and the previous call sign for each entry in which they are available.
A cross reference record takes the form of 'old,new' with no other
information in the record.  A record can be identified as a cross
reference either one of two ways:

    First, if the record length is less than 15 characters, then
    it is a cross reference record.

    Secondly, if the record contains only one comma "," , then
    it is a cross reference record.

It is not necessary to test for both cases, either will do.

When a cross reference record is encountered, you must fetch the second
field and restart the search to return the primary reference.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern char *Composite( char * );

class QRZ 
{
  private:
    char          criteria;
    index_header  idxhdr;
    char          *data;
    char          *index;
    char          *top;
    FILE          *idxfile;
    long          idxsize;
    FILE          *datafile;
    long          dataoffset;
    long          databytesread;
    char          *dfptr;
    char          *endofline;
    char          *idxptr;
    int           found;
    char          recbuffer[512];
    unsigned int  datarecsize;
    long          numkeys;
    int           keylen;
    void          OpenQRZFiles( const char * );
    int           FindCallsign( char * );
    int           FindName( char * );
    int           FindState( char * ); 
    int           FindZip( char * );
    int           ReadDataBlock( long );
    int           nextrec();
	bool		  hasImage;
        
    char *Qcall;
    char *Qlname;
    char *Qfname;
    char *Qdob;
    char *Qefdate;
    char *Qexpdate;
    char *Qmail_str;
    char *Qmail_city;
    char *Qmail_st;
    char *Qmail_zip;
    char *Qopclass;
    char *Qp_call;
    char *Qimagefname;
    char *Qp_class;

    int  QRZvalid;
        
  public:
    QRZ( const char * );
    QRZ( const char *, char );
    ~QRZ();

    int  CallComp( char *, char * );
    int  CompState( const char *, const char *, const char * );

    int  getQRZvalid();
    void NewDBpath( const char * );

    int  FindRecord( char * );
    int  NextRecord();
    int  ReadRec();
    int  GetCount( char * );
    char *GetCall();
    const char *GetLname();
    const char *GetFname();
    const char *GetDOB();
    const char *GetEFdate();
    const char *GetEXPdate();
    const char *GetStreet();
    const char *GetCity();
    const char *GetState();
    const char *GetZIP();
    const char *GetOPclass();
    const char *GetPriorCall();
    const char *GetPriorClass();
    const char *GetImageFileName();
    char *CSV_Record();
    char *Fmt_Record();
	bool ImageExists();
	const char *ImageFileName() {return Qimagefname;};
};

extern void SetQRZdirectory(char *dir);
extern int  filename_expand(char *to,int tolen, const char *from);

#define QRZHEADER

#endif
