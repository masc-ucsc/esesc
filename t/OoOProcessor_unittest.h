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
#include "MemObj.h"
#include "GMemorySystem.h"
#include "MemorySystem.h"

#include "SescConf.h"
#include "GStats.h"
#include "PowerModel.h"

#include "BootLoader.h"

class OoOProcessorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    FlowID pId = 1;

    // Build Sesc config, power model
    std::ifstream in("crafty.input");
    std::streambuf *cinbuf = std::cin.rdbuf(); //save old buf
    std::cin.rdbuf(in.rdbuf()); //redirect std::cin to in.txt!

    int argc  = 1;
    const char *argv[1] = {"esesc"};

    BootLoader::plug(argc, argv);
    BootLoader::boot();


/*
    std::string word;
    std::cin >> word;           //input from the file in.txt
    std::cout << word << "  ";  //output to the file out.txt
    */

/*
    int argc  = 0;
    const char **argv;
    SescConf = new SConfig(argc, argv);
    if (!SescConf->check()) {
        FAIL() << "ESESC configuration incorrect.";
    }
    FlowID nsimu = SescConf->getRecordSize("","cpusimu");
    std::cout << "I: cpusimu size " << nsimu << "\n";

    GMemorySystem *gms = 0;
    gms = new MemorySystem(pId);
    gms->buildMemorySystem();
    */
    /*
    //IL1 = declareMemoryObj(def_block, "IL1");
    const char *def_block = SescConf->getCharPtr("", "cpusimu", pId);
    SescConf->isCharPtr(def_block, "IL1");
    std::vector<char *> vPars = SescConf->getSplitCharPtr(def_block, "IL1");
  if (!vPars.size()) {
      FAIL() << "Section " << def_block << " field IL1 does not describe a MemoryObj.\n";
    SescConf->notCorrect();
  }
  */
   
   // ----------------------------------------------
   /*
   // MemObj *ret = finishDeclareMemoryObj(vPars)
   MemObj *IL1;
   bool shared = false;
  const char *device_descr_section = vPars[0];
  char *device_name = (vPars.size() > 1) ? vPars[1] : 0;

  if (vPars.size() > 2) {

    if (strcasecmp(vPars[2], "shared") == 0) {

      //I(vPars.size() == 3);
      shared = true;

    } else if (strcasecmp(vPars[2], "sharedBy") == 0) {

      //I(vPars.size() == 4);
      int32_t sharedBy = atoi(vPars[3]);
      //delete[] vPars[3];
      FAIL() << "SharedBy should be bigger than zero.\n";
      //GMSG(sharedBy <= 0, "ERROR: SharedBy should be bigger than zero (field %s)", device_name);

      int32_t nId = pId / sharedBy;
      //device_name = GMemorySystem::privatizeDeviceName(device_name, nId);
      char *ret=new char[strlen(device_name) + 8 + (int)log10((float)pId+10)];

      sprintf(ret,"%s(%d)", device_name, pId);

      delete[] device_name;
      device_name  = ret;
      shared = true;
    }


  } else if (device_name) {

    if (strcasecmp(device_name, "shared") == 0) {
      //delete[] device_name;
      //device_name = 0;
      shared = true;
    }

  }

  SescConf->isCharPtr(device_descr_section, "deviceType");
  const char *device_type = SescConf->getCharPtr(device_descr_section, "deviceType");

  bool done = false;
  if (device_name) {

      if (!shared) {
          //device_name = privatizeDeviceName(device_name, pId);
          char *ret=new char[strlen(device_name) + 8 + (int)log10((float)pId+10)];

          sprintf(ret,"%s(%d)", device_name, pId);

          delete[] device_name;
          device_name  = ret;
          //

          MemObj *memdev = gms->searchMemoryObj(shared, device_descr_section, device_name); 

          if (memdev) {
              //delete[] device_name;
              //delete[] device_descr_section;
              //return memdev;
              IL1 = memdev;
              done = true;
          }

      } else {
           std::cout << "HERE!!!!\n";
          //device_name = buildUniqueName(device_type);
      }
  }

  if (!done) {

        //MemObj *GMemorySystem::buildMemoryObj(const char *type, const char *section, const char *name) 
  if (!(   strcasecmp(device_type, "dummy")   == 0
        || strcasecmp(device_type, "cache")   == 0
        || strcasecmp(device_type, "icache")  == 0
        || strcasecmp(device_type, "scache")  == 0
        || strcasecmp(device_type, "stridePrefetcher")  == 0
#if 0        
        //To Be implemented
        || strcasecmp(type, "markovPrefetcher")  == 0
        || strcasecmp(type, "taggedPrefetcher")  == 0
#endif
        || strcasecmp(device_type, "splitter")  == 0
        || strcasecmp(device_type, "siftsplitter")  == 0
        || strcasecmp(device_type, "smpcache")  == 0
        || strcasecmp(device_type, "memxbar")   == 0 )) {
    FAIL() << "Invalid memory type " << device_type;
    SescConf->notCorrect ();
  }

  MemObj *newMem = new DummyMemObj(device_descr_section, device_name);
  //=-----

      if (newMem) { // Would be 0 in known-error mode
          //getMemoryObjContainer(shared)->addMemoryObj(device_name, newMem);
        static MemoryObjContainer sharedMemoryObjContainer;
        sharedMemoryObjContainer.addMemoryObj(device_name, newMem);
      }

      IL1 = newMem;
  }
  IL1->getRouter()->fillRouteTables();
  */
  // ----------------------------------------------

    // Build memory sub-system
    //MemObj *IL1 = gms->declareMemoryObj(def_block, "IL1");
    //gms->finishDeclareMemoryOb
    /*
    gms->buildMemorySystem();

    // Build OoO Processor
    cpu0Id_ = static_cast<CPU_t>(0);
    //cpu1Id_ = static_cast<CPU_t>(0);
    //cpu2Id_ = static_cast<CPU_t>(2);

    GProcessor  *proc0 = 0;
    proc0 = new OoOProcessor(gms, cpu0Id_);
    */
  }

  virtual void TearDown() {
      BootLoader::report("done");
      BootLoader::unboot();
      BootLoader::unplug();
  }

    GProcessor *proc0_;
    GProcessor *proc1_;
    GProcessor *proc2_;

    CPU_t cpu0Id_;
    CPU_t cpu1Id_;
    CPU_t cpu2Id_;
};

#endif
