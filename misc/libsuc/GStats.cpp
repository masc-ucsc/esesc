/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
   Basilio Fraguela
   Smruti Sarangi
   Luis Ceze
   Milos Prvulovic

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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>

#include "GStats.h"
#include "Report.h"

/*********************** GStats */

GStats::Container GStats::store;

GStats::GStats() 
{

}

GStats::~GStats() 
{
  unsubscribe();
  free(name);
}

char *GStats::getText(const char *format, va_list ap)
{
  char strid[1024];

  vsprintf(strid, format, ap);

  return strdup(strid);
}

void GStats::subscribe() 
{

  I(strcmp(getName(),"")!=0);

  if(store.find(getName()) != store.end()) {
    MSG("ERROR: gstats is added twice with name [%s]. Use another name",getName());
    I(0);
  }

  store[getName()] = this;
}

void GStats::unsubscribe() 
{
  I(name);

  ContainerIter it = store.find(name);
  if( it != store.end()) {
    store.erase(it);
  }
}

void GStats::report(const char *str)
{
  Report::field("#BEGIN GStats::report %s", str);

  for(ContainerIter it = store.begin(); it != store.end(); it++) {
    it->second->reportValue();
  }

  Report::field("#END GStats::report %s", str);
}

GStats *GStats::getRef(const char *str) {

  ContainerIter it = store.find(str);
  if( it != store.end())
    return it->second;

  return 0;
}

/*********************** GStatsCntr */

GStatsCntr::GStatsCntr(const char *format,...)
{
  I(format!=0);      // Mandatory to pass a description
  I(format[0] != 0); // Empty string not valid

  char *str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  data    = 0;

  name = str;
  I(*name!=0);
  subscribe();
}

double GStatsCntr::getDouble() const
{
  return (double)data;
}

void GStatsCntr::reportValue() const
{
  Report::field("%s=%f", name, data);
}

int64_t GStatsCntr::getSamples() const 
{ 
  return data;
}

/*********************** GStatsAvg */

GStatsAvg::GStatsAvg(const char *format,...)
{
  char *str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  data  = 0;
  nData = 0;

  name = str;
  subscribe();
}

double GStatsAvg::getDouble() const
{
  return ((double)data)/nData;
}

void GStatsAvg::reportValue() const
{
  Report::field("%s:v=%f:n=%lld", name, getDouble(), nData);
}

int64_t GStatsAvg::getSamples() const 
{
  return nData;
}

/*********************** GStatsMax */

GStatsMax::GStatsMax(const char *format,...)
{
  char *str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  maxValue = 0;
  nData    = 0;

  name     = str;

  subscribe();
}

void GStatsMax::reportValue() const
{
  Report::field("%s:max=%f:n=%lld", name, maxValue, nData);
}

void GStatsMax::sample(const double v) 
{
  maxValue = v > maxValue ? v : maxValue;
  nData++;
}

int64_t GStatsMax::getSamples() const 
{
  return nData;
}

/*********************** GStatsHist */

GStatsHist::GStatsHist(const char *format,...) : numSample(0), cumulative(0)
{
  char *str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  numSample  = 0;
  cumulative = 0;

  name = str;
  subscribe();
}

void GStatsHist::reportValue() const
{
  I(H.empty()); // call stop before
    
  uint32_t maxKey = 0;

  for(Histogram::const_iterator it=H.begin();it!=H.end();it++) {
    Report::field("%s(%lu)=%f",name,it->first,it->second);
    if(it->first > maxKey)
      maxKey = it->first;
  }
  long double div = cumulative; // cummulative has 64bits (double has 54bits mantisa)
  div /= numSample;

  Report::field("%s:max=%lu" ,name,maxKey);
  Report::field("%s:v=%f"    ,name,(double)div);
  Report::field("%s:n=%f"   ,name,numSample);
}

void GStatsHist::sample(uint32_t key, double weight)
{
  if(H.find(key)==H.end())
    H[key]=0;

  H[key]+=weight;

  numSample += weight;
  cumulative += weight * key;
}

int64_t GStatsHist::getSamples() const 
{
  return static_cast<int64_t>(numSample);
}

