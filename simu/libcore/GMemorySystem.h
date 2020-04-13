// Contributed by Basilio Fraguela
//                Jose Renau
//                Smruti Sarangi
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef GMEMORYSYSTEM_H
#define GMEMORYSYSTEM_H

#include <strings.h>

#include <vector>

#include "estl.h"
#include "nanassert.h"

class MemObj;

// Class for comparison to be used in hashes of char * where the
// content is to be compared
class MemObjCaseeqstr {
public:
  inline bool operator()(const char *s1, const char *s2) const {
    return strcasecmp(s1, s2) == 0;
  }
};

class MemoryObjContainer {
private:
  std::vector<MemObj *>                   mem_node;
  typedef std::map<std::string, MemObj *> StrToMemoryObjMapper;
  // typedef HASH_MAP<const char *, MemObj *, HASH<const char*>, MemObjCaseeqstr> StrToMemoryObjMapper;
  StrToMemoryObjMapper intlMemoryObjContainer;

public:
  void addMemoryObj(const char *device_name, MemObj *obj);

  MemObj *searchMemoryObj(const char *section, const char *name) const;
  MemObj *searchMemoryObj(const char *name) const;

  void clear();
};

class GMemorySystem {
private:
  typedef std::map<std::string, uint32_t> StrCounterType;
  // typedef HASH_MAP<const char*, uint32_t, HASH<const char*>, MemObjCaseeqstr > StrCounterType;
  static StrCounterType usedNames;

  static MemoryObjContainer sharedMemoryObjContainer;
  MemoryObjContainer *      localMemoryObjContainer;

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

  MemObj *DL1;  // Data L1 cache
  MemObj *IL1;  // Instruction L1 cache
  MemObj *pref; // Prefetcher
  MemObj *vpc;  // Speculative virtual predictor cache

protected:
  const uint32_t coreId;

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
  MemObj *finishDeclareMemoryObj(std::vector<char *> vPars, char *name_suffix = NULL);

  uint32_t getNumMemObjs() {
    return MemObjNames.size();
  }
  void addMemObjName(const char *name) {
    MemObjNames.push_back(name);
  }
  std::string getMemObjName(uint32_t i) {
    return MemObjNames[i];
  }

  uint32_t getCoreId() const {
    return coreId;
  };
  MemObj *getDL1() const {
    return DL1;
  };
  MemObj *getIL1() const {
    return IL1;
  };
  MemObj *getvpc() const {
    return vpc;
  };
  MemObj *getPrefetcher() const {
    return pref;
  };
};

class DummyMemorySystem : public GMemorySystem {
private:
protected:
public:
  DummyMemorySystem(int32_t coreId);
  ~DummyMemorySystem();
};

#endif /* GMEMORYSYSTEM_H */
