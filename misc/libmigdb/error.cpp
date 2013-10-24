/**[txh]********************************************************************

  Copyright (c) 2004 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Error.
  Comment:
  Translates error numbers into messages.
  
***************************************************************************/

#include "mi_gdb.h"

static
const char *error_strs[]=
{
 "Ok",
 "Out of memory",
 "Pipe creation",
 "Fork failed",
 "GDB not running",
 "Parser failed",
 "Unknown asyn response",
 "Unknown result response",
 "Error from gdb",
 "Time out in gdb response",
 "GDB suddenly died",
 "Can't execute X terminal",
 "Failed to create temporal",
 "Can't execute the debugger"
};

const char *mi_get_error_str()
{
 if (mi_error<0 || mi_error>MI_LAST_ERROR)
    return "Unknown";
 return error_strs[mi_error];
}
