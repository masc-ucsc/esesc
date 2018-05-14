#ifndef PORTMANAGER_H
#define PORTMANAGER_H

#include "MemRequest.h"
#include "Port.h"

#include "nanassert.h"

class MemObj;

class PortManager {
private:
protected:
  MemObj *mobj;

public:
  PortManager(MemObj *_mobj) {
    mobj = _mobj;
  };
  virtual ~PortManager(){};

  virtual void blockFill(MemRequest *mreq) = 0;

  virtual void   req(MemRequest *mreq)                    = 0;
  virtual void   startPrefetch(MemRequest *mreq)          = 0;
  virtual Time_t reqDone(MemRequest *mreq, bool retrying) = 0;
  virtual Time_t reqAckDone(MemRequest *mreq)             = 0;
  virtual void   reqRetire(MemRequest *mreq)              = 0;

  virtual void reqAck(MemRequest *mreq) = 0;

  virtual void setState(MemRequest *mreq) = 0;
  // FUTURE virtual void setStateDone(MemRequest *mreq) = 0;

  virtual void setStateAck(MemRequest *mreq) = 0;
  virtual void disp(MemRequest *mreq)        = 0;

  virtual bool isBusy(AddrType addr) const = 0;

  static PortManager *create(const char *section, MemObj *mobj);
};

class PortManagerBanked : public PortManager {
private:
protected:
  PortGeneric **bkPort;
  PortGeneric * sendFillPort;

  bool    dupPrefetchTag;
  bool    dropPrefetchFill;
  int32_t maxPrefetch; // 0 means share with maxRequest
  int32_t curPrefetch;

  int32_t maxRequests;
  int32_t curRequests;

  TimeDelta_t ncDelay;
  TimeDelta_t hitDelay;
  TimeDelta_t missDelay;

  TimeDelta_t tagDelay;
  TimeDelta_t dataDelay;

  uint32_t numBanks;
  int32_t  numBanksMask;

  uint32_t lineSize;
  uint32_t bankShift;
  uint32_t bankSize;
  uint32_t recvFillWidth;

  Time_t blockTime;

  std::list<MemRequest *> overflow;

  Time_t snoopFillBankUse(MemRequest *mreq);

  Time_t calcNextBankSlot(AddrType addr);
  Time_t nextBankSlot(AddrType addr, bool en);
  void   nextBankSlotUntil(AddrType addr, Time_t until, bool en);
  void   req2(MemRequest *mreq);

public:
  PortManagerBanked(const char *section, MemObj *_mobj);
  virtual ~PortManagerBanked(){};

  void   blockFill(MemRequest *mreq);
  void   req(MemRequest *mreq);
  void   startPrefetch(MemRequest *mreq);
  Time_t reqDone(MemRequest *mreq, bool retrying);
  Time_t reqAckDone(MemRequest *mreq);
  void   reqRetire(MemRequest *mreq);

  void reqAck(MemRequest *mreq);
  void setState(MemRequest *mreq);
  void setStateAck(MemRequest *mreq);
  void disp(MemRequest *mreq);

  bool isBusy(AddrType addr) const;
};

#endif
