/* License & includes {{{1 */
/*
  ESESC: Super ESCalar simulator
  Copyright (C) 2010 University of California, Santa Cruz.

  Contributed by Luis Ceze
  Karin Strauss
  Jose Renau

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

#ifndef SL0CACHE_H
#define SL0CACHE_H

#include "Cache.h"
#define w_GLOBAL_HIST 4
enum wcb_valid_status_t {w_INVALID, w_VALID, w_PENDING};

/* }}} */

class SL0Cache: public Cache {
 protected:
  class WCBLine {
  public:
    void reset(int sz) {
      stid.resize(sz);
      data.resize(sz);
      present.resize(sz);
      tag   = 0;
      valid = w_INVALID;
    }
    std::vector<Time_t>   stid;
    std::vector<DataType> data;
    std::vector<bool>     present; // 1 bit per word
    AddrType              tag;
    wcb_valid_status_t        valid;   // cache line available?
  };
  class WCB {
  protected:
    
    const uint32_t size;
    const AddrType log2AddrLs;
    const AddrType wordMask;
    const uint32_t nWords;
    
    uint32_t mru;
    uint32_t random;
    uint32_t global_hist_index;    

    DataType global_hist_data[w_GLOBAL_HIST];
    AddrType global_hist_addr[w_GLOBAL_HIST];
    Time_t   global_hist_stid[w_GLOBAL_HIST];
    // FIXME: use a faster HASH_MAP per word
    std::vector<WCBLine> line;

  public:
    WCBLine displaced;
  WCB(int32_t sz, int32_t l) : size(sz) ,log2AddrLs(l) ,wordMask((1<<(l))-1), nWords(1<<l) {      
      mru    = 0;
      global_hist_index = 0;
      displaced.reset(nWords);
      line.resize(size);
      for(uint32_t i=0;i<w_GLOBAL_HIST;i++){
        global_hist_data[i] = 0;
        global_hist_addr[i] = 0;
        global_hist_stid[i] = 0;
      }
      for(uint32_t i=0;i<size;i++) 
        line[i].reset(nWords);
      random = 0;

    }                           
    uint32_t getnWords() const { return nWords; }
    int32_t calcWord(AddrType addr) const {
      return (addr & wordMask)>>2;
    }

    bool loadCheck(const DInst *dinst, bool &correct) {
      AddrType addr = dinst->getAddr();
      DataType data = dinst->getData();

      random++;
      if (random>=size)
        random = 0;
      AddrType tag = addr >> log2AddrLs;
      for(uint32_t i=0;i<size;i++) {
        if( line[i].tag != tag )
          continue;
        if( line[i].valid!=w_VALID )
          continue;
        int32_t word = calcWord(addr);
        if(line[i].present[word] == false ){
          line[i].present[word] = true; //simulate fetching the word from the cache.
          line[i].data[word] = data;
          line[i].stid[word] = dinst->getID();
          return false;
        }
  printf("hit\n");
        mru = i;
        correct = (data == line[i].data[word]);

#if 0
        if(!correct){
          if (line[i].stid[word] > dinst->getID()){ //fastST
            for(uint32_t j=0;j<w_GLOBAL_HIST;j++){
              if(global_hist_addr[j]!=addr)
                continue;
              if(global_hist_stid[j] > dinst->getID())
                continue;
              if(global_hist_data[j] == data){
                return true;
              }
            }
            printf("word %u line %u ",word,i);
            printf("fastST @%lld id=%lld pc=0x%llx addr=0x%llx tag=0x%llx [0x%x vs 0x%x] line_id = %lld : %lld\n"
       ,(long long)globalClock, (long long)dinst->getID(), dinst->getPC(), addr, tag, data 
       ,line[i].data[word], (long long)line[i].stid[word]
       ,(long long)line[i].stid[word]-(long long)dinst->getID());    
          }
          else //fastLD
            printf("word %u line %u ",word,i);

          printf("fastLD @%lld id=%lld pc=0x%llx addr=0x%llx tag=0x%llx [0x%x vs 0x%x] line_id = %lld : %lld\n"
     ,(long long)globalClock, (long long)dinst->getID(), dinst->getPC(), addr, tag, data 
     ,line[i].data[word], (long long)line[i].stid[word]
     ,(long long)line[i].stid[word]-(long long)dinst->getID());
        }
#endif
        return true;
      }
      return false;
    }

    bool detect_stallread(const DInst *dinst) { //return true if read should be stalled.
      AddrType addr = dinst->getAddr();
      int32_t word = calcWord(addr);
      AddrType tag = addr >> log2AddrLs;

      for(uint32_t i=0;i<size;i++) {
        if( line[i].tag != tag )
          continue;
        if( line[i].valid==w_INVALID )
          continue;
        // true stalls, false lets the read go
        if (line[i].valid==w_PENDING && (line[i].stid[word] < dinst->getID()))
          return true;
        // For multiversion WCB, we may need to look for older lines in the WCB
        return false;
      }
      return false;
    }

    bool detect_stallwrite(const DInst *dinst) { //return true if write should be stalled.
      AddrType addr = dinst->getAddr();
      int32_t word = calcWord(addr);
      AddrType tag = addr >> log2AddrLs;

      for(uint32_t i=0;i<size;i++) {
        if( line[i].tag != tag )
          continue;
        if( line[i].valid==w_INVALID )
          continue;
        if( !line[i].valid==w_PENDING && (line[i].stid[word] != dinst->getID()))
          return true;
 
        return false;
      }
      return false;
    }

    bool evict() { //return true if eviction is performed, false if not
      for(uint32_t i=mru+1;i<size;i++) {
        if(line[i].valid==w_INVALID)
          continue;
        line[i].valid=w_INVALID; //should write this line into the cache once it's written
        return true;
      }
      for(uint32_t i=0;i<mru;i++) {
        if(line[i].valid==w_INVALID)
          continue;
        line[i].valid=w_INVALID; //should write this line into the cache once it's written
        return true;
      }
      return false;
    }

    bool write(const DInst *dinst) { //return true if a displacement occurs
      AddrType addr = dinst->getAddr();
      DataType data = dinst->getData();
      Time_t   stid = dinst->getID();
      AddrType tag  = addr >> log2AddrLs;
      random++;
      if (random>=size)
        random = 0;
      uint32_t pos = mru + random;
      if (pos>=size)
        pos -= size;

      int32_t word = calcWord(addr);

      for(uint32_t i=0;i<size;i++) {
        if(line[i].valid==w_INVALID ) {
 //we should never find a valid, because an STa come first and mark the line w_PENDING
          pos = i;
          continue;
        }
        if(line[i].tag != tag )
          continue;
  if(line[i].present[word]){
    if(line[i].stid[word] > stid)
      return false; //don't perform write because a newer store is already in the WCB
    global_hist_addr[global_hist_index] = addr;
    global_hist_data[global_hist_index] = line[i].data[word];
    global_hist_stid[global_hist_index] = line[i].stid[word];
    global_hist_index++;
    if(global_hist_index>=w_GLOBAL_HIST)
    global_hist_index = 0;       
  }
        line[i].data[word]    = data;
        line[i].stid[word]    = stid;
        line[i].present[word] = true;
        line[i].valid         = w_VALID;
  //printf("STd_e id=%lld: @%lld pc=0x%llx addr=0x%llx tag=0x%llx line_num=%u word=%u data=0x%x\n",
  //       (long long)dinst->getID(),(long long)globalClock,dinst->getPC(),addr,tag,i,word,data);
        return false;
      }
      //This code may not be needed because STa allocates a WCB line
      //so we should always find a matching line above
      /*BEGIN REMOVAL SECTION*/
      /*      I(pos<size);
      bool disp=false; 
      if (line[pos].valid==VALID) {
        if(line[pos].present[word]==true){ //there's already a word here, save it
          global_hist_addr[global_hist_index] = addr;
          global_hist_data[global_hist_index] = line[pos].data[word];
          global_hist_stid[global_hist_index] = line[pos].stid[word];
          global_hist_index++;
          if(global_hist_index>=w_GLOBAL_HIST)
            global_hist_index = 0;       
        }

        displaced = line[pos];
        disp      = true;
        for(uint32_t i=0;i<nWords;i++) {
          line[pos].present[i]=false;
          line[pos].stid[i]=0;
        }
      }
      line[pos].data[word]    = data;
      line[pos].stid[word]    = stid;
      line[pos].present[word] = true;
      line[pos].tag           = tag;
      line[pos].valid         = w_VALID;
      printf("STd_n id=%lld: @%lld pc=0x%llx addr=0x%llx tag=0x%llx line_num=%u word=%u data=0x%x\n",
       (long long)dinst->getID(),(long long)globalClock,dinst->getPC(),addr,tag,pos,word,data);
       return disp;*/
      /*END REMOVAL SECTION*/
      return false;
    }

    bool writeAddress(const DInst *dinst) {
      AddrType addr = dinst->getAddr();
      Time_t   stid = dinst->getID();
      AddrType tag = addr >> log2AddrLs;
      int32_t word = calcWord(addr);

      random++;
      if (random>=size)
        random = 0;

      uint32_t pos = mru + random;
      if (pos>=size)
        pos -= size;

      for(uint32_t i=0;i<size;i++) {
        if(line[i].valid==w_INVALID ) {
          pos = i;
          continue;
        }
        if(line[i].tag != tag )
          continue;
  if(line[i].present[word] && (line[i].stid[word] > stid))
    return false; //don't perform write because a newer store is already in the WCB
  //for now if an older store is in WCB, overwrite it, with no eviction.
        line[i].stid[word]    = stid;
        line[i].present[word] = false;
        line[i].valid         = w_PENDING;
  //printf("STa_e id=%lld: @%lld pc=0x%llx addr=0x%llx tag=0x%llx line_num=%u word=%u\n",
  //       (long long)dinst->getID(),(long long)globalClock,dinst->getPC(),addr,tag,i,word);
        return false;
      }
      I(pos<size);
      bool disp=false;

      if (line[pos].valid==w_VALID) { //note that only STa does displacements
        displaced = line[pos];
        disp      = true;
        for(uint32_t i=0;i<nWords;i++) {
          line[pos].present[i]=false;
          line[pos].stid[i] = 0;
        }
      }
      line[pos].stid[word]    = stid;
      line[pos].present[word] = false;
      line[pos].tag           = tag;
      line[pos].valid         = w_PENDING;
      // printf("STa_n id=%lld: @%lld pc=0x%llx addr=0x%llx tag=0x%llx line_num=%u word=%u\n",
      //     (long long)dinst->getID(),(long long)globalClock,dinst->getPC(),addr,tag,pos,word);

      return disp;
    }
  }; 

  class DataTypeHashFunc {
    public:
      size_t operator()(const DataType p) const {
        return((uint32_t) p);
      }
  };

  typedef HASH_MAP<AddrType, DataType, DataTypeHashFunc > CacheData;
  CacheData cacheData;
  WCB       wcb;

  GStatsCntr   wcbPass;
  GStatsCntr   wcbFail;
  GStatsCntr   dataPass;
  GStatsCntr   dataFail;

  /* BEGIN Private Cache customization functions */
  void displaceLine(AddrType addr, MemRequest *mreq, bool modified);
  void doWriteback(MemRequest *wbreq);

  void doRead(MemRequest  *req, bool ignoreCanIssue=false);
  void doWrite(MemRequest *req, bool ignoreCanIssue=false);

  void doBusRead(MemRequest *req, bool ignoreCanIssue=false);
  void doPushDown(MemRequest *req);

  void doInvalidate(MemRequest *req);
  /* END Private Cache customization functions */

 public:
  SL0Cache(MemorySystem *gms, const char *descr_section, const char *name = NULL);
  virtual ~SL0Cache();

  TimeDelta_t ffread(AddrType addr, DataType data);
  TimeDelta_t ffwrite(AddrType addr, DataType data);
  void        ffinvalidate(AddrType addr, int32_t lineSize);

  bool canAcceptRead(DInst *dinst);
  bool canAcceptWrite(DInst *dinst);
  void writeAddress(MemRequest *req);

};

#endif
