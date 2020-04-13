/*
   EESESC: Enhance Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Jose Renau

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#ifndef ESTL_H
#define ESTL_H

// Diferent compilers have slightly different calling conventions for
// STL. This file has a cross-compiler implementation.

#include <unordered_map>
#include <set>
#include <map>
#define HASH_MAP std::unordered_map
#define HASH_SET std::set
#define HASH     std::hash
#define HASH_MULTIMAP std::multimap

#endif // ESTL_H
