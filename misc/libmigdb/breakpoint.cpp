/**[txh]********************************************************************

  Copyright (c) 2004 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Breakpoint table commands.
  Comments:
  GDB/MI commands for the "Breakpoint Table Commands" section.@p

@<pre>
gdb command:          Implemented?

-break-after          Yes
-break-condition      Yes
-break-delete         Yes
-break-disable        Yes
-break-enable         Yes
-break-info           N.A. (info break NUMBER) (*)
-break-insert         Yes
-break-list           No (*)
-break-watch          Yes
@</pre>

(*) I think the program should keep track of the breakpoints, so it will
be implemented when I have more time.@p

***************************************************************************/

#include "mi_gdb.h"

/* Low level versions. */

void mi_break_insert_fl(mi_h *h, const char *file, int line)
{
 mi_send(h,"-break-insert %s:%d\n",file,line);
}

void mi_break_insert(mi_h *h, int temporary, int hard_assist,
                     const char *cond, int count, int thread,
                     const char *where)
{
 char s_count[32];
 char s_thread[32];

 if (count>=0)
    snprintf(s_count,32,"%d",count);
 if (thread>=0)
    snprintf(s_thread,32,"%d",thread);
 if (cond)
    // Conditions may contain spaces, in fact, if they don't gdb will add
    // them after parsing. Enclosing the expression with "" solves the
    // problem.
    mi_send(h,"-break-insert %s %s -c \"%s\" %s %s %s %s %s\n",
            temporary   ? "-t" : "",
            hard_assist ? "-h" : "",
            cond,
            count>=0    ? "-i" : "", count>=0  ? s_count  : "",
            thread>=0   ? "-p" : "", thread>=0 ? s_thread : "",
            where);
 else
    mi_send(h,"-break-insert %s %s %s %s %s %s %s\n",
            temporary   ? "-t" : "",
            hard_assist ? "-h" : "",
            count>=0    ? "-i" : "", count>=0  ? s_count  : "",
            thread>=0   ? "-p" : "", thread>=0 ? s_thread : "",
            where);
}

void mi_break_insert_flf(mi_h *h, const char *file, int line, int temporary,
                         int hard_assist, const char *cond, int count,
                         int thread)
{
 char s_count[32];
 char s_thread[32];

 if (count>=0)
    snprintf(s_count,32,"%d",count);
 if (thread>=0)
    snprintf(s_thread,32,"%d",thread);
 mi_send(h,"-break-insert %s %s %s %s %s %s %s %s %s:%d\n",
         temporary   ? "-t" : "",
         hard_assist ? "-h" : "",
         cond        ? "-c" : "", cond      ? cond     : "",
         count>=0    ? "-i" : "", count>=0  ? s_count  : "",
         thread>=0   ? "-p" : "", thread>=0 ? s_thread : "",
         file,line);
}

void mi_break_delete(mi_h *h, int number)
{
 mi_send(h,"-break-delete %d\n",number);
}

void mi_break_after(mi_h *h, int number, int count)
{
 mi_send(h,"-break-after %d %d\n",number,count);
}

void mi_break_condition(mi_h *h, int number, const char *condition)
{
 mi_send(h,"-break-condition %d %s\n",number,condition);
}

void mi_break_enable(mi_h *h, int number)
{
 mi_send(h,"-break-enable %d\n",number);
}

void mi_break_disable(mi_h *h, int number)
{
 mi_send(h,"-break-disable %d\n",number);
}

void mi_break_watch(mi_h *h, enum mi_wp_mode mode, const char *exp)
{
 if (mode==wm_write)
    mi_send(h,"-break-watch \"%s\"\n",exp);
 else
    mi_send(h,"-break-watch -%c \"%s\"\n",mode==wm_rw ? 'a' : 'r',exp);
}

/* High level versions. */

/**[txh]********************************************************************

  Description:
  Insert a breakpoint at file:line.

  Command: -break-insert file:line
  Return: A new mi_bkpt structure with info about the breakpoint. NULL on
error.
  
***************************************************************************/

mi_bkpt *gmi_break_insert(mi_h *h, const char *file, int line)
{
 mi_break_insert_fl(h,file,line);
 return mi_res_bkpt(h);
}

/**[txh]********************************************************************

  Description:
  Insert a breakpoint, all available options.
  
  Command: -break-insert
  Return: A new mi_bkpt structure with info about the breakpoint. NULL on
error.
  
***************************************************************************/

mi_bkpt *gmi_break_insert_full(mi_h *h, int temporary, int hard_assist,
                               const char *cond, int count, int thread,
                               const char *where)
{
 mi_break_insert(h,temporary,hard_assist,cond,count,thread,where);
 return mi_res_bkpt(h);
}

/**[txh]********************************************************************

  Description:
  Insert a breakpoint, all available options.
  
  Command: -break-insert [ops] file:line
  Return: A new mi_bkpt structure with info about the breakpoint. NULL on
error.
  
***************************************************************************/

mi_bkpt *gmi_break_insert_full_fl(mi_h *h, const char *file, int line,
                                  int temporary, int hard_assist,
                                  const char *cond, int count, int thread)
{
 mi_break_insert_flf(h,file,line,temporary,hard_assist,cond,count,thread);
 return mi_res_bkpt(h);
}

/**[txh]********************************************************************

  Description:
  Remove a breakpoint.

  Command: -break-delete
  Return: !=0 OK. Note that gdb always says OK, but errors can be sent to the
console.
  
***************************************************************************/

int gmi_break_delete(mi_h *h, int number)
{
 mi_break_delete(h,number);
 return mi_res_simple_done(h);
}

/**[txh]********************************************************************

  Description:
  Modify the "ignore" count for a breakpoint.

  Command: -break-after
  Return: !=0 OK. Note that gdb always says OK, but errors can be sent to the
console.
  
***************************************************************************/

int gmi_break_set_times(mi_h *h, int number, int count)
{
 mi_break_after(h,number,count);
 return mi_res_simple_done(h);
}

/**[txh]********************************************************************

  Description:
  Associate a condition with the breakpoint.

  Command: -break-condition
  Return: !=0 OK
  
***************************************************************************/

int gmi_break_set_condition(mi_h *h, int number, const char *condition)
{
 mi_break_condition(h,number,condition);
 return mi_res_simple_done(h);
}

/**[txh]********************************************************************

  Description:
  Enable or disable a breakpoint.

  Command: -break-enable + -break-disable
  Return: !=0 OK. Note that gdb always says OK, but errors can be sent to the
console.
  
***************************************************************************/

int gmi_break_state(mi_h *h, int number, int enable)
{
 if (enable)
    mi_break_enable(h,number);
 else
    mi_break_disable(h,number);
 return mi_res_simple_done(h);
}

/**[txh]********************************************************************

  Description:
  Set a watchpoint. It doesn't work for remote targets!

  Command: -break-watch
  Return: A new mi_wp structure with info about the watchpoint. NULL on
error.
  
***************************************************************************/

mi_wp *gmi_break_watch(mi_h *h, enum mi_wp_mode mode, const char *exp)
{
 mi_break_watch(h,mode,exp);
 return mi_res_wp(h);
}

