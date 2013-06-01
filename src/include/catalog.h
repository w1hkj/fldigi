// ----------------------------------------------------------------------------
// catalog.h
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

#ifndef CATALOG_H
#define CATALOG_h

#include <map>
#include <iostream>
#include <typeinfo>

#include "debug.h"
#include "record_loader.h"

/// This wraps a record type and allows to load a cvs file and access it using a key.
template< class Key, class Record, Key (Record::*Method)(void) const, class Terminal >
class Catalog : public RecordLoader< Terminal >
{
	/// The keying method might return a reference instead of a value.
	template< class Type > struct deref { typedef Type type ; };
	template< class Type > struct deref< Type & > { typedef Type type ; };

	/// If the index function return value is for example a string reference then KeyType is a string.
	template< class Type > struct deref< const Type & > { typedef Type type ; };
protected:
	/// The key is a method of the record type.
	typedef typename deref< Key >::type KeyType ;

	/// The key must be unique.
	typedef std::map< KeyType, Record > CatalogType ;
	typedef typename CatalogType::iterator IteratorType ;

	/// This stores all the object loaded from a file.
	CatalogType m_catalog ;

	bool FillAndTest() {
		int nbRec = this->LoadAndRegister();
		if( nbRec < 0 ) {
			return false ;
		}
		else {
			LOG_INFO("record=%s nb_recs=%d", typeid(Record).name(), nbRec );
			return true ;
		}
	}
public:
	void Clear() {
		m_catalog.clear();
	}

	/** TODO: Decouple record reading, then insertion. This will allow
	 * several indices on the same data copy, with a shared record.
	 * This can be done by creating a handle to a record and having
	 * two different virtual methods: One reading the record and returning an abstract
	 * handle, and another one storing the abstract handle in the map.
	 * By default, ReadRecord will call the two, as it does now.
	 * But this allow to read once, and store several times in several containers,
	 * each of them with a different index.
	 */
	bool ReadRecord( std::istream & istrm ) {
		Record tmp ;
		istrm >> tmp ;
		if( istrm || istrm.eof() ) {
			m_catalog[ (tmp.*Method)() ] = tmp ;
			return true ;
		}
		return false;
	}

	/// Returns a NULL pointer if the key cannot be found.
	static const Record * FindFromKey( Key key )
	{
		// This uses an intermediary typed variable, otherwise
		// there is an ambiguity if this is a multiply-indexed catalog.
		Terminal & refTerm = RecordLoader< Terminal >::InstCatalog();
		Catalog & refCtlg = refTerm ;
		CatalogType & refCat = refCtlg.m_catalog;

		typename CatalogType::const_iterator it = refCat.find( key );
		return ( it == refCat.end() ) ? NULL : &it->second ;
	}

	bool Fill() {
		return FillAndTest();
	}

}; // Catalog


#endif // CATALOG_h
