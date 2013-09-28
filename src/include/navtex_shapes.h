//
//	navtex_shapes.h
//
// Copyright (C) 2013
//		Remi Chateauneu, F4ECW
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#ifndef _NAVTEX_SHAPES_H
#define _NAVTEX_SHAPES_H

#include <map>
#include <string>

#include "coordinate.h"

/// This extracts from raw text, single coordinates, paths, geographic shapes.
struct ShapesSetT : std::map< std::string, CoordinatesShapeT >
{
	ShapesSetT( const std::string & rawtText );
};

#endif // _NAVTEX_SHAPES_H
