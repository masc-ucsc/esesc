/**[txh]********************************************************************

  Copyright (c) 2004 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Stack manipulation.
  Comments:
  GDB/MI commands for the "Stack Manipulation" section.@p

@<pre>
gdb command:              Implemented?

-stack-info-frame         Yes, implemented as "frame"
-stack-info-depth         Yes
-stack-list-arguments     Yes
-stack-list-frames        Yes
-stack-list-locals        Yes
-stack-select-frame       Yes
@</pre>

***************************************************************************/

#include "mi_gdb.h"

/* Low level versions. */

void mi_stack_list_frames(mi_h *h, int from, int to)
{
 if (from<0)
    mi_send(h,"-stack-list-frames\n");
 else
    mi_send(h,"-stack-list-frames %d %d\n",from,to);
}

void mi_stack_list_arguments(mi_h *h, int show, int from, int to)
{
 if (from<0)
    mi_send(h,"-stack-list-arguments %d\n",show);
 else
    mi_send(h,"-stack-list-arguments %d %d %d\n",show,from,to);
}

void mi_stack_info_frame(mi_h *h)
{
 mi_send(h,"frame\n");
}

void mi_stack_info_depth(mi_h *h, int depth)
{
 if (depth<0)
    mi_send(h,"-stack-info-depth\n");
 else
    mi_send(h,"-stack-info-depth %d\n",depth);
}

void mi_stack_select_frame(mi_h *h, int framenum)
{
 mi_send(h,"-stack-select-frame %d\n",framenum);
}

void mi_stack_list_locals(mi_h *h, int show)
{
 mi_send(h,"-stack-list-locals %d\n",show);
}

/* High level versions. */

/**[txh]********************************************************************

  Description:
  List of frames. Arguments aren't filled.

  Command: -stack-list-frames
  Return:  A new list of mi_frames or NULL on error.
  
***************************************************************************/

mi_frames *gmi_stack_list_frames(mi_h *h)
{
 mi_stack_list_frames(h,-1,-1);
 return mi_res_frames_array(h,"stack");
}

/**[txh]********************************************************************

  Description: 
  List of frames. Arguments aren't filled. Only the frames in the @var{from}
 - @var{to} range are returned.
  
  Command: -stack-list-frames
  Return:  A new list of mi_frames or NULL on error.
  
***************************************************************************/

mi_frames *gmi_stack_list_frames_r(mi_h *h, int from, int to)
{
 mi_stack_list_frames(h,from,to);
 return mi_res_frames_array(h,"stack");
}

/**[txh]********************************************************************

  Description:
  List arguments. Only @var{level} and @var{args} filled.
  
  Command: -stack-list-arguments
  Return:  A new list of mi_frames or NULL on error.
  
***************************************************************************/

mi_frames *gmi_stack_list_arguments(mi_h *h, int show)
{
 mi_stack_list_arguments(h,show,-1,-1);
 return mi_res_frames_array(h,"stack-args");
}

/**[txh]********************************************************************

  Description:
  List arguments. Only @var{level} and @var{args} filled. Only for the
frames in the @var{from} - @var{to} range.
  
  Command: -stack-list-arguments
  Return:  A new list of mi_frames or NULL on error.
  
***************************************************************************/

mi_frames *gmi_stack_list_arguments_r(mi_h *h, int show, int from, int to)
{
 mi_stack_list_arguments(h,show,from,to);
 return mi_res_frames_array(h,"stack-args");
}

/**[txh]********************************************************************

  Description:
  Information about the current frame, including args.

  Command: -stack-info-frame [using frame]
  Return: A new mi_frames or NULL on error.
  
***************************************************************************/

mi_frames *gmi_stack_info_frame(mi_h *h)
{
 mi_stack_info_frame(h);
 return mi_res_frame(h);
}

/**[txh]********************************************************************

  Description:
  Stack info depth.

  Command: -stack-info-depth
  Return: The depth or -1 on error.
  
***************************************************************************/

int gmi_stack_info_depth(mi_h *h, int max_depth)
{
 mi_results *r;
 int ret=-1;

 mi_stack_info_depth(h,max_depth);
 r=mi_res_done_var(h,"depth");
 if (r && r->type==t_const)
   {
    ret=atoi(r->v.cstr);
    mi_free_results(r);
   }
 return ret;
}

/**[txh]********************************************************************

  Description:
  Set stack info depth.

  Command: -stack-info-depth [no args]
  Return: The depth or -1 on error.
  Example: 
  
***************************************************************************/

int gmi_stack_info_depth_get(mi_h *h)
{
 return gmi_stack_info_depth(h,-1);
}

/**[txh]********************************************************************

  Description:
  Change current frame.

  Command: -stack-select-frame
  Return: !=0 OK
  
***************************************************************************/

int gmi_stack_select_frame(mi_h *h, int framenum)
{
 mi_stack_select_frame(h,framenum);
 return mi_res_simple_done(h);
}

/**[txh]********************************************************************

  Description:
  List of local vars.

  Command: -stack-list-locals
  Return: A new mi_results tree containing the variables or NULL on error.
  
***************************************************************************/

mi_results *gmi_stack_list_locals(mi_h *h, int show)
{
 mi_stack_list_locals(h,show);
 return mi_res_done_var(h,"locals");
}

