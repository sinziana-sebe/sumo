/****************************************************************************/
/// @file    Boundary.cpp
/// @author  Daniel Krajzewicz
/// @date    Sept 2002
/// @version $Id$
///
// A class that stores the 2D geometrical boundary
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
// copyright : (C) 2001-2007
//  by DLR (http://www.dlr.de/) and ZAIK (http://www.zaik.uni-koeln.de/AFS)
/****************************************************************************/
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/
// ===========================================================================
// compiler pragmas
// ===========================================================================
#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif


// ===========================================================================
// included modules
// ===========================================================================
#ifdef WIN32
#include <windows_config.h>
#else
#include <config.h>
#endif
#include <utility>

#include "GeomHelper.h"
#include "Boundary.h"
#include "Position2DVector.h"
#include "Position2D.h"

#ifdef CHECK_MEMORY_LEAKS
#include <foreign/nvwa/debug_new.h>
#endif // CHECK_MEMORY_LEAKS


// ===========================================================================
// method definitions
// ===========================================================================
Boundary::Boundary()
        : _xmin(10000000000.0), _xmax(-10000000000.0),
        _ymin(10000000000.0), _ymax(-10000000000.0),
        myWasInitialised(false)
{}


Boundary::Boundary(SUMOReal x1, SUMOReal y1, SUMOReal x2, SUMOReal y2)
        : _xmin(10000000000.0), _xmax(-10000000000.0),
        _ymin(10000000000.0), _ymax(-10000000000.0),
        myWasInitialised(false)
{
    add(x1, y1);
    add(x2, y2);
}


Boundary::~Boundary()
{}


void
Boundary::reset()
{
    _xmin = 10000000000.0;
    _xmax = -10000000000.0;
    _ymin = 10000000000.0;
    _ymax = -10000000000.0;
    myWasInitialised = false;
}


void
Boundary::add(SUMOReal x, SUMOReal y)
{
    if(!myWasInitialised) {
        _ymin = y;
        _ymax = y;
        _xmin = x;
        _xmax = x;
    } else {
        _xmin = _xmin < x ? _xmin : x;
        _xmax = _xmax > x ? _xmax : x;
        _ymin = _ymin < y ? _ymin : y;
        _ymax = _ymax > y ? _ymax : y;
    }
    myWasInitialised = true;
}


void
Boundary::add(const Position2D &p)
{
    add(p.x(), p.y());
}


void
Boundary::add(const Boundary &p)
{
    add(p.xmin(), p.ymin());
    add(p.xmax(), p.ymax());
}


Position2D
Boundary::getCenter() const
{
    return Position2D((_xmin+_xmax)/(SUMOReal) 2.0, (_ymin+_ymax)/(SUMOReal) 2.0);
}


SUMOReal
Boundary::xmin() const
{
    return _xmin;
}


SUMOReal
Boundary::xmax() const
{
    return _xmax;
}


SUMOReal
Boundary::ymin() const
{
    return _ymin;
}


SUMOReal
Boundary::ymax() const
{
    return _ymax;
}


SUMOReal
Boundary::getWidth() const
{
    return _xmax - _xmin;
}


SUMOReal
Boundary::getHeight() const
{
    return _ymax - _ymin;
}


bool
Boundary::around(const Position2D &p, SUMOReal offset) const
{
    return
        (p.x()<=_xmax+offset && p.x()>=_xmin-offset) &&
        (p.y()<=_ymax+offset && p.y()>=_ymin-offset);
}


bool
Boundary::overlapsWith(const AbstractPoly &p, SUMOReal offset) const
{
    if (
        // check whether one of my points lies within the given poly
        partialWithin(p, offset) ||
        // check whether the polygon lies within me
        p.partialWithin(*this, offset)) {
        return true;
    }
    // check whether the bounderies cross
    return
        p.crosses(Position2D(_xmax+offset, _ymax+offset), Position2D(_xmin-offset, _ymax+offset))
        ||
        p.crosses(Position2D(_xmin-offset, _ymax+offset), Position2D(_xmin-offset, _ymin-offset))
        ||
        p.crosses(Position2D(_xmin-offset, _ymin-offset), Position2D(_xmax+offset, _ymin-offset))
        ||
        p.crosses(Position2D(_xmax+offset, _ymin-offset), Position2D(_xmax+offset, _ymax+offset));
}


bool
Boundary::crosses(const Position2D &p1, const Position2D &p2) const
{
    return
        GeomHelper::intersects(p1, p2, Position2D(_xmax, _ymax), Position2D(_xmin, _ymax))
        ||
        GeomHelper::intersects(p1, p2, Position2D(_xmin, _ymax), Position2D(_xmin, _ymin))
        ||
        GeomHelper::intersects(p1, p2, Position2D(_xmin, _ymin), Position2D(_xmax, _ymin))
        ||
        GeomHelper::intersects(p1, p2, Position2D(_xmax, _ymin), Position2D(_xmax, _ymax));
}


bool
Boundary::partialWithin(const AbstractPoly &poly, SUMOReal offset) const
{
    return
        poly.around(Position2D(_xmax, _ymax), offset) ||
        poly.around(Position2D(_xmin, _ymax), offset) ||
        poly.around(Position2D(_xmax, _ymin), offset) ||
        poly.around(Position2D(_xmin, _ymin), offset);
}


Boundary &
Boundary::grow(SUMOReal by)
{
    _xmax += by;
    _ymax += by;
    _xmin -= by;
    _ymin -= by;
    return *this;
}


void
Boundary::flipY()
{
    _ymin *= -1.0;
    _ymax *= -1.0;
    SUMOReal tmp = _ymin;
    _ymin = _ymax;
    _ymax = tmp;
}



std::ostream &
operator<<(std::ostream &os, const Boundary &b)
{
    os << b._xmin << "," << b._ymin << "," << b._xmax << "," << b._ymax;
    return os;
}


void
Boundary::set(SUMOReal xmin, SUMOReal ymin, SUMOReal xmax, SUMOReal ymax)
{
    _xmin = xmin;
    _ymin = ymin;
    _xmax = xmax;
    _ymax = ymax;
}


void
Boundary::moveby(SUMOReal x, SUMOReal y)
{
    _xmin += x;
    _ymin += y;
    _xmax += x;
    _ymax += y;
}



/****************************************************************************/

