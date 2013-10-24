/* License & includes {{{1 */
/* 
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Meeta Sinha-Verma

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef CacheEq_H
#define CacheEq_H
#include "muParserTest.h"

#define _USE_MATH_DEFINES		

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <iostream>
#include <locale>
#include <limits>
#include <ios> 
#include <iomanip>
#include <numeric>

#include "muParser.h"

using namespace std;
using namespace mu;


class CacheEq {
private:
 mu::Parser dynamic_eq; // Equation for dynamic power consumption
 mu::Parser leakage_eq; // Equation for leakage power consumption
 double tech; //technology node
 double assoc; // associativity
 double line_size; // line_size
 double size; // Size


public:
CacheEq();
~CacheEq();
CacheEq(double tech, double assoc, double line_size, double size);

void SetEquation(const char* eq_type);
value_type EvalEquation(const char* eq_type);
void testCacheEq();
};

#endif
