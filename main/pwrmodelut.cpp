/* License & includes {{{1 */
/*
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Jose Renau

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

#include "GStats.h"
#include "PowerModel.h"
#include "SescConf.h"

/* }}} */

int main(int argc, const char **argv)
/* main {{{1 */
{
  SescConf            = new SConfig(argc, argv);
  const char *section = NULL;
  LOG("Running in Debug Mode");

  // Create GStats for testing
  GStatsCntr *testGStat = new GStatsCntr("testCounter");
  testGStat->add(1000);

  // Create PowerModel from configuration file
  section = (char *)SescConf->getCharPtr("", "pwrmodel", 0);
  LOG("Section is: %s", section);
  PowerModel *p = new PowerModel();
  LOG("Created PowerModel object");
  p->plug(section);
  p->printStatus();
  p->testWrapper();
  LOG("EXECUTION COMPLETE");
  return 0;
}
/* }}} */
