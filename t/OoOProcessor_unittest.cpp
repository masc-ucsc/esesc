#include <iostream>
#include <stdlib.h>
#include <stdint.h>

/*
 * Code for SetUp is modeled after simu/libsampler/BootLoader.cpp
 */

#include "OoOProcessor_unittest.h"


TEST_F(OoOProcessorTest, InitializesOkay)
{
    ASSERT_TRUE( proc0_ != NULL );
        //EXPECT_EQ(0, 0);
}
