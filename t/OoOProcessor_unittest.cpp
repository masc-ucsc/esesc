#include <iostream>
#include <stdlib.h>
#include <stdint.h>

#include "Report.h"
#include "GStats.h"

#include "OoOProcessor_unittest.h"
//#include "callback.h"

#ifndef SUCCESS
#define SUCCESS     0
#endif
#ifndef ERROR
#define ERROR       1
#endif

/****** Prototypes *******/
static int printProcessorStats(GProcessor *proc);

/******** Tests *********/
TEST_F(OoOProcessorTest, ExecutesOkay)
{

    EXPECT_EQ(printProcessorStats(processor_[0]), SUCCESS);
    
    BootLoader::boot();
    
    /*
    FlowID sFid = 0;
     //gettimeofday(&stTime, 0);

    ASSERT_TRUE(SescConf->lock());

      SescConf->dump(); 
      processor_[sFid]->setActive();
        for (int i =0; i < 50; i++) {
          processor_[sFid]->fetch(sFid);
          processor_[sFid]->execute();
        }
        */

    EXPECT_EQ(printProcessorStats(processor_[0]), SUCCESS);


    {

        char str[255];
        GStats *gref = 0;
        FlowID id = 0;

        // Failed when running craft.armel, but I saw: clockTicks=12613068 nCommitted=11345423
        // Clock ticks
        sprintf(str, "P(%d):clockTicks", id); 
        gref = GStats::getRef(str);
        ASSERT_TRUE( gref and gref->getSamples() > 0 );
        // Number of instructions committed
        sprintf(str, "P(%d):nCommitted", id); 
        gref = GStats::getRef(str);
        ASSERT_TRUE( gref and gref->getSamples() > 0 );
    }
    /*
    //std::cout << "Gstat name=" << GStats::getName() << ", samples=" << GStats::getSamples() <<"\n";
      FlowID sFid = 0;

    // Get starting counts
      char str[255];
      sprintf(str, "P(%d):clockTicks", sFid); //FIXME: needs mapping
      GStats *gref = 0;
      gref = GStats::getRef(str);

      ASSERT_TRUE( gref != NULL );
      uint64_t cticks        = gref->getSamples();
      std::cout << "Had clockticks = " << cticks << "\n";

 
    // Execute instructions on processor
    // Some info
    printf("Proc0: id=%u flushing=%u replayr=%u maxflow=%u robempty=%u\n",
        processor_[sFid]->getId(), processor_[sFid]->isFlushing(), processor_[sFid]->isReplayRecovering(),
        processor_[sFid]->getMaxFlows(), processor_[sFid]->isROBEmpty());    
    printf("\t rrobUsed=%ld robUsed=%ld nReplyInst=%ld noFetch=%ld noFetch2=%ld nFreeze=%ld clockTicks=%ld",
        ((GStats*)GStats::getRef("P(0):rrobUsed"))->getSamples(),
        ((GStats*)GStats::getRef("P(0):robUsed"))->getSamples(),
        ((GStats*)GStats::getRef("P(0):nReplyInst"))->getSamples(),
        ((GStats*)GStats::getRef("P(0):noFetch"))->getSamples(),
        ((GStats*)GStats::getRef("P(0):noFetch2"))->getSamples(),
        ((GStats*)GStats::getRef("P(0):nFreeze"))->getSamples(),
        ((GStats*)GStats::getRef("P(0):clockTicks"))->getSamples());

           //" lastWallClock=%u wallClock=%u throttlingRGStats::
           */
    /*
      processor_[sFid]->setActive();
        for (int i =0; i < 10; i++) {
          processor_[sFid]->fetch(sFid);
          processor_[sFid]->execute();
        }
        */
/*
    // Check counts
      gref = GStats::getRef(str);
      ASSERT_TRUE( gref != NULL );
      cticks        = gref->getSamples();
      std::cout << "Have clockticks = " << cticks << "\n";
    //BootLoader::boot();
    */

    //GStats::report("Cache Stats");
    //Report::close();

}

/********************** Static Helper Functions ************************/

/**
 * Prints information using GStats about a given processor.
 * @param GProcessor instance to get stats for.
 * @return SUCCESS or ERROR
 */
static int printProcessorStats(GProcessor *proc) {
    FlowID id;
    uint64_t stats[8] = { 0 };

    if (!proc || dynamic_cast<GProcessor*>(proc) == NULL)
    {
        printf("ERROR: proc argument not of GProcessor* type.\n");
        return ERROR;
    }

    id = proc->getId();

  char str[255];
  GStats *gref = 0;
  int i = 0;

    // Clock ticks
    sprintf(str, "P(%d):clockTicks", id); 
    gref = GStats::getRef(str);
    if (!gref) {
        printf("ERROR: Failed to get GStats clockTicks\n");
        return ERROR;
    }
    stats[i++] = gref->getSamples();
    // nCommitted
    sprintf(str, "P(%d):nCommitted", id); 
    gref = GStats::getRef(str);
    if (!gref) {
        printf("ERROR: Failed to get GStats nCommitted\n");
        return ERROR;
    }
    // rrobUsed
    sprintf(str, "P(%d)_rrobUsed", id); 
    gref = GStats::getRef(str);
    if (!gref) {
        printf("ERROR: Failed to get GStats _rrobUsed\n");
        return ERROR;
    }
    stats[i++] = gref->getSamples();
    // robUsed
    sprintf(str, "P(%d)_robUsed", id); 
    gref = GStats::getRef(str);
    if (!gref) {
        printf("ERROR: Failed to get GStats _robUsed\n");
        return ERROR;
    }
    stats[i++] = gref->getSamples();
    // nReplayInst
    sprintf(str, "P(%d)_nReplayInst", id); 
    gref = GStats::getRef(str);
    if (!gref) {
        printf("ERROR: Failed to get GStats _nReplayInst\n");
        return ERROR;
    }
    stats[i++] = gref->getSamples();
    // noFetch
    sprintf(str, "P(%d):noFetch", id); 
    gref = GStats::getRef(str);
    if (!gref) {
        printf("ERROR: Failed to get GStats noFetch\n");
        return ERROR;
    }
    stats[i++] = gref->getSamples();
    // noFetch2
    sprintf(str, "P(%d):noFetch2", id); 
    gref = GStats::getRef(str);
    if (!gref) {
        printf("ERROR: Failed to get GStats noFetch2\n");
        return ERROR;
    }
    stats[i++] = gref->getSamples();
    // nFreeze 
    sprintf(str, "P(%d):nFreeze", id);
    gref = GStats::getRef(str);
    if (!gref) {
        printf("ERROR: Failed to get GStats nFreeze\n");
        return ERROR;
    }
    stats[i] = gref->getSamples();

    i = 0;
    printf("-------------- Proc%u Stats -----------------\n"
           "\tflushing=%u replayr=%u maxflow=%u robempty=%u\n",
        proc->getId(), proc->isFlushing(), proc->isReplayRecovering(),
        proc->getMaxFlows(), proc->isROBEmpty());    
    printf("\tclockTicks=%ld nCommitted=%ld rrobUsed=%ld robUsed=%ld nReplyInst=%ld noFetch=%ld noFetch2=%ld nFreeze=%ld \n\n",
        stats[0], stats[1], stats[2], stats[3], stats[4], stats[5], stats[6], stats[7]);
    
    return SUCCESS;
}

