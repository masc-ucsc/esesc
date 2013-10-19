/**[txh]********************************************************************

  Copyright (c) 2004 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Program control.
  Comments:
  GDB/MI commands for the "Program Control" section.@p

@<pre>
gdb command:                   Implemented?

-exec-abort                    N.A. (*) (kill, but with non-interactive options)
-exec-arguments                Yes
-exec-continue                 Yes  ASYNC
-exec-finish                   Yes  ASYNC
-exec-interrupt                Yes  ASYNC
-exec-next                     Yes  ASYNC
-exec-next-instruction         Yes  ASYNC
-exec-return                   Yes
-exec-run                      Yes  ASYNC
-exec-show-arguments           N.A. (show args) see gmi_stack_info_frame
-exec-step                     Yes  ASYNC
-exec-step-instruction         Yes  ASYNC
-exec-until                    Yes  ASYNC
-file-exec-and-symbols         Yes
-file-exec-file                No
-file-list-exec-sections       N.A. (info file)
-file-list-exec-source-files   N.A.
-file-list-shared-libraries    N.A.
-file-list-symbol-files        N.A.
-file-symbol-file              Yes
@</pre>

(*)  gmi_exec_kill implements it, but you should ensure that
gmi_gdb_set("confirm","off") was called.@p

GDB Bug workaround for -file-exec-and-symbols and -file-symbol-file: This
is complex, but a real bug. When you set a breakpoint you never know the
name of the file as it appears in the debug info. So you can be specifying
an absolute file name or a relative file name. The reference point could be
different than the one used in the debug info. To solve all the combinations
gdb does a search trying various combinations. GDB isn't very smart so you
must at least specify the working directory and the directory where the
binary is located to get a good chance (+ user options to solve the rest).
Once you did it gdb can find the file by doing transformations to the
"canonical" filename. This search works OK for already loaded symtabs
(symbol tables), but it have a bug when the search is done for psymtabs
(partial symtabs). The bug is in the use of source_full_path_of (source.c).
This function calls openp indicating try_cwd_first. It makes the search file
if the psymtab file name have at least one dirseparator. It means that
psymtabs for files compiled with relative paths will fail. The search for
symtabs uses symtab_to_filename, it calls open_source_file which finally
calls openp without try_cwd_first.@*
To workaround this bug we must ensure gdb loads *all* the symtabs to memory.
And here comes another problem -file-exec-and-symbols doesn't support it
according to docs. In real life that's a wrapper for "file", but as nobody
can say it won't change we must use the CLI command.
  
***************************************************************************/

#include <signal.h>
#include "mi_gdb.h"

/* Low level versions. */

void mi_file_exec_and_symbols(mi_h *h, const char *file)
{
 if (mi_get_workaround(MI_PSYM_SEARCH))
    mi_send(h,"file %s -readnow\n",file);
 else
    mi_send(h,"-file-exec-and-symbols %s\n",file);
}

void mi_exec_arguments(mi_h *h, const char *args)
{
 mi_send(h,"-exec-arguments %s\n",args);
}

void mi_exec_run(mi_h *h)
{
 mi_send(h,"-exec-run\n");
}

void mi_exec_continue(mi_h *h)
{
 mi_send(h,"-exec-continue\n");
}

void mi_target_terminal(mi_h *h, const char *tty_name)
{
 mi_send(h,"tty %s\n",tty_name);
}

void mi_file_symbol_file(mi_h *h, const char *file)
{
 if (mi_get_workaround(MI_PSYM_SEARCH))
    mi_send(h,"symbol-file %s -readnow\n",file);
 else
    mi_send(h,"-file-symbol-file %s\n",file);
}

void mi_exec_finish(mi_h *h)
{
 mi_send(h,"-exec-finish\n");
}

void mi_exec_interrupt(mi_h *h)
{
 mi_send(h,"-exec-interrupt\n");
}

void mi_exec_next(mi_h *h, int count)
{
 if (count>1)
    mi_send(h,"-exec-next %d\n",count);
 else
    mi_send(h,"-exec-next\n");
}

void mi_exec_next_instruction(mi_h *h)
{
 mi_send(h,"-exec-next-instruction\n");
}

void mi_exec_step(mi_h *h, int count)
{
 if (count>1)
    mi_send(h,"-exec-step %d\n",count);
 else
    mi_send(h,"-exec-step\n");
}

void mi_exec_step_instruction(mi_h *h)
{
 mi_send(h,"-exec-step-instruction\n");
}

void mi_exec_until(mi_h *h, const char *file, int line)
{
 if (!file)
    mi_send(h,"-exec-until\n");
 else
    mi_send(h,"-exec-until %s:%d\n",file,line);
}

void mi_exec_until_addr(mi_h *h, void *addr)
{
 mi_send(h,"-exec-until *%p\n",addr);
}

void mi_exec_return(mi_h *h)
{
 mi_send(h,"-exec-return\n");
}

void mi_exec_kill(mi_h *h)
{
 mi_send(h,"kill\n");
}

/* High level versions. */

/**[txh]********************************************************************

  Description:
  Specify the executable and arguments for local debug.

  Command: -file-exec-and-symbols + -exec-arguments
  Return: !=0 OK
  
***************************************************************************/

int gmi_set_exec(mi_h *h, const char *file, const char *args)
{
 mi_file_exec_and_symbols(h,file);
 if (!mi_res_simple_done(h))
    return 0;
 if (!args)
    return 1;
 mi_exec_arguments(h,args);
 return mi_res_simple_done(h);
}

/**[txh]********************************************************************

  Description:
  Start running the executable. Remote sessions starts running.

  Command: -exec-run
  Return: !=0 OK
  
***************************************************************************/

int gmi_exec_run(mi_h *h)
{
 mi_exec_run(h);
 return mi_res_simple_running(h);
}

/**[txh]********************************************************************

  Description:
  Continue the execution after a "stop".

  Command: -exec-continue
  Return: !=0 OK
  
***************************************************************************/

int gmi_exec_continue(mi_h *h)
{
 mi_exec_continue(h);
 return mi_res_simple_running(h);
}

/**[txh]********************************************************************

  Description:
  Indicate which terminal will use the target program. For local sessions.

  Command: tty
  Return: !=0 OK
  Example: 
  
***************************************************************************/

int gmi_target_terminal(mi_h *h, const char *tty_name)
{
 mi_target_terminal(h,tty_name);
 return mi_res_simple_done(h);
}

/**[txh]********************************************************************

  Description:
  Specify what's the local copy that have debug info. For remote sessions.

  Command: -file-symbol-file
  Return: !=0 OK
  
***************************************************************************/

int gmi_file_symbol_file(mi_h *h, const char *file)
{
 mi_file_symbol_file(h,file);
 return mi_res_simple_done(h);
}

/**[txh]********************************************************************

  Description:
  Continue until function return, the return value is included in the async
response.

  Command: -exec-finish
  Return: !=0 OK.
  
***************************************************************************/

int gmi_exec_finish(mi_h *h)
{
 mi_exec_finish(h);
 return mi_res_simple_running(h);
}

/**[txh]********************************************************************

  Description:
  Stop the program using SIGINT. The corresponding command should be
-exec-interrupt but not even gdb 6.1.1 can do it because the "async" mode
isn't really working.

  Command: -exec-interrupt [replacement]
  Return: Always 1
  Example: 
  
***************************************************************************/

int gmi_exec_interrupt(mi_h *h)
{
 // **** IMPORTANT!!! **** Not even gdb 6.1.1 can do it because the "async"
 // mode isn't really working.
 //mi_exec_interrupt(h);
 //return mi_res_simple_running(h);

 kill(h->pid,SIGINT);
 return 1; // How can I know?
}

/**[txh]********************************************************************

  Description:
  Next line of code.

  Command: -exec-next
  Return: !=0 OK
  
***************************************************************************/

int gmi_exec_next(mi_h *h)
{
 mi_exec_next(h,1);
 return mi_res_simple_running(h);
}

/**[txh]********************************************************************

  Description:
  Skip count lines of code.

  Command: -exec-next count
  Return: !=0 OK

***************************************************************************/

int gmi_exec_next_cnt(mi_h *h, int count)
{
 mi_exec_next(h,count);
 return mi_res_simple_running(h);
}

/**[txh]********************************************************************

  Description:
  Next line of assembler code.

  Command: -exec-next-instruction
  Return: !=0 OK

***************************************************************************/

int gmi_exec_next_instruction(mi_h *h)
{
 mi_exec_next_instruction(h);
 return mi_res_simple_running(h);
}

/**[txh]********************************************************************

  Description:
  Next line of code. Get inside functions.

  Command: -exec-step
  Return: !=0 OK
  
***************************************************************************/

int gmi_exec_step(mi_h *h)
{
 mi_exec_step(h,1);
 return mi_res_simple_running(h);
}

/**[txh]********************************************************************

  Description:
  Next count lines of code. Get inside functions.

  Command: -exec-step count
  Return: !=0 OK

***************************************************************************/

int gmi_exec_step_cnt(mi_h *h, int count)
{
 mi_exec_step(h,count);
 return mi_res_simple_running(h);
}

/**[txh]********************************************************************

  Description:
  Next line of assembler code. Get inside calls.

  Command: -exec-step-instruction
  Return: !=0 OK
  
***************************************************************************/

int gmi_exec_step_instruction(mi_h *h)
{
 mi_exec_step_instruction(h);
 return mi_res_simple_running(h);
}

/**[txh]********************************************************************

  Description:
  Execute until location is reached. If file is NULL then is until next
line.

  Command: -exec-until
  Return: !=0 OK
  
***************************************************************************/

int gmi_exec_until(mi_h *h, const char *file, int line)
{
 mi_exec_until(h,file,line);
 return mi_res_simple_running(h);
}

/**[txh]********************************************************************

  Description:
  Execute until location is reached.

  Command: -exec-until (using *address)
  Return: !=0 OK
  
***************************************************************************/

int gmi_exec_until_addr(mi_h *h, void *addr)
{
 mi_exec_until_addr(h,addr);
 return mi_res_simple_running(h);
}

/**[txh]********************************************************************

  Description:
  Return to previous frame inmediatly.

  Command: -exec-return
  Return: A pointer to a new mi_frames structure indicating the current
location. NULL on error.
  
***************************************************************************/

mi_frames *gmi_exec_return(mi_h *h)
{
 mi_exec_return(h);
 return mi_res_frame(h);
}

/**[txh]********************************************************************

  Description:
  Just kill the program. That's what -exec-abort should do, but it isn't
implemented by gdb. This implementation only works if the interactive mode
is disabled (gmi_gdb_set("confirm","off")).

  Command: -exec-abort [using kill]
  Return: !=0 OK
  
***************************************************************************/

int gmi_exec_kill(mi_h *h)
{
 mi_exec_kill(h);
 return mi_res_simple_done(h);
}

