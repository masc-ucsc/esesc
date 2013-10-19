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

#include "SRAM.h"
#include "PeqParser.h"

#include "SescConf.h"
#include <stdlib.h>

using namespace std;
using namespace mu;

////////////////////////////////////////////////////////////
SRAM::SRAM()
{
  tech = 32;
  ports = 1;
  width = 32;
  size = 4096;
  leakage_eq.SetVarFactory(AddVariable, &leakage_eq);
}

SRAM::SRAM(double tech, double ports, double width, double size)
	:tech(tech), ports(ports), width(width), size(size)
{
}


SRAM::~SRAM()
{
}

void SRAM::SetEquation(const char* eq_type)
{
	const char *EqExpression;
	if(ports < 3) {
		if(ports*size < 4097) // Size smaller than 4KB
			EqExpression = SescConf->getCharPtr("SRAM_Small1", eq_type);
		else
			EqExpression = SescConf->getCharPtr("SRAM_Large1",eq_type);
	}
	else {
		if(ports*size < 10241) // Size smaller than 10KB
			EqExpression = SescConf->getCharPtr("SRAM_Small2",eq_type);
		else
			EqExpression = SescConf->getCharPtr("SRAM_Large2",eq_type);
	}

	if(strcasecmp(eq_type, "dynamic") == 0 ) {
	// Declare Variables of dyanmic equation and link to object variables
	// Since muParser only takes variables as double. All variables should be
	// declared as double
		dynamic_eq.DefineVar("tech",&tech);
		dynamic_eq.DefineVar("ports",&ports);
		dynamic_eq.DefineVar("width",&width);
		dynamic_eq.DefineVar("size",&size);
		dynamic_eq.SetExpr((string_type)EqExpression);
	}
	else
		leakage_eq.SetExpr((string_type)EqExpression);
}
value_type SRAM::EvalEquation(const char* eq_type)
{
	if(strcasecmp(eq_type,"dynamic") == 0) 
		return dynamic_eq.Eval();
	else
		return leakage_eq.Eval();
}

void SRAM::testSRAM()
{
	SetEquation("dynamic");
	SetEquation("leakage");	

/*	this->SetParserExpression(); */
	string_type s_expr = dynamic_eq.GetExpr();
	cout<<"Dynamic Equation is "<<s_expr<<"\n";
	value_type dynamic = EvalEquation("dynamic");
	cout<<"Dynamic Power is "<<dynamic<<"\n";
	s_expr = leakage_eq.GetExpr();
	cout<<"Leakage Equation is "<<s_expr<<"\n";
}



//---------------------------------------------------------------------------
