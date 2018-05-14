/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Basilio Fraguela
                  Smruti Sarangi
      Milos Prvulovic

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

ESESC is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with ESESC; see the file COPYING. If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#include <limits.h>
#include <string.h>
#include <strings.h>

#include "Config.h"
#include "Report.h"
#include "alloca.h"

Config::Record::~Record(void) {

  if(type == RCCharPtr)
    free(v.CharPtr);
}

Config::Record::Record(bool val)
    : env(false)
    , used(false)
    , printed(false) {

  type   = RCBool;
  v.Bool = val;
  vrec   = false;
  X      = 0;
  Y      = 0;
}

Config::Record::Record(int32_t val)
    : env(false)
    , used(false)
    , printed(false) {

  type  = RCInt;
  v.Int = val;
  vrec  = false;
  X     = 0;
  Y     = 0;
}

Config::Record::Record(double val)
    : env(false)
    , used(false)
    , printed(false) {

  type     = RCDouble;
  v.Double = val;
  vrec     = false;
  X        = 0;
  Y        = 0;
}

Config::Record::Record(const char *val)
    : env(false)
    , used(false)
    , printed(false) {

  type      = RCCharPtr;
  v.CharPtr = strdup(val);
  vrec      = false;
  X         = 0;
  Y         = 0;
}

Config::Record::Record(bool val, int32_t x, int32_t y)
    : env(false)
    , used(false)
    , printed(false) {

  type   = RCBool;
  v.Bool = val;
  vrec   = true;
  X      = x;
  Y      = y;
}

Config::Record::Record(int32_t val, int32_t x, int32_t y)
    : env(false)
    , used(false)
    , printed(false) {

  type  = RCInt;
  v.Int = val;
  vrec  = true;
  X     = x;
  Y     = y;
}

Config::Record::Record(double val, int32_t x, int32_t y)
    : env(false)
    , used(false)
    , printed(false) {

  type     = RCDouble;
  v.Double = val;
  vrec     = true;
  X        = x;
  Y        = y;
}

Config::Record::Record(const char *val, int32_t x, int32_t y)
    : env(false)
    , used(false)
    , printed(false) {

  type      = RCCharPtr;
  v.CharPtr = strdup(val);
  vrec      = true;
  X         = x;
  Y         = y;
}

void Config::Record::dump(const char *prev, const char *post) {

  const char *pre;
  char *      cadena = (char *)malloc(strlen(prev) + 128);

  if(vrec && X == Y) {
    sprintf(cadena, "%s[%d]", prev, X);
    pre = cadena;
  } else if(vrec) {
    sprintf(cadena, "%s[%d:%d]", prev, X, Y);
    pre = cadena;
  } else {
    pre = prev;
  }

  if(type == RCInt)
    Report::field("%-10s=%d%s", pre, v.Int, post);
  else if(type == RCDouble)
    Report::field("%-10s=%e%s", pre, v.Double, post);
  else if(type == RCBool)
    Report::field("%-10s=%s%s", pre, v.Bool ? "true" : "false", post);
  else if(type == RCCharPtr) {
    if(strchr(v.CharPtr, ' '))
      Report::field("%-10s=\"%s\"%s", pre, v.CharPtr, post);
    else
      Report::field("%-10s=\'%s\'%s", pre, v.CharPtr, post);
  } else {
    I(0); // Unknown type
  }

  free(cadena);
}

/* ----------------------------------------  */
bool readConfigFile(Config *ptr, FILE *fp, const char *fpname); /* Defined in conflex.y */

Config::Config(const char *name, const char *envstr)
    : envstart(strdup(envstr))
    , errorReading(false) {

  fpname     = 0;
  errorFound = false;
  locked     = false;

  fp = fopen(name, "r");
  if(fp == 0) {
    MSG("Config:: Impossible to open the file [%s]", name);
    exit(-1);
  }

  fpname = strdup(name);

  if(!readConfigFile(this, fp, fpname)) {
    MSG("ERROR: could not read configuration file %s", fpname);
    exit(-1);
  }

  if(errorReading) {
    MSG("ERROR: could not process configuration file %s", fpname);
    exit(-1);
  }

  // clear references
  hashRecord_t::iterator hiter = hashRecord.begin();
  for(; hiter != hashRecord.end(); hiter++)
    hiter->second->setUnUsed();

  fclose(fp);
}

Config::~Config(void) {

  hashRecord_t::iterator hiter = hashRecord.begin();

  for(; hiter != hashRecord.end(); hiter++) {
    delete hiter->second; // Record *
  }

  free((void *)fpname);
  free((void *)envstart);
}

ssize_t Config::getRecordMin(const char *block, const char *name) {

  KeyIndex key(block, name);

  ssize_t                              min = LONG_MAX;
  typedef hashRecord_t::const_iterator I;
  std::pair<I, I>                      b = hashRecord.equal_range(key);
  for(I pos = b.first; pos != b.second; ++pos) {
    if(!pos->second->isVRecord()) {
      MSG("ERROR: trying to read record [%s:%s] as a vector while it is not", block, name);
      notCorrect();
    }
    if(pos->second->getVectorFirst() < min)
      min = pos->second->getVectorFirst();
  }
  if(min == LONG_MAX)
    min = 0;

  return min;
}

ssize_t Config::getRecordMax(const char *block, const char *name) {

  KeyIndex key(block, name);

  ssize_t                              min = 0;
  typedef hashRecord_t::const_iterator I;
  std::pair<I, I>                      b = hashRecord.equal_range(key);
  for(I pos = b.first; pos != b.second; ++pos) {
    if(!pos->second->isVRecord()) {
      MSG("ERROR: trying to read record [%s:%s] as a vector while it is not", block, name);
      notCorrect();
    }
    if(pos->second->getVectorLast() > min)
      min = pos->second->getVectorLast();
  }

  return min;
}

const Config::Record *Config::getRecord(const char *block, const char *name, int32_t vectorPos) {

  KeyIndex key(block, name);

  typedef hashRecord_t::const_iterator I;
  std::pair<I, I>                      b = hashRecord.equal_range(key);
  for(I pos = b.first; pos != b.second; ++pos) {
    if((pos->second->getVectorFirst() <= vectorPos && pos->second->getVectorLast() >= vectorPos)) {

      //       if( !pos->second->isUsed()  )
      //       MSG("found [%s] [%s] [%d-%d]%d",block,name
      //          ,pos->second->getVectorFirst(),pos->second->getVectorLast(),vectorPos);

      pos->second->setUsed();

      return pos->second;
    }
  }

  // error Message
  return 0;
}

void Config::addRecord(const char *block, const char *name, Config::Record *rec) {

  KeyIndex key(block, name);

  std::pair<const KeyIndex, Config::Record *> entry(key, rec);

  // Check that if it already exists in the hashRecord, it doesn't
  // collide in dimensions.

  typedef hashRecord_t::const_iterator I;

  std::pair<I, I> b = hashRecord.equal_range(key);

  for(I pos = b.first; pos != b.second; ++pos) {
    if((pos->second->getVectorFirst() <= rec->getVectorFirst() && pos->second->getVectorLast() >= rec->getVectorFirst()) ||
       (pos->second->getVectorLast() >= rec->getVectorLast() && pos->second->getVectorFirst() <= rec->getVectorLast()) ||
       (rec->getVectorFirst() <= pos->second->getVectorFirst() && rec->getVectorLast() >= pos->second->getVectorFirst()) ||
       (rec->getVectorLast() >= pos->second->getVectorLast() && rec->getVectorFirst() <= pos->second->getVectorLast())) {
      MSG("Config:: overlap between %s[%d:%d] and %s[%d:%d] in section %s", pos->first.s2.c_str(), pos->second->getVectorFirst(),
          pos->second->getVectorLast(), name, rec->getVectorFirst(), rec->getVectorLast(), block);
      errorReading = true;
    }
  }

  hashRecord.insert(entry);
}

void Config::copyVariable(const char *block, const char *name, const char *val) {

  // MSG("1.variable [%s] [%s]=[%s]",block,name,val);

  const char *env;
  Record *    rec;

  if((env = getEnvVar(block, name))) {
    rec = new Record(env);
    rec->setEnv();
  } else {
    const Record *recR = getRecord(block, val, 0);

    if(recR == 0)
      recR = getRecord("", val, 0);
    if(recR)
      rec = new Record(*recR);
    else
      rec = new Record(val);
  }

  addRecord(block, name, rec);
}

void Config::addRecord(const char *block, const char *name, const char *val) {

  const char *env;
  Record *    rec;

  //  MSG("1.variable [%s] [%s]=[%s]",block,name,val);

  if((env = getEnvVar(block, name))) {
    rec = new Record(env);
    rec->setEnv();
  } else
    rec = new Record(val);

  addRecord(block, name, rec);
}

void Config::addVRecord(const char *block, const char *name, const char *val, int32_t X, int32_t Y) {

  Record *rec = NULL;

  if(X > Y) {
    MSG("Config:: [%s] %s[%d:%d]=[%s] vector invalid limits %d < %d ", block, name, X, Y, val, X, Y);
    errorReading = true;
  }

  const char *env = getEnvVar(block, name);
  if(env) {
    MSG("Config::addVRecord Not allowed to override vectors with environment variables");
    //    rec = new Record(env,X,Y);
    //    rec->setEnv();
    errorReading = true;
  } else {
    rec = new Record(val, X, Y);
  }

  addRecord(block, name, rec);
}

void Config::addVRecord(const char *block, const char *name, double val, int32_t X, int32_t Y) {

  Record *rec;

  if(X > Y) {
    MSG("Config:: [%s] %s[%d:%d]=[%g] vector invalid limits %d < %d ", block, name, X, Y, val, X, Y);
    errorReading = true;
  }

  const char *env = getEnvVar(block, name);
  if(env) {
    MSG("Config::addVRecord Not allowed to override vectors with environment variables");
    I(0); // halt here otherwise NULL ptr dereference in addRecord
          //        rec = new Record(env,X,Y);
          //        rec->setEnv();
    errorReading = true;
  }

  rec = new Record(val, X, Y);

  addRecord(block, name, rec);
}

void Config::addVRecord(const char *block, const char *name, int32_t val, int32_t X, int32_t Y) {

  Record *rec;

  if(X > Y) {
    MSG("Config:: [%s] %s[%d:%d]=[%d] vector invalid limits %d < %d ", block, name, X, Y, val, X, Y);
    errorReading = true;
  }

  const char *env = getEnvVar(block, name);
  if(env) {
    MSG("Config::addVRecord Not allowed to override vectors with environment variables");
    //        rec = new Record(env,X,Y);
    //        rec->setEnv();
    errorReading = true;
  }

  rec = new Record(val, X, Y);

  addRecord(block, name, rec);
}

void Config::addRecord(const char *block, const char *name, bool val) {

  const char *env;
  Record *    rec;

  if((env = getEnvVar(block, name))) {
    if(strncasecmp(env, "true", 4) == 0 || strncasecmp(env, "1", 1) == 0)
      val = true;
    else if(strncasecmp(env, "false", 5) == 0 || strncasecmp(env, "0", 1) == 0)
      val = false;
    else {
      MSG("Config:: Environment variable [%s] is not a valid bool (true/false)", env);
      errorReading = true;
    }
    rec = new Record(val);
    rec->setEnv();
  } else {
    rec = new Record(val);
  }

  addRecord(block, name, rec);
}

void Config::addRecord(const char *block, const char *name, int32_t val) {

  const char *env;
  Record *    rec;

  if((env = getEnvVar(block, name))) {
    if(isdigit(env[0])) {
      val = strtol(env, 0, 10);
    } else {
      MSG("Config:: Environment %s_%s variable [%s] is not a valid long", block, name, env);
      I(0);
      errorReading = true;
    }
    rec = new Record(val);
    rec->setEnv();
  } else {
    rec = new Record(val);
  }

  addRecord(block, name, rec);
}

void Config::addRecord(const char *block, const char *name, double val) {

  const char *env;
  Record *    rec;

  if((env = getEnvVar(block, name))) {
    if(isdigit(env[0])) {
      val = atof(env);
    } else {
      MSG("Config:: Environment %s_%s variable [%s] is not a valid double", block, name, env);
      errorReading = true;
    }
    rec = new Record(val);
    rec->setEnv();
  } else {
    rec = new Record(val);
  }

  addRecord(block, name, rec);
}

bool Config::getBool(const char *block, const char *name, int32_t vectorPos) {

  const Record *rec = getRecord(block, name, vectorPos);

  if(rec)
    return rec->getBool();

  MSG("Config::getBool for %s in %s[%d] not found in config file.", name, block, vectorPos);
  notCorrect();

  return false;
}

bool Config::checkBool(const char *block, const char *name, int32_t vectorPos) {

  const Record *rec = getRecord(block, name, vectorPos);

  if(rec)
    return rec->isBool();

  return false;
}

double Config::getDouble(const char *block, const char *name, int32_t vectorPos) {

  const Record *rec = getRecord(block, name, vectorPos);
  if(rec) {
    if(rec->isDouble())
      return rec->getDouble();
    else if(rec->isInt())
      return rec->getInt();
  }

  MSG("Config::getDouble for %s in %s[%d] not found in config file.", name, block, vectorPos);
  notCorrect();

  return 0;
}
bool Config::checkDouble(const char *block, const char *name, int32_t vectorPos) {

  const Record *rec = getRecord(block, name, vectorPos);
  if(rec)
    return rec->isDouble();

  return false;
}

int32_t Config::getInt(const char *block, const char *name, int32_t vectorPos) {

  const Record *rec = getRecord(block, name, vectorPos);

  if(rec)
    return rec->getInt();

  MSG("Config::getInt for %s in %s[%d] not found in config file.", name, block, vectorPos);
  notCorrect();

  return 0;
}

bool Config::checkInt(const char *block, const char *name, int32_t vectorPos) {

  const Record *rec = getRecord(block, name, vectorPos);
  if(rec)
    return rec->isInt();

  return false;
}

const char *Config::getCharPtr(const char *block, const char *name, int32_t vectorPos) {

  const Record *rec = getRecord(block, name, vectorPos);

  if(rec) {
    //    MSG("Config::getCharPtr for %s in %s[%d] found in config file.",name, block, vectorPos);
    return rec->getCharPtr();
  }

  MSG("Config::getCharPtr for %s in %s[%d] not found in config file.", name, block, vectorPos);
  notCorrect();

  return "";
}

bool Config::checkCharPtr(const char *block, const char *name, int32_t vectorPos) {

  const Record *rec = getRecord(block, name, vectorPos);

  if(rec)
    return rec->isCharPtr();

  return false;
}

extern char **environ;

const char *Config::getEnvVar(const char *block, const char *name) {

  size_t envLen = 0;
  char * envVar = 0;
  // Determine the variable name prefix (incl. the '=' char)
  // Note: the variable name prefix MUST include the '=' character,
  // otherwise the match can be wrong, e.g. when looking for
  // "ESESC_issue=3" we may find "ESESC_issueX_inorder=false" and use
  // "_inorder=false" as the value of "ESESC_issue".
  if(block[0] == 0) // Empty string
    envLen = snprintf(envVar, 0, "%s_%s=", envstart, name);
  else
    envLen = snprintf(envVar, 0, "%s_%s_%s=", envstart, block, name);
  // Allocate envVar (on the stack)
  envVar = static_cast<char *>(alloca(envLen + 1));
  // Now put the variable name prefix in envVar
  if(block[0] == 0) // Empty string
    snprintf(envVar, envLen + 1, "%s_%s=", envstart, name);
  else
    snprintf(envVar, envLen + 1, "%s_%s_%s=", envstart, block, name);
  // Look for envVar in environ
  const char *envVal = 0;
  for(int32_t i = 0; environ[i] != 0; i++) {
    if(strncasecmp(envVar, environ[i], envLen) == 0) {
      envVal = &environ[i][envLen];
      break;
    }
  }
  return envVal;
}

void Config::dump(bool showAll) {

  hashRecord_t::iterator hiter;
  const char *           block;

  Report::field("#BEGIN Configuration used. Extracted from \"%s\":", fpname);

  for(hiter = hashRecord.begin(); hiter != hashRecord.end(); hiter++)
    hiter->second->setUnPrinted();

  do {
    block = 0;
    hiter = hashRecord.begin();

    // First print the records with empty section
    for(; hiter != hashRecord.end(); hiter++) {
      if(!(showAll || hiter->second->isUsed()))
        continue;

      if(hiter->second->isPrinted()) // Already printed
        continue;

      if(!hiter->first.s1.empty())
        continue;

      hiter->second->setPrinted();

      if(hiter->second->isEnv())
        hiter->second->dump(hiter->first.s2.c_str(), " # Environment Variable");
      else
        hiter->second->dump(hiter->first.s2.c_str(), "");
    }

    // Print the other sections
    block = 0;
    hiter = hashRecord.begin();
    for(; hiter != hashRecord.end(); hiter++) {
      if((!showAll) && (!hiter->second->isUsed()))
        continue;
      if(hiter->second->isPrinted()) // Already printed
        continue;

      if(block == 0) {
        block = hiter->first.s1.c_str();
        Report::field("[%s]", block);
      }

      if(strcmp(hiter->first.s1.c_str(), block))
        continue;

      hiter->second->setPrinted();

      if(hiter->second->isEnv())
        hiter->second->dump(hiter->first.s2.c_str(), " # Environment Variable\n");
      else
        hiter->second->dump(hiter->first.s2.c_str(), "");
    }
  } while(block);

  Report::field("#END Configuration used. Extracted from \"%s\":", fpname);
  Report::flush();
}

bool Config::lock() {

  if(errorFound) {
    return false;
  }

  locked = true;

  return true;
}

// Restriction functions
void Config::notCorrect() {

  I(0);
  errorFound = true;
  if(locked) {
    MSG("Config::Constraint not meet while configuration already locked??");
    exit(-2);
  }
}

bool Config::isPower2(const char *block, const char *name, int32_t vectorPos) {
  const Record *rec = getRecord(block, name, vectorPos);

  if(rec == 0) {
    MSG("Config::isPower2 for %s in %s[%d] not found in config file.", name, block, vectorPos);
    notCorrect();
    return false;
  }

  double val;

  if(rec->isDouble())
    val = rec->getDouble();
  else if(rec->isInt())
    val = rec->getInt();
  else {
    MSG("Config::isPower2 for %s[%d] [%s] not satisfied. It's not a number", block, vectorPos, name);
    notCorrect();
    return false;
  }

  uint32_t v = (uint32_t)val;

  if(v != val) {
    MSG("Config::isPower2 for %s[%d] [%s] not satisfied(1)", block, vectorPos, name);
    notCorrect();
    return false;
  }

  if((v & (v - 1)) != 0) {
    MSG("Config::isPower2 for %s[%d] [%s] not satisfied(2)", block, vectorPos, name);
    notCorrect();
    return false;
  }

  return true;
}

bool Config::isBetween(const char *block, const char *name, double llim, double ulim, int32_t vectorPos) {

  const Record *rec = getRecord(block, name, vectorPos);

  if(rec == 0) {
    MSG("Config::isBetween for %s in %s[%d] not found in config file.", name, block, vectorPos);
    notCorrect();
    return false;
  }

  double val;

  if(rec->isDouble())
    val = rec->getDouble();
  else if(rec->isInt())
    val = rec->getInt();
  else {
    MSG("Config::isBetween for %s[%d] [%s] not satisfied. It's not a number", block, vectorPos, name);
    notCorrect();
    return false;
  }

  if(val < llim || val > ulim) {
    MSG("Config::isBetween for %s[%d] [%s] not satisfied(1)", block, vectorPos, name);
    notCorrect();
    return false;
  }

  return true;
}

bool Config::isGT(const char *block, const char *name, double llim, int32_t vectorPos) {

  const Record *rec = getRecord(block, name, vectorPos);

  if(rec == 0) {
    MSG("Config::isGT for %s in %s[%d] not found in config file.", name, block, vectorPos);
    notCorrect();
    return false;
  }

  double val;

  if(rec->isDouble())
    val = rec->getDouble();
  else if(rec->isInt())
    val = rec->getInt();
  else {
    MSG("Config::isGT for %s[%d] [%s] not satisfied. It's not a number", block, vectorPos, name);
    notCorrect();
    return false;
  }

  if(val <= llim) {
    MSG("Config::isGT for %s[%d] [%s] not satisfied(1)", block, vectorPos, name);
    notCorrect();
    return false;
  }

  return true;
}

bool Config::isLT(const char *block, const char *name, double ulim, int32_t vectorPos) {

  const Record *rec = getRecord(block, name, vectorPos);

  if(rec == 0) {
    MSG("Config::isLT for %s in %s[%d] not found in config file.", name, block, vectorPos);
    notCorrect();
    return false;
  }

  double val;

  if(rec->isDouble())
    val = rec->getDouble();
  else if(rec->isInt())
    val = rec->getInt();
  else {
    MSG("Config::isLT for %s[%d] [%s] not satisfied. It's not a number", block, vectorPos, name);
    notCorrect();
    return false;
  }

  if(val >= ulim) {
    MSG("Config::isLT for %s[%d] [%s] not satisfied(1)", block, vectorPos, name);
    notCorrect();
    return false;
  }

  return true;
}

bool Config::isBool(const char *block, const char *name, int32_t vectorPos) {
  const Record *rec = getRecord(block, name, vectorPos);

  if(rec == 0) {
    MSG("Config::isBool for %s in %s[%d] not found in config file.", name, block, vectorPos);
    notCorrect();
    return false;
  }

  if(!rec->isBool()) {
    MSG("Config::isBool for %s[%d] [%s] not satisfied", block, vectorPos, name);
    notCorrect();
    return false;
  }

  return true;
}

bool Config::isInt(const char *block, const char *name, int32_t vectorPos) {

  const Record *rec = getRecord(block, name, vectorPos);

  if(rec == 0) {
    MSG("Config::isInt for %s in %s[%d] not found in config file.", name, block, vectorPos);
    notCorrect();
    return false;
  }

  if(!rec->isInt()) {
    MSG("Config::isInt for %s[%d] [%s] not satisfied", block, vectorPos, name);
    notCorrect();
    return false;
  }

  return true;
}

bool Config::isDouble(const char *block, const char *name, int32_t vectorPos) {
  const Record *rec = getRecord(block, name, vectorPos);

  if(rec == 0) {
    MSG("Config::isDouble for %s in %s[%d] not found in config file.", name, block, vectorPos);
    notCorrect();
    return false;
  }

  if(!rec->isDouble()) {
    MSG("Config::isDouble for %s[%d] [%s] not satisfied", block, vectorPos, name);
    notCorrect();
    return false;
  }

  return true;
}

bool Config::isCharPtr(const char *block, const char *name, int32_t vectorPos) {
  const Record *rec = getRecord(block, name, vectorPos);

  if(rec == 0) {
    MSG("Config::isCharPtr for %s in %s[%d] not found in config file.", name, block, vectorPos);
    notCorrect();
    return false;
  }

  if(!rec->isCharPtr()) {
    MSG("Config::isCharPtr for %s[%d] [%s] not satisfied", block, vectorPos, name);
    notCorrect();
    return false;
  }

  return true;
}

bool Config::isInList(const char *block, const char *name, const char *l1, const char *l2, const char *l3, const char *l4,
                      const char *l5, const char *l6, const char *l7, const int32_t vectorPos) {
  const Record *rec = getRecord(block, name, vectorPos);

  if(rec == 0) {
    MSG("Config::isInList for %s in %s[%d] not found in config file.", name, block, vectorPos);
    notCorrect();
    return false;
  }

  const char *val = rec->getCharPtr();

  if(l1)
    if(strcasecmp(val, l1) == 0)
      return true;

  if(l2)
    if(strcasecmp(val, l2) == 0)
      return true;

  if(l3)
    if(strcasecmp(val, l3) == 0)
      return true;

  if(l4)
    if(strcasecmp(val, l4) == 0)
      return true;

  if(l5)
    if(strcasecmp(val, l5) == 0)
      return true;

  if(l6)
    if(strcasecmp(val, l6) == 0)
      return true;

  if(l7)
    if(strcasecmp(val, l7) == 0)
      return true;

  MSG("Config::isInList [%s] [%s] not valid name found", block, name);

  notCorrect();
  return false;
}

void Config::updateRecord(const char *block, const char *name, double v, int32_t vectorPos) {
  KeyIndex key(block, name);

  Record *rec = NULL;

  typedef hashRecord_t::const_iterator I;
  std::pair<I, I>                      b = hashRecord.equal_range(key);
  for(I pos = b.first; pos != b.second; ++pos) {
    if((pos->second->getVectorFirst() <= vectorPos && pos->second->getVectorLast() >= vectorPos)) {
      rec = pos->second;
      break;
    }
  }

  if(rec) {
    I(rec->isDouble());
    rec->setDouble(v);
  } else
    addRecord(block, name, v);
}

void Config::updateRecord(const char *block, const char *name, const char *val, int32_t vectorPos) {
  KeyIndex key(block, name);

  Record *rec = NULL;

  typedef hashRecord_t::const_iterator I;
  std::pair<I, I>                      b = hashRecord.equal_range(key);
  for(I pos = b.first; pos != b.second; ++pos) {
    if((pos->second->getVectorFirst() <= vectorPos && pos->second->getVectorLast() >= vectorPos)) {
      rec = pos->second;
      break;
    }
  }

  if(rec) {
    I(rec->isCharPtr());
    rec->setCharPtr(val);
  } else {
    addVRecord(block, name, val, vectorPos, vectorPos);
  }
}

void Config::getAllSections(std::vector<char *> &sections) {
  hashRecord_t::const_iterator u = hashRecord.begin();

  while(u != hashRecord.end()) {
    std::pair<KeyIndex, Record *> t = *u;
    KeyIndex                      k = t.first;

    const char *block = k.s1.c_str();

    // check if it is there
    bool exists = false;

    std::vector<char *>::iterator it = sections.begin();
    while(it != sections.end()) {
      char *v = *it;
      if(strcmp(v, block) == 0) {
        exists = true;
        break;
      }
      it++;
    }

    if(!exists)
      sections.push_back(strdup(block));

    u++;
  }
}
