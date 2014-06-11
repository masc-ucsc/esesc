
#include "SescConf.h"
#include "PortManager.h"
#ifdef ENABLE_NBSD
#include "NBSDPortManagerArbitrer.h"
#endif


PortManager *PortManager::create(const char *section, MemObj *mobj)
{
  if (SescConf->checkCharPtr(section, "port")) {
    const char *sub  = SescConf->getCharPtr(section, "port");
    const char *type = SescConf->getCharPtr(sub,"type");
    if (strcasecmp(type,"banked") == 0) {
      return new PortManagerBanked(sub, mobj);
#ifdef ENABLE_NBSD
    }else if (strcasecmp(type,"arbitrer") == 0) {
      return new PortManagerArbitrer(sub, mobj);
#endif
    }else{
      MSG("ERROR: %s PortManager %s type %s is unknown",section, sub, type);
      SescConf->notCorrect();
    }

  }

  return new PortManagerBanked(section,mobj);
}

PortManagerBanked::PortManagerBanked(const char *section, MemObj *_mobj)
  : PortManager(_mobj)
{
  int numPorts = SescConf->getInt(section, "bkNumPorts");
  int portOccp = SescConf->getInt(section, "bkPortOccp");

  char tmpName[512];
  const char *name = mobj->getName();

	numBanks   = SescConf->getInt(section, "numBanks");
	SescConf->isBetween(section, "numBanks", 1, 1024);  // More than 1024???? (more likely a bug in the conf)
  SescConf->isPower2(section,"numBanks");
  int32_t log2numBanks = log2i(numBanks);
  if (numBanks>1)
    numBanksMask = (1<<log2numBanks)-1;
  else
    numBanksMask = 0;

  bkPort = new PortGeneric* [numBanks]; 
  for (uint32_t i = 0; i < numBanks; i++){
    sprintf(tmpName, "%s_bk(%d)", name,i);
    bkPort[i] = PortGeneric::create(tmpName, numPorts, portOccp);
    I(bkPort[i]);
  }
  I(bkPort[0]);
  sprintf(tmpName, "%s_ack", name);
  ackPort = PortGeneric::create(tmpName,numPorts,portOccp);

  maxRequests = SescConf->getInt(section, "maxRequests");
  if(maxRequests == 0)
    maxRequests = 32768; // It should be enough

  curRequests = 0;

  if (SescConf->checkInt(section,"bankShift")) {
     bankShift = SescConf->getInt(section,"bankShift");
  }else{
    uint32_t lineSize = SescConf->getInt(section,"bsize");
    bankShift = log2i(lineSize);
  }
}

Time_t PortManagerBanked::nextBankSlot(AddrType addr, bool en) 
{ 
  if (numBanksMask == 0)
    return bkPort[0]->nextSlot(en); 

  int32_t bank = (addr>>bankShift) & numBanksMask;

  return bkPort[bank]->nextSlot(en); 
}

Time_t PortManagerBanked::reqDone(MemRequest *mreq)
{
  curRequests--;
  I(curRequests>=0);
  return ackPort->nextSlot(mreq->getStatsFlag());
}

bool PortManagerBanked::isBusy(AddrType addr) const
{
  if(curRequests >= maxRequests)
    return true;

  return false;
}

void PortManagerBanked::req(MemRequest *mreq)
/* main processor read entry point {{{1 */
{
  curRequests++;
	mreq->redoReqAbs(nextBankSlot(mreq->getAddr(), mreq->getStatsFlag()));
}
// }}}

void PortManagerBanked::reqAck(MemRequest *mreq)
/* request Ack {{{1 */
{
	mreq->redoReqAckAbs(nextBankSlot(mreq->getAddr(), mreq->getStatsFlag()));
}
// }}}

void PortManagerBanked::setState(MemRequest *mreq)
/* set state {{{1 */
{
	mreq->redoSetStateAbs(nextBankSlot(mreq->getAddr(), mreq->getStatsFlag()));
}
// }}}

void PortManagerBanked::setStateAck(MemRequest *mreq)
/* set state ack {{{1 */
{
	mreq->redoSetStateAckAbs(nextBankSlot(mreq->getAddr(), mreq->getStatsFlag()));
}
// }}}

void PortManagerBanked::disp(MemRequest *mreq)
/* displace a CCache line {{{1 */
{
	mreq->redoDispAbs(nextBankSlot(mreq->getAddr(), mreq->getStatsFlag()));
}
// }}}
