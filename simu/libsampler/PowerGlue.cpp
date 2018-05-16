// Contributed by Ehsan K.Ardestani
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

#include "PowerGlue.h"
#include "Bundle.h"
#include "GMemorySystem.h"
#include "GProcessor.h"
#include "Report.h"
#include "SescConf.h"
#include "Wrapper.h"
#include <cstring>

PowerGlue::PowerGlue() {
  nStats = 0;
}

void PowerGlue::plug(const char *section, ChipEnergyBundle *eBundle) {
  energyBundle = eBundle;
  pwr_sec      = section;
  reFloorplan  = SescConf->getBool(section, "reFloorplan");
}

void PowerGlue::createStatCounters() {

  nStats = 0;
  createCoreStats();
  createMemStats();
}

void PowerGlue::createCoreStats() {

  uint32_t ncores = SescConf->getRecordSize("", "cpusimu");
  for(FlowID i = 0; i < ncores; i++) {
    const char *section = SescConf->getCharPtr("", "cpusimu", i);

    const char *temp_sec = 0;
    bool        inorder  = SescConf->getBool(section, "inorder");
    if(SescConf->checkInt(section, "sp_per_sm")) {
      if(SescConf->getInt(section, "sp_per_sm") >= 1) {
        inorder = true; // GPU
      }
    }
    if(inorder) { // probably GPU?
      temp_sec = "inorderPwrCounterTemplate";
    } else {
      temp_sec = "OoOPwrCounterTemplate";
    }
    createCoreStats(temp_sec, i);
  }
}

void PowerGlue::createCoreStats(const char *temp_sec, uint32_t i) {

  char *str;

  uint32_t statsCount = SescConf->getRecordSize(temp_sec, "template");

  for(uint32_t j = 0; j < statsCount; j++) {
    const char *stats_string = SescConf->getCharPtr(temp_sec, "template", j);
    char        s[32];
    sprintf(s, "P(%d)", i);
    str = replicateVar(stats_string, s, s);
    addVRecord(pwr_sec, str);
    nStats++;
  }
}

void PowerGlue::createCoreDescr(const char *temp_sec, uint32_t i) {

  uint32_t    statsCount      = SescConf->getRecordSize(temp_sec, "template");
  const char *layoutDescr_sec = SescConf->getCharPtr("SescTherm", "layoutDescr", 0);

  for(uint32_t j = 0; j < statsCount; j++) {
    const char *stats_string = SescConf->getCharPtr(temp_sec, "template", j);
    char        s[32];
    sprintf(s, "P(%d)", i);
    string ret_str = replicateVar(stats_string, s, s);

    size_t pos = 0;
    while((pos = ret_str.find('+')) != string::npos) {
      ret_str.erase(pos, 1);
    }
    addVRecord(layoutDescr_sec, ret_str.c_str());
    nStats++;
  }
}

void PowerGlue::autoCreateCoreDescr(const char *temp_sec) { /*{{{*/
  // uint32_t statsCount = SescConf->getRecordSize(temp_sec, "template");
  const char *layoutDescr_sec = SescConf->getCharPtr("SescTherm", "layoutDescr", 0);
  for(uint32_t j = 0; j < energyBundle->coreEIdx; j++) {
    string bname = energyBundle->cntrs[j].getName();
    char   ret_str[512];
    sprintf(ret_str, "blockMatch[%d] = %s", j, bname.c_str());
    addVRecord(layoutDescr_sec, ret_str);
    nStats++;
  }
} /*}}}*/

void PowerGlue::createMemStats() { /*{{{*/

  GMemorySystem *Memsys = getSimu(0)->getMemorySystem();
  // Just a pointer to memory system to get the created objects
  for(uint32_t j = 0; j < Memsys->getNumMemObjs(); j++) {
    string name = Memsys->getMemObjName(j);
    if((name.find("DL1") != string::npos) && (name.find("IL1") != string::npos)) {
      // FIXME!!! Shouldn't it be ||?
      // FIXME!!! Crude way of finding out which one is the first level data cache.
      I(fprintf(stderr, "\nNOT Using template for object [%s]", name.c_str()));
      continue;
    }

    char *pname = privateName(&name); // all the objects will have a (%d) at the end

    size_t pos = name.find("MemBus");
    if(pos != string::npos) {
      const char *temp_sec = "MCPwrCounterTemplate";
      I(fprintf(stderr, "\nUsing template[%s] for object [%s]", temp_sec, name.c_str()));
      createMemObjStats(temp_sec, pname, name.c_str());
      continue;
    }

    pos = name.find("TLB");
    if(pos != string::npos) {
      const char *temp_sec = "TLBPwrCounterTemplate";
      I(fprintf(stderr, "\nUsing template[%s] for object [%s]", temp_sec, name.c_str()));
      createMemObjStats(temp_sec, pname, name.c_str());
      continue;
    }

    pos = name.find("Split");
    if(pos != string::npos) {
      I(fprintf(stderr, "\nNo Power Stats for Splitter %s\n", name.c_str()));
      continue;
    }

    pos = name.find("Join");
    if(pos != string::npos) {
      I(fprintf(stderr, "\nNo Power Stats for Joiner %s\n", name.c_str()));
      continue;
    }

    const char *temp_sec = "MemPwrCounterTemplate";
    createMemObjStats(temp_sec, pname, name.c_str());
    I(fprintf(stderr, "\nUsing template[%s] for object [%s]", temp_sec, name.c_str()));
  }
} /*}}}*/

void PowerGlue::createMemObjStats(const char *temp_sec, const char *pname, const char *name) {

  uint32_t statsCount = SescConf->getRecordSize(temp_sec, "template");
  char *   str;

  for(uint32_t i = 0; i < statsCount; i++) {

    const char *stats_string = SescConf->getCharPtr(temp_sec, "template", i);
    str                      = replicateVar(stats_string, pname, name);
    addVRecord(pwr_sec, str);
    nStats++;
  }
}

void PowerGlue::createCoreLayoutDescr() {

#if 1
  const char *temp_sec = "LayoutDescrTemplate";
  autoCreateCoreDescr(temp_sec);
#else

  uint32_t ncores = SescConf->getRecordSize("", "cpusimu");
  for(FlowID i = 0; i < ncores; i++) {
    const char *section = SescConf->getCharPtr("", "cpusimu", i);

    const char *temp_sec = 0;
    bool        inorder  = SescConf->getBool(section, "inorder");
    if(inorder) { // probably GPU?
      temp_sec = "inorderLayoutDescrTemplate";
    } else {
      temp_sec = "OoOLayoutDescrTemplate";
    }
    createCoreDescr(temp_sec, i);
  }
#endif
}

void PowerGlue::createMemLayoutDescr() {

  const char *   layoutDescr_sec = SescConf->getCharPtr("SescTherm", "layoutDescr", 0);
  GMemorySystem *Memsys          = getSimu(0)->getMemorySystem();
  // Just a pointer to memory system to get the created objects
  for(uint32_t j = 0; j < Memsys->getNumMemObjs(); j++) {
    char   str[1024];
    string name = Memsys->getMemObjName(j);
    if((name.find("DL1") != string::npos) || (name.find("IL1") != string::npos) || (name.find("nice") != string::npos) || // Memory
       (name.find("TLB") != string::npos))
      continue;
    char *pname = privateName(&name); // all the objects will have a (%d) at the end
    sprintf(str, "blockMatch[%d] = %s", nStats++, pname);
    addVRecord(layoutDescr_sec, str);
    free(pname);
  }
}

char *PowerGlue::privateName(const string *name) {
  size_t pos = name->find("(");
  if(pos != string::npos)
    return strdup(name->c_str());

  string pname = (*name) + "(0)";
  return strdup(pname.c_str());
}

char *PowerGlue::replicateVar(const char *format, const char *s1, const char *s) {

  uint32_t    nvar = 0;
  const char *end  = format;

  while(*end) {
    if(*end == '+') {
      nvar++;
    }
    end++;
  }

  char *ret;

  switch(nvar) {
  case 0:
    ret = getText(format, nStats);
    break;
  case 1:
    ret = getText(format, nStats, s1);
    break;
  case 2:
    ret = getText(format, nStats, s1, s);
    break;
  case 3:
    ret = getText(format, nStats, s1, s, s);
    break;
  case 4:
    ret = getText(format, nStats, s1, s, s, s);
    break;
  case 5:
    ret = getText(format, nStats, s1, s, s, s, s);
    break;
  case 6:
    ret = getText(format, nStats, s1, s, s, s, s, s);
    break;
  case 7:
    ret = getText(format, nStats, s1, s, s, s, s, s, s);
    break;
  case 8:
    ret = getText(format, nStats, s1, s, s, s, s, s, s, s);
    break;
  case 9:
    ret = getText(format, nStats, s1, s, s, s, s, s, s, s, s);
    break;
  case 10:
    ret = getText(format, nStats, s1, s, s, s, s, s, s, s, s, s);
    break;
  default:
    printf("\nError! A template has %d vaiables\n", nvar);
    printf("The number of parameters to be filled in the template is limited to 8/\n");
    exit(-1); // exit right away to avoid writing junk!
    break;
  }

  return ret;
}

char *PowerGlue::getText(const char *format, ...) {

  char    str[2048];
  va_list vList;

  va_start(vList, format);
  vsprintf(str, format, vList);
  va_end(vList);

  return strdup(str);
}

void PowerGlue::dumpFlpDescr(uint32_t coreEIdx) {

  if(!reFloorplan)
    return;

  FILE *sysfp = fopen("sysFlp.desc", "w");
  if(!sysfp) {
    printf("Error! Cannot open sysFlp.desc for write.\n");
    SescConf->notCorrect();
  }

  /*********** Dump System FLP Descriptor ***********/
  /* system consists of cores, L2s, L3s, and MC   */
  fprintf(sysfp, "# comment lines begin with a '#'\n");
  fprintf(sysfp, "# comments and empty lines are ignored\n");
  fprintf(sysfp, "# Area and aspect ratios of blocks\n");
  fprintf(sysfp, "# Line Format: <unit-name>\t<area-in-m2>\t<min-aspect-ratio>\t<max-aspect-ratio>\t<rotable>\n");
  fprintf(sysfp, "\n\n\n");

  double totalArea = 0.0;
  for(uint32_t i = 0; i < energyBundle->cntrs.size(); i++) {
    string name = (*energyBundle).cntrs[i].getName();
    if(name.find("MemBus") != string::npos)
      fprintf(sysfp, "%s\t%e\t%d\t%d\t%d\n", (*energyBundle).cntrs[i].getName(), (*energyBundle).cntrs[i].getArea(), 1, 10, 1);
    else
      fprintf(sysfp, "%s\t%e\t%d\t%d\t%d\n", (*energyBundle).cntrs[i].getName(), (*energyBundle).cntrs[i].getArea(), 1, 4, 1);
    totalArea += (*energyBundle).cntrs[i].getArea();
  }

  fprintf(sysfp, "# Total Area: %g\n", totalArea);

  fprintf(sysfp, "\n\n\n");
  fprintf(sysfp, "# Connectivity information\n");
  fprintf(sysfp, "# Line format <unit1-name>\t<unit2-name>\t<wire_density>\n");
  fprintf(sysfp, "\n\n\n");

  for(uint32_t i = 0; i < energyBundle->cntrs.size(); i++) {
    for(uint32_t j = 0; j < size(energyBundle->cntrs[i].getCoreConn()); j++) {
      int density = 1;
      if(i < coreEIdx)
        density = 10;
      char *str = getStr(energyBundle->cntrs[i].getCoreConn(), j);
      fprintf(sysfp, "%s\t%s\t%d\n", (*energyBundle).cntrs[i].getName(), str, density);
      free(str);
    }
  }

  fclose(sysfp);

  /*********** Dump sys TDP ***********/

  sysfp = fopen("sysTDP.p", "w");
  if(!sysfp) {
    printf("Error! Cannot open sysTDP.p for write.\n");
    SescConf->notCorrect();
  }

  for(uint32_t i = 0; i < energyBundle->cntrs.size(); i++) {
    fprintf(sysfp, "%s\t%e\n", (*energyBundle).cntrs[i].getName(), (*energyBundle).cntrs[i].getTdp());
  }
  fclose(sysfp);

  /* Maybe call HotFloorplan? */
  printf("McPAT counters generated\n");
  printf("Floorplan descriptors (sysFlp.desc) and TDP (sysTDP.p) generated.\n");
  printf("You need to lunch reFloorplan.rb from the conf directory\n");
  printf("To run esesc, set the 'reFloorplan = flase' in pwth.conf\n");
  exit(0);
}

uint32_t PowerGlue::size(const char *str) {
  if(strlen(str) == 0)
    return 0;

  const char *end = str;
  while(*end && *end == ' ')
    end++; // skip the spaces
  uint32_t ret = 0;
  while(*(end++)) {
    if(*end == ' ') {
      while(*end && *end == ' ')
        end++; // skip the rest of spaces
      ret++;
    }
  }
  return ++ret;
}

char *PowerGlue::getStr(const char *str, uint32_t i) {

  char        ret_str[254];
  const char *end = str;
  while(*end && *end == ' ')
    end++; // skip the spaces
  const char *start = end;
  uint32_t    ret   = 0;

  while(*(++end)) {
    if(*end == ' ') {
      if(ret == i)
        break;
      while(*end && *end == ' ')
        end++; // skip the rest of spaces
      if(*end) {
        ret++;
        start = end;
      }
    }
  }

  I(i == ret);
  uint32_t len = end - start;
  I(len < 254);
  strncpy(ret_str, start, len);
  ret_str[len] = '\0';
  return strdup(ret_str);
}

void PowerGlue::checkStatCounters(const char *section) {

  // a trivial check to see if there is any obvious mismatch
  // between stats.conf and eses.conf

  // check cores
  uint32_t ncores = SescConf->getRecordSize("", "cpusimu");
  uint32_t nstats = SescConf->getRecordSize(section, "stats");
  bool     found  = false;

  for(FlowID i = 0; i < ncores; i++) {
    char name[64];
    found = false;
    sprintf(name, "P(%d)", i);
    for(uint32_t j = 0; j < nstats; j++) {
      string stats_string = SescConf->getCharPtr(section, "stats", j);
      size_t pos          = stats_string.find(name);
      if(pos != string::npos) {
        found = true;
        break;
      }
    }

    if(!found) {
      printf("\n\nError! stats.conf does not match the configuration.\n");
      printf("Cannot find stats for '%s'.\n", name);
      printf("rerun eSESC with 'reFloorplan = true' in pwth.conf to update the stats.\n");
      exit(-1);
    }
  }

  // also check if there are more cores in the stat than specified in esesc.conf
  {
    char name[64];
    found = false;
    sprintf(name, "P(%d)", ncores);
    for(uint32_t j = 0; j < nstats; j++) {
      string stats_string = SescConf->getCharPtr(section, "stats", j);
      size_t pos          = stats_string.find(name);
      if(pos != string::npos) {
        found = true;
        break;
      }
    }

    if(found) {
      printf("\n\nError! stats.conf does not match the configuration.\n");
      printf("stats.conf contains more cores.\n");
      printf("rerun eSESC with 'reFloorplan = true' in pwth.conf to update the stats.\n");
      exit(-1);
    }
  }

  // check memory
  GMemorySystem *Memsys = getSimu(0)->getMemorySystem();
  for(uint32_t j = 0; j < Memsys->getNumMemObjs(); j++) {
    string name = Memsys->getMemObjName(j);
    found       = false;
    for(uint32_t j = 0; j < nstats; j++) {
      string stats_string = SescConf->getCharPtr(section, "stats", j);
      size_t pos          = stats_string.find(name);
      if(pos != string::npos) {
        found = true;
        break;
      }
    }

    if(!found) {
      printf("\n\nError! stats.conf does not match the configuration.\n");
      printf("Cannot find stats for '%s'.\n", name.c_str());
      printf("rerun eSESC with 'reFloorplan = true' in pwth.conf to update the stats.\n");
      exit(-1);
    }
  }
}

void PowerGlue::addVRecord(const char *sec, const char *str) {
  const char *end   = str;
  const char *start = str;
  char *      name;
  char *      val;

  while(*end) {
    if(*end == '[')
      break;
    end++;
  }

  size_t len = end - start;
  name       = static_cast<char *>(std::malloc(len + 1));
  name[len]  = '\0';
  std::strncpy(name, start, len);

  while(*end) {
    if(*end == '=')
      break;
    end++;
  }

  end++;
  while(*end) {
    if(*end != ' ')
      break;
    end++;
  }
  len      = strlen(str) - len;
  val      = static_cast<char *>(std::malloc(len + 1));
  val[len] = '\0';
  std::strncpy(val, end, len);

  SescConf->addVRecord(sec, name, val, nStats, nStats);
}
