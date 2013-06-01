// ----------------------------------------------------------------------------
// record_loader.h
//
// Copyright (C) 2013
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
#ifndef RECORD_LOADER_H
#define RECORD_LOADER_H

#include <iostream>
#include <string>

/** This wraps the logic of loading a datafile made of distinct records.
 * This class is used for loading files belonging to the user, which
 * are not part of the fldigi distribution and are not downloaded
 * from the support web sites not stored in the fldigi directories.
 */
class RecordLoaderContainer
{
public:
	virtual void Clear() = 0 ;

	/// Reads just one record.
	virtual bool ReadRecord( std::istream & ) = 0 ;

	/// Loads a file and stores it for later lookup. Returns the number of records, or -1.
	int LoadFromFile(const std::string & filnam );

}; // RecordLoaderContainer

/// This adds the logic of a data file downloaded from the web and locally stored.
class RecordLoaderInterface : public virtual RecordLoaderContainer
{
	/// No assignement, this would break the unique registering of singletons.
	RecordLoaderInterface & operator=( const RecordLoaderInterface & );

public:
	RecordLoaderInterface();

	/// Loads a file and stores it for later lookup. Returns the number of records, or -1.
	int LoadAndRegister();

	std::string ContentSize() const ;

	virtual ~RecordLoaderInterface();

	/// Base name. In case the URL is not nice enough to build a clean data filename.
	virtual std::string base_filename() const ;

	/// The place where we store the data locally.
	std::pair< std::string, bool > storage_filename(bool create_dir = false) const ;

	virtual const char * Url() const { return NULL; }

	virtual const char * Description() const = 0 ;

	std::string Timestamp() const;

	static void SetDataDir( const std::string & data_dir );
}; // RecordLoaderInterface

/// Loads records from a file or an Url.
template< class Catalog >
class RecordLoaderSingleton
{
	static Catalog s_cata_inst ;
public:
	/// This calls a constructor which register the object in a global array.
	static Catalog & InstCatalog() {
		return s_cata_inst ;
	}
};

/// This ensures that an object is created at startup time.
template<class Catalog> Catalog RecordLoaderSingleton< Catalog >::s_cata_inst = Catalog();

/// Loads tabular records from a file.
template< class Catalog >
struct RecordLoader
: public virtual RecordLoaderInterface
, public RecordLoaderSingleton<Catalog>
{
}; // RecordLoader

void createRecordLoader();


// ----------------------------------------------------------------------------

/// This is a read-only replacement for std::stringstream.
struct imemstream : public std::streambuf, public std::istream {
	/// Faster than stringstream because no copy.
	imemstream(char* s, std::size_t n) : std::istream( this )
	{
		setg(s, s, s + n);
	}
	/// Faster than stringstream because no copy.
	imemstream(const std::string & r) : std::istream( this )
	{
		char * s = const_cast< char * >( r.c_str() );
		setg(s, s, s + r.size());
	}
};
// ----------------------------------------------------------------------------

/// Tells if type is a char[]. Used for SFINAE.
template< class T >
struct DtTyp {
	/// In the general case, data types are not char arrays.
	struct Any {};
};

/// Matches if the type is a char[].
template< size_t N >
struct DtTyp< char[N] > {
	struct Array {};
	static const size_t Size = N ;
};

/// Reads all chars until after the delimiter.
bool read_until_delim( char delim, std::istream & istrm );

/// Reads a char followed by the delimiter.
bool read_until_delim( char delim, std::istream & istrm, char & ref, const char dflt );

/// Reads a double up to the given delimiter.
bool read_until_delim( char delim, std::istream & istrm, double & ref );

/// Reads a string up to the given delimiter.
bool read_until_delim( char delim, std::istream & istrm, std::string & ref );

/// For reading from a string with tokens separated by a char. Used to load CSV files.
template< typename Tp >
bool read_until_delim( char delim, std::istream & istrm, Tp & ref, typename DtTyp< Tp >::Any = typename DtTyp< Tp >::Any() )
{
	std::string parsed_str ;
	if( ! std::getline( istrm, parsed_str, delim ) ) {
		return false ;
	}
	imemstream sstrm( parsed_str );
	sstrm >> ref ;
	return sstrm ;
}

/// Same, with a default value if there is nothing to read.
template< typename Tp >
bool read_until_delim( char delim, std::istream & istrm, Tp & ref, const Tp dflt, typename DtTyp< Tp >::Any = typename DtTyp< Tp >::Any() )
{
	std::string parsed_str ;
	if( ! std::getline( istrm, parsed_str, delim ) ) {
		return false ;
	}
	if( parsed_str.empty() ) {
		ref = dflt ;
		return true;
	}
	imemstream sstrm( parsed_str );
	sstrm >> ref ;
	return sstrm ;
}

/// For reading from a string with tokens separated by a char to a fixed-size array.
template< typename Tp >
bool read_until_delim( char delim, std::istream & istrm, Tp & ref, typename DtTyp< Tp >::Array = typename DtTyp< Tp >::Array() )
{
	istrm.getline( ref, DtTyp< Tp >::Size, delim );
	// Should we return an error if buffer is too small?
	return istrm ;
}

/// Same, with a default value if there is nothing to read. Fixed-size array.
template< typename Tp >
bool read_until_delim( char delim, std::istream & istrm, Tp & ref, const Tp dflt, typename DtTyp< Tp >::Array = typename DtTyp< Tp >::Array() )
{
	istrm.getline( ref, DtTyp< Tp >::Size, delim );
	// If nothing to read, copy the default value.
	if( ref[0] == '\0' ) {
		strncpy( ref, dflt, DtTyp< Tp >::Size - 1 );
	}
	// Should we return an error if buffer is too small?
	return istrm;
}




#endif // RECORD_LOADER_H
