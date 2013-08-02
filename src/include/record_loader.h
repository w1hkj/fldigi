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

#include <iosfwd>
#include <string>

class RecordLoaderInterface
{
	RecordLoaderInterface & operator=( const RecordLoaderInterface & );

public:
	RecordLoaderInterface();

	virtual void Clear() = 0 ;

	/// Reads just one record.
	virtual bool ReadRecord( std::istream & ) = 0 ;

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
	static Catalog & InstCatalog() {
		return s_cata_inst ;
	}
};

template<class Catalog> Catalog RecordLoaderSingleton< Catalog >::s_cata_inst = Catalog();

/// Loads tabular records from a file.
template< class Catalog >
struct RecordLoader : public RecordLoaderInterface, public RecordLoaderSingleton<Catalog>
{
}; // RecordLoader

void createRecordLoader();

#endif // RECORD_LOADER_H
