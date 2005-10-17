//---------------------------------------------------------------------------//
//                        DistributionCont.cpp -
//  A container for distributions
//                           -------------------
//  project              : SUMO - Simulation of Urban MObility
//  begin                : Sept 2002
//  copyright            : (C) 2002 by Daniel Krajzewicz
//  organisation         : IVF/DLR http://ivf.dlr.de
//  email                : Daniel.Krajzewicz@dlr.de
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//---------------------------------------------------------------------------//
namespace
{
    const char rcsid[] =
    "$Id$";
}
// $Log$
// Revision 1.8  2005/10/17 09:23:35  dkrajzew
// c4503 warning removed
//
// Revision 1.7  2005/10/07 11:43:51  dkrajzew
// THIRD LARGE CODE RECHECK: patched problems on Linux/Windows configs
//
// Revision 1.6  2005/09/15 12:17:45  dkrajzew
// LARGE CODE RECHECK
//
// Revision 1.5  2005/04/28 09:02:48  dkrajzew
// level3 warnings removed
//
// Revision 1.4  2004/01/28 12:35:37  dkrajzew
// documentation added
//
// Revision 1.3  2003/06/06 11:01:09  dkrajzew
// windows eol removed
//
// Revision 1.2  2003/06/05 14:33:44  dkrajzew
// class templates applied; documentation added
//
/* =========================================================================
 * compiler pragmas
 * ======================================================================= */
#pragma warning(disable: 4786)
#pragma warning(disable: 4503)


/* =========================================================================
 * included modules
 * ======================================================================= */
#ifdef HAVE_CONFIG_H
#ifdef WIN32
#include <windows_config.h>
#else
#include <config.h>
#endif
#endif // HAVE_CONFIG_H

#include "DistributionCont.h"

#ifdef _DEBUG
#include <utils/dev/debug_new.h>
#endif // _DEBUG


/* =========================================================================
 * static variable definitions
 * ======================================================================= */
DistributionCont::TypedDistDict DistributionCont::myDict;


/* =========================================================================
 * method definitions
 * ======================================================================= */
bool
DistributionCont::dictionary(const std::string &type, const std::string &id,
                           Distribution *d)
{
    TypedDistDict::iterator i=myDict.find(type);

    if(i==myDict.end()) {
        myDict[type][id] = d;
        return true;
    }
    DistDict &dict = (*i).second;
    DistDict::iterator j=dict.find(id);
    if(j==dict.end()) {
        myDict[type][id] = d;
        return true;
    }
    return false;
}


Distribution *
DistributionCont::dictionary(const std::string &type,
                           const std::string &id)
{
    TypedDistDict::iterator i=myDict.find(type);
    if(i==myDict.end()) {
        return 0;
    }
    DistDict &dict = (*i).second;
    DistDict::iterator j=dict.find(id);
    if(j==dict.end()) {
        return 0;
    }
    return (*j).second;
}


/**************** DO NOT DEFINE ANYTHING AFTER THE INCLUDE *****************/

// Local Variables:
// mode:C++
// End:


