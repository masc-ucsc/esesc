#ifndef POWERSTATS_H
#define POWERSTATS_H

/* License & includes {{{1 */
/* 
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Gabriel Southern
                  Jose Renau

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <vector>

#include "nanassert.h"
#include "GStats.h"

/* }}} */

class PowerStats {
  /* PowerStats class definition {{{1 */
  private:
	  const char *name; 
	  uint64_t    last_value;

	  class Container {
	    public:
		    Container(GStats *g, int32_t s) {
			    gstat  = g;
			    scalar = s;
		    }
		    int32_t getValue() const {
			    return gstat->getSamples() * scalar;
		    }
		    GStats *gstat;
		    int32_t scalar;
	  };

	  std::vector<Container> stats;
  
  public:
	  PowerStats(const char* str);
	  ~PowerStats();
	  void addGStat(const char *str, const int32_t scale);
	  uint32_t getActivity();
	  const char* getName() const {
		  return name;
	  }
};
/* }}} */

#endif // POWERSTATS_H
