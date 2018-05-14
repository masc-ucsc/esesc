// Contributed by Basilio Fraguela
//                Jose Renau
//                James Tuck
//                Smruti Sarangi
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <math.h>

#include "DrawArch.h"
#include "GMemorySystem.h"
#include "MemObj.h"
#include "SescConf.h"

MemoryObjContainer            GMemorySystem::sharedMemoryObjContainer;
GMemorySystem::StrCounterType GMemorySystem::usedNames;
std::vector<std::string>      GMemorySystem::MemObjNames;
DrawArch                      arch;

//////////////////////////////////////////////
// MemoryObjContainer

void MemoryObjContainer::addMemoryObj(const char *device_name, MemObj *obj) {
  intlMemoryObjContainer[device_name] = obj;
  mem_node.push_back(obj);
}

MemObj *MemoryObjContainer::searchMemoryObj(const char *descr_section, const char *device_name) const {
  I(descr_section);
  I(device_name);

  StrToMemoryObjMapper::const_iterator it = intlMemoryObjContainer.find(device_name);

  if(it != intlMemoryObjContainer.end()) {

    const char *descrSection = (*it).second->getSection();

    if(strcasecmp(descrSection, descr_section)) {

      MSG("Two versions of MemoryObject [%s] with different definitions [%s] and [%s]", device_name, descrSection, descr_section);
      exit(-1);
    }

    return (*it).second;
  }

  return NULL;
}

/* Only returns a pointer if there is only one with that name */
MemObj *MemoryObjContainer::searchMemoryObj(const char *device_name) const {
  I(device_name);

  if(intlMemoryObjContainer.count(device_name) != 1)
    return NULL;

  return (*(intlMemoryObjContainer.find(device_name))).second;
}

void MemoryObjContainer::clear() {
  intlMemoryObjContainer.clear();
}

//////////////////////////////////////////////
// GMemorySystem

GMemorySystem::GMemorySystem(int32_t processorId)
    : coreId(processorId) {
  localMemoryObjContainer = new MemoryObjContainer();

  DL1  = 0;
  IL1  = 0;
  vpc  = 0;
  pref = 0;

  priv_counter = 0;
}

GMemorySystem::~GMemorySystem() {
  if(DL1)
    delete DL1;

  if(IL1 && DL1 != IL1)
    delete IL1;

  if(vpc)
    delete vpc;

  if(pref)
    delete pref;

  delete localMemoryObjContainer;
}

MemObj *GMemorySystem::buildMemoryObj(const char *type, const char *section, const char *name) {
  if(!(strcasecmp(type, "dummy") == 0 || strcasecmp(type, "cache") == 0 || strcasecmp(type, "icache") == 0 ||
       strcasecmp(type, "scache") == 0
       /* prefetcher */
       || strcasecmp(type, "markovPrefetcher") == 0 || strcasecmp(type, "stridePrefetcher") == 0 ||
       strcasecmp(type, "Prefetcher") == 0
       /* #ifdef FERMI {{{1 */
       || strcasecmp(type, "splitter") == 0 ||
       strcasecmp(type, "siftsplitter") == 0
       /* #endif }}} */
       || strcasecmp(type, "smpcache") == 0 || strcasecmp(type, "memxbar") == 0)) {
    MSG("Invalid memory type [%s]", type);
    SescConf->notCorrect();
  }

  return new DummyMemObj(section, name);
}

void GMemorySystem::buildMemorySystem() {
  SescConf->isCharPtr("", "cpusimu", coreId);

  const char *def_block = SescConf->getCharPtr("", "cpusimu", coreId);

  IL1 = declareMemoryObj(def_block, "IL1");
  IL1->getRouter()->fillRouteTables();
  IL1->setCoreIL1(coreId);
  if(strcasecmp(IL1->getDeviceType(), "TLB") == 0)
    IL1->getRouter()->getDownNode()->setCoreIL1(coreId);

  if(SescConf->checkCharPtr("cpusimu", "VPC", coreId)) {

    vpc = declareMemoryObj(def_block, "VPC");
    vpc->getRouter()->fillRouteTables();

    if(vpc == IL1) {
      MSG("ERROR: you can not set the VPC to the same cache as the IL1");
      SescConf->notCorrect();
    }

    DL1 = declareMemoryObj(def_block, "DL1");
    DL1->setCoreDL1(coreId);
    if(strcasecmp(DL1->getDeviceType(), "TLB") == 0)
      DL1->getRouter()->getDownNode()->setCoreDL1(coreId);
  } else {
    DL1 = declareMemoryObj(def_block, "DL1");
    DL1->getRouter()->fillRouteTables();
    DL1->setCoreDL1(coreId);
    if(strcasecmp(DL1->getDeviceType(), "TLB") == 0)
      DL1->getRouter()->getDownNode()->setCoreDL1(coreId);
    else if(strcasecmp(DL1->getDeviceType(), "Prefetcher") == 0)
      DL1->getRouter()->getDownNode()->setCoreDL1(coreId);
  }

  if(DL1 == vpc) {
    MSG("ERROR: you can not set the VPC to the same cache as the DL1");
    SescConf->notCorrect();
  }
}

char *GMemorySystem::buildUniqueName(const char *device_type) {
  int32_t num;

  StrCounterType::iterator it = usedNames.find(device_type);
  if(it == usedNames.end()) {

    usedNames[device_type] = 0;
    num                    = 0;

  } else
    num = ++(*it).second;

  size_t size = strlen(device_type);
  char * ret  = (char *)malloc(size + 6 + (int)log10((float)num + 10));
  sprintf(ret, "%s(%d)", device_type, num);

  return ret;
}

char *GMemorySystem::privatizeDeviceName(char *given_name, int32_t num) {
  char *ret = new char[strlen(given_name) + 8 + (int)log10((float)num + 10)];

  sprintf(ret, "%s(%d)", given_name, num);

  delete[] given_name;

  return ret;
}

MemObj *GMemorySystem::searchMemoryObj(bool shared, const char *section, const char *name) const {
  return getMemoryObjContainer(shared)->searchMemoryObj(section, name);
}

MemObj *GMemorySystem::searchMemoryObj(bool shared, const char *name) const {
  return getMemoryObjContainer(shared)->searchMemoryObj(name);
}

MemObj *GMemorySystem::declareMemoryObj_uniqueName(char *name, char *device_descr_section) {
  std::vector<char *> vPars;
  vPars.push_back(device_descr_section);
  vPars.push_back(name);
  vPars.push_back(strdup("shared"));
  // TODO: add a third vPars here to be compatible with the code below
  MemObj *ret = finishDeclareMemoryObj(vPars);
  return ret;
}

MemObj *GMemorySystem::declareMemoryObj(const char *block, const char *field) {
  SescConf->isCharPtr(block, field);
  std::vector<char *> vPars = SescConf->getSplitCharPtr(block, field);
  if(!vPars.size()) {
    MSG("ERROR: Section [%s] field [%s] does not describe a MemoryObj\n", block, field);
    MSG("ERROR: Required format: memoryDevice = descriptionSection [name] [shared|private]\n");
    SescConf->notCorrect();
    I(0);
    return 0; // Known-error mode
  }

  MemObj *ret = finishDeclareMemoryObj(vPars);
  I(ret); // Users of declareMemoryObj dereference without NULL check so pointer must be valid
  return ret;
}

MemObj *GMemorySystem::finishDeclareMemoryObj(std::vector<char *> vPars, char *name_suffix) {
  bool shared     = false; // Private by default
  bool privatized = false;

  const char *device_descr_section = vPars[0];
  char *      device_name          = (vPars.size() > 1) ? vPars[1] : 0;

  if(vPars.size() > 2) {

    if(strcasecmp(vPars[2], "shared") == 0) {

      I(vPars.size() == 3);
      shared = true;

    } else if(strcasecmp(vPars[2], "sharedBy") == 0) {

      I(vPars.size() == 4);
      int32_t sharedBy = atoi(vPars[3]);
      // delete[] vPars[3];
      GMSG(sharedBy <= 0, "ERROR: SharedBy should be bigger than zero (field %s)", device_name);
      I(sharedBy > 0);

      int32_t nId = coreId / sharedBy;
      device_name = privatizeDeviceName(device_name, nId);
      shared      = true;
      privatized  = true;
    }

  } else if(device_name) {
    if(strcasecmp(device_name, "shared") == 0) {
      shared = true;
    }
  }

  SescConf->isCharPtr(device_descr_section, "deviceType");
  const char *device_type = SescConf->getCharPtr(device_descr_section, "deviceType");

  /* If the device has been given a name, we may be refering to an
   * already existing device in the system, so let's search
   * it. Anonymous devices (no name given) are always unique, and only
   * one reference to them may exist in the system.
   */

  if(device_name) {

    if(!privatized) {
      if(shared) {
        device_name = privatizeDeviceName(device_name, 0);
      } else {
        // device_name = privatizeDeviceName(device_name, priv_counter++);
        device_name = privatizeDeviceName(device_name, coreId);
      }
    }
    char *final_dev_name = device_name;

    if(name_suffix != NULL) {
      final_dev_name = new char[strlen(device_name) + strlen(name_suffix) + 8];
      sprintf(final_dev_name, "%s%s", device_name, name_suffix);
    }

    device_name = final_dev_name;

    MemObj *memdev = searchMemoryObj(shared, device_descr_section, device_name);

    if(memdev) {
      // delete[] device_name;
      // delete[] device_descr_section;
      return memdev;
    }

  } else
    device_name = buildUniqueName(device_type);

  MemObj *newMem = buildMemoryObj(device_type, device_descr_section, device_name);

  if(newMem) // Would be 0 in known-error mode
    getMemoryObjContainer(shared)->addMemoryObj(device_name, newMem);

  return newMem;
}

DummyMemorySystem::DummyMemorySystem(int32_t coreId)
    : GMemorySystem(coreId) {
  // Do nothing
}

DummyMemorySystem::~DummyMemorySystem() {
  // Do nothing
}
