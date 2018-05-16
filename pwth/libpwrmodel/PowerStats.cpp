// Contributed by Gabriel Southern
//                Jose Renau
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

#include "PowerStats.h"

/* }}} */

PowerStats::PowerStats(const char *str)
    /* constructor {{{1 */
    : name(strdup(str)) {
  last_value = 0;
}
/* }}} */

PowerStats::~PowerStats()
/* destructor {{{1 */
{
  free(const_cast<char *>(name));
}
/* }}} */

void PowerStats::addGStat(const char *str, const int32_t scale)
/* add a GStat {{{1 */
{
  I(str != 0);
  I(str[0] != 0);

  GStats *ref = GStats::getRef(str);
  if(ref) {
    Container c(ref, scale);
    stats.push_back(c);
  } else if(strcmp(str, "testCounter")) {
    MSG("WARNING: GStat '%s' needed by PowerModel not found", str);
    I(0);
  }
}
/* }}} */

uint32_t PowerStats::getActivity()
/* getActivity {{{1 */
{
  uint32_t total = 0;

  for(size_t i = 0; i < stats.size(); i++)
    total += stats[i].getValue();

  uint32_t temp = total - last_value;
  last_value    = total;
  return temp;
}
/* }}} */
