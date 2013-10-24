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

#include "PowerStats.h"

/* }}} */

PowerStats::PowerStats(const char *str)
  /* constructor {{{1 */
  : name(strdup(str)) {
  last_value = 0;
}
/* }}} */

PowerStats::~PowerStats()
  /* destructor {{{1 */
{
  free(const_cast <char *> (name));
}
/* }}} */

void PowerStats::addGStat(const char *str, const int32_t scale)
  /* add a GStat {{{1 */
{
  I(str != 0);
  I(str[0] != 0);

  GStats *ref = GStats::getRef(str);
  if (ref) {
    Container c(ref, scale);
    stats.push_back(c);
  } else if (strcmp(str, "testCounter")) {
    MSG("WARNING: GStat '%s' needed by PowerModel not found", str);
		I(0);
  }
}
/* }}} */

uint32_t PowerStats::getActivity()
  /* getActivity {{{1 */
{
  uint32_t total = 0;

  for(size_t i = 0; i < stats.size(); i++) 
    total += stats[i].getValue();
  
  uint32_t temp = total - last_value;
  last_value    = total;
  return temp;
}
/* }}} */

