// Contributed by Jose Renau
//                Basilio Fraguela
//					      Smruti Sarangi
//					      Luis Ceze
//					      Milos Prvulovic
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

#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "GStatsTherm.h"
#include "Report.h"

/*********************** GStatsThermCntr */

GStatsThermCntr::GStatsThermCntr(const char *format, ...) {
  I(format != 0);    // Mandatory to pass a description
  I(format[0] != 0); // Empty string not valid

  char *  str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  data    = 0;
  alldata = 0;

  name = str;
  I(*name != 0);
  subscribe();
}

double GStatsThermCntr::getDouble() const {
  I(data == 0); // Call the end/sample before dumping/using it (partial statistics being gathered)

  return (double)alldata;
}

void GStatsThermCntr::reportValue() const {
  // Report::field("%s=%lld", name, alldata);
  Report::field("%s=%Lg", name, alldata);
}

int64_t GStatsThermCntr::getSamples() const {
  // In a counter, the # samples == value
  return static_cast<int64_t>(getDouble());
}

double GStatsThermCntr::getValue() const {
  I(data == 0); // Call the end/sample before dumping/using it (partial statistics being gathered)

  return data;
}

void GStatsThermCntr::stop(double weight) {
  alldata = alldata + (data * weight);
  data    = 0;
}

void GStatsThermCntr::start() {
  data = 0;
}
/*********************** GStatsThermMax */

GStatsThermMax::GStatsThermMax(const char *format, ...) {
  char *  str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  maxValue = DBL_MIN;

  frozen = false;
  name   = str;

  subscribe();
}

void GStatsThermMax::reportValue() const {
  Report::field("%s:max=%g", name, maxValue);
}

void GStatsThermMax::sample(const double v) {
  maxValue = (v > maxValue && !frozen) ? v : maxValue;
}

void GStatsThermMax::stop(double weight) {
}

void GStatsThermMax::start() {
}
