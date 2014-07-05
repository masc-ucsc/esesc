
/* BPred_unittest
 *
 * Branch predictor unit test. Uses the boot loader to start a system with default settings
 * (typically read from esesc.conf). Tests are documented below.
 *
 */
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <map>
#include <string>

#include "Report.h"
#include "GStats.h"

#include "TestCommon.h"
#include "BPred_unittest.h"

/**************************** Tests **********************************/
/**
 * Checks if banch predictions are made after running the system.
 */
TEST_F(BPredTest,PredictsOkay)
{

  EXPECT_EQ(TestCommon::printProcessorStats(0), SUCCESS);

  BootLoader::boot();

  EXPECT_EQ(TestCommon::printProcessorStats(0), SUCCESS);

  // Check stats
  {

    std::vector<std::string> statNames;
    statNames.push_back(string("nBranches"));
    statNames.push_back(string("nTaken"));
    statNames.push_back(string("nMiss"));
    std::map<std::string, GStats*> stats = TestCommon::getStats(0, statNames, "_BPred:");
    ASSERT_TRUE( !stats.empty() );

    ASSERT_TRUE( stats["nBranches"] && stats["nBranches"]->getSamples() > 0 );
    ASSERT_TRUE( stats["nTaken"] && stats["nTaken"]->getSamples() > 0 );
    ASSERT_TRUE( stats["nMiss"] && (stats["nMiss"]->getSamples() < stats["nBranches"]->getSamples()) );
  }
}


