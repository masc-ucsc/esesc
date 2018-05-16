/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela
                  Luis Ceze
                  Smruti Sarangi
                  Paul Sack

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

#include <ctype.h>
#include <string.h>

#include "SescConf.h"

SConfig *SescConf = 0;

const char *SConfig::jump_spaces(const char *p) {

  while(isspace(*p))
    p++;

  return p;
}

char *SConfig::auxstrndup(const char *source, int32_t len) {

  char *q = new char[len + 1];

  strncpy(q, source, len);
  q[len] = 0;

  return q;
}

const char *SConfig::getConfName(int argc, const char **argv) {

  for(int i = 0; i < argc; i++) {
    if(argv[i][0] != '-' || argv[i][1] != 'c')
      continue;

    // -c found
    if(argv[i][2] == 0) {
      if(argc == (i + 1)) {
        MSG("ERROR: Invalid command line call. Use foo -c esesc.conf");
        exit(-2);
      }

      return argv[i + 1];
    }
    return &argv[i][2];
  }

  if(getenv("ESESCCONF"))
    return getenv("ESESCCONF");

  return "esesc.conf";
}

/* END Aux func */

SConfig::SConfig(int argc, const char **argv)
    : Config(getConfName(argc, argv), "ESESC") {
}

const char *SConfig::getEnvVar(const char *block, const char *name) {

  const char *val = Config::getEnvVar("", name);

  if(val)
    return val;

  return Config::getEnvVar(block, name);
}

const SConfig::Record *SConfig::getRecord(const char *block, const char *name, int32_t vectorPos) {

  const Record *rec = Config::getRecord(block, name, vectorPos);
  if(rec)
    return rec;

  // Use Indirection when neither of [block]name or []name exists. Indirection
  // can not handle vector inside a block.

  rec = Config::getRecord("", block, vectorPos);
  if(rec == 0)
    return Config::getRecord(block, name, vectorPos);

  const char *secName = rec->getCharPtr();

  if(secName == 0)
    return rec;

  return Config::getRecord(secName, name, 0);
}

std::vector<char *> SConfig::getSplitCharPtr(const char *block, const char *name, int32_t vectorPos) {
  std::vector<char *> vRes;
  const char *        q;

  const char *source = getCharPtr(block, name, vectorPos);
  source             = jump_spaces(source);

  while(*source) {
    q = source;
    while(isalnum(*q) || *q == '_') {
      q++;
    }

    if(source == q)
      break; // May be an error could be yielded

    vRes.push_back(auxstrndup(source, q - source));
    q = jump_spaces(q);

    source = q;
  }

  return vRes;
}
