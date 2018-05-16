// Contributed by Jose Renau
//                Basilio Fraguela
//                Smruti Sarangi
//	              Luis Ceze
//	              James Tuck
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

#ifndef GSTATS_THERMAL_H
#define GSTATS_THERMAL_H
#include "estl.h" // hash_map

#include <list>
#include <stdarg.h>
#include <vector>

#include "callback.h"
#include "nanassert.h"

#include "GStats.h"

class GStatsThermCntr : public GStats {
private:
  long double data;
  long double alldata;

protected:
public:
  GStatsThermCntr(const char *format, ...);

  GStatsThermCntr &operator+=(const double v) {
    data += v;
    return *this;
  }

  void add(const double v) {
    data += v;
  }
  double  getValue() const;
  double  getDouble() const;
  int64_t getSamples() const;
  void    reportValue() const;
  void    start();
  void    stop(double weight = 1);
};

class GStatsThermMax : public GStats {
private:
protected:
  double maxValue;
  bool   frozen;

public:
  GStatsThermMax(const char *format, ...);

  void sample(const double v);

  void reportValue() const;

  double getDouble() const {
    return maxValue;
  }
  void start();
  void stop(double weight = 1);
};

#endif // GSTATS_THERMAL_H
