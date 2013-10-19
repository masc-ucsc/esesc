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
  GStatsThermCntr(const char *format,...);

  GStatsThermCntr & operator += (const double v) {
    data += v;
    return *this;
  }

  void add(const double v) {
    data += v;
  }
  double getValue() const;
  double  getDouble() const;
  int64_t getSamples() const;
  void reportValue() const;
  void start();
  void stop(double weight=1);
};



class GStatsThermMax : public GStats {
private:
protected:
  double maxValue;
  bool    frozen;
public:
  GStatsThermMax(const char *format,...);

  void sample(const double v);

  void reportValue() const;

  double  getDouble() const{
		return maxValue;
	}
  void start();
  void stop(double weight=1);
};


#endif   // GSTATS_THERMAL_H
