#ifndef BPred_Test_H
#define BPred_Test_H
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <typeinfo>

#include "gtest/gtest.h"
#include "TestCommon.h"

#include "BootLoader.h"
#include "GProcessor.h"
#include "MemObj.h"
#include "GMemorySystem.h"
#include "MemorySystem.h"
#include "BPred.h"

#include "SescConf.h"
#include "GStats.h"

#define MAX_PROCESSORS 16


class BPredTest : public ::testing::Test {
  protected:
    /** 
     * Constructor for BPredTest will boot system using BootLoader.
     */
    virtual void SetUp() {
      const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
      std::cout << "Building test fixture for " << test_info->name() << ".\n";

      // Plug the system to ready for boot
      int argc  = 1;
      const char *argv[1] = {"esesc"};
      BootLoader::plug(argc, argv);

      // Make sure processors were instantiated
      maxProcessorId_ = TaskHandler::getNumCPUS();
      if (maxProcessorId_ < 1) {
        FAIL() << "No processors were initialized.";
      }
      std::cout << "Found " << maxProcessorId_ << " cpus.\n";

      // Grab a reference to each processor
      for(FlowID i = 0; i < maxProcessorId_ && i < MAX_PROCESSORS; i++) {
        processorId_[i] = i;
        processor_[i] = TaskHandler::getSimu(i);
        std::cout << "Processor " << i << " is of type " << typeid(processor_[i]).name() << "\n";
      }

    }

    virtual void TearDown() {
      const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
      std::cout << "Tearing down test fixture for " << test_info->name() << ".\n";
      BootLoader::report("done");
      BootLoader::unboot();
      BootLoader::unplug();

    }

    GProcessor *processor_[MAX_PROCESSORS];
    FlowID processorId_[MAX_PROCESSORS];
    FlowID maxProcessorId_;

};

#endif
