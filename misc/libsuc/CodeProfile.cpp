// Contributed by Jose Renau
//
// The ESESC/BSD License
//
// Copyright (c) 2015-2016, Regents of the University of California and 
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>

#include "CodeProfile.h"
#include "Report.h"
#include "SescConf.h"

CodeProfile::CodeProfile(const char *format,...)
{
  I(format!=0);      // Mandatory to pass a description
  I(format[0] != 0); // Empty string not valid

  char *str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  name = str;
  I(*name!=0);
  subscribe();

  last_nCommitted = 0;
  last_clockTicks = 0;

  nTotal = 0;
}

double CodeProfile::getDouble() const {
  return 0;
}

void CodeProfile::reportValue() const {

  for(Prof::const_iterator it=prof.begin();it!=prof.end();it++) {
    ProfEntry e = it->second;
    //if (e.n/nTotal<0.001)
      //continue; // Just print things over 1%

    Report::field("%s_%llx:n=%4.3f:cpi=%f:wt=%f:et=%f:flush=%lld",name,it->first,e.n/nTotal,e.sum_cpi/e.n,e.sum_wt/e.n,e.sum_et/e.n,e.sum_flush);
  }
}

void CodeProfile::reportBinValue() const {
  Report::binField(0);
}

void CodeProfile::reportScheme() const {
  Report::scheme(name, "8");
}

int64_t CodeProfile::getSamples() const { 
  return 0;
}

void CodeProfile::flushValue() {
  prof.clear();
}

void CodeProfile::sample(const uint64_t pc, const double nCommitted, const double clockTicks, double wt, double et, bool flush) {

  double delta_nCommitted = nCommitted - last_nCommitted;
  double delta_clockTicks = clockTicks - last_clockTicks;

  last_nCommitted = nCommitted;
  last_clockTicks = clockTicks;

  if (delta_nCommitted==0)
    return;

  double cpi = delta_clockTicks/delta_nCommitted;

  nTotal++;

  //MSG("%llx, %g",pc,cpi);

  if (prof.find(pc) == prof.end()) {
    ProfEntry e;
    e.n   = 1;
    e.sum_cpi = cpi;
    e.sum_wt  = wt;
    e.sum_et  = et;
    e.sum_flush  = flush?1:0;
    prof[pc] = e;
  }else{
    prof[pc].sum_cpi += cpi;
    prof[pc].sum_wt  += wt;
    prof[pc].sum_et  += et;
    prof[pc].sum_flush  += flush?1:0;
    prof[pc].n++;
  }

}


