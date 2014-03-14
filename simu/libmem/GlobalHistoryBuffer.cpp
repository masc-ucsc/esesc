#if 0
#include "GlobalHistoryBuffer.h"
#include "CacheCore.h"

GHB::GHB(MemorySystem* current, 
        const char *section, 
        const char *name)
:MemObj(section, name)
  ,halfMiss("%s:halfMiss", name)
  ,miss("%s:miss", name)
  ,hit("%s:hits", name)
  ,predictions("%s:predictions", name)
  ,accesses("%s:accesses", name)
  ,unitStrideStreams("%s:unitStrideStreams", name)
  ,nonUnitStrideStreams("%s:nonUnitStrideStreams", name)
  ,ignoredStreams("%s:ignoredStreams", name)
{

  //MemObj *lower_level = NULL;

  // If MemLatency is 200 and busOcc is 10, then there can be at most 20
  // requests without saturating the bus. (VERIFY???) something like half sound
  // conservative (10 pending requests)
  SescConf->isInt(section, "maxRequests");
  maxPendingRequests = SescConf->getInt(section, "maxRequests");

  SescConf->isInt(section, "depth");
  depth = SescConf->getInt(section, "depth");

  SescConf->isInt(section, "missWindow");
  missWindow = SescConf->getInt(section, "missWindow");

  SescConf->isInt(section, "maxStride");
  maxStride = SescConf->getInt(section, "maxStride");

  SescConf->isInt(section, "hitDelay");
  hitDelay = SescConf->getInt(section, "hitDelay");

  SescConf->isInt(section, "missDelay");
  missDelay = SescConf->getInt(section, "missDelay");

  SescConf->isInt(section, "learnHitDelay");
  learnHitDelay = SescConf->getInt(section, "learnHitDelay");

  SescConf->isInt(section, "learnMissDelay");
  learnMissDelay = SescConf->getInt(section, "learnMissDelay");

  //FIXME: FastQueue<HistBuffObj*> *HistoryBuffer;                          //Global history buffer
  
  SescConf->isInt(section, "ghbentries");
  GHBentries = SescConf->getInt(section, "ghbentries");

  SescConf->isInt(section, "ghbindexbits");
  GHBindexbits=   SescConf->getInt(section, "ghbindexbits");                 // max value can be log2(ghbentries)?

  const char *prefetchertype = SescConf->getCharPtr(section, "prefetcher");
  if (strcasecmp(prefetchertype,"Stride") == 0){
    GHBLocalizingMethod = 0;
    GHBDetectionMethod  = 1;
  } else if (strcasecmp(prefetchertype,"Markov") == 0){ 
    GHBLocalizingMethod = 0;
    GHBDetectionMethod  = 1;
  } else if (strcasecmp(prefetchertype,"Distance") == 0){
    GHBLocalizingMethod = 0;
    GHBDetectionMethod  = 1;
  } else {
    MSG("Prefetching Method %s not yet supported",prefetchertype);
    SescConf->notCorrect();
  }

  const char *buffSection = SescConf->getCharPtr(section, "buffCache");
  if (buffSection) {
    buff = BuffType::create(buffSection, "", name);

    SescConf->isInt(buffSection, "bknumPorts");
    numBuffPorts = SescConf->getInt(buffSection, "bknumPorts");

    SescConf->isInt(buffSection, "bkportOccp");
    buffPortOccp = SescConf->getInt(buffSection, "bkportOccp");
  }

  defaultMask  = (2<<GHBindexbits)-1;
 
  const char* mshrSection = SescConf->getCharPtr(section,"MSHR");
  char tmpName[512];
  sprintf(tmpName, "%s_MSHR", name);
  mshr      = MSHR::create(tmpName, mshrSection,buff->getLineSize());
}



Time_t GHB::nextReadSlot(       const MemRequest *mreq){
  I(0);
  //This function should not be called.
  return globalClock;
}

Time_t GHB::nextWriteSlot(      const MemRequest *mreq){
  I(0);
  //This function should not be called.
  return globalClock;
}

Time_t GHB::nextBusReadSlot(    const MemRequest *mreq){
  return (globalClock+1); //FIXME
}

Time_t GHB::nextPushDownSlot(   const MemRequest *mreq){
  return (globalClock+1); //FIXME
}

Time_t GHB::nextPushUpSlot(     const MemRequest *mreq){
  return (globalClock+1); //FIXME
}

Time_t GHB::nextInvalidateSlot( const MemRequest *mreq){
  return (globalClock+1); //FIXME
}

// processor direct requests
void GHB::read(MemRequest  *mreq){
  I(0); // Should not be a first level object
}

void GHB::write(MemRequest *mreq){
  I(0); // Should not be a first level object
}

void GHB::writeAddress(MemRequest *mreq){
  I(0); // Should not be a first level object
}

// DOWN
void GHB::busRead(MemRequest *mreq){
  AddrType paddr = mreq->getAddr() & defaultMask;
  bLine *l = buff->readLine(paddr);

  ///****** HIT *******/
  if(l) { 
    hit.inc(mreq->getStatsFlag());
    router->fwdPushUp(mreq, hitDelay);
    return;
  }

  if (!mshr->canAccept(paddr)) {
    CallbackBase *cb  = busReadAckCB::create(this, mreq);

    pendingRequests++;
    mshr->addEntry(paddr, cb);
    return;
  }

  pendingRequests++;
  mshr->addEntry(paddr);
  router->fwdBusRead(mreq);
}

void GHB::busReadAck(MemRequest *mreq)
{
  pendingRequests--;
  AddrType paddr = mreq->getAddr() & defaultMask;
  mshr->retire(paddr);
  router->fwdPushUp(mreq);
}

void GHB::pushDown(MemRequest *mreq){
  bLine *l = buff->writeLine(mreq->getAddr()); // Also for energy

  if (mreq->isInvalidate()) {
    if (l)
      l->invalidate();

    mreq->decPending();
    if (!mreq->hasPending()) {
      MemRequest *parent = mreq->getParent();
      router->fwdPushDown(parent);
    }

    mreq->destroy();
    return;
  }

  I(mreq->isWriteback());

  router->fwdPushDown(mreq);
}

// UP
void GHB::pushUp(MemRequest *mreq){

  if(mreq->isHomeNode()) {
    buff->fillLine(mreq->getAddr());
    mreq->destroy();
    return;
  }
  //learnMiss(mreq->getAddr());
  busReadAck(mreq);
}


void GHB::invalidate(MemRequest *mreq){
  uint32_t paddr = mreq->getAddr() & defaultMask; // FIXME: Maybe delete the defaultMask

  //nextBuffSlot();

  bLine *l = buff->readLine(paddr);
  if(l)
    l->invalidate();

  // broadcast the invalidate through the upper nodes
  router->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(),hitDelay /*delay*/);
}

// Status/state
uint16_t GHB::getLineSize() const{
  return buff->getLineSize();
}

bool GHB::canAcceptRead(DInst *dinst) const{ //CHECKME:
  if (pendingRequests < maxPendingRequests)
    return true;
  return false;
}

bool GHB::canAcceptWrite(DInst *dinst) const{ //CHECKME:
  if (pendingRequests < maxPendingRequests)
    return true;
  return false;
}

TimeDelta_t GHB::ffread(AddrType addr, DataType data){
  I(0);
  return router->ffread(addr,data) /*+ delay*/;
}

TimeDelta_t GHB::ffwrite(AddrType addr, DataType data){
  I(0);
  return router->ffwrite(addr,data) /*+ delay*/ ;
}

void        GHB::ffinvalidate(AddrType addr, int32_t lineSize){
  I(0);
}

#endif
