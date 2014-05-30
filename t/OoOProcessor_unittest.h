#ifndef OoO_Processor_H
#define OoO_Processor_H
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "GProcessor.h"
#include "OoOProcessor.h"
#include "SMTProcessor.h"
#include "InOrderProcessor.h"
#include "GPUSMProcessor.h"
#include "GMemorySystem.h"
#include "MemorySystem.h"

#include "SescConf.h"
#include "GStats.h"
#include "PowerModel.h"

class OoOProcessorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    int32_t pId = 1;

    // Build Sesc config, power model
    std::ifstream in("crafty.input");
    std::streambuf *cinbuf = std::cin.rdbuf(); //save old buf
    std::cin.rdbuf(in.rdbuf()); //redirect std::cin to in.txt!

    std::string word;
    std::cin >> word;           //input from the file in.txt
    std::cout << word << "  ";  //output to the file out.txt

    int argc  = 0;
    const char **argv;
    SescConf = new SConfig(argc, argv);
    if (!SescConf->check()) {
        FAIL() << "ESESC configuration incorrect.";
    }
    FlowID nsimu = SescConf->getRecordSize("","cpusimu");
    std::cout << "I: cpusimu size " << nsimu << "\n";
    const char *section = SescConf->getCharPtr("","cpusimu",0);

    const char *def_block = SescConf->getCharPtr("", "cpusimu", pId);
    //IL1 = declareMemoryObj(def_block, "IL1");
    SescConf->isCharPtr(def_block, "IL1");
    /*
    std::vector<char *> vPars = SescConf->getSplitCharPtr(def_block, "IL1");

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
    */
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
