/* License & includes {{{1 */
/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Ehsan Ardestani
                  Edward Kuang

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

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "memparse2.h"
using std::cout;
using std::endl;
using std::ifstream;
/* }}} */

const char* MemVerify::DELIMITER = " ";

/* Setup and reading of conf file {{{1 */
MemVerify::MemVerify(void) {
  //Set up auxiliary map for parsing different sections in shared.conf
  sharedconf_parse["[DL1_core]"] = false;
  sharedconf_parse["[PrivL2]"]   = false;
  sharedconf_parse["[L3Cache]"]  = false;
  sharedconf_parse["instWidth"]  = false;

  //Set up map for storing values read in shared.conf
  sharedconf_values["L1_bytes"]  = -1;
  sharedconf_values["L2_bytes"]  = -1;
  sharedconf_values["L3_bytes"]  = -1;
  sharedconf_values["instWidth"] = -1;

  //set up map for cache access order, i.e. no access to L3 before L2
  cache_consistency["L1"] = 0;
  cache_consistency["L2"] = 0;
  cache_consistency["L3"] = 0;
  cache_consistency["bigMem"] = 0; 

  read_configuration();
}

void MemVerify::read_configuration(void) {
  ifstream fin("./shared.conf");

  //error file not found
  if (!fin.good()) {
    cout << "shared.conf not found" << endl;
    exit(EXIT_FAILURE);
  }

  while (!fin.eof()) {
    const char* token[MAX_TOKENS] = {};
    char buffer[MAX_CHARS_PER_LINE];
    fin.getline(buffer, MAX_CHARS_PER_LINE);

    //parse line and place tokens into array, max 3 tokens
    if (!(token[0] = strtok(buffer, DELIMITER)))
      continue;

    for (int n = 1; n < MAX_TOKENS; n++) {
      token[n] = strtok(NULL, DELIMITER);
      if (!token[n])
        break;
      
    }
    //Determine if parsing the correct section
    parse_conf_sections(token);

    //After determining section, find specific bsize within section
    parse_conf_values(token);

  }
}

//Mark the sections we are in b/c most section's fields have the same name
void MemVerify::parse_conf_sections(const char** token) {
  map<string, bool>::iterator it = sharedconf_parse.find(string(token[0]));
  if (it != sharedconf_parse.end())
    it->second = true; 
}

//Once we are in a section (determined by parse_conf_sections()), find bsize or
//instWidth and set the value in the appropriate section, setting sharedconf_parse 
//to false means we have finished reading that section
void MemVerify::parse_conf_values(const char** token) {
  if (strcmp(token[0], "bsize") != 0 && strcmp(token[0], "instWidth") != 0)
    return;

  //Iterate the map and find out if we are in a section
  map<string, bool>::iterator it;
  for (it = sharedconf_parse.begin(); it != sharedconf_parse.end(); it++) {
    if (it->second) {  
      sharedconf_values[it->first] = atoi(token[2]);
      sharedconf_parse[it->first]  = false;
      //cout << "token: " << it->first << " and its value " << sharedconf_values[it->first] << endl;
    }
  }
}

//Values found in sharedconf_values map are byte size, divide to get word size
int MemVerify::get_wordSize(const char* str) {
  int instWidth_bytes = sharedconf_values["instWidth"]/8;
  return sharedconf_values[string(str)]/instWidth_bytes;
}

/* }}} */

/* For each memory access, check which caches were accessed using the func
 * cacheAccessSum(). Use the result with readAddrsAccessed() to see if the
 * cache access was valid. For ex, access 7 should be in L1 with 32B cacheline.
 * Default: 32bit instrWidth / 8 = 4 bytes.
 *         (32byte L1 cacheline) / (4byte instrWidth) = 8 instrs/cache line
 */
void MemVerify::test1_verify(CacheDebugAccess* cda) {
  int L1_words = get_wordSize("[DL1_core]");
  int L2_words = get_wordSize("[PrivL2]");
  int L3_words = get_wordSize("[L3Cache]");

  //printf("i:%d, L1:%d, L2:%d, L3:%d\n", instWidth_bytes, L1_words, L2_words, L3_words);
  //printf("sumaddr access: %d--------------------------------\n", cda->cacheAccessSum());
  
  int acc = cda->readAddrsAccessed();   //Which address access we're on, i.e. 0,1,2,3...
  switch (cda->cacheAccessSum()) {      //reading which caches were accessed in this address
    case 1:           //L1 cache
      if (acc % L1_words == 0)
        printf("ERROR in L1 cache\n");
      
      set_read_order("L1");
      break;

    case 2:           //L1 + L2 cache
      if (acc % L1_words != 0) 
        printf("ERROR IN L2 CACHE\n");
      
      set_read_order("L2");
      break;

    case 3:           //L1 + L2 + L3
      if (acc % L2_words != 0) 
        printf("ERROR IN L3 CACHE ACCESS\n");
      
      set_read_order("L3");
      break;

    case 4:           //big mem, check previous cache accesses also
      if (acc % L3_words != 0) 
        printf("ERROR IN BIGMEM CACHE ACCESS\n");

      set_read_order("bigMem");
      break;

    default:
      printf("Should not reach a default case!\n");
  }
}

//Set cache access by incrementation, at the end we'll have the exact
//accesses to every cache level
void MemVerify::set_read_order(const char* access) {
  string s = string(access);
  cache_consistency[s]++;
  //printf("cache_consistency[%s] = %d\n", access, cache_consistency[s]);
}

//We are simulating what should happen with input values for the different cache
//levels. Calculate a total of 33 addresses (may need to change to accom. instdwith).
void MemVerify::check_read_order2(void) {
  int L1_wordSize= get_wordSize("[DL1_core]");
  int L2_wordSize= get_wordSize("[PrivL2]");
  int L3_wordSize= get_wordSize("[L3Cache]");

  //Subtract 1 because we have read that part of the cacheline in the prev cache
  //Ex L3_cacheline is  16 words, and L2_cacheline is 8 words. It can only bring the
  //cacheline down to L2 one more time. Hence we have 16/8 = 2. 2-1 = 1
  int L1_rem = L1_wordSize - 1;
  int L2_rem = L2_wordSize / L1_wordSize - 1;
  int L3_rem = L3_wordSize / L2_wordSize - 1;

  printf("L1_wordSize= %d, L2_wordSize= %d, L3_wordSize= %d\n",
      L1_wordSize, L2_wordSize, L3_wordSize);

  printf("L1_rem= %d, L2_rem= %d, L3_rem= %d\n",
      L1_rem, L2_rem, L3_rem);

  int L1_count = 0, L2_count = 0, L3_count = 0, bigMem_count = 1;

  //Simulate up to 33 addresses. Lx_count holds the reads to that cache. Lx_rem
  //holds the remaining accesses to that cache. When all caches are read, go to
  //bigMem and reset all.
  while (L1_count + L2_count + L3_count + bigMem_count < 33) {
    if (L1_rem > 0) {
      L1_count += L1_rem;
      L1_rem = 0;
    }
    else if (L2_rem > 0) {
      L2_rem--;
      L2_count++;
      L1_rem += (L1_wordSize - 1);
    }
    else if (L3_rem > 0) {
      L3_rem--;
      L3_count++;
      L2_rem++;
      L1_rem += (L1_wordSize - 1);
    }
    else {      //reset because we have reached bigmem
      L1_rem = L1_wordSize - 1;
      L2_rem = L2_wordSize / L1_wordSize - 1;
      L3_rem = L3_wordSize / L2_wordSize - 1;
      bigMem_count++; 
    }
  }

  printf("L1_count= %d, L2_count= %d, L3_count= %d, bigMem_count= %d\n",
      L1_count, L2_count, L3_count, bigMem_count);

  //Compare the output of the cache to what the output should be
  if (cache_consistency["L1"] != L1_count || cache_consistency["L2"] != L2_count ||
      cache_consistency["L3"] != L3_count || cache_consistency["bigMem"] != bigMem_count)
    printf("We have a problem!\n");
  else
    printf("Correct cache!!!!!!!!!!!\n");
  
}

