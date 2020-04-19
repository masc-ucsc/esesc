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

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "GStats.h"
#include "Report.h"
#include "SescConf.h"

/*********************** GStats */

GStats::Container GStats::store;

GStats::GStats()
    : name(NULL) {
}

GStats::~GStats() {
  unsubscribe();
  free(name);
}

char *GStats::getText(const char *format, va_list ap) {
  char strid[1024];

  vsprintf(strid, format, ap);

  return strdup(strid);
}

void GStats::subscribe() {

  I(strcmp(getName(), "") != 0);

  if(store.find(getName()) != store.end()) {
    MSG("ERROR: gstats is added twice with name [%s]. Use another name", getName());
    I(0);
  }

  store[getName()] = this;
}

void GStats::unsubscribe() {
  I(name);

  ContainerIter it = store.find(name);
  if(it != store.end()) {
    store.erase(it);
  }
}

void GStats::report(const char *str) {
  Report::field("#BEGIN GStats::report %s", str);

  for(ContainerIter it = store.begin(); it != store.end(); it++) {
    it->second->reportValue();
  }

  Report::field("#END GStats::report %s", str);
}

void GStats::flushValue() {
}

void GStats::flush() {
  for(ContainerIter it = store.begin(); it != store.end(); it++) {
    it->second->flushValue();
  }
}

GStats *GStats::getRef(const char *str) {

  ContainerIter it = store.find(str);
  if(it != store.end())
    return it->second;

  return 0;
}

/*********************** GStatsCntr */

GStatsCntr::GStatsCntr(const char *format, ...) {
  I(format != 0);    // Mandatory to pass a description
  I(format[0] != 0); // Empty string not valid

  char *  str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  data = 0;

  name = str;
  I(*name != 0);
  subscribe();
}

double GStatsCntr::getDouble() const {
  return data;
}

void GStatsCntr::reportValue() const {
  Report::field("%s=%f", name, data);
}

int64_t GStatsCntr::getSamples() const {
  return (int64_t)data;
}

void GStatsCntr::flushValue() {
  data = 0;
}

/*********************** GStatsAvg */

GStatsAvg::GStatsAvg(const char *format, ...) {
  char *  str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  data  = 0;
  nData = 0;

  name = str;
  subscribe();
}

double GStatsAvg::getDouble() const {
  return ((double)data) / nData;
}

void GStatsAvg::sample(const double v, bool en) {
  data += en ? v : 0;
  nData += en ? 1 : 0;
}

void GStatsAvg::reportValue() const {
  Report::field("%s:n=%lld::v=%f", name, nData, getDouble()); // n first for power
}

int64_t GStatsAvg::getSamples() const {
  return nData;
}

void GStatsAvg::flushValue() {
  data  = 0;
  nData = 0;
}

/*********************** GStatsMax */

GStatsMax::GStatsMax(const char *format, ...) {
  char *  str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  maxValue = 0;
  nData    = 0;

  name = str;

  subscribe();
}

void GStatsMax::reportValue() const {
  Report::field("%s:max=%f:n=%lld", name, maxValue, nData);
}

void GStatsMax::sample(const double v, bool en) {
  if(!en)
    return;
  maxValue = v > maxValue ? v : maxValue;
  nData++;
}

int64_t GStatsMax::getSamples() const {
  return nData;
}

void GStatsMax::flushValue() {
  maxValue = 0;
  nData    = 0;
}

/*********************** GStatsHist */

GStatsHist::GStatsHist(const char *format, ...)
    : numSample(0)
    , cumulative(0) {
  char *  str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  numSample  = 0;
  cumulative = 0;

  name = str;
  subscribe();
}

void GStatsHist::reportValue() const {
  int32_t maxKey = 0;

  for(Histogram::const_iterator it = H.begin(); it != H.end(); it++) {
    Report::field("%s(%d)=%f", name, it->first, it->second);
    if(it->first > maxKey)
      maxKey = it->first;
  }
  long double div = cumulative; // cummulative has 64bits (double has 54bits mantisa)
  div /= numSample;

  Report::field("%s:max=%d", name, maxKey);
  Report::field("%s:v=%f", name, (double)div);
  Report::field("%s:n=%f", name, numSample);
}

void GStatsHist::sample(bool enable, int32_t key, double weight) {
  if(enable) {
    if(H.find(key) == H.end())
      H[key] = 0;

    H[key] += weight;

    numSample += weight;
    cumulative += weight * key;
  }
}

int64_t GStatsHist::getSamples() const {
  return static_cast<int64_t>(numSample);
}

void GStatsHist::flushValue() {
  H.clear();

  numSample  = 0;
  cumulative = 0;
}
