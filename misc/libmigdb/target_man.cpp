/**[txh]********************************************************************

  Copyright (c) 2004-2007 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Target manipulation.
  Comments:
  GDB/MI commands for the "Target Manipulation" section.@p

@<pre>
-target-attach                  Yes (implemented using attach)
-target-compare-sections        N.A. (compare-sections)
-target-detach                  Yes
-target-download                Yes
-target-exec-status             N.A.
-target-list-available-targets  N.A. (help target)
-target-list-current-targets    N.A. (info file among other things)
-target-list-parameters         N.A.
-target-select                  Yes
@</pre>

***************************************************************************/

#include "mi_gdb.h"

/* Low level versions. */

void mi_target_select(mi_h *h, const char *type, const char *params)
{
 mi_send(h,"-target-select %s %s\n",type,params);
}

/* Note: -target-attach isn't currently implemented :-( (gdb 6.1.1) */
void mi_target_attach(mi_h *h, pid_t pid)
{
 mi_send(h,"attach %d\n",pid);
}

void mi_target_detach(mi_h *h)
{
 mi_send(h,"-target-detach\n");
}

void mi_target_download(mi_h *h)
{
 mi_send(h,"-target-download\n");
}

/* High level versions. */

/**[txh]********************************************************************

  Description:
  Connect to a remote gdbserver using the specified methode.

  Command: -target-select
  Return: !=0 OK
  
***************************************************************************/

int gmi_target_select(mi_h *h, const char *type, const char *params)
{
 mi_target_select(h,type,params);
 if (!mi_res_simple_connected(h))
    return 0;
 mi_send_target_commands(h);
 return 1;
}

/**[txh]********************************************************************

  Description:
  Attach to an already running process.

  Command: -target-attach [using attach]
  Return: The frame of the current location, NULL on error.
  
***************************************************************************/

mi_frames *gmi_target_attach(mi_h *h, pid_t pid)
{
 mi_target_attach(h,pid);
 //return mi_res_simple_done(h);
 return mi_res_frame(h);
}

/**[txh]********************************************************************

  Description:
  Detach from an attached process.

  Command: -target-detach
  Return: !=0 OK
  
***************************************************************************/

int gmi_target_detach(mi_h *h)
{
 mi_target_detach(h);
 return mi_res_simple_done(h);
}

/**[txh]********************************************************************

  Description:
  Loads the executable onto the remote target.

  Command: -target-download
  Return: !=0 OK
  
***************************************************************************/

int gmi_target_download(mi_h *h)
{
 mi_target_download(h);
 // TODO: this response have some data
 return mi_res_simple_done(h);
}

