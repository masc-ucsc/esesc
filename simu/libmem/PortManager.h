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
  PortManager(MemObj *_mobj) { mobj = _mobj; };
  virtual ~PortManager() {};

  virtual void req(MemRequest *mreq        ) = 0;
  virtual Time_t reqDone(MemRequest *mreq) = 0;

  virtual void reqAck(MemRequest *mreq     ) = 0;

  virtual void setState(MemRequest *mreq   ) = 0;
  // FUTURE virtual void setStateDone(MemRequest *mreq) = 0;

  virtual void setStateAck(MemRequest *mreq) = 0;
  virtual void disp(MemRequest *mreq       ) = 0;

  virtual bool isBusy(AddrType addr) const = 0;

  static PortManager *create(const char *section, MemObj *mobj);
};

class PortManagerBanked : public PortManager {
private:
protected:
  PortGeneric **bkPort;
  PortGeneric *ackPort;

  int32_t     maxRequests;
  int32_t     curRequests;

	uint32_t    numBanks;
  int32_t     numBanksMask;

  uint32_t    bankShift;

  Time_t nextBankSlot(AddrType addr, bool en);
public:
  PortManagerBanked(const char *section, MemObj *_mobj);
  virtual ~PortManagerBanked() { };

  void req(MemRequest *mreq        );
  Time_t reqDone(MemRequest *mreq);

  void reqAck(MemRequest *mreq     );
  void setState(MemRequest *mreq   );
  void setStateAck(MemRequest *mreq);
  void disp(MemRequest *mreq       );

  bool isBusy(AddrType addr) const;

};

#endif

