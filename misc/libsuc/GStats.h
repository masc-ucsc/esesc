/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Basilio Fraguela
                  Smruti Sarangi
      Luis Ceze
      James Tuck

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef GSTATSD_H
#define GSTATSD_H

#include "estl.h" // hash_map

#include <list>
#include <stdarg.h>
#include <vector>

#include "callback.h"
#include "nanassert.h"

struct compare_uint64_t {
  bool operator () (uint64_t one, uint64_t two) const {
    return one == two;
  } 
};

struct hash_uint64_t {
  size_t operator () (uint64_t key) const {
    return key;
  } 
};

class GStats_strcasecmp {
public:
  inline bool operator()(const char* s1, const char* s2) const {
    return strcasecmp(s1, s2) == 0;
  }
};


class GStats {
private:
  typedef HASH_MAP<const char *, GStats *, HASH<const char*>, GStats_strcasecmp > Container;
  typedef HASH_MAP<const char *, GStats *, HASH<const char*>, GStats_strcasecmp >::iterator ContainerIter;
  static Container store;

protected:
  char *name;

  char *getText(const char *format, va_list ap);
  void subscribe();
  void unsubscribe();

public:
  int32_t gd;

  static void report(const char *str);
  static GStats *getRef(const char *str);

  GStats();
  virtual ~GStats();

  void prepareTrace();

  virtual void reportValue() const =0;

  virtual double getDouble() const {
    MSG("getDouble Not supported by this class %s",name);
    return 0;
  }

  const char *getName() const { return name; }
  virtual int64_t getSamples() const;

  // Some stats just need to be active all the time no matter what the EmSampler says (E.g: sampler stats)
};

class GStatsCntr : public GStats {
private:
  int64_t data;
protected:
public:
  GStatsCntr(const char *format,...);

  GStatsCntr & operator += (const int64_t v) {
    data += v;
    return *this;
  }

  void add(const int64_t v, bool en = true) {
    data += en ? v : 0;
  }
  void inc(bool en = true) {
    data += en ? 1 : 0;
  }

  void dec(bool en = true) {
    data -= en ? 1 : 0;
  }

  int64_t getValue() const;

  double  getDouble() const;
  int64_t getSamples() const;

  void reportValue() const;
};

class GStatsAvg : public GStats {
private:
protected:
  int64_t data;
  int64_t nData;
public:
  GStatsAvg(const char *format,...);
  GStatsAvg() { }

  virtual void sample(const int64_t v, bool en = true) {
    data += en ? v : 0;
    nData += en ? 1 : 0;
  }

  virtual void msamples(const int64_t v, int64_t n, bool en = true) {
    data  += en ? v : 0;
    nData += en ? n : 0;
  }

  double  getDouble() const;
  int64_t getSamples() const;
  void    reset() { data = 0; nData = 0; };

  virtual void reportValue() const;
};

class GStatsMax : public GStats {
private:
protected:
  double maxValue;
  int64_t nData;

public:
  GStatsMax(const char *format,...);

  void sample(const double v);

  void reportValue() const;
};

class GStatsHist : public GStats {
private:
protected:
  
  typedef HASH_MAP<uint32_t, uint64_t> Histogram;

  uint64_t numSample;
  uint64_t cumulative;

  Histogram H;

public:
  GStatsHist(const char *format,...);
  GStatsHist() { }

  void sample(uint32_t key, double weight=1);
  void reportValue() const;
};

#endif   // GSTATSD_H
