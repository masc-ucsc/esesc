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
  virtual int64_t getSamples() const = 0;
};

class GStatsCntr : public GStats {
private:
  double data;
protected:
public:
  GStatsCntr(const char *format,...);

  GStatsCntr & operator += (const double v) {
    data += v;
    return *this;
  }

  void add(const double v, bool en = true) {
    data += en ? v : 0;
  }
  void inc(bool en = true) {
    data += en ? 1 : 0;
  }

  void dec(bool en = true) {
    data -= en ? 1 : 0;
  }

  double  getDouble() const;
  int64_t getSamples() const;

  void reportValue() const;
};

class GStatsAvg : public GStats {
private:
protected:
  double data;
  int64_t nData;
public:
  GStatsAvg(const char *format,...);
  GStatsAvg() { }

  void    reset() { data = 0; nData = 0; };
  double  getDouble() const;

  virtual void sample(const double v, bool en = true) {
    data  += en ? v : 0;
    nData += en ? 1 : 0;
  }
  int64_t getSamples() const;

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
  int64_t getSamples() const;

  void reportValue() const;
};

class GStatsHist : public GStats {
private:
protected:
  
  typedef HASH_MAP<uint32_t, double> Histogram;

  double numSample;
  double cumulative;

  Histogram H;

public:
  GStatsHist(const char *format,...);
  GStatsHist() { }

  void sample(uint32_t key, double weight=1);
  int64_t getSamples() const;

  void reportValue() const;
};

#endif   // GSTATSD_H
