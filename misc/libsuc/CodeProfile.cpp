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

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "CodeProfile.h"
#include "Report.h"
#include "SescConf.h"

CodeProfile::CodeProfile(const char *format, ...) {
  I(format != 0);    // Mandatory to pass a description
  I(format[0] != 0); // Empty string not valid

  char *  str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  name = str;
  I(*name != 0);
  subscribe();

  last_nCommitted = 0;
  last_clockTicks = 0;

  nTotal = 0;
}

double CodeProfile::getDouble() const {
  return 0;
}

void CodeProfile::reportValue() const {

  for(Prof::const_iterator it = prof.begin(); it != prof.end(); it++) {
    ProfEntry e = it->second;
    // if (e.n/nTotal<0.001)
    // continue; // Just print things over 1%

    Report::field("%s_%llx:n=%4.3f:cpi=%f:wt=%f:et=%f:flush=%lld:prefetch=%lld:ldbr=%d:bp1_hit=%lld:bp1_miss=%lld:bp2_hit=%lld:bp2_miss=%lld:bp3_hit=%lld:bp3_miss=%lld:bp_hit2_miss3=%lld:bp_hit3_miss2=%lld:no_tl=%lld:on_time_tl=%lld:late_tl=%lld", name, it->first, e.n / nTotal, e.sum_cpi / e.n, e.sum_wt / e.n, e.sum_et / e.n, e.sum_flush, e.sum_prefetch, e.ldbr, e.sum_bp1_hit, e.sum_bp1_miss, e.sum_bp2_hit, e.sum_bp2_miss, e.sum_bp3_hit, e.sum_bp3_miss, e.sum_hit2_miss3, e.sum_hit3_miss2, e.sum_no_tl, e.sum_on_time_tl, e.sum_late_tl);
  }
}

int64_t CodeProfile::getSamples() const {
  return 0;
}

void CodeProfile::flushValue() {
  prof.clear();
}

void CodeProfile::sample(const uint64_t pc, const double nCommitted, const double clockTicks, double wt, double et, bool flush, bool prefetch, int ldbr, bool bp1_miss, bool bp2_miss, bool bp3_miss, bool bp1_hit, bool bp2_hit, bool bp3_hit, bool hit2_miss3, bool hit3_miss2, bool tl1_pred, bool tl1_unpred, bool tl2_pred, bool tl2_unpred, int trig_ld_status) {

  double delta_nCommitted = nCommitted - last_nCommitted;
  double delta_clockTicks = clockTicks - last_clockTicks;

  last_nCommitted = nCommitted;
  last_clockTicks = clockTicks;

  if(delta_nCommitted == 0)
    return;

  double cpi = delta_clockTicks / delta_nCommitted;

  nTotal++;

  // MSG("%llx, %g",pc,cpi);

  if(prof.find(pc) == prof.end()) {
    ProfEntry e;
    e.n            = 1;
    e.sum_cpi      = cpi;
    e.sum_wt       = wt;
    e.sum_et       = et;
    e.sum_flush    = flush ? 1 : 0;
    if(ldbr > 0)
      e.ldbr         = ldbr;
    e.sum_bp1_hit  = bp1_hit ? 1 : 0;
    e.sum_bp1_miss = bp1_miss ? 1 : 0;
    e.sum_bp2_hit  = bp2_hit ? 1 : 0;
    e.sum_bp2_miss = bp2_miss ? 1 : 0;
    if(bp2_miss) {
      e.sum_trig_ld1_pred = tl1_pred ? 1 : 0;
      e.sum_trig_ld1_unpred = tl1_unpred ? 1 : 0;
      e.sum_trig_ld2_pred = tl2_pred ? 1 : 0;
      e.sum_trig_ld2_unpred = tl2_unpred ? 1 : 0;
    }
    e.sum_bp3_hit  = bp3_hit ? 1 : 0;
    e.sum_bp3_miss = bp3_miss ? 1 : 0;
    e.sum_hit2_miss3 = hit2_miss3 ? 1 : 0;
    e.sum_hit3_miss2 = hit3_miss2 ? 1 : 0;
    e.sum_no_tl = (trig_ld_status == -1) ? 1 : 0;
    e.sum_late_tl = (trig_ld_status > 0) ? 1 : 0;
    e.sum_on_time_tl = (trig_ld_status == 0) ? 1 : 0;
    e.sum_prefetch = prefetch ? 1 : 0;
    prof[pc]       = e;
  } else {
    prof[pc].sum_cpi += cpi;
    prof[pc].sum_wt += wt;
    prof[pc].sum_et += et;
    prof[pc].sum_flush += flush ? 1 : 0;
    if(ldbr > 0)
      prof[pc].ldbr         = ldbr;
    prof[pc].sum_bp1_hit += bp1_hit ? 1 : 0;
    prof[pc].sum_bp1_miss += bp1_miss ? 1 : 0;
    prof[pc].sum_bp2_hit += bp2_hit ? 1 : 0;
    prof[pc].sum_bp2_miss += bp2_miss ? 1 : 0;
    prof[pc].sum_bp3_hit += bp3_hit ? 1 : 0;
    prof[pc].sum_bp3_miss += bp3_miss ? 1 : 0;
    prof[pc].sum_hit2_miss3 += hit2_miss3 ? 1 : 0;
    prof[pc].sum_hit3_miss2 += hit3_miss2 ? 1 : 0;
    prof[pc].sum_no_tl += (trig_ld_status == -1) ? 1 : 0;
    prof[pc].sum_late_tl += (trig_ld_status > 0) ? 1 : 0;
    prof[pc].sum_on_time_tl += (trig_ld_status == 0) ? 1 : 0;
    if(bp2_miss) {
      prof[pc].sum_trig_ld1_pred += tl1_pred ? 1 : 0;
      prof[pc].sum_trig_ld1_unpred += tl1_unpred ? 1 : 0;
      prof[pc].sum_trig_ld2_pred += tl2_pred ? 1 : 0;
      prof[pc].sum_trig_ld2_unpred += tl2_unpred ? 1 : 0;
    }
    prof[pc].sum_prefetch += prefetch ? 1 : 0;
    prof[pc].n++;
  }
}
