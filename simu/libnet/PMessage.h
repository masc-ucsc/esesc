#ifndef PMESSAGE_H
#define PMESSAGE_H

#include "nanassert.h"

#include "pool.h"
#include "Message.h"
#include "MemRequest.h"

enum MessageType {
  DefaultMessage = 0
  ,InvalidateMsg
  ,InvalidateAckMsg
  ,TaskSpawnPE2PTQ
  
  // Add your message types here
  ,TestMsg
  ,TestAckMsg

  // For M3T CacheCoherence (CC)
  ,CCRd
  ,CCRdXAck
  ,CCRdAck
  ,CCWr
  ,CCWrAck
  ,CCWrBack
  ,CCWrBackAck
  ,CCInv
  ,CCInvAck
  ,CCNack
  ,CCFailedRd
  ,CCFailedWr
  ,CCNewOwner
  ,CCNewOwnerAck
  ,CCNewSharer
  ,CCNewSharerAck
  ,CCRmSharer
  ,CCRmSharerAck
  ,CCRmLine
  ,CCRmLineAck

  // For M3T off-chip access
  ,OCRd
  ,OCRdAck
  ,OCWr

  // M3T TaskScalar Specific Messages
  ,TSLaunchMsg
  ,TSLaunchAckMsg
  ,TSPTWUnmappedMsg
  ,TSTSTUnmappedMsg
  ,TSRestartMsg
  ,TSDeathSentenceMsg
  ,TSKillMsg
  ,TSExecutedMsg
  ,TSCommitMsg
  ,TSCommitAckMsg
  ,TSAPOAKMsg
  ,TSAPTCCMsg
  ,TSSpecFinishedMsg

  // TODO: INTEGRATE WITH COHERENCE
  ,TSReadMsg
  ,TSReadAckMsg

  // libdsm messages
  ,DSMMemReadMsg
  ,DSMMemWriteMsg
  ,DSMMemReadAckMsg
  ,DSMMemWriteAckMsg
  ,DSMReadMissMsg
  ,DSMWriteMissMsg
  ,DSMInvalidateMsg
  ,DSMReadMissAckMsg
  ,DSMWriteMissAckMsg
  ,DSMInvalidateAckMsg
  ,DSMDataMsg

  // upper limit
  ,MaxMessageType
};

class ProtocolBase;

class PMessage : public Message {
private:

  static ushort maxMsgSize; //!< Maximum size that a message can have.
  static pool<PMessage> msgPool;
  
protected:

  ProtocolBase *srcPB;
  ProtocolBase *dstPB;

  MessageType  msgType;
  MemRequest *mreq;

  void redirect(NetDevice_t newDstID);  
  void forwardMessage(ProtocolBase *dstPB);
  void forwardMessage(ProtocolBase *dstPB, MessageType t);

public:

  PMessage();
  ~PMessage() { }

  static PMessage *createMsg(MessageType msgType, 
			     ProtocolBase *srcPB,
			     ProtocolBase *dstPB,
			     MemRequest *memReq);

  const char *getName() const;
  static const char *getName(MessageType t);
  void setType(MessageType t);
  MessageType getType() const {
    return msgType;
  }

  ProtocolBase *getSrcPB() const {
    return srcPB;
  }
  ProtocolBase *getDstPB() const {
    return dstPB;
  }

  void reverseDirection();

  void garbageCollect() {
    refCount--;
    if (refCount == 0)
      msgPool.in(this);
  }

  //! Get a unique id for any pair of MessageType and NetDevice
  static int32_t getUniqueProtID(MessageType t, NetDevice_t p) {
    I(sizeof(NetDevice_t)<=2);
    I(MaxMessageType<32700); // less than 16 bits
	 int32_t id = p;
	 id = (id<<16) ^ static_cast<int>(t);
    return id;
  }
  
  int32_t getUniqueProtID() const;

  MemRequest *getMemRequest() {
    return mreq;
  }

  void setInterConnection(InterConnection *intercon) {
    I(intercon);
    interConnection = intercon;
  }

  bool isValidMsgType(MessageType mt) {
    return (mt >= 0 || mt < MaxMessageType);
  }

  //! returns the maximum message size that can be sent in the network. 
  //! Used for debugging and to decide the minimum size of several queues.
  ushort getMaxMsgSize() const {
    return maxMsgSize;
  }

  void setupMessage(ProtocolBase *src, 
		    ProtocolBase *dst, 
		    MessageType t);

  virtual void trace(const char *format,...);
};

#endif // PMESSAGE_H
