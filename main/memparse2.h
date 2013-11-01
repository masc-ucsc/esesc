/* License & includes {{{1 */
/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau, Edward Kuang

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
#ifndef memparse2_h
#define memparse2_h

#include <string>
#include <map>
#include "MemStruct.h"

using namespace std;
/* }}} */

class MemVerify {
  private:
    static const int MAX_CHARS_PER_LINE = 512;
    static const int MAX_TOKENS         = 3;    //How many tokens to read on a line
    static const char* DELIMITER;

    map<string, bool> sharedconf_parse;         //Determine if we've hit a section
    map<string, int > sharedconf_values;        //Parse value and place into this map

    map<string, int > cache_consistency;        //Make sure mem accesses are in order   
                                                //i.e. no access to L2 2x in a row
  public:
    MemVerify(void); 

    void read_configuration(void);

    void parse_conf_sections(const char**);

    void parse_conf_values(const char**);

    int get_wordSize(const char*);

    void test1_verify(CacheDebugAccess*);

    void set_read_order(const char*);
    
    void check_read_order2(void);

    //void check_read_order3(void);

};

#endif
