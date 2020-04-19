// Contributed by Jose Renau
//                Basilio Fraguela
//                James Tuck
//                Smruti Sarangi
//                James Tuck
//                Luis Ceze
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

#ifndef GSTATSD_H
#define GSTATSD_H

#include "estl.h" // hash_map

#include <list>
#include <stdarg.h>
#include <vector>

#include "callback.h"
#include "nanassert.h"

class GStats_strcasecmp {
public:
  bool operator()(const std::string &s1, const std::string &s2) const noexcept {
    return ::strcasecmp(s1.c_str(), s2.c_str()) < 0;
  }
};

class GStats {
private:
  typedef std::map<std::string, GStats *, GStats_strcasecmp>           Container;
  typedef Container::iterator ContainerIter;
  static Container store;

protected:
  char *name;

  char *getText(const char *format, va_list ap);
  void  subscribe();
  void  unsubscribe();

public:
  int32_t gd;

  static void report(const char *str);

  static GStats *getRef(const char *str);

  GStats();
  virtual ~GStats();

  void prepareTrace();

  virtual void reportValue() const = 0;

  virtual void flushValue();
  static void  flush();

  virtual double getDouble() const {
    MSG("getDouble Not supported by this class %s", name);
    return 0;
  }

  const char *getName() const {
    return name;
  }
  virtual int64_t getSamples() const = 0;
};

class GStatsCntr : public GStats {
private:
  double data;

protected:
public:
  GStatsCntr(const char *format, ...);

  GStatsCntr &operator+=(const double v) {
    data += v;
    return *this;
  }

  void add(const double v, bool en = true) {
    data += en ? v : 0;
  }
  void inc(bool en = true) {
    data += en ? 1 : 0;
  }

  void dec(bool en) {
    data -= en ? 1 : 0;
  }

  double  getDouble() const;
  int64_t getSamples() const;

  void reportValue() const;

  void flushValue();
};

class GStatsAvg : public GStats {
private:
protected:
  double  data;
  int64_t nData;

public:
  GStatsAvg(const char *format, ...);
  GStatsAvg() {
  }

  void reset() {
    data  = 0;
    nData = 0;
  };
  double getDouble() const;

  virtual void sample(const double v, bool en);
  int64_t      getSamples() const;

  virtual void reportValue() const;

  void flushValue();
};

class GStatsMax : public GStats {
private:
protected:
  double  maxValue;
  int64_t nData;

public:
  GStatsMax(const char *format, ...);

  void    sample(const double v, bool en);
  int64_t getSamples() const;

  void reportValue() const;

  void flushValue();
};

class GStatsHist : public GStats {
private:
protected:
  typedef HASH_MAP<int32_t, double> Histogram;

  double numSample;
  double cumulative;

  Histogram H;

public:
  GStatsHist(const char *format, ...);
  GStatsHist() {
  }

  void    sample(bool enable, int32_t key, double weight = 1);
  int64_t getSamples() const;

  void reportValue() const;

  void flushValue();
};

#endif // GSTATSD_H
