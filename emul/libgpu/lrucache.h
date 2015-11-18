/*
ESESC: Super ESCalar simulator
Copyright (C) 2006 University California, Santa Cruz.

Contributed by  Alamelu Sankaranarayanan

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

#ifndef APPROXLRUCACHE_H
#define APPROXLRUCACHE_H

class cachemeta{
  public: 
    uint64_t refcount;
    uint64_t refinterval;
    bool valid;

    cachemeta(uint64_t l_refinterval){
      refcount    = 1;
      refinterval = l_refinterval;
      valid       = false; // Not yet fetched from the lower levels
    }

    void setValid(bool flag){
      valid = flag;
    }
};


class sort_map{
  public:
    uint64_t addr;
    cachemeta* line;
};

extern bool sort_by_refcount(const sort_map& a ,const sort_map& b);

class lrucache {
  uint32_t number_of_entries;
  uint32_t valid_entries;
  uint32_t linesize;

  std::map <uint64_t, cachemeta* > mylrucache;

  public:
  uint32_t getEntries(){
    return number_of_entries;
  }

  void setEntries(uint32_t n){
    I(n>0);
    //I(n is a power of 2);
    number_of_entries = n;
  }

  uint32_t getlinesize(){
    return linesize;
  }

  void setlinesize(uint32_t n){
    I(n>0);
    //I(n is a power of 2);
    linesize = n;
  }

  bool checkAddr(uint64_t addr, uint64_t refinterval){
    //Check the map if the address is present. 
    std::map <uint64_t, cachemeta* > :: iterator it;
    bool found = false;
    cachemeta* tmpentry;
    uint32_t shift   = log2i(linesize);
    uint64_t sh_addr = addr >> shift;

    it = mylrucache.find(sh_addr);

    if(it != mylrucache.end()) {
      //Address found

      cachemeta* tmp = mylrucache[sh_addr];
      if ((refinterval - tmp->refinterval ) > 16 ){
        tmp->setValid(true);
        found = true;
      }
    } else {
      //Address not found.
      if (valid_entries >=  number_of_entries ){
        evictLRUentry(); //Evict the least recently used entry
        valid_entries--;
      }

      //Add the new address to the cache
      tmpentry = new cachemeta(refinterval);
      mylrucache[sh_addr] = tmpentry;
      valid_entries++;
    }

    return found;

  }

  void evictLRUentry(){
    vector< sort_map > v;
    vector< sort_map >::iterator itv;
    std::map <uint64_t, cachemeta* > :: iterator it;
    sort_map sm;


    for (it = mylrucache.begin(); it != mylrucache.end(); ++it)
    {
      sm.addr = (*it).first; 
      sm.line = (*it).second;
      v.push_back(sm);
    }

    sort(v.begin(),v.end(),sort_by_refcount);

    uint64_t local_addr = v[0].addr;
    it=mylrucache.find(local_addr);

    mylrucache.erase(it);
  }

};


#endif
