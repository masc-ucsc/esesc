// Contributed by Meeta Sinha-Verma
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "CacheEq.h"
#include "PeqParser.h"

#include "SescConf.h"
#include <stdlib.h>

using namespace std;
using namespace mu;

////////////////////////////////////////////////////////////
CacheEq::CacheEq() {
  tech      = 0.032;
  assoc     = 1;
  line_size = 32;
  size      = 4096;
  leakage_eq.SetVarFactory(AddVariable, &leakage_eq);
}

CacheEq::CacheEq(double tech, double assoc, double line_size, double size)
    : tech(tech)
    , assoc(assoc)
    , line_size(line_size)
    , size(size) {
  leakage_eq.SetVarFactory(AddVariable, &leakage_eq);
}

CacheEq::~CacheEq() {
}

void CacheEq::SetEquation(const char *eq_type) {
  const char *EqExpression;
  if(assoc <= 4 && size < 65536 && line_size < 64) {
    EqExpression = SescConf->getCharPtr("CacheEq_Small", eq_type);
  } else {
    EqExpression = SescConf->getCharPtr("CacheEq_Large", eq_type);
  }

  if(strcasecmp(eq_type, "dynamic") == 0) {
    // Declare Variables of dyanmic equation and link to object variables
    // Since muParser only takes variables as double. All variables should be
    // declared as double
    dynamic_eq.DefineVar("tech", &tech);
    dynamic_eq.DefineVar("assoc", &assoc);
    dynamic_eq.DefineVar("line_size", &line_size);
    dynamic_eq.DefineVar("size", &size);
    dynamic_eq.SetExpr((string_type)EqExpression);
  } else
    leakage_eq.SetExpr((string_type)EqExpression);
}
value_type CacheEq::EvalEquation(const char *eq_type) {
  if(strcasecmp(eq_type, "dynamic") == 0)
    return dynamic_eq.Eval();
  else
    return leakage_eq.Eval();
}

void CacheEq::testCacheEq() {
  SetEquation("dynamic");
  SetEquation("leakage");

  /*	this->SetParserExpression(); */
  string_type s_expr = dynamic_eq.GetExpr();
  cout << "Dynamic Equation is " << s_expr << "\n";
  value_type dynamic = EvalEquation("dynamic");
  cout << "Dynamic Power is " << dynamic << "\n";
  s_expr = leakage_eq.GetExpr();
  cout << "Leakage Equation is " << s_expr << "\n";
}

//---------------------------------------------------------------------------
