/* License & includes {{{1 */
/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Luis Ceze
                  Jose Renau
                  Karin Strauss
                  Milos Prvulovic

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <math.h>

#include "SescConf.h"

#include "CCache.h"
#include "NICECache.h"
#include "SL0Cache.h"
#include "VPC.h"
#include "TLB.h"
#include "Bus.h"
#include "StridePrefetcher.h"
#include "GlobalHistoryBuffer.h"
#include "Splitter.h"
#include "MemXBar.h"
#include "UnMemXBar.h"
#include "MemorySystem.h"
#include "MemController.h"
#include "MarkovPrefetcher.h"


#include "DrawArch.h"
extern DrawArch arch;


/* }}} */


MemorySystem::MemorySystem(int32_t processorId)
  /* constructor {{{1 */
  : GMemorySystem(processorId)
{

}
/* }}} */

MemObj *MemorySystem::buildMemoryObj(const char *device_type, const char *dev_section, const char *dev_name)
  /* build the correct memory object {{{1 */
{
  // Returns new created MemoryObj or NULL in known-error mode 
  MemObj *mdev=0;
  uint32_t devtype = 0; //CCache

  std::string mystr("");
  // You may insert here the further specializations you may need
  if (!strcasecmp(device_type, "cache") || !strcasecmp(device_type, "icache")) {
    mdev = new CCache(this, dev_section, dev_name);
    devtype = 0;
  } else if (!strcasecmp(device_type, "nicecache")) {
    mdev = new NICECache(this, dev_section, dev_name);
    devtype = 1;
  } else if (!strcasecmp(device_type, "SL0")) {
    //mdev = new SL0Cache(this, dev_section, dev_name);
		I(0);
    devtype = 2;
  } else if (!strcasecmp(device_type, "VPC")) {
    //mdev = new VPC(this, dev_section, dev_name);
		I(0);
    devtype = 3;
  } else if (!strcasecmp(device_type, "ghb")) {
    //mdev = new GHB(this, dev_section, dev_name);
		I(0);
    devtype = 4;

  } else if (!strcasecmp(device_type, "stridePrefetcher")) {
    mdev = new StridePrefetcher(this, dev_section, dev_name);
    devtype = 5;
  } else if (!strcasecmp(device_type, "markovPrefetcher")) {
    mdev = new MarkovPrefetcher(this, dev_section, dev_name);
    devtype = 6;
/*
  } else if (!strcasecmp(device_type, "taggedPrefetcher")) {
    mdev = new TaggedPrefetcher(this, dev_section, dev_name);
    devtype = 7;
*/
  }  else if (!strcasecmp(device_type, "bus")) {
    mdev = new Bus(this, dev_section, dev_name);
    devtype = 8;
  } else if (!strcasecmp(device_type, "tlb")) {
    mdev = new TLB(this, dev_section, dev_name);
    devtype = 9;
  } else if (!strcasecmp(device_type, "splitter")) {
    //mdev = new Splitter(this, dev_section, dev_name);
		I(0);
    devtype = 10;
  } else if (!strcasecmp(device_type, "memxbar")) {
    mdev = new MemXBar(this, dev_section, dev_name);    
    devtype = 11;
  } else if (!strcasecmp(device_type, "unmemxbar")) {
    mdev = new UnMemXBar(this, dev_section, dev_name);    
    devtype = 12;
  }  else if (!strcasecmp(device_type, "memcontroller")) {
    mdev = new MemController(this, dev_section, dev_name);
    devtype = 13;
  } else if (!strcasecmp(device_type, "void")) {
    return NULL;
  } else {
    // Check the lower level because it may have it
    return GMemorySystem::buildMemoryObj(device_type, dev_section, dev_name);
  }


#if GOOGLE_GRAPH_API
#else
  mystr += "\n\"";
  mystr += mdev->getName();

  switch(devtype){
    case 0: //CCache
    case 1: //NiceCache
    case 2: //SL0
    case 3: //VPC
      mystr += "\"[shape=record,sides=5,peripheries=2,color=darkseagreen,style=filled]";
      //Fetch the CCache related parameters : Size, bsize, name, inclusive, ports
      break;
    case 4: //GHB
    case 5: //stridePrefetcher
    case 6: //markovPrefetcher
    case 7: //taggedPrefetcher
      mystr += "\"[shape=record,sides=5,peripheries=1,color=lightcoral,style=filled]";
      break;
    case 8://bus
      mystr += "\"[shape=record,sides=5,peripheries=1,color=lightpink,style=filled]";
      break;
    case 10: //Splitter
    case 11: //MemXBar
      mystr += "\"[shape=record,sides=5,peripheries=1,color=thistle,style=filled]";
      break;
    case 12: //memcontroller
      mystr += "\"[shape=record,sides=5,peripheries=1,color=skyblue,style=filled]";
      break;
    default:
      mystr += "\"[shape=record,sides=5,peripheries=3,color=white,style=filled]";
      break;
  }
  arch.addObj(mystr);
#endif

  I(mdev);
  addMemObjName(mdev->getName());
#ifdef DEBUG
  LOG("Added: %s", mdev->getName());
#endif

  return mdev;
}
/* }}} */
