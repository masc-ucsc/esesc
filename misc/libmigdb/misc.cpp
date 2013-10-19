/**[txh]********************************************************************

  Copyright (c) 2004 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Miscellaneous commands.
  Comments:
  GDB/MI commands for the "Miscellaneous Commands" section.@p

@<pre>
gdb command:       Implemented?

-gdb-exit          Yes
-gdb-set           Yes
-gdb-show          Yes
-gdb-version       Yes
@</pre>

GDB Bug workaround for "-gdb-show architecture": gdb 6.1 and olders doesn't
report it in "value", but they give the output of "show architecture". In
6.4 we observed that not even a clue is reported. So now we always use
"show architecture".

***************************************************************************/

#include <string.h>
#include "mi_gdb.h"

/* Low level versions. */

void mi_gdb_exit(mi_h *h)
{
 mi_send(h,"-gdb-exit\n");
}

void mi_gdb_version(mi_h *h)
{
 mi_send(h,"-gdb-version\n");
}

void mi_gdb_set(mi_h *h, const char *var, const char *val)
{
 mi_send(h,"-gdb-set %s %s\n",var,val);
}

void mi_gdb_show(mi_h *h, const char *var)
{
 if (strcmp(var,"architecture")==0)
    mi_send(h,"show %s\n",var);
 else
    mi_send(h,"-gdb-show %s\n",var);
}

/* High level versions. */

/**[txh]********************************************************************

  Description:
  Exit gdb killing the child is it is running.

  Command: -gdb-exit

***************************************************************************/

void gmi_gdb_exit(mi_h *h)
{
 mi_gdb_exit(h);
 mi_res_simple_exit(h);
}

/**[txh]********************************************************************

  Description:
  Send the version to the console.

  Command: -gdb-version
  Return: !=0 OK
  
***************************************************************************/

int gmi_gdb_version(mi_h *h)
{
 mi_gdb_version(h);
 return mi_res_simple_done(h);
}

/**[txh]********************************************************************

  Description:
  Set a gdb variable.

  Command: -gdb-set
  Return: !=0 OK
  
***************************************************************************/

int gmi_gdb_set(mi_h *h, const char *var, const char *val)
{
 mi_gdb_set(h,var,val);
 return mi_res_simple_done(h);
}

/**[txh]********************************************************************

  Description:
  Get a gdb variable.

  Command: -gdb-show
  Return: The current value of the variable or NULL on error.
  
***************************************************************************/

char *gmi_gdb_show(mi_h *h, const char *var)
{
 mi_gdb_show(h,var);
 return mi_res_value(h);
}

