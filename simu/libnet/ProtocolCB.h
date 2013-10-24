#ifndef PROTOCOLCB_H
#define PROTOCOLCB_H

#include "nanassert.h"

#include <vector>
#include <algorithm>

class Message;

class ProtocolCBBase {
protected:
public:
  virtual void call(Message *msg) = 0;
};

template<class ClassType, void (ClassType::*memberPtr)(Message *)> 
class ProtocolCB : public ProtocolCBBase {
  ClassType *instance;
public:
  ProtocolCB(ClassType *i) {
    instance = i;
  };
  
  void call(Message *msg) {
    (instance->*memberPtr)(msg);
  };
};


class ProtocolCBContainer {
private:
  typedef std::vector<ProtocolCBBase *> CBacksType;
  
  CBacksType cbacks;
protected:
public:
  void add(ProtocolCBBase *pcb) {
    I(std::find(cbacks.begin(),cbacks.end(),pcb) == cbacks.end());
    cbacks.push_back(pcb);
  };
  void remove(ProtocolCBBase *pcb) {
    CBacksType::iterator it = std::find(cbacks.begin(),cbacks.end(),pcb);
    I( it == cbacks.end() );
    
    for(CBacksType::iterator head = it+1 ; head != cbacks.end() ; head++, it++ ) {
      *it = *head;
    }
    
    cbacks.pop_back();
    
    I( std::find(cbacks.begin(),cbacks.end(),pcb) == cbacks.end() );
  };
  
  void call(Message *msg) {
    for(CBacksType::const_iterator it = cbacks.begin() ; it != cbacks.end() ; it++ ) {
      (*it)->call(msg);
    }
  };
};


#endif // PROTOCOLCB_H
