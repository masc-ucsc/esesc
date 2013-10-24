#ifndef _NETIDENTIFIERS_H
#define _NETIDENTIFIERS_H

typedef ushort RouterID_t;

enum PortID_t { 
  DISABLED_PORT = 0
  ,UP_PORT, DOWN_PORT, RIGHT_PORT, LEFT_PORT
  ,LOCAL_PORT1 = 100, LOCAL_PORT2, LOCAL_PORT3, LOCAL_PORT4, MAX_PORTS = 200
};


inline PortID_t&operator++(PortID_t &p, int32_t i) {
  I(i==0); // The int32_t parameters has no sense. a p++ always passes zero????
  p = PortID_t(p+1);
  I(p<=MAX_PORTS);
  return p;
};

inline PortID_t&operator--(PortID_t &p, int32_t i){
  I(i==0); // The int32_t parameters has no sense. a p++ always passes zero????
  p = PortID_t(p-1);
  I(p>=DISABLED_PORT);
  return p;
};


// A NetDevice is a "device" attached to an specific router
// (RouterID_t) and port (PortID_t). For each combination of
// router:port there may be several "devices" attached. Protocols work
// using NetDevice, while routers work with RouterID and PortID
typedef ushort NetDevice_t;

//const NetDevice_t BCastNID = (NetDevice_t)-1;


#endif // _NETIDENTIFIERS_H
