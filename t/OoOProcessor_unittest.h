#ifndef OoO_Processor_H
#define OoO_Processor_H

#include "gtest/gtest.h"

#include "OoOProcessor.h"
#include "GMemorySystem.h"
#include "MemorySystem.h"

class OoOProcessorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    int32_t pId = 1;

    // Build memory sub-system
    GMemorySystem *gms = 0;
    gms = new MemorySystem(pId);
    gms->buildMemorySystem();

    // Build OoO Processor
    cpu0Id_ = static_cast<CPU_t>(0);
    //cpu1Id_ = static_cast<CPU_t>(0);
    //cpu2Id_ = static_cast<CPU_t>(2);

    GProcessor  *proc0 = 0;
    proc0 = new OoOProcessor(gms, cpu0Id_);
  }

  // virtual void TearDown() {}

    GProcessor *proc0_;
    GProcessor *proc1_;
    GProcessor *proc2_;

    CPU_t cpu0Id_;
    CPU_t cpu1Id_;
    CPU_t cpu2Id_;
};

#endif
