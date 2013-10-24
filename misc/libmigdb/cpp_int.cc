/**[txh]********************************************************************

  Copyright (c) 2004-2007 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: C++ Interface.
  Comments:
  Implements a very simple (naive ;-) C++ wrapper.@p

***************************************************************************/

#include <string.h>
#include <limits.h>
#include "mi_gdb.h"

/**[txh]********************************************************************

  Description:
  Initializes a debugger object. It starts in the "disconnected" state.
Use @x{::Connect} after it.
  
***************************************************************************/

MIDebugger::MIDebugger()
{
 state=disconnected;
 h=NULL;
 aux_tty=NULL;
 waitingTempBkpt=0;
 targetEndian=enUnknown;
 targetArch=arUnknown;
}

/**[txh]********************************************************************

  Description:
  This is the destructor for the class. It tries to change the state to
"disconnected" doing the needed actions.
  
***************************************************************************/

MIDebugger::~MIDebugger()
{
 if (state==running)
   {
    Stop();
    mi_stop *rs;
    // TODO: Some kind of time-out
    while (!Poll(rs));
    mi_free_stop(rs);
    state=stopped;
   }
 if (state==stopped)
   {
    Kill();
    state=target_specified;
   }
 if (state==target_specified)
   {
    TargetUnselect();
    state=connected;
   }
 if (state==connected)
    Disconnect();
 // Here state==disconnected
}

/**[txh]********************************************************************

  Description:
  Connects to gdb. Currently only local connections are supported, that's
a gdb limitation. Call it when in "unconnected" state, on success it will
change to the "connected" state.  After it you should call one of the
SelectTarget members. @x{::SelectTargetX11}, @x{::SelectTargetLinux} or
@x{::SelectTargetRemote}. To finish the connection use @x{::Disconnect}.

  Return: !=0 OK.
  
***************************************************************************/

int MIDebugger::Connect(bool )
{
 if (state==disconnected)
   {
    h=mi_connect_local();
    if (h!=NULL)
      {
       state=connected;
       return 1;
      }
   }
 return 0;
}

/**[txh]********************************************************************

  Description:
  Finishes the connection to gdb. Call when in "connected" state, on success
it will change to "disconnected" state. This function first tries to exit
from gdb and then close the connection. But if gdb fails to exit it will be
killed.
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::Disconnect()
{
 if (state==connected)
   {
    gmi_gdb_exit(h);
    mi_disconnect(h);
    state=disconnected;
    return 1;
   }
 return 0;
}

/**[txh]********************************************************************

  Description:
  Protected member that implements @x{::SelectTargetX11} and
@x{::SelectTargetLinux}.
  
  Return: !=0 OK.
  
***************************************************************************/

int MIDebugger::SelectTargetTTY(const char *exec, const char *args,
                                const char *auxtty, dMode m)
{
 if (state!=connected)
    return 0;

 targetEndian=enUnknown;
 targetArch=arUnknown;
 mode=m;
 if (!gmi_set_exec(h,exec,args))
    return 0;

 const char *tty_name;
 #ifndef __CYGWIN__
 if (!auxtty)
   {
    aux_tty=m==dmLinux ? gmi_look_for_free_vt() : gmi_start_xterm();
    if (!aux_tty)
       return 0;
    tty_name=aux_tty->tty;
   }
 else
   {
    tty_name=auxtty;
   }
 if (!gmi_target_terminal(h,tty_name))
    return 0;
 #else
 tty_name=NULL;
 if (!gmi_gdb_set(h,"new-console","on"))
    return 0;
 #endif

 state=target_specified;
 preRun=false;
 return 1;
}

/**[txh]********************************************************************

  Description:
  Starts a debug session for X11. It opens an xterm console for the program
to debug and tells gdb which executable to debug and the command line
options to pass. You can specify an already existing tty console to be used.
Can be called when the state is "connected". On success will change to the
"target_specified" state. After it you can use @x{::Run} or use the members
to define breakpoints and similar stuff. To finish it use
@x{::TargetUnselect}.
  
  Return: !=0 OK.
  
***************************************************************************/

int MIDebugger::SelectTargetX11(const char *exec, const char *args,
                                const char *auxtty)
{
 return SelectTargetTTY(exec,args,auxtty,dmX11);
}


/**[txh]********************************************************************

  Description:
  Starts a debug session for Linux console. It selects an empty VT for the
program to debug and tells gdb which executable to debug and the command line
options to pass. You can specify an already existing tty console to be used.
Can be called when the state is "connected". On success will change to the
"target_specified" state. After it you can use @x{::Run} or use the members
to define breakpoints and similar stuff. To finish it use
@x{::TargetUnselect}.
  
  Return: !=0 OK.
  
***************************************************************************/

int MIDebugger::SelectTargetLinux(const char *exec, const char *args,
                                  const char *auxtty)
{
 return SelectTargetTTY(exec,args,auxtty,dmLinux);
}

/**[txh]********************************************************************

  Description:
  Starts a remote session. The other end should be running gdbserver. You
must specify a local copy of the program to debug with debug info. The remote
copy can be stripped. The @var{rtype} and @var{rparams} selects the protocol
and the remote machine. Read gdb docs to know more about the available
options. If @var{rtype} is omitted "extended-remote" protocol is used.
Can be called when the state is "connected". On success will change to the
"target_specified" state. After it you can use @x{::Run} or use the members
to define breakpoints and similar stuff. To finish it use
@x{::TargetUnselect}. Note that when gdb uses remote debugging the remote
program starts running. The @x{::Run} member knows about it.
  
  Return: !=0 OK.
  Example:
  o->SelectTargetRemote("./exec_file","192.168.1.65:5000");
  
***************************************************************************/

int MIDebugger::SelectTargetRemote(const char *exec, const char *rparams,
                                   const char *rtype, bool download)
{
 if (state!=connected)
    return 0;

 mode=dmRemote;
 preRun=true;
 targetEndian=enUnknown;
 targetArch=arUnknown;
 if (rtype==NULL)
    rtype="extended-remote";

 /* Tell gdb to load symbols from the local copy. */
 int res=download ? gmi_set_exec(h,exec,NULL) : gmi_file_symbol_file(h,exec);
 if (!res)
    return 0;
 /* Select the target */
 if (!gmi_target_select(h,rtype,rparams))
    return 0;
 /* Download the binary */
 if (download)
   {
    if (!gmi_target_download(h))
       return 0;
   }

 state=target_specified;
 return 1;
}

/**[txh]********************************************************************

  Description:
  Starts a local session using an already running process.
  
  Return: !=0 OK.
  
***************************************************************************/

mi_frames *MIDebugger::SelectTargetPID(const char *exec, int pid)
{
 if (state!=connected)
    return NULL;

 mode=dmPID;
 preRun=false;
 targetEndian=enUnknown;
 targetArch=arUnknown;

 mi_frames *res=gmi_target_attach(h,pid);
 if (res)
   {
    state=stopped;

    /* Tell gdb to load symbols from the local copy. */
    if (!gmi_file_symbol_file(h,exec))
      {
       mi_free_frames(res);
       return NULL;
      }
   }

 return res;
}

/**[txh]********************************************************************

  Description:
  Used to unselect the current target. When X11 mode it closes the auxiliar
terminal. For remote debugging it uses "detach". Can be called when in
"target_specified" state. On success it changes to "connected" state.
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::TargetUnselect()
{
 switch (mode)
   {
    case dmX11:
    case dmLinux:
         if (state!=target_specified)
            return 0;
         if (aux_tty)
           {
            gmi_end_aux_term(aux_tty);
            aux_tty=NULL;
           }
         break;
    case dmPID:
    case dmRemote:
         if (state!=target_specified)
           {
            if (state!=stopped || !gmi_target_detach(h))
               return 0;
           }
         break;
   }
 state=connected;
 return 1;
}

/**[txh]********************************************************************

  Description:
  Starts running the program. You should set breakpoint before it. Can be
called when state is "target_specified". On success will change to "running"
state. After it you should poll for async responses using @x{::Poll}. The
program can stop for many reasons asynchronously and also exit. This
information is known using Poll. You can stop the program using @x{::Stop}.
  
  Return: !=0 OK.
  
***************************************************************************/

int MIDebugger::Run()
{
 if (state!=target_specified)
    return 0;

 int res;
 if (preRun)
    res=gmi_exec_continue(h);
 else
    res=gmi_exec_run(h);
 if (res)
    state=running;

 return res;
}

/**[txh]********************************************************************

  Description:
  Stops the program execution. GDB sends an interrupt signal to the program.
Can be called when the state is "running". It won't switch to "stopped"
state automatically. Instead you must poll for async events and wait for a
stopped notification. After it you can call @x{::Continue} to resume
execution.
  
  Return: 
  Example: !=0 OK
  
***************************************************************************/

int MIDebugger::Stop()
{
 if (state!=running)
    return 0;
 return gmi_exec_interrupt(h);
}

/**[txh]********************************************************************

  Description:
  Polls gdb looking for async responses. Currently it just looks for
"stopped" messages. You must call it when the state is "running". But the
function will poll gdb even if the state isn't "running". When a stopped
message is received the state changes to stopped or target_specified (the
last is when we get some exit).
  
  Return: !=0 if we got a response. The @var{rs} pointer will point to an
mi_stop structure if we got it or will be NULL if we didn't.
  
***************************************************************************/

int MIDebugger::Poll(mi_stop *&rs)
{
 if (state==disconnected || !mi_get_response(h))
    return 0;

 mi_stop *res=mi_res_stop(h);
 if (res)
   {
    if (res->reason==sr_exited_signalled ||
        res->reason==sr_exited ||
        res->reason==sr_exited_normally)
       // When we use a PID the exit makes it invalid, so we don't have a
       // valid target to re-run.
       state=mode==dmPID ? connected : target_specified;
    else
       state=stopped;
    if (res->reason==sr_unknown && waitingTempBkpt)
      {
       waitingTempBkpt=0;
       res->reason=sr_bkpt_hit;
      }
   }
 else
   {// We got an error. It looks like most async commands returns running even
    // before they are sure the process is running. Latter we get the real
    // error. So I'm assuming the program is stopped.
    // Lamentably -target-exec-status isn't implemented and even in this case
    // if the program is really running as real async isn't implemented it
    // will fail anyways.
    if (state==running)
       state=stopped;
   }
 rs=res;
 return 1;
}

/**[txh]********************************************************************

  Description:
  Resumes execution after the program "stopped". Can be called when the state
is stopped. On success will change to "running" state.
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::Continue()
{
 if (state!=stopped)
    return 0;
 int res=gmi_exec_continue(h);
 if (res)
    state=running;
 return res;
}

/**[txh]********************************************************************

  Description:
  Starts program execution or resumes it. When the state is target_specified
it calls @x{::Run} otherwise it uses @x{::Continue}. Can be called when the
state is "target_specified" or "stopped". On success will change to
"running" state.
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::RunOrContinue()
{
 if (state==target_specified)
    return Run();
 return Continue();
}

/**[txh]********************************************************************

  Description:
  Kills the program you are debugging. Can be called when the state is
"stopped" or "running". On success changes the state to "target_specified".
Note that if you want to restart the program you can just call @x{::Run} and
if you want to just stop the program call @x{::Stop}.

  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::Kill()
{
 if (state!=stopped && state!=running)
    return 0;
 /* GDB/MI doesn't implement it (yet), so we use the regular kill. */
 /* Ensure confirm is off. */
 char *prev=gmi_gdb_show(h,"confirm");
 if (!prev)
    return 0;
 if (strcmp(prev,"off"))
   {
    if (!gmi_gdb_set(h,"confirm","off"))
      {
       free(prev);
       return 0;
      }
   }
 else
   {
    free(prev);
    prev=NULL;
   }
 /* Do the kill. */
 int res=gmi_exec_kill(h);
 /* Revert confirm option if needed. */
 if (prev)
   {
    gmi_gdb_set(h,"confirm",prev);
    free(prev);
   }

 if (res)
    state=target_specified;

 return res;
}

/**[txh]********************************************************************

  Description:
  Inserts a breakpoint at @var{file} and @var{line}. Can be called when the
state is "stopped" or "target_specified".
  
  Return: An mi_bkpt structure or NULL if error.
  
***************************************************************************/

mi_bkpt *MIDebugger::Breakpoint(const char *file, int line)
{
 if (state!=stopped && state!=target_specified)
    return NULL;
 return gmi_break_insert(h,file,line);
}

/**[txh]********************************************************************

  Description:
  Inserts a breakpoint at @var{where}, all options available. Can be called
when the state is "stopped" or "target_specified".
  
  Return: An mi_bkpt structure or NULL if error.
  
***************************************************************************/

mi_bkpt *MIDebugger::Breakpoint(const char *where, bool temporary,
                                const char *cond, int count, int thread,
                                bool hard_assist)
{
 if (state!=stopped && state!=target_specified)
    return NULL;
 return gmi_break_insert_full(h,temporary,hard_assist,cond,count,thread,where);
}


const int maxWhere=PATH_MAX+256;

mi_bkpt *MIDebugger::Breakpoint(mi_bkpt *b)
{
 if (state!=stopped && state!=target_specified)
    return NULL;

 char buf[maxWhere];
 buf[0]=0;
 switch (b->mode)
   {
    case m_file_line:
         snprintf(buf,maxWhere,"%s:%d",b->file,b->line);
         break;
    case m_function:
         snprintf(buf,maxWhere,"%s",b->func);
         break;
    case m_file_function:
         snprintf(buf,maxWhere,"%s:%s",b->file,b->func);
         break;
    case m_address:
         snprintf(buf,maxWhere,"*%p",b->addr);
         break;
   }
 return Breakpoint(buf,b->disp==d_del,b->cond,b->ignore,b->thread,
                   b->type==t_hw);
}

/**[txh]********************************************************************

  Description:
  Inserts a breakpoint at @var{file} and @var{line} all options available.
Can be called when the state is "stopped" or "target_specified".
  
  Return: An mi_bkpt structure or NULL if error.
  
***************************************************************************/

mi_bkpt *MIDebugger::BreakpointFull(const char *file, int line,
                                    bool temporary, const char *cond,
                                    int count, int thread, bool hard_assist)
{
 if (state!=stopped && state!=target_specified)
    return NULL;
 return gmi_break_insert_full_fl(h,file,line,temporary,hard_assist,cond,
                                 count,thread);
}

/**[txh]********************************************************************

  Description:
  Removes the specified breakpoint. It doesn't free the structure. Can be
called when the state is "stopped" or "target_specified".
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::BreakDelete(mi_bkpt *b)
{
 if ((state!=stopped && state!=target_specified) || !b)
    return 0;
 return gmi_break_delete(h,b->number);
}

/**[txh]********************************************************************

  Description:
  Inserts a watchpoint for the specified expression. Can be called when the
state is "stopped" or "target_specified".
  
  Return: An mi_wp structure or NULL if error.
  
***************************************************************************/

mi_wp *MIDebugger::Watchpoint(enum mi_wp_mode mode, const char *exp)
{
 if (state!=stopped && state!=target_specified)
    return NULL;
 return gmi_break_watch(h,mode,exp);
}

/**[txh]********************************************************************

  Description:
  Removes the specified watchpoint. It doesn't free the structure. Can be
called when the state is "stopped" or "target_specified".
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::WatchDelete(mi_wp *w)
{
 if ((state!=stopped && state!=target_specified) || !w)
    return 0;
 return gmi_break_delete(h,w->number);
}

/**[txh]********************************************************************

  Description:
  Puts a temporal breakpoint in main function and starts running. Can be
called when the state is "target_specified". If successful the state will
change to "running".
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::RunToMain()
{
 if (state!=target_specified)
    return 0;
 mi_bkpt *b=Breakpoint(mi_get_main_func(),true);
 if (!b)
    return 0;
 mi_free_bkpt(b);
 waitingTempBkpt=1;
 return Run();
}

/**[txh]********************************************************************

  Description:
  Executes upto the next line, doesn't follow function calls. The @var{inst}
argument is for assembler. If the state is "target_specified" it will go to
the first line in the main function. If the state is "stopped" will use the
next command. If successfully the state will change to "running".
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::StepOver(bool inst)
{
 int res=0;

 if (state==target_specified)
   {// We aren't running
    // Walk to main
    return RunToMain();
   }
 if (state==stopped)
   {
    if (inst)
       res=gmi_exec_next_instruction(h);
    else
       res=gmi_exec_next(h);
    if (res)
       state=running;
   }
 return res;
}

/**[txh]********************************************************************

  Description:
  Executes until the specified point. If the state is "target_specified" it
uses a temporal breakpoint. If the state is "stopped" it uses -exec-until.
Fails for any other state.
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::GoTo(const char *file, int line)
{
 int res=0;

 if (state==target_specified)
   {// We aren't running
    // Use a temporal breakpoint
    int l=strlen(file)+32;
    char buf[l];
    snprintf(buf,l,"%s:%d",file,line);
    mi_bkpt *b=Breakpoint(buf,true);
    if (b)
      {
       mi_free_bkpt(b);
       waitingTempBkpt=1;
       res=Run();
      }
   }
 else if (state==stopped)
   {
    res=gmi_exec_until(h,file,line);
    if (res)
       state=running;
   }
 return res;
}

/**[txh]********************************************************************

  Description:
  Executes until the specified point. If the state is "target_specified" it
uses a temporal breakpoint. If the state is "stopped" it uses -exec-until.
Fails for any other state.
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::GoTo(void *addr)
{
 int res=0;

 if (state==target_specified)
   {// We aren't running
    // Use a temporal breakpoint
    char buf[32];
    snprintf(buf,32,"*%p",addr);
    mi_bkpt *b=Breakpoint(buf,true);
    if (b)
      {
       mi_free_bkpt(b);
       waitingTempBkpt=1;
       res=Run();
      }
   }
 else if (state==stopped)
   {
    res=gmi_exec_until_addr(h,addr);
    if (res)
       state=running;
   }
 return res;
}


/**[txh]********************************************************************

  Description:
  Resumes execution until the end of the current funtion is reached. Only
usable when we are in the "stopped" state.
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::FinishFun()
{
 if (state!=stopped)
    return 0;
 int res=gmi_exec_finish(h);
 if (res)
    state=running;
 return res;
}

/**[txh]********************************************************************

  Description:
  Returns immediately. Only usable when we are in the "stopped" state.
  
  Return: !=NULL OK, the returned frame is the current location. That's a
synchronous function.
  
***************************************************************************/

mi_frames *MIDebugger::ReturnNow()
{
 if (state!=stopped)
    return 0;
 return gmi_exec_return(h);
}

/**[txh]********************************************************************

  Description:
  Returns the current list of frames.
  
  Return: !=NULL OK, the list of frames is returned.
  
***************************************************************************/

mi_frames *MIDebugger::CallStack(bool args)
{
 if (state!=stopped)
    return 0;
 mi_frames *fr1=gmi_stack_list_frames(h);
 if (fr1 && args)
   {// Get the function arguments
    mi_frames *fr2=gmi_stack_list_arguments(h,1);
    if (fr2)
      {// Transfer them to the other list
       mi_frames *p=fr1, *p2=fr2;
       while (p2 && p)
         {
          p->args=p2->args;
          p2->args=NULL;
          p2=p2->next;
          p=p->next;
         }
       mi_free_frames(fr2);
      }
   }
 return fr1;
}

/**[txh]********************************************************************

  Description:
  Executes upto the next line, it follows function calls. The @var{inst}
argument is for assembler. If the state is "target_specified" it will go to
the first line in the main function. If the state is "stopped" will use the
next command. If successfully the state will change to "running".
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::TraceInto(bool inst)
{
 int res=0;

 if (state==target_specified)
   {// We aren't running
    // Walk to main
    return RunToMain();
   }
 if (state==stopped)
   {
    if (inst)
       res=gmi_exec_step_instruction(h);
    else
       res=gmi_exec_step(h);
    if (res)
       state=running;
   }
 return res;
}

/**[txh]********************************************************************

  Description:
  Evaluates the provided expression. If we get an error the error
description is returned instead. Can't be called if "disconnected" or
"running".
  
  Return: The result of the expression (use free) or NULL.
  
***************************************************************************/

char *MIDebugger::EvalExpression(const char *exp)
{
 if (state==disconnected ||
     state==running) // No async :-(
    return NULL;
 // Evaluate it
 mi_error=MI_OK;
 char *res=gmi_data_evaluate_expression(h,exp);
 if (!res && mi_error_from_gdb)
   {// Not valid, return the error
    res=strdup(mi_error_from_gdb);
   }
 return res;
}

/**[txh]********************************************************************

  Description:
  Modifies the provided expression. If we get an error the error
description is returned instead. Can't be called if "disconnected" or
"running".
  
  Return: The result of the expression (use free) or NULL.
  
***************************************************************************/

char *MIDebugger::ModifyExpression(char *exp, char *newVal)
{
 if (state==disconnected ||
     state==running) // No async :-(
    return NULL;
 // Create an assignment
 int l1=strlen(exp);
 int l2=strlen(newVal);
 char b[l1+l2+2], *s=b;
 memcpy(s,exp,l1);
 s+=l1;
 *s='=';
 memcpy(++s,newVal,l2);
 s[l2]=0;
 // Evaluate it
 char *res=gmi_data_evaluate_expression(h,b);
 if (!res && mi_error_from_gdb)
   {// Not valid, return the error
    res=strdup(mi_error_from_gdb);
   }
 return res;
}

/**[txh]********************************************************************

  Description:
  Sends a command to gdb.
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::Send(const char *command)
{
 if (state==disconnected ||
     state==running) // No async :-(
    return 0;
 // TODO: detect and use -interpreter-exec?
 mi_send(h,"%s\n",command);
 return mi_res_simple_done(h);
}


/**[txh]********************************************************************

  Description:
  Fills the type and value fields of the mi_gvar provided list.
  
  Return: !=0 OK
  
***************************************************************************/

int MIDebugger::FillTypeVal(mi_gvar *var)
{
 while (var)
   {
    if (!var->type && !gmi_var_info_type(h,var))
       return 0;
    if (!var->value && !gmi_var_evaluate_expression(h,var))
       return 0;
    var=var->next;
   }
 return 1;
}

int MIDebugger::FillOneTypeVal(mi_gvar *var)
{
 if (!var)
    return 0;

 int ok=1;
 if (!var->type && !gmi_var_info_type(h,var))
   {
    var->type=strdup("");
    ok=0;
   }
 if (!var->value && !gmi_var_evaluate_expression(h,var))
   {
    var->value=strdup("");
    ok=0;
   }
 return ok;
}

int MIDebugger::AssigngVar(mi_gvar *var, const char *exp)
{
 if (state!=stopped)
    return 0;
 return gmi_var_assign(h,var,exp);
}

char *MIDebugger::Show(const char *var)
{
 if (state==running || state==disconnected)
    return 0;
 // GDB 5.x doesn't reply all in the response record, just to the console :-(
 h->catch_console=1;
 if (h->catched_console)
   {
    free(h->catched_console);
    h->catched_console=NULL;
   }
 char *res=gmi_gdb_show(h,var);
 h->catch_console=0;
 if (!res && h->catched_console)
   {
    res=h->catched_console;
    h->catched_console=NULL;
   }
 return res;
}

MIDebugger::endianType MIDebugger::GetTargetEndian()
{
 if (targetEndian!=enUnknown)
    return targetEndian;
 if (state!=stopped && state!=target_specified)
    return enUnknown;

 char *end=Show("endian");
 if (end)
   {
    if (strstr(end,"big"))
       targetEndian=enBig;
    else if (strstr(end,"little"))
       targetEndian=enLittle;
    free(end);
   }
 return targetEndian;
}

MIDebugger::archType MIDebugger::GetTargetArchitecture()
{
 if (targetArch!=arUnknown)
    return targetArch;
 if (state!=stopped && state!=target_specified)
    return arUnknown;

 char *end=Show("architecture");
 if (end)
   {
    if (strstr(end,"i386"))
       targetArch=arIA32;
    else if (strstr(end,"sparc"))
       targetArch=arSPARC;
    else if (strstr(end,"pic14"))
       targetArch=arPIC14;
    else if (strstr(end,"avr"))
       targetArch=arAVR;
    free(end);
   }
 return targetArch;
}

int MIDebugger::GetErrorNumberSt()
{
 if (mi_error==MI_GDB_DIED)
   {
    state=target_specified;
    TargetUnselect();
    state=connected;
    Disconnect();
   }
 return mi_error;
}

int MIDebugger::UpdateRegisters(mi_chg_reg *regs)
{
 int updated=0;
 mi_chg_reg *chg=GetChangedRegisters();
 if (chg)
   {
    mi_chg_reg *r=regs, *c;
    while (r)
      {
       c=chg;
       while (c && c->reg!=r->reg)
          c=c->next;
       if (c)
         {
          r->updated=1;
          free(r->val);
          r->val=c->val;
          c->val=NULL;
          updated++;
         }
       else
          r->updated=0;
       r=r->next;
      }
   }
 return updated;
}

