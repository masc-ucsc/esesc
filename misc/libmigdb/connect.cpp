/**[txh]********************************************************************

  Copyright (c) 2004-2009 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Connect.
  Comments:
  This module handles the dialog with gdb, including starting and stopping
gdb.@p

GDB Bug workaround for "file -readnow": I tried to workaround a bug using
it but looks like this option also have bugs!!!! so I have to use the
command line option --readnow.
It also have a bug!!!! when the binary is changed and gdb must reload it
this option is ignored. So it looks like we have no solution but 3 gdb bugs
in a row.

***************************************************************************/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "mi_gdb.h"

#ifndef TEMP_FAILURE_RETRY
 #define TEMP_FAILURE_RETRY(a) (a)
#endif

int mi_error=MI_OK;
char *mi_error_from_gdb=NULL;
static char *gdb_exe=NULL;
static char *xterm_exe=NULL;
static char *gdb_start=NULL;
static char *gdb_conn=NULL;
static char *main_func=NULL;
static char  disable_psym_search_workaround=0;

mi_h *mi_alloc_h()
{
 mi_h *h=(mi_h *)calloc(1,sizeof(mi_h));
 if (!h)
   {
    mi_error=MI_OUT_OF_MEMORY;
    return NULL;
   }
 h->to_gdb[0]=h->to_gdb[1]=h->from_gdb[0]=h->from_gdb[1]=-1;
 h->pid=-1;
 return h;
}

int mi_check_running_pid(pid_t pid)
{
 int status;

 if (pid<=0)
    return 0;
 /* If waitpid returns the number of our child means it communicated
    to as a termination status. */
 if (waitpid(pid,&status,WNOHANG)==pid)
   {
    pid=0;
    return 0;
   }
 return 1;
}

int mi_check_running(mi_h *h)
{
 return !h->died && mi_check_running_pid(h->pid);
}

void mi_kill_child(pid_t pid)
{
 kill(pid,SIGTERM);
 usleep(100000);
 if (mi_check_running_pid(pid))
   {
    int status;
    kill(pid,SIGKILL);
    waitpid(pid,&status,0);
   }
}

void mi_free_h(mi_h **handle)
{
 mi_h *h=*handle;
 if (h->to_gdb[0]>=0)
    close(h->to_gdb[0]);
 if (h->to)
    fclose(h->to);
 else if (h->to_gdb[1]>=0)
    close(h->to_gdb[1]);
 if (h->from)
    fclose(h->from);
 else if (h->from_gdb[0]>=0)
    close(h->from_gdb[0]);
 if (h->from_gdb[1]>=0)
    close(h->from_gdb[1]);
 if (mi_check_running(h))
   {/* GDB is running! */
    mi_kill_child(h->pid);
   }
 if (h->line)
    free(h->line);
 mi_free_output(h->po);
 free(h->catched_console);
 free(h);
 *handle=NULL;
}

void mi_set_nonblk(int h)
{
 int flf;
 flf=fcntl(h,F_GETFL,0);
 flf=flf | O_NONBLOCK;
 fcntl(h,F_SETFL,flf);
}

int mi_getline(mi_h *h)
{
 char c;

 while (read(h->from_gdb[0],&c,1)==1)
   {
    if (h->lread>=h->llen)
      {
       h->llen=h->lread+128;
       h->line=(char *)realloc(h->line,h->llen);
       if (!h->line)
         {
          h->llen=0;
          h->lread=0;
          return -1;
         }
      }
    if (c=='\n')
      {
       int ret=h->lread;
       h->line[ret]=0;
       h->lread=0;
       return ret;
      }
    h->line[h->lread]=c;
    h->lread++;
   }
 return 0;
}

char *get_cstr(mi_output *o)
{
 if (!o->c || o->c->type!=t_const)
    return NULL;
 return o->c->v.cstr;
}

int mi_get_response(mi_h *h)
{
 int l=mi_getline(h);
 if (!l)
    return 0;

 if (h->from_gdb_echo)
    h->from_gdb_echo(h->line,h->from_gdb_echo_data);
 if (strncmp(h->line,"(gdb)",5)==0)
   {/* End of response. */
    return 1;
   }
 else
   {/* Add to the response. */
    mi_output *o;
    int add=1, is_exit=0;
    o=mi_parse_gdb_output(h->line);

    if (!o)
       return 0;
    /* Tunneled streams callbacks. */
    if (o->type==MI_T_OUT_OF_BAND && o->stype==MI_ST_STREAM)
      {
       char *aux;
       add=0;
       switch (o->sstype)
         {
          case MI_SST_CONSOLE:
               aux=get_cstr(o);
               if (h->console)
                  h->console(aux,h->console_data);
               if (h->catch_console && aux)
                 {
                  h->catch_console--;
                  if (!h->catch_console)
                    {
                     free(h->catched_console);
                     h->catched_console=strdup(aux);
                    }
                 }
               break;
          case MI_SST_TARGET:
               /* This one seems to be useless. */
               if (h->target)
                  h->target(get_cstr(o),h->target_data);
               break;
          case MI_SST_LOG:
               if (h->log)
                  h->log(get_cstr(o),h->log_data);
               break;
         }
      }
    else if (o->type==MI_T_OUT_OF_BAND && o->stype==MI_ST_ASYNC)
      {
       if (h->async)
          h->async(o,h->async_data);
      }
    else if (o->type==MI_T_RESULT_RECORD && o->tclass==MI_CL_ERROR)
      {/* Error from gdb, record it. */
       mi_error=MI_FROM_GDB;
       free(mi_error_from_gdb);
       mi_error_from_gdb=NULL;
       if (o->c && strcmp(o->c->var,"msg")==0 && o->c->type==t_const)
          mi_error_from_gdb=strdup(o->c->v.cstr);
      }
    is_exit=(o->type==MI_T_RESULT_RECORD && o->tclass==MI_CL_EXIT);
    /* Add to the list of responses. */
    if (add)
      {
       if (h->last)
          h->last->next=o;
       else
          h->po=o;
       h->last=o;
      }
    else
       mi_free_output(o);
    /* Exit RR means gdb exited, we won't get a new prompt ;-) */
    if (is_exit)
       return 1;
   }

 return 0;
}

mi_output *mi_retire_response(mi_h *h)
{
 mi_output *ret=h->po;
 h->po=h->last=NULL;
 return ret;
}

mi_output *mi_get_response_blk(mi_h *h)
{
 int r;
 /* Sometimes gdb dies. */
 if (!mi_check_running(h))
   {
    h->died=1;
    mi_error=MI_GDB_DIED;
    return NULL;
   }
 do
   {
    if (1)
      {
       /*
        That's a must. If we just keep trying to read and failing things
        become really sloooowwww. Instead we try and if it fails we wait
        until something is available.
        TODO: Implement something with the time out, a callback to ask the
        application is we have to wait or not could be a good thing.
       */
       fd_set set;
       struct timeval timeout;
       int ret;

       r=mi_get_response(h);
       if (r)
          return mi_retire_response(h);

       FD_ZERO(&set);
       FD_SET(h->from_gdb[0],&set);
       timeout.tv_sec=h->time_out;
       timeout.tv_usec=0;
       ret=TEMP_FAILURE_RETRY(select(FD_SETSIZE,&set,NULL,NULL,&timeout));
       if (!ret)
         {
          if (!mi_check_running(h))
            {
             h->died=1;
             mi_error=MI_GDB_DIED;
             return NULL;
            }
          if (h->time_out_cb)
             ret=h->time_out_cb(h->time_out_cb_data);
          if (!ret)
            {
             mi_error=MI_GDB_TIME_OUT;
             return NULL;
            }
         }
      }
    else
      {
       r=mi_get_response(h);
       if (r)
          return mi_retire_response(h);
       else
          usleep(100);
      }
   }
 while (!r);

 return NULL;
}

void mi_send_commands(mi_h *h, const char *file)
{
 FILE *f;
 char b[PATH_MAX];

 //printf("File: %s\n",file);
 if (!file)
    return;
 f=fopen(file,"rt");
 if (!f)
    return;
 while (!feof(f))
   {
    if (fgets(b,PATH_MAX,f))
      {
       //printf("Send: %s\n",b);
       mi_send(h,b);
       mi_res_simple_done(h);
      }
   }
 fclose(f);
}

void mi_send_target_commands(mi_h *h)
{
 mi_send_commands(h,gdb_conn);
}

/**[txh]********************************************************************

  Description:
  Connect to a local copy of gdb. Note that the mi_h structure is something
similar to a "FILE *" for stdio.
  
  Return: A new mi_h structure or NULL on error.
  
***************************************************************************/

mi_h *mi_connect_local()
{
 mi_h *h;
 const char *gdb=mi_get_gdb_exe();

 /* Start without error. */
 mi_error=MI_OK;
 /* Verify we have a GDB binary. */
 if (access(gdb,X_OK))
   {
    mi_error=MI_MISSING_GDB;
    return NULL;
   }
 /* Alloc the handle structure. */
 h=mi_alloc_h();
 if (!h)
    return h;
 h->time_out=MI_DEFAULT_TIME_OUT;
 /* Create the pipes to connect with the child. */
 if (pipe(h->to_gdb) || pipe(h->from_gdb))
   {
    mi_error=MI_PIPE_CREATE;
    mi_free_h(&h);
    return NULL;
   }
 mi_set_nonblk(h->to_gdb[1]);
 mi_set_nonblk(h->from_gdb[0]);
 /* Associate streams to the file handles. */
 h->to=fdopen(h->to_gdb[1],"w");
 h->from=fdopen(h->from_gdb[0],"r");
 if (!h->to || !h->from)
   {
    mi_error=MI_PIPE_CREATE;
    mi_free_h(&h);
    return NULL;
   }
 /* Create the child. */
 h->pid=fork();
 if (h->pid==0)
   {/* We are the child. */
    const char *argv[5];
    /* Connect stdin/out to the pipes. */
    dup2(h->to_gdb[0],STDIN_FILENO);
    dup2(h->from_gdb[1],STDOUT_FILENO);
    /* Pass the control to gdb. */
    argv[0]=gdb; /* Is that OK? */
    argv[1]="--interpreter=mi";
    argv[2]="--quiet";
    argv[3]=disable_psym_search_workaround ? 0 : "--readnow";
    argv[4]=0;
    execvp(argv[0],(char * const *)argv);
    /* We get here only if exec failed. */
    _exit(127);
   }
 /* We are the parent. */
 if (h->pid==-1)
   {/* Fork failed. */
    mi_error=MI_FORK;
    mi_free_h(&h);
    return NULL;
   }
 if (!mi_check_running(h))
   {
    mi_error=MI_DEBUGGER_RUN;
    mi_free_h(&h);
    return NULL;
   }
 /* Wait for the prompt. */
 mi_get_response_blk(h);
 /* Send the start-up commands */
 mi_send_commands(h,gdb_start);

 return h;
}

/**[txh]********************************************************************

  Description:
  Close connection. You should ask gdb to quit first @x{gmi_gdb_exit}.
  
***************************************************************************/

void mi_disconnect(mi_h *h)
{
 mi_free_h(&h);
 free(mi_error_from_gdb);
 mi_error_from_gdb=NULL;
}

void mi_set_console_cb(mi_h *h, stream_cb cb, void *data)
{
 h->console=cb;
 h->console_data=data;
}

void mi_set_target_cb(mi_h *h, stream_cb cb, void *data)
{
 h->target=cb;
 h->target_data=data;
}

void mi_set_log_cb(mi_h *h, stream_cb cb, void *data)
{
 h->log=cb;
 h->log_data=data;
}

stream_cb mi_get_console_cb(mi_h *h, void **data)
{
 if (data)
    *data=h->console_data;
 return h->console;
}

stream_cb mi_get_target_cb(mi_h *h, void **data)
{
 if (data)
    *data=h->target_data;
 return h->target;
}

stream_cb mi_get_log_cb(mi_h *h, void **data)
{
 if (data)
    *data=h->log_data;
 return h->log;
}

void mi_set_async_cb(mi_h *h, async_cb cb, void *data)
{
 h->async=cb;
 h->async_data=data;
}

async_cb mi_get_async_cb(mi_h *h, void **data)
{
 if (data)
    *data=h->async_data;
 return h->async;
}

void mi_set_to_gdb_cb(mi_h *h, stream_cb cb, void *data)
{
 h->to_gdb_echo=cb;
 h->to_gdb_echo_data=data;
}

void mi_set_from_gdb_cb(mi_h *h, stream_cb cb, void *data)
{
 h->from_gdb_echo=cb;
 h->from_gdb_echo_data=data;
}

stream_cb mi_get_to_gdb_cb(mi_h *h, void **data)
{
 if (data)
    *data=h->to_gdb_echo_data;
 return h->to_gdb_echo;
}

stream_cb mi_get_from_gdb_cb(mi_h *h, void **data)
{
 if (data)
    *data=h->from_gdb_echo_data;
 return h->from_gdb_echo;
}

void mi_set_time_out_cb(mi_h *h, tm_cb cb, void *data)
{
 h->time_out_cb=cb;
 h->time_out_cb_data=data;
}

tm_cb mi_get_time_out_cb(mi_h *h, void **data)
{
 if (data)
    *data=h->time_out_cb_data;
 return h->time_out_cb;
}

void mi_set_time_out(mi_h *h, int to)
{
 h->time_out=to;
}

int mi_get_time_out(mi_h *h)
{
 return h->time_out;
}

int mi_send(mi_h *h, const char *format, ...)
{
 int ret;
 char *str;
 va_list argptr;

 if (h->died)
    return 0;

 va_start(argptr,format);
 ret=vasprintf(&str,format,argptr);
 va_end(argptr);
 fputs(str,h->to);
 fflush(h->to);
 if (h->to_gdb_echo)
    h->to_gdb_echo(str,h->to_gdb_echo_data);
 free(str);

 return ret;
}

void mi_clean_up_globals()
{
 free(gdb_exe);
 gdb_exe=NULL;
 free(xterm_exe);
 xterm_exe=NULL;
 free(gdb_start);
 gdb_start=NULL;
 free(gdb_conn);
 gdb_conn=NULL;
 free(main_func);
 main_func=NULL;
}

void mi_register_exit()
{
 static int registered=0;
 if (!registered)
   {
    registered=1;
    atexit(mi_clean_up_globals);
   }
}

void mi_set_gdb_exe(const char *name)
{
 free(gdb_exe);
 gdb_exe=name ? strdup(name) : NULL;
 mi_register_exit();
}

void mi_set_gdb_start(const char *name)
{
 free(gdb_start);
 gdb_start=name ? strdup(name) : NULL;
 mi_register_exit();
}

void mi_set_gdb_conn(const char *name)
{
 free(gdb_conn);
 gdb_conn=name ? strdup(name) : NULL;
 mi_register_exit();
}

static
char *mi_search_in_path(const char *file)
{
 char *path, *pt, *r;
 char test[PATH_MAX];
 struct stat st;

 path=getenv("PATH");
 if (!path)
    return NULL;
 pt=strdup(path);
 r=strtok(pt,":");
 while (r)
   {
    strcpy(test,r);
    strcat(test,"/");
    strcat(test,file);
    if (stat(test,&st)==0 && S_ISREG(st.st_mode))
      {
       free(pt);
       return strdup(test);
      }
    r=strtok(NULL,":");
   }
 free(pt);
 return NULL;
}

const char *mi_get_gdb_exe()
{
 if (!gdb_exe)
   {/* Look for gdb in path */
    gdb_exe=mi_search_in_path("gdb");
    if (!gdb_exe)
       return "/usr/bin/gdb";
   }
 return gdb_exe;
}

const char *mi_get_gdb_start()
{
 return gdb_start;
}

const char *mi_get_gdb_conn()
{
 return gdb_conn;
}

void mi_set_xterm_exe(const char *name)
{
 free(xterm_exe);
 xterm_exe=name ? strdup(name) : NULL;
 mi_register_exit();
}

const char *mi_get_xterm_exe()
{
 if (!xterm_exe)
   {/* Look for xterm in path */
    xterm_exe=mi_search_in_path("xterm");
    if (!xterm_exe)
       return "/usr/bin/X11/xterm";
   }
 return xterm_exe;
}

void mi_set_main_func(const char *name)
{
 free(main_func);
 main_func=name ? strdup(name) : NULL;
 mi_register_exit();
}

const char *mi_get_main_func()
{
 if (main_func)
    return main_func;
 return "main";
}

/**[txh]********************************************************************

  Description:
  Opens a new xterm to be used by the child process to debug.
  
  Return: A new mi_aux_term structure, you can use @x{gmi_end_aux_term} to
release it.
  
***************************************************************************/

mi_aux_term *gmi_start_xterm()
{
 char nsh[14]="/tmp/shXXXXXX";
 char ntt[14]="/tmp/ttXXXXXX";
 const char *xterm;
 struct stat st;
 int hsh, htt=-1;
 mi_aux_term *res=NULL;
 FILE *f;
 pid_t pid;
 char buf[PATH_MAX];

 /* Verify we have an X terminal. */
 xterm=mi_get_xterm_exe();
 if (access(xterm,X_OK))
   {
    mi_error=MI_MISSING_XTERM;
    return NULL;
   }

 /* Create 2 temporals. */
 hsh=mkstemp(nsh);
 if (hsh==-1)
   {
    mi_error=MI_CREATE_TEMPORAL;
    return NULL;
   }
 htt=mkstemp(ntt);
 if (htt==-1)
   {
    close(hsh);
    unlink(nsh);
    mi_error=MI_CREATE_TEMPORAL;
    return NULL;
   }
 close(htt);
 /* Create the script. */
 f=fdopen(hsh,"w");
 if (!f)
   {
    close(hsh);
    unlink(nsh);
    unlink(ntt);
    mi_error=MI_CREATE_TEMPORAL;
    return NULL;
   }
 fprintf(f,"#!/bin/sh\n");
 fprintf(f,"tty > %s\n",ntt);
 fprintf(f,"rm %s\n",nsh);
 fprintf(f,"sleep 365d\n");
 fclose(f);
 /* Spawn xterm. */
 /* Create the child. */
 pid=fork();
 if (pid==0)
   {/* We are the child. */
    char *argv[5];
    /* Pass the control to gdb. */
    argv[0]=const_cast<char *>(mi_get_xterm_exe()); /* Is that ok? */
    argv[1]=const_cast<char *>("-e");
    argv[2]=const_cast<char *>("bin/sh");
    argv[3]=nsh;
    argv[4]=0;
    execvp(argv[0],argv);
    /* We get here only if exec failed. */
    unlink(nsh);
    unlink(ntt);
    _exit(127);
   }
 /* We are the parent. */
 if (pid==-1)
   {/* Fork failed. */
    unlink(nsh);
    unlink(ntt);
    mi_error=MI_FORK;
    return NULL;
   }
 /* Wait until the shell is deleted. */
 while (stat(nsh,&st)==0)
   usleep(1000);
 /* Try to read the tty name. */
 f=fopen(ntt,"rt");
 if (f)
   {
    if (fgets(buf,PATH_MAX,f))
      {
       char *s; /* Strip the \n. */
       for (s=buf; *s && *s!='\n'; s++);
       *s=0;
       res=(mi_aux_term *)malloc(sizeof(mi_aux_term));
       if (res)
         {
          res->pid=pid;
          res->tty=strdup(buf);
         }
      }
    fclose(f);
   }
 unlink(ntt);
 return res;
}

void mi_free_aux_term(mi_aux_term *t)
{
 if (!t)
    return;
 free(t->tty);
 free(t);
}

/**[txh]********************************************************************

  Description:
  Closes the auxiliar terminal and releases the allocated memory.
  
***************************************************************************/

void gmi_end_aux_term(mi_aux_term *t)
{
 if (!t)
    return;
 if (t->pid!=-1 && mi_check_running_pid(t->pid))
    mi_kill_child(t->pid);
 mi_free_aux_term(t);
}

/**[txh]********************************************************************

  Description:
  Forces the MI version. Currently the library can't detect it so you must
force it manually. GDB 5.x implemented MI v1 and 6.x v2.
  
***************************************************************************/

void mi_force_version(mi_h *h, unsigned vMajor, unsigned vMiddle,
                      unsigned vMinor)
{
 h->version=MI_VERSION2U(vMajor,vMiddle,vMinor);
}

/**[txh]********************************************************************

  Description:
  Dis/Enables the @var{wa} workaround for a bug in gdb.

***************************************************************************/

void mi_set_workaround(unsigned wa, int enable)
{
 switch (wa)
   {
    case MI_PSYM_SEARCH:
         disable_psym_search_workaround=enable ? 0 : 1;
         break;
   }
}

/**[txh]********************************************************************

  Description:
  Finds if the @var{wa} workaround for a bug in gdb is enabled.
  
  Return: !=0 if enabled.
  
***************************************************************************/

int mi_get_workaround(unsigned wa)
{
 switch (wa)
   {
    case MI_PSYM_SEARCH:
         return disable_psym_search_workaround==0;
   }
 return 0;
}

