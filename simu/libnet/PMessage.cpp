
#include <string.h>

#include "PMessage.h"
#include "ProtocolBase.h"


ushort PMessage::maxMsgSize;
pool<PMessage> PMessage::msgPool;

// For each message type there is an associated size and name
typedef struct _SizeNameInfo {
  ushort size;
  const char *name;
} SizeNameInfo;

//assume 8 word line size..this is reasonable
static SizeNameInfo MessageType2SizeName[] = {
    {4,  "Default Message"},
    {7,  "Invalidate"},
    {4,  "InvalidateAck"},
    {4,  "TaskSpawnPE2PTQ"},
    {10, "Test"},
    {3,  "TestAck"},
    {8,  "CCRd"},
    {35, "CCRdXAck"}, // (3+linesize)
    {35, "CCRdAck"},  // (3+linesize)
    {8,  "CCWr"},
    {35, "CCWrAck"},  // (3+linesize)
    {36, "CCWrBack"}, // (4+linesize)
    {4,  "CCWrBackAck"},
    {7,  "CCInv"},
    {3,  "CCInvAck"},
    {3,  "CCNack"},
    {7,  "CCFailedRd"},
    {7,  "CCFailedWr"},
    {9,  "CCNewOwner"},
    {3,  "CCNewOwnerAck"},
    {9,  "CCNewSharer"},
    {3,  "CCNewSharerAck"},
    {8,  "CCRmSharer"},
    {3,  "CCRmSharerAck"},
    {7,  "CCRmLine "},
    {7,  "CCRmLineAck"},

    {8,  "OCRd"},
    {35, "OCRdAck"},  // (3+linesize)
    {38, "OCWr"},

    {8,  "TSLaunch"},
    {2,  "TSLaunchAck"},
    {4,  "TSPTWUnmapped"},
    {4,  "TSTSTUnmapped"},
    {7,  "TSRestartMsg"},
    {3,  "TSDeathSentence"},
    {7,  "TSKill"},
    {4,  "TSExecute"},
    {4,  "TSCommit"},
    {2,  "TSCommitAck"},
    {6,  "TSAPOAK"},
    {6,  "TSAPTCC"},
    {2,  "TSSpecFinished"},
    
    {7,  "TSRead"},
    {3,  "TSReadAck"},
    {0,  "DSM"},
    {0,  "MaxReadAck"}};

PMessage::PMessage()
{
  setType(DefaultMessage);
}

PMessage *PMessage::createMsg(MessageType msgType, 
			      ProtocolBase *srcPB,
			      ProtocolBase *dstPB,
			      MemRequest *memReq) 
{
  
  PMessage *msg = msgPool.out();
  
  msg->setupMessage(srcPB, dstPB, msgType);
  msg->mreq = memReq;
  
  return msg;
}


int32_t PMessage::getUniqueProtID() const
{
  return getUniqueProtID(msgType,dstPB->getNetDeviceID());
}


void PMessage::forwardMessage(ProtocolBase *newDstPB)
{
  srcPB = dstPB;
  dstPB = newDstPB;
  
  init(srcPB->getRouterID(),srcPB->getPortID(),
       dstPB->getRouterID(),dstPB->getPortID());

}

void PMessage::forwardMessage(ProtocolBase *newDstPB, MessageType t)
{
  srcPB = dstPB;
  dstPB = newDstPB;
  
  init(srcPB->getRouterID(), srcPB->getPortID(),
       dstPB->getRouterID(), dstPB->getPortID());

  setType(t);
}

void PMessage::setupMessage(ProtocolBase *src, 
			    ProtocolBase *dst, 
			    MessageType t)
{
  srcPB = src;
  dstPB = dst;

  init(srcPB->getRouterID(), srcPB->getPortID(),
       dstPB->getRouterID(), dstPB->getPortID());

#ifdef DEBUG
  setType(t);
#else
  if (t != msgType)  // speed hack for non-debug
    setType(t);
#endif
}

void PMessage::reverseDirection() 
{
  ProtocolBase *tmp = srcPB;
  srcPB = dstPB;
  dstPB = tmp;
  
  init(srcPB->getRouterID(),srcPB->getPortID(),
       dstPB->getRouterID(),dstPB->getPortID());
}

void PMessage::redirect(NetDevice_t newDstID) 
{
  srcPB = dstPB;
  dstPB = srcPB->getProtocolBase(newDstID);
  
  init(srcPB->getRouterID(),srcPB->getPortID(),
       dstPB->getRouterID(),dstPB->getPortID());
}

void PMessage::trace(const char *format,...)
{
  va_list ap;

  fprintf(stderr, "%6lld ", globalClock);

  fprintf(stderr, "\ttype[%d]\t",msgType);

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

const char *PMessage::getName() const 
{
  return MessageType2SizeName[msgType].name;
}

const char *PMessage::getName(MessageType t) 
{
  return MessageType2SizeName[t].name;
}

void PMessage::setType(MessageType t) 
{
  msgType = t;
  setSize(MessageType2SizeName[t].size);
}
