/**[txh]********************************************************************

  Copyright (c) 2004 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Thread commands.
  Comments:
  GDB/MI commands for the "Thread Commands" section.@p

@<pre>
gdb command:              Implemented?
-thread-info              N.A.
-thread-list-all-threads  Yes, implemented as "info threads"
-thread-list-ids          Yes
-thread-select            Yes
@</pre>

***************************************************************************/

#include "mi_gdb.h"

/* Low level versions. */

void mi_thread_list_ids(mi_h *h)
{
 mi_send(h,"-thread-list-ids\n");
}

void mi_thread_select(mi_h *h, int id)
{
 mi_send(h,"-thread-select %d\n",id);
}

void mi_thread_list_all_threads(mi_h *h)
{
 mi_send(h,"info threads\n");
}

/* High level versions. */

/**[txh]********************************************************************

  Description:
  List available thread ids.

  Command: -thread-list-ids
  Return: !=0 OK
  
***************************************************************************/

int gmi_thread_list_ids(mi_h *h, int **list)
{
 mi_thread_list_ids(h);
 return mi_res_thread_ids(h,list);
}

/**[txh]********************************************************************

  Description:
  Select a thread.

  Command: -thread-select
  Return: A new mi_frames or NULL on error.
  
***************************************************************************/

mi_frames *gmi_thread_select(mi_h *h, int id)
{
 mi_thread_select(h,id);
 return mi_res_frame(h);
}

/**[txh]********************************************************************

  Description:
  Get a list of frames for each available thread. Implemented using "info
thread".

  Command: -thread-list-all-threads
  Return: A kist of frames, NULL on error
  
***************************************************************************/

mi_frames *gmi_thread_list_all_threads(mi_h *h)
{
 mi_thread_list_all_threads(h);
 return mi_res_frames_list(h);
}

