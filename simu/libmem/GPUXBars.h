#ifndef GPUXBARS_H
#define GPUXBARS_H

#include "GXBar.h"
#include "MemXBar.h"
#include "UnMemXBar.h"

extern bool MIMDmode;

/******************************************************************
 * PECacheXBar is meant to be the splitter for the tinyCaches.
 * It will have exactly one object on top, so it need not be
 * multiplexed to the correct location.
 * It will have multiple objects below it. Thus anything going down
 * has to be multiplexed.
 * ***************************************************************/
class PECacheXBAR: public MemXBar
{/*{{{*/

  uint32_t addrHash(AddrType addr, uint32_t LineSize, uint32_t Modfactor, uint32_t numLowerBanks, const ExtraParameters* xdata = NULL) const
  {/*{{{*/
    if (bypassMode == bypass_global){
      if (xdata->memaccess == GlobalMem) {
        return numLowerBanks; //FIXME, It is now bypassing all the tinycaches, because it does not know which PE to go to.
      }
    } else if (bypassMode == bypass_shared) {
      if (xdata->memaccess == SharedMem) {
        return numLowerBanks; //FIXME, It is now bypassing all the tinycaches, because it does not know which PE to go to.
      }
    }

    if (MIMDmode) {
      return 0;
    } else {
      return (xdata->pe_id % numLowerBanks);
    }

  }/*}}}*/

  public:
  PECacheXBAR(MemorySystem* current ,const char *section ,const char *name)
    : MemXBar(section, name)
  {/*{{{*/
    I(current);
    char * tmp;
    MSG("building a PECacheXBar named:%s\n",name);
    lower_level_banks = NULL;

    FlowID num_lls = SescConf->getRecordSize(section,"lowerLevel");
    I((num_lls+1) == numLowerLevelBanks);

    lower_level_banks = new MemObj*[num_lls+1]; // One more for the bypass
    XBar_rw_req      = new GStatsCntr* [num_lls+1];

    for(size_t i=0;i<num_lls;i++) {
      std::vector<char *> vPars = SescConf->getSplitCharPtr(section, "lowerLevel", i);
      tmp = (char*)malloc(255);
      sprintf(tmp,"(%lu)",i);
      //        vPars[1] = tmp;
      if (!vPars.size()) {
        MSG("ERROR: Section [%s] field [%s[%d]] does not describe a MemoryObj\n", section, "lowerLevel", i);
        MSG("ERROR: Required format: memoryDevice = descriptionSection [name] [shared|private]\n");
        SescConf->notCorrect();
        I(0);
      }

      lower_level_banks[i] = current->finishDeclareMemoryObj(vPars, tmp);
      addLowerLevel(lower_level_banks[i]);
      XBar_rw_req[i]   = new GStatsCntr("%s_to_%s:rw_req",name,lower_level_banks[i]->getName());
    }

    //Add the Bypass level
    if (SescConf->checkCharPtr(section, "bypassMode")) {
      bypassMode = bypass_none;
      const char *bypass    = SescConf->getCharPtr(section,"bypassMode");
      if (strcasecmp(bypass, "bypass_global") == 0) {
        bypassMode = bypass_global;
      } else if (strcasecmp(bypass, "bypass_shared") == 0) {
        bypassMode = bypass_shared;
      }

      std::vector<char *> vPars = SescConf->getSplitCharPtr(section, "lowerLevelBypass");
      if (!vPars.size()) {
        MSG("ERROR: Section [%s] field [%s] does not describe a MemoryObj\n", section, "lowerLevel");
        MSG("ERROR: Required format: memoryDevice = descriptionSection [name] [shared|private]\n");
        SescConf->notCorrect();
        I(0);
      }

      lower_level_banks[num_lls] = current->finishDeclareMemoryObj(vPars);
      addLowerLevel(lower_level_banks[num_lls]);
      XBar_rw_req[num_lls]   = new GStatsCntr("%s_to_%s:rw_req",name,lower_level_banks[num_lls]->getName());
    }
  }/*}}}*/

  void doSetState(MemRequest *mreq)
  {  /*{{{*/
    //MSG("%s calling doSetState",name);
    uint32_t pos = addrHash(mreq->getAddr(),LineSize, Modfactor,numLowerLevelBanks,&(mreq->getExtraParams()));
    mreq->convert2SetStateAck(ma_setInvalid,false); // same as a miss (not sharing here)
    router->scheduleSetStateAckPos(pos,mreq,1);
  }/*}}}*/
};/*}}}*/


/******************************************************************
 * PECacheUnXBar is meant to be the joiner for the tinyCaches.
 * It will have multiple objects on top, so anything going up
 * must be multiplexed to the correct location.
 * It can have multiple objects below it, but it will be through
 * a splitter, so it can safely assume that it has only one object
 * below it. Thus anything going down need not be multiplexed.
 * ***************************************************************/
class PECacheUnXBAR: public UnMemXBar
{/*{{{*/

  uint32_t addrHash(AddrType addr, uint32_t LineSize, uint32_t Modfactor, uint32_t numLowerBanks, const ExtraParameters* xdata = NULL) const
  {/*{{{*/
    if (bypassMode == bypass_global){
      if (xdata->memaccess == GlobalMem) {
        return numLowerBanks;
      }
    } else if (bypassMode == bypass_shared){
      if (xdata->memaccess == SharedMem) {
        return numLowerBanks;
      }
    }

    if (MIMDmode) {
      return 0;
    } else {
      return (xdata->pe_id % numLowerBanks);
    }
  }/*}}}*/

  public:
  PECacheUnXBAR(MemorySystem* current ,const char *section ,const char *name)
    : UnMemXBar(current,section, name)
  {/*{{{*/
    if (SescConf->checkCharPtr(section, "bypassMode")) {
      bypassMode = bypass_none;
      const char *bypass    = SescConf->getCharPtr(section,"bypassMode");
      if (strcasecmp(bypass, "bypass_global") == 0) {
        bypassMode = bypass_global;
      } else if (strcasecmp(bypass, "bypass_shared") == 0) {
        bypassMode = bypass_shared;
      }
    }
  }/*}}}*/

  void doSetState(MemRequest *mreq)
  {  /*{{{*/
    //MSG("%s calling doSetState",name);
    mreq->convert2SetStateAck(ma_setInvalid,false); // same as a miss (not sharing here)
    router->scheduleSetStateAck(mreq,1);
  }/*}}}*/

};
/*}}}*/


/******************************************************************
 * ScratchXBar is meant to be the splitter for global & shared memory.
 * It can have either multiple objects on top, or just a single object.
 * So anything going up should be multiplexed to the correct location.
 * It will have multiple objects below it, Thus anything going down
 * should be multiplexed.
 * ***************************************************************/
class ScratchXBAR: public MemXBar
{ /*{{{*/
  uint32_t addrHash(AddrType addr, uint32_t LineSize, uint32_t Modfactor, uint32_t numLowerBanks, const ExtraParameters* xdata = NULL) const
  { /*{{{*/
    if (xdata->sharedAddr){
      //Goes to Scratch location
      if (xdata != NULL){
        //MSG("PE %d Warp %d Splitter: Scratch Address",xdata->pe_id, xdata->warp_id);
      } else {
        MSG("GPUXBars.h:27----> Why is xdata NULL??");
      }
      return 1;
    }
    if (xdata != NULL){
      //MSG("PE %d Warp %d Splitter: Global Address",xdata->pe_id, xdata->warp_id);
    } else {
      MSG(" GPUXBars.h:34---->Why is xdata NULL??");
    }

    return 0;

  }/*}}}*/

  public:
  ScratchXBAR(MemorySystem* current ,const char *section ,const char *name)
    : MemXBar(section, name)
  {/*{{{*/
    I(current);
    MSG("building a ScratchXBar named:%s\n",name);
    lower_level_banks = NULL;

    FlowID num_lls = SescConf->getRecordSize(section,"lowerLevel");
    I(num_lls == numLowerLevelBanks);
    I(num_lls == 2);
    lower_level_banks = new MemObj*[num_lls];
    XBar_rw_req      = new GStatsCntr* [numLowerLevelBanks];

    for(size_t i=0;i<num_lls;i++) {
      std::vector<char *> vPars = SescConf->getSplitCharPtr(section, "lowerLevel", i);
      if (!vPars.size()) {
        MSG("ERROR: Section [%s] field [%s[%d]] does not describe a MemoryObj\n", section, "lowerLevel", i);
        MSG("ERROR: Required format: memoryDevice = descriptionSection [name] [shared|private]\n");
        SescConf->notCorrect();
        I(0);
      }
      lower_level_banks[i] = current->finishDeclareMemoryObj(vPars);

      XBar_rw_req[i]   = new GStatsCntr("%s_to_%s:rw_req",name,lower_level_banks[i]->getName());

      addLowerLevel(lower_level_banks[i]);
    }
  }/*}}}*/

  void doSetState(MemRequest *mreq)
  {  /* setStateAck (down) {{{1 */
    //MSG("%s calling doSetState",name);
    uint32_t pos = addrHash(mreq->getAddr(),LineSize, Modfactor,numLowerLevelBanks,&(mreq->getExtraParams()));
    mreq->convert2SetStateAck(ma_setInvalid,false); // same as a miss (not sharing here)
    router->scheduleSetStateAckPos(pos,mreq,1);
  } /* }}} */

};/*}}}*/


/******************************************************************
 * ScratchUnXBar is meant to be the joiner for global & shared memory.
 * It may have multiple objects on top, so anything going up
 * must be multiplexed to the correct location.
 * It can have multiple objects below it, but it will be through
 * a splitter, so it can safely assume that it has only one object
 * below it. Thus anything going down need not be multiplexed.
 * ***************************************************************/
class ScratchUnXBAR : public UnMemXBar
{ /*{{{*/
  uint32_t addrHash(AddrType addr, uint32_t LineSize, uint32_t Modfactor, uint32_t numLowerBanks, const ExtraParameters* xdata = NULL) const
  {/*{{{*/
    if (xdata->sharedAddr){
      //Goes to Scratch location
      if (xdata != NULL)
        //MSG("PE %d Warp %d Scratch Joiner: Shared Address",xdata->pe_id, xdata->warp_id);
        I(0); //Should never happen!!
      return 1;
    }
    if (xdata != NULL){
      //MSG("PE %d Warp %d Scratch Joiner: Global Address",xdata->pe_id, xdata->warp_id);
    } else {
      I(0); //Should not happen!
      MSG("GPUXBars.h:82----> Why is xdata NULL??");
    }
    return 0;
  }/*}}}*/

  public:
  ScratchUnXBAR(MemorySystem* current ,const char *section ,const char *name)
    : UnMemXBar(current,section, name)
  {/*{{{*/
  }/*}}}*/

  void doSetState(MemRequest *mreq)
  {  /*{{{*/
    //MSG("%s calling doSetState",name);
    mreq->convert2SetStateAck(ma_setInvalid,false); // same as a miss (not sharing here)
    router->scheduleSetStateAck(mreq,1);
  }/*}}}*/


}; /*}}}*/


#endif
