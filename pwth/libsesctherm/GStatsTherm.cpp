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
#include <float.h>

#include "GStatsTherm.h"
#include "Report.h"

/*********************** GStatsThermCntr */

GStatsThermCntr::GStatsThermCntr(const char *format,...)
{
  I(format!=0);      // Mandatory to pass a description
  I(format[0] != 0); // Empty string not valid

  char *str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  data    = 0;
  alldata = 0;

  name = str;
  I(*name!=0);
  subscribe();
}

double GStatsThermCntr::getDouble() const
{
	I(data==0); // Call the end/sample before dumping/using it (partial statistics being gathered)

  return (double)alldata;
}

void GStatsThermCntr::reportValue() const
{
  //Report::field("%s=%lld", name, alldata);
  Report::field("%s=%Lg", name, alldata);
}

int64_t GStatsThermCntr::getSamples() const 
{ 
	// In a counter, the # samples == value
	return static_cast<int64_t>(getDouble());
}

double GStatsThermCntr::getValue() const 
{
	I(data==0); // Call the end/sample before dumping/using it (partial statistics being gathered)

	return data;
}

void GStatsThermCntr::stop(double weight)
{
	alldata = alldata + (data*weight);
	data   = 0;
}

void GStatsThermCntr::start()
{
	data = 0;
}
/*********************** GStatsThermMax */

GStatsThermMax::GStatsThermMax(const char *format,...)
{
  char *str;
  va_list ap;

  va_start(ap, format);
  str = getText(format, ap);
  va_end(ap);

  maxValue = DBL_MIN;

  frozen   = false;
  name     = str;

  subscribe();
}

void GStatsThermMax::reportValue() const
{
  Report::field("%s:max=%g", name, maxValue);
}

void GStatsThermMax::sample(const double v) 
{
	maxValue = (v > maxValue && !frozen) ? v : maxValue;
}

void GStatsThermMax::stop(double weight)
{
}

void GStatsThermMax::start()
{
}


