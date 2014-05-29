#include <iostream>
#include <stdlib.h>
#include <stdint.h>

#include "OoOProcessor_unittest.h"


TEST_F(OoOProcessorTest, InitializesOkay)
{
    ASSERT_TRUE( proc0_ != NULL );
        //EXPECT_EQ(0, 0);
}
