/* License & includes {{{1 */
/*
ESESC: Enhanced Super ESCalar simulator
Copyright (C) 2009 University of California, Santa Cruz.

Contributed by Jose Renau

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

#include <stdio.h>
#include <string.h>

#include "SescConf.h"
#include "EmuSampler.h"
#include "EmulInterface.h"

/* }}} */

  EmulInterface::EmulInterface(const char *sect)
: section(strdup( sect ))
  /* Base class EmulInterface constructor {{{1 */
{
  sampler  = 0;
  numFlows = 0;
  cputype = CPU;
  const char* emultype = SescConf->getCharPtr(section,"type");


  FlowID nemul = SescConf->getRecordSize("","cpuemul");
  for(FlowID i=0;i<nemul;i++) {
    const char *section = SescConf->getCharPtr("","cpuemul",i);
    const char *type    = SescConf->getCharPtr(section,"type");
    if(strcasecmp(type,emultype) == 0 ) {
      numFlows++;
    }
  }



}
/* }}} */

EmulInterface::~EmulInterface() 
  /* EmulInterface destructor {{{1 */
{
}
/* }}} */

void EmulInterface::setSampler(EmuSampler *a_sampler, FlowID fid)
{
  //I(sampler==0);
  sampler = a_sampler;
}



