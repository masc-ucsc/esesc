
#include "PortManager.h"
#include "SescConf.h"

PortManager *PortManager::create(const char *section, MemObj *mobj) {
  if(SescConf->checkCharPtr(section, "port")) {
    const char *sub  = SescConf->getCharPtr(section, "port");
    const char *type = SescConf->getCharPtr(sub, "type");
    if(strcasecmp(type, "banked") == 0) {
      return new PortManagerBanked(sub, mobj);
    } else {
      MSG("ERROR: %s PortManager %s type %s is unknown", section, sub, type);
      SescConf->notCorrect();
    }
  }

  return new PortManagerBanked(section, mobj);
}

PortManagerBanked::PortManagerBanked(const char *section, MemObj *_mobj)
    : PortManager(_mobj) {
  int numPorts = SescConf->getInt(section, "bkNumPorts");
  int portOccp = SescConf->getInt(section, "bkPortOccp");

  char        tmpName[512];
  const char *name = mobj->getName();

  hitDelay  = SescConf->getInt(section, "hitDelay");
  missDelay = SescConf->getInt(section, "missDelay");
  // SescConf->isBetween(section,"missDelay",0,hitDelay);

  if(SescConf->checkInt(section, "ncDelay")) {
    ncDelay = SescConf->getInt(section, "ncDelay");
  } else {
    ncDelay = missDelay;
  }

  if(SescConf->checkInt(section, "ncDelay")) {
    ncDelay = SescConf->getInt(section, "ncDelay");
  } else {
    ncDelay = missDelay;
  }

  dataDelay = hitDelay - missDelay;
  tagDelay  = hitDelay - dataDelay;

  numBanks = SescConf->getInt(section, "numBanks");
  SescConf->isBetween(section, "numBanks", 1, 1024); // More than 1024???? (more likely a bug in the conf)
  SescConf->isPower2(section, "numBanks");
  int32_t log2numBanks = log2i(numBanks);
  if(numBanks > 1)
    numBanksMask = (1 << log2numBanks) - 1;
  else
    numBanksMask = 0;

  bkPort = new PortGeneric *[numBanks];
  for(uint32_t i = 0; i < numBanks; i++) {
    sprintf(tmpName, "%s_bk(%d)", name, i);
    bkPort[i] = PortGeneric::create(tmpName, numPorts, portOccp);
    I(bkPort[i]);
  }
  I(bkPort[0]);
  int fillPorts = 1;
  int fillOccp  = 1;
  if(SescConf->checkInt(section, "sendFillPortOccp")) {
    fillPorts = SescConf->getInt(section, "sendFillNumPorts");
    fillOccp  = SescConf->getInt(section, "sendFillPortOccp");
  }
  sprintf(tmpName, "%s_sendFill", name);
  sendFillPort = PortGeneric::create(tmpName, fillPorts, fillOccp);

  maxRequests = SescConf->getInt(section, "maxRequests");
  if(maxRequests == 0)
    maxRequests = 32768; // It should be enough

  maxPrefetch = 32; // by default share with normal prefetch
  if(SescConf->checkInt(section, "maxPrefetch")) {
    maxPrefetch = SescConf->getInt(section, "maxPrefetch");
  }
  dupPrefetchTag = true;
  if(SescConf->checkBool(section, "dupPrefetchTag")) {
    dupPrefetchTag = SescConf->getBool(section, "dupPrefetchTag");
  }
  dropPrefetchFill = true;
  if(SescConf->checkBool(section, "dropPrefetchFill")) {
    dropPrefetchFill = SescConf->getBool(section, "dropPrefetchFill");
  }

  curRequests = 0;
  curPrefetch = 0;

  lineSize = SescConf->getInt(section, "bsize");
  if(SescConf->checkInt(section, "bankShift")) {
    bankShift = SescConf->getInt(section, "bankShift");
    bankSize  = 1 << bankShift;
  } else {
    bankShift = log2i(lineSize);
    bankSize  = lineSize;
  }
  if(SescConf->checkInt(section, "recvFillWidth")) {
    recvFillWidth = SescConf->getInt(section, "recvFillWidth");
    SescConf->isPower2(section, "recvFillWidth");
    SescConf->isBetween(section, "recvFillWidth", 1, lineSize);
  } else {
    recvFillWidth = lineSize;
  }

  blockTime = 0;
}

Time_t PortManagerBanked::nextBankSlot(AddrType addr, bool en) {
  int32_t bank = (addr >> bankShift) & numBanksMask;

  return bkPort[bank]->nextSlot(en);
}

Time_t PortManagerBanked::calcNextBankSlot(AddrType addr) {
  int32_t bank = (addr >> bankShift) & numBanksMask;

  return bkPort[bank]->calcNextSlot();
}

void PortManagerBanked::nextBankSlotUntil(AddrType addr, Time_t until, bool en) {
  uint32_t bank = (addr >> bankShift) & numBanksMask;

  bkPort[bank]->occupyUntil(until);
}

Time_t PortManagerBanked::reqDone(MemRequest *mreq, bool retrying) {

  if(mreq->isWarmup() || mreq->isDropped())
    return globalClock + 1;

  if(dropPrefetchFill && mreq->isPrefetch() && sendFillPort->calcNextSlot() > (globalClock + 8)) {
    mreq->setDropped();
    return globalClock + 1;
  }

  Time_t when = sendFillPort->nextSlot(mreq->getStatsFlag());

  if(!retrying && !mreq->isNonCacheable())
    when += dataDelay;

  // TRACE
  //  if (strcmp(mobj->getName(),"L2(0)")==0)
  //    MSG("%5lld @%lld %-8s Adone %12llx curReq=%d",mreq->getID(),when,mobj->getName(),mreq->getAddr(),curRequests);

  return when;
}

Time_t PortManagerBanked::reqAckDone(MemRequest *mreq) {
  if(mreq->isWarmup() || mreq->isDropped())
    return globalClock + 1;

  if(dropPrefetchFill && mreq->isPrefetch() && sendFillPort->calcNextSlot() > (globalClock + 8)) {
    mreq->setDropped();
    return globalClock + 1;
  }

  Time_t when = sendFillPort->nextSlot(mreq->getStatsFlag()); // tag access simultaneously, no charge here

  // TRACE
  //  if (strcmp(mobj->getName(),"L2(0)")==0)
  //  MSG("%5lld @%lld %-8s  done %12llx curReq=%d",mreq->getID(),when,mobj->getName(),mreq->getAddr(),curRequests);

  return when + 1;
}

bool PortManagerBanked::isBusy(AddrType addr) const {
  if(curRequests >= (maxRequests / 2)) // Reserve half for reads which do not check isBusy
    return true;

  return false;
}

void PortManagerBanked::startPrefetch(MemRequest *mreq) {
  I(mreq->isPrefetch());
  I(!mreq->isDropped());

  if(maxPrefetch) {
    curPrefetch++;
  } else {
    curRequests++;
  }
}

void PortManagerBanked::reqRetire(MemRequest *mreq) {

  if(mreq->isPrefetch() && maxPrefetch) {
    curPrefetch--;
    // MSG("--%d %s",mreq->getID(), mobj->getName());
  } else {
    curRequests--;
  }
  I(curRequests >= 0);
  I(curPrefetch >= 0);

  GI(curPrefetch, maxPrefetch); // curPrefech == 0 unless maxPrefetch

  while(!overflow.empty()) {
    MemRequest *oreq = overflow.back();
    overflow.pop_back();
    req2(oreq);
    if(curRequests >= maxRequests)
      break;
    if(curPrefetch > maxPrefetch)
      break;
  }
}

void PortManagerBanked::req2(MemRequest *mreq) {
  // I(curRequests<=maxRequests && !mreq->isWarmup());

  I(!mreq->isDropped());

  if(!mreq->isRetrying()) {
    if(mreq->isPrefetch() && maxPrefetch) {
      curPrefetch++;
    } else {
      curRequests++;
    }
  }

  // TRACE
  //  if (strcmp(mobj->getName(),"L2(0)")==0)
  // MSG("%5lld @%lld %-8s req   %12llx curReq=%d",mreq->getID(),globalClock,mobj->getName(),mreq->getAddr(),curRequests);

  if(mreq->isWarmup())
    mreq->redoReq();
  else if(mreq->isNonCacheable())
    mreq->redoReqAbs(globalClock + ncDelay);
  else if(dupPrefetchTag && mreq->isPrefetch())
    mreq->redoReqAbs(globalClock + tagDelay);
  else
    mreq->redoReqAbs(nextBankSlot(mreq->getAddr(), mreq->getStatsFlag()) + tagDelay);
}
void PortManagerBanked::req(MemRequest *mreq)
/* main processor read entry point {{{1 */
{
  if(!mreq->isRetrying() && !mreq->isPrefetch()) {
    if(curRequests >= maxRequests) {
      overflow.push_front(mreq);
      return;
    }
    while(!overflow.empty()) {
      MemRequest *oreq = overflow.back();
      overflow.pop_back();
      req2(oreq);
      if(curRequests >= maxRequests)
        break;
      if(curPrefetch > maxPrefetch)
        break;
    }
    if(!overflow.empty()) {
      overflow.push_front(mreq);
      return;
    }
  }

  req2(mreq);
}
// }}}

Time_t PortManagerBanked::snoopFillBankUse(MemRequest *mreq) {

  if(mreq->isNonCacheable())
    return globalClock;

  if(mreq->isPrefetch() && mreq->isDropped())
    return globalClock;

  Time_t max    = globalClock;
  Time_t max_fc = 0;
  for(int fc = 0; fc < lineSize; fc += recvFillWidth) {
    for(int i = 0; i < recvFillWidth; i += bankSize) {
      Time_t t = nextBankSlot(mreq->getAddr() + fc + i, mreq->getStatsFlag());
      if((t + max_fc) > max)
        max = t + max_fc;
    }
    max_fc++;
  }

#if 0
  // Make sure that all the banks are busy until the max time
  Time_t cur_fc = 0;
  for(int fc = 0; fc<lineSize ;  fc += recvFillWidth) {
    cur_fc++;
    for(int i = 0;i<recvFillWidth;i += bankSize) {
      nextBankSlotUntil(mreq->getAddr()+fc+i,max-max_fc+cur_fc, mreq->getStatsFlag());
    }
  }
#endif

  return max;
}

void PortManagerBanked::blockFill(MemRequest *mreq)
// Block the cache ports for fill requests {{{1
{
  if(mreq->isDropped())
    return;

  if(dropPrefetchFill && mreq->isPrefetch() && blockTime > (globalClock + 8)) {
    mreq->setDropped();
    return;
  }

  blockTime = snoopFillBankUse(mreq);
}
// }}}

void PortManagerBanked::reqAck(MemRequest *mreq)
/* request Ack {{{1 */
{

  Time_t until;
  if(dropPrefetchFill && mreq->isPrefetch() && blockTime > (globalClock + 8)) {
    mreq->setDropped();
    until = globalClock + 1;
  } else if(mreq->isWarmup() || mreq->isDropped()) {
    until = globalClock + 1;
  } else {
    until = snoopFillBankUse(mreq);
  }

  blockTime = until;

  mreq->redoReqAckAbs(until);
}
// }}}

void PortManagerBanked::setState(MemRequest *mreq)
/* set state {{{1 */
{
  mreq->redoSetStateAbs(globalClock + 1);
}
// }}}

void PortManagerBanked::setStateAck(MemRequest *mreq)
/* set state ack {{{1 */
{
  mreq->redoSetStateAckAbs(globalClock + 1);
}
// }}}

void PortManagerBanked::disp(MemRequest *mreq)
/* displace a CCache line {{{1 */
{
  Time_t t  = snoopFillBankUse(mreq);
  blockTime = t;
  mreq->redoDispAbs(t);
}
// }}}
