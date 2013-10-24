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

#ifndef PEQPARSER_H
#define PEQPARSER_H

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


// Forward declarations

value_type* AddVariable(const char_type *a_szName, void *a_pUserData);
void ListVar(const mu::ParserBase &parser);
void ListConst(const mu::ParserBase &parser);
void ListExprVar(const mu::ParserBase &parser);

class PeqParser {
private:
 mu::Parser parser;
public:
PeqParser();
~PeqParser();
void SetParserExpression();
value_type ParserEvalEq();
void testParser();
};

#endif
