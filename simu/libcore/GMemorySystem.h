/*
  ESESC: Super ESCalar simulator
  Copyright (C) 2010 University of California, Santa Cruz.

  Contributed by Basilio Fraguela
  Jose Renau
  Smruti Sarangi

  This file is part of ESESC.

  ESESC is free software; you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software Foundation;
  either version 2, or (at your option) any later version.

  ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE. See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
  Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#ifndef GMEMORYSYSTEM_H
#define GMEMORYSYSTEM_H

#include "estl.h"

#include "MemObj.h"

//Class for comparison to be used in hashes of char * where the
//content is to be compared
class MemObjCaseeqstr {
 public:
  inline bool operator()(const char* s1, const char* s2) const {
    return strcasecmp(s1, s2) == 0;
  }
};

class MemoryObjContainer {
 private:
  std::vector<MemObj *> mem_node;
  typedef HASH_MAP<const char *, MemObj *, HASH<const char*>, MemObjCaseeqstr> StrToMemoryObjMapper;
  StrToMemoryObjMapper intlMemoryObjContainer;

 public:
  void addMemoryObj(const char *device_name, MemObj *obj);

  MemObj *searchMemoryObj(const char *section, const char *name) const;
  MemObj *searchMemoryObj(const char *name) const;

  void clear();
};

class GMemorySystem {
 private:

  typedef HASH_MAP<const char*, uint32_t, HASH<const char*>, MemObjCaseeqstr > StrCounterType;
  static StrCounterType usedNames;
  
  static MemoryObjContainer sharedMemoryObjContainer;
  MemoryObjContainer *localMemoryObjContainer;
  
  const MemoryObjContainer *getMemoryObjContainer(bool shared) const {
    MemoryObjContainer *mo = shared ? &sharedMemoryObjContainer : localMemoryObjContainer;
    I(mo);
    return mo;
  }

  MemoryObjContainer *getMemoryObjContainer(bool shared) {
    MemoryObjContainer *mo = shared ? &sharedMemoryObjContainer : localMemoryObjContainer;
    I(mo);
    return mo;
  }

  static std::vector<std::string> MemObjNames;

  MemObj   *DL1; // Data L1 cache
  MemObj   *IL1; // Instruction L1 cache
  MemObj   *vpc; // Speculative virtual predictor cache

 protected:
  const uint32_t Id;

  char *buildUniqueName(const char *device_type);

  uint32_t priv_counter;

  static char *privatizeDeviceName(char *given_name, int32_t num);

  virtual MemObj *buildMemoryObj(const char *type, const char *section, const char *name);

 public:
  GMemorySystem(int32_t processorId);
  virtual ~GMemorySystem();

  // The code can not be located in constructor because it is nor possible to
  // operate with virtual functions at construction time
  virtual void buildMemorySystem();

  MemObj *searchMemoryObj(bool shared, const char *section, const char *name) const;
  MemObj *searchMemoryObj(bool shared, const char *name) const;

  
  MemObj *declareMemoryObj_uniqueName(char *name, char *device_descr_section);
  MemObj *declareMemoryObj(const char *block, const char *field);
  MemObj *finishDeclareMemoryObj(std::vector<char *> vPars);


  uint32_t getNumMemObjs() { return MemObjNames.size(); }
  void addMemObjName(const char *name) { MemObjNames.push_back(name); }
  std::string getMemObjName(uint32_t i) { return MemObjNames[i]; }

  uint32_t getId() const  { return Id;  };
  MemObj *getDL1() const { return DL1; };
  MemObj *getIL1() const { return IL1; };
  MemObj *getvpc() const { return vpc; };
};

class DummyMemorySystem : public GMemorySystem {
 private:
 protected:
 public:
  DummyMemorySystem(int32_t id);
  ~DummyMemorySystem();
};

#endif /* GMEMORYSYSTEM_H */
