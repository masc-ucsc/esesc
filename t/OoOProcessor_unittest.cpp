
/* OoOProcessor_unittest
 *
 * OoOProcessor unit test. Uses the boot loader to start a system with default settings
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

#include "OoOProcessor_unittest.h"
#include "TestCommon.h"

/**************************** Tests **********************************/
/**
 * Runs the system and checks if clock ticks were made and instruction committed.
 */
TEST_F(OoOProcessorTest, ExecutesOkay)
{

  EXPECT_EQ(TestCommon::printProcessorStats(0), SUCCESS);

  BootLoader::boot();

  EXPECT_EQ(TestCommon::printProcessorStats(0), SUCCESS);

  // Check stats
  {

    std::vector<std::string> statNames;
    statNames.push_back(string("clockTicks"));
    statNames.push_back(string("nCommitted"));
    std::map<std::string, GStats*> stats = TestCommon::getStats(0, statNames);
    ASSERT_TRUE( !stats.empty() );

    ASSERT_TRUE( stats["clockTicks"] && stats["clockTicks"]->getSamples() > 0 );
    ASSERT_TRUE( stats["nCommitted"] && stats["nCommitted"]->getSamples() > 0 );
    // Failed when running craft.armel, but I saw: clockTicks=12613068 nCommitted=11345423
  }
}

