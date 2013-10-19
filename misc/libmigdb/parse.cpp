/**[txh]********************************************************************

  Copyright (c) 2004-2007 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Parser.
  Comments:
  Parses the output of gdb. It basically converts the text from gdb into a
tree (could be a complex one) that we can easily interpret using C code.
  
***************************************************************************/

#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "mi_gdb.h"

mi_results *mi_get_result(const char *str, const char **end);
int mi_get_value(mi_results *r, const char *str, const char **end);


/* GDB BUG!!!! I got:
^error,msg="Problem parsing arguments: data-evaluate-expression ""1+2"""
Afects gdb 2002-04-01-cvs and 6.1.1 for sure.
That's an heuristical workaround.
*/
static inline
int EndOfStr(const char *s)
{
 if (*s=='"')
   {
    s++;
    return !*s || *s==',' || *s==']' || *s=='}';
   }
 return 0;
}

int mi_get_cstring_r(mi_results *r, const char *str, const char **end)
{
 const char *s;
 char *d;
 int len;

 if (*str!='"')
   {
    mi_error=MI_PARSER;
    return 0;
   }
 str++;
 /* Meassure. */
 for (s=str, len=0; *s && !EndOfStr(s); s++)
    {
     if (*s=='\\')
       {
        if (!*s)
          {
           mi_error=MI_PARSER;
           return 0;
          }
        s++;
       }
     len++;
    }
 /* Copy. */
 r->type=t_const;
 d=r->v.cstr=mi_malloc(len+1);
 if (!r->v.cstr)
    return 0;
 for (s=str; *s && !EndOfStr(s); s++, d++)
    {
     if (*s=='\\')
       {
        s++;
        switch (*s)
          {
           case 'n':
                *d='\n';
                break;
           case 't':
                *d='\t';
                break;
           default:
                *d=*s;
          }
       }
     else
        *d=*s;
    }
 *d=0;
 if (end)
    *end=s+1;

 return 1;
}

/* TODO: What's a valid variable name?
   I'll assume a-zA-Z0-9_- */
inline
int mi_is_var_name_char(char c)
{
 return isalnum(c) || c=='-' || c=='_';
}

char *mi_get_var_name(const char *str, const char **end)
{
 const char *s;
 char *r;
 int l;
 /* Meassure. */
 for (s=str; *s && mi_is_var_name_char(*s); s++);
 if (*s!='=')
   {
    mi_error=MI_PARSER;
    return NULL;
   }
 /* Allocate. */
 l=s-str;
 r=mi_malloc(l+1);
 /* Copy. */
 memcpy(r,str,l);
 r[l]=0;
 if (end)
    *end=s+1;
 return r;
}


int mi_get_list_res(mi_results *r, const char *str, const char **end, char closeC)
{
 mi_results *last_r, *rs;

 last_r=NULL;
 do
   {
    rs=mi_get_result(str,&str);
    if (last_r)
       last_r->next=rs;
    else
       r->v.rs=rs;
    last_r=rs;
    if (*str==closeC)
      {
       *end=str+1;
       return 1;
      }
    if (*str!=',')
       break;
    str++;
   }
 while (1);

 mi_error=MI_PARSER;
 return 0;
}

#ifdef __APPLE__
int mi_get_tuple_val(mi_results *r, const char *str, const char **end)
{
 mi_results *last_r, *rs;

 last_r=NULL;
 do
   {
    rs=mi_alloc_results();
    if (!rs || !mi_get_value(rs,str,&str))
      {
       mi_free_results(rs);
       return 0;
      }
    /* Note that rs->var is NULL, that indicates that's just a value and not
       a result. */
    if (last_r)
       last_r->next=rs;
    else
       r->v.rs=rs;
    last_r=rs;
    if (*str=='}')
      {
       *end=str+1;
       return 1;
      }
    if (*str!=',')
       break;
    str++;
   }
 while (1);

 mi_error=MI_PARSER;
 return 0;
}
#endif /* __APPLE__ */

int mi_get_tuple(mi_results *r, const char *str, const char **end)
{
 if (*str!='{')
   {
    mi_error=MI_PARSER;
    return 0;
   }
 r->type=t_tuple;
 str++;
 if (*str=='}')
   {/* Special case: empty tuple */
    *end=str+1;
    return 1;
   }
 #ifdef __APPLE__
 if (mi_is_var_name_char(*str))
    return mi_get_list_res(r,str,end,'}');
 return mi_get_tuple_val(r,str,end);
 #else /* __APPLE__ */
 return mi_get_list_res(r,str,end,'}');
 #endif /* __APPLE__ */
}

int mi_get_list_val(mi_results *r, const char *str, const char **end)
{
 mi_results *last_r, *rs;

 last_r=NULL;
 do
   {
    rs=mi_alloc_results();
    if (!rs || !mi_get_value(rs,str,&str))
      {
       mi_free_results(rs);
       return 0;
      }
    /* Note that rs->var is NULL, that indicates that's just a value and not
       a result. */
    if (last_r)
       last_r->next=rs;
    else
       r->v.rs=rs;
    last_r=rs;
    if (*str==']')
      {
       *end=str+1;
       return 1;
      }
    if (*str!=',')
       break;
    str++;
   }
 while (1);

 mi_error=MI_PARSER;
 return 0;
}

int mi_get_list(mi_results *r, const char *str, const char **end)
{
 if (*str!='[')
   {
    mi_error=MI_PARSER;
    return 0;
   }
 r->type=t_list;
 str++;
 if (*str==']')
   {/* Special case: empty list */
    *end=str+1;
    return 1;
   }
 /* Comment: I think they could choose () for values. Is confusing in this way. */
 if (mi_is_var_name_char(*str))
    return mi_get_list_res(r,str,end,']');
 return mi_get_list_val(r,str,end);
}

int mi_get_value(mi_results *r, const char *str, const char **end)
{
 switch (str[0])
   {
    case '"':
         return mi_get_cstring_r(r,str,end);
    case '{':
         return mi_get_tuple(r,str,end);
    case '[':
         return mi_get_list(r,str,end);
   }
 mi_error=MI_PARSER;
 return 0;
}

mi_results *mi_get_result(const char *str, const char **end)
{
 char *var;
 mi_results *r;

 var=mi_get_var_name(str,&str);
 if (!var)
    return NULL;

 r=mi_alloc_results();
 if (!r)
   {
    free(var);
    return NULL;
   }
 r->var=var;

 if (!mi_get_value(r,str,end))
   {
    mi_free_results(r);
    return NULL;
   }

 return r;
}

mi_output *mi_get_results_alone(mi_output *r,const char *str)
{
 mi_results *last_r, *rs;

 /* * results */
 last_r=NULL;
 do
   {
    if (!*str)
       return r;
    if (*str!=',')
      {
       mi_error=MI_PARSER;
       break;
      }
    str++;
    rs=mi_get_result(str,&str);
    if (!rs)
       break;
    if (!last_r)
       r->c=rs;
    else
       last_r->next=rs;
    last_r=rs;
   }
 while (1);
 mi_free_output(r);
 return NULL;
}

mi_output *mi_parse_result_record(mi_output *r,const char *str)
{
 r->type=MI_T_RESULT_RECORD;

 /* Solve the result-class. */
 if (strncmp(str,"done",4)==0)
   {
    str+=4;
    r->tclass=MI_CL_DONE;
   }
 else if (strncmp(str,"running",7)==0)
   {
    str+=7;
    r->tclass=MI_CL_RUNNING;
   }
 else if (strncmp(str,"connected",9)==0)
   {
    str+=9;
    r->tclass=MI_CL_CONNECTED;
   }
 else if (strncmp(str,"error",5)==0)
   {
    str+=5;
    r->tclass=MI_CL_ERROR;
   }
 else if (strncmp(str,"exit",4)==0)
   {
    str+=4;
    r->tclass=MI_CL_EXIT;
   }
 else
   {
    mi_error=MI_UNKNOWN_RESULT;
    return NULL;
   }

 return mi_get_results_alone(r,str);
}

mi_output *mi_parse_asyn(mi_output *r,const char *str)
{
 r->type=MI_T_OUT_OF_BAND;
 r->stype=MI_ST_ASYNC;
 /* async-class. */
 if (strncmp(str,"stopped",7)==0)
   {
    r->tclass=MI_CL_STOPPED;
    str+=7;
    return mi_get_results_alone(r,str);
   }
 if (strncmp(str,"download",8)==0)
   {
    r->tclass=MI_CL_DOWNLOAD;
    str+=8;
    return mi_get_results_alone(r,str);
   }
 mi_error=MI_UNKNOWN_ASYNC;
 mi_free_output(r);
 return NULL;
}

mi_output *mi_parse_exec_asyn(mi_output *r,const char *str)
{
 r->sstype=MI_SST_EXEC;
 return mi_parse_asyn(r,str);
}

mi_output *mi_parse_status_asyn(mi_output *r,const char *str)
{
 r->sstype=MI_SST_STATUS;
 return mi_parse_asyn(r,str);
}

mi_output *mi_parse_notify_asyn(mi_output *r,const char *str)
{
 r->sstype=MI_SST_NOTIFY;
 return mi_parse_asyn(r,str);
}

mi_output *mi_console(mi_output *r,const char *str)
{
 r->type=MI_T_OUT_OF_BAND;
 r->stype=MI_ST_STREAM;
 r->c=mi_alloc_results();
 if (!r->c || !mi_get_cstring_r(r->c,str,NULL))
   {
    mi_free_output(r);
    return NULL;
   }
 return r;
}

mi_output *mi_console_stream(mi_output *r,const char *str)
{
 r->sstype=MI_SST_CONSOLE;
 return mi_console(r,str);
}

mi_output *mi_target_stream(mi_output *r,const char *str)
{
 r->sstype=MI_SST_TARGET;
 return mi_console(r,str);
}

mi_output *mi_log_stream(mi_output *r,const char *str)
{
 r->sstype=MI_SST_LOG;
 return mi_console(r,str);
}

mi_output *mi_parse_gdb_output(const char *str)
{
 char type=str[0];

 mi_output *r=mi_alloc_output();
 if (!r)
   {
    mi_error=MI_OUT_OF_MEMORY;
    return NULL;
   }
 str++;
 switch (type)
   {
    case '^':
         return mi_parse_result_record(r,str);
    case '*':
         return mi_parse_exec_asyn(r,str);
    case '+':
         return mi_parse_status_asyn(r,str);
    case '=':
         return mi_parse_notify_asyn(r,str);
    case '~':
         return mi_console_stream(r,str);
    case '@':
         return mi_target_stream(r,str);
    case '&':
         return mi_log_stream(r,str);
   }   
 mi_error=MI_PARSER;
 return NULL;
}

mi_output *mi_get_rrecord(mi_output *r)
{
 if (!r)
    return NULL;
 while (r)
   {
    if (r->type==MI_T_RESULT_RECORD)
       return r;
    r=r->next;
   }
 return r;
}

mi_results *mi_get_var_r(mi_results *r, const char *var)
{
 while (r)
   {
    if (strcmp(r->var,var)==0)
       return r;
    r=r->next;
   }
 return NULL;
}

mi_results *mi_get_var(mi_output *res, const char *var)
{
 if (!res)
    return NULL;
 return mi_get_var_r(res->c,var);
}

int mi_get_async_stop_reason(mi_output *r, char **reason)
{
 int found_stopped=0;

 *reason=NULL;
 while (r)
   {
    if (r->type==MI_T_RESULT_RECORD && r->tclass==MI_CL_ERROR)
      {
       if (r->c->type==t_const)
          *reason=r->c->v.cstr;
       return 0;
      }
    if (r->type==MI_T_OUT_OF_BAND && r->stype==MI_ST_ASYNC &&
        r->sstype==MI_SST_EXEC && r->tclass==MI_CL_STOPPED)
      {
       mi_results *p=r->c;
       found_stopped=1;
       while (p)
         {
          if (strcmp(p->var,"reason")==0)
            {
             *reason=p->v.cstr;
             return 1;
            }
          p=p->next;
         }
      }
    r=r->next;
   }
 if (*reason==NULL && found_stopped)
   {
    *reason=strdup("unknown (temp bkpt?)");
    return 1;
   }
 return 0;
}

mi_frames *mi_get_async_frame(mi_output *r)
{
 while (r)
   {
    if (r->type==MI_T_OUT_OF_BAND && r->stype==MI_ST_ASYNC &&
        r->sstype==MI_SST_EXEC && r->tclass==MI_CL_STOPPED)
      {
       mi_results *p=r->c;
       while (p)
         {
          if (strcmp(p->var,"frame")==0)
             return mi_parse_frame(p->v.rs);
          p=p->next;
         }
      }
    r=r->next;
   }
 return NULL;
}

int mi_res_simple(mi_h *h, int tclass, int accert_ret)
{
 mi_output *r, *res;
 int ret=0;

 r=mi_get_response_blk(h);
 res=mi_get_rrecord(r);

 if (res)
    ret=res->tclass==tclass;
 mi_free_output(r);

 return ret;
}


int mi_res_simple_done(mi_h *h)
{
 return mi_res_simple(h,MI_CL_DONE,0);
}

int mi_res_simple_exit(mi_h *h)
{
 return mi_res_simple(h,MI_CL_EXIT,1);
}

int mi_res_simple_running(mi_h *h)
{
 return mi_res_simple(h,MI_CL_RUNNING,0);
}

int mi_res_simple_connected(mi_h *h)
{
 return mi_res_simple(h,MI_CL_CONNECTED,0);
}

mi_results *mi_res_var(mi_h *h, const char *var, int tclass)
{
 mi_output *r, *res;
 mi_results *the_var=NULL;

 r=mi_get_response_blk(h);
 /* All the code that follows is "NULL" tolerant. */
 /* Look for the result-record. */
 res=mi_get_rrecord(r);
 /* Look for the desired var. */
 if (res && res->tclass==tclass)
    the_var=mi_get_var(res,var);
 /* Release all but the one we want. */
 mi_free_output_but(r,NULL,the_var);
 return the_var;
}

mi_results *mi_res_done_var(mi_h *h, const char *var)
{
 return mi_res_var(h,var,MI_CL_DONE);
}

mi_frames *mi_parse_frame(mi_results *c)
{
 mi_frames *res=mi_alloc_frames();
 char *end;

 if (res)
   {
    while (c)
      {
       if (c->type==t_const)
         {
          if (strcmp(c->var,"level")==0)
             res->level=atoi(c->v.cstr);
          else if (strcmp(c->var,"addr")==0)
             res->addr=(void *)strtoul(c->v.cstr,&end,0);
          else if (strcmp(c->var,"func")==0)
            {
             res->func=c->v.cstr;
             c->v.cstr=NULL;
            }
          else if (strcmp(c->var,"file")==0)
            {
             res->file=c->v.cstr;
             c->v.cstr=NULL;
            }
          else if (strcmp(c->var,"from")==0)
            {
             res->from=c->v.cstr;
             c->v.cstr=NULL;
            }
          else if (strcmp(c->var,"line")==0)
             res->line=atoi(c->v.cstr);
         }
       else if (c->type==t_list && strcmp(c->var,"args")==0)
         {
          res->args=c->v.rs;
          c->v.rs=NULL;
         }
       c=c->next;
      }
   }
 return res;
}

mi_frames *mi_res_frame(mi_h *h)
{
 mi_results *r=mi_res_done_var(h,"frame");
 mi_frames *f=NULL;

 if (r && r->type==t_tuple)
    f=mi_parse_frame(r->v.rs);
 mi_free_results(r);
 return f;
}

mi_frames *mi_res_frames_array(mi_h *h, const char *var)
{
 mi_results *r=mi_res_done_var(h,var), *c;
 mi_frames *res=NULL, *nframe, *last=NULL;

 if (!r)
    return NULL;
#ifdef __APPLE__
 if (r->type!=t_list && r->type!=t_tuple)
#else
 if (r->type!=t_list)
#endif
   {
    mi_free_results(r);
    return NULL;
   }
 c=r->v.rs;
 while (c)
   {
    if (strcmp(c->var,"frame")==0 && c->type==t_tuple)
      {
       nframe=mi_parse_frame(c->v.rs);
       if (nframe)
         {
          if (!last)
             res=nframe;
          else
             last->next=nframe;
          last=nframe;
         }
      }
    c=c->next;
   }
 mi_free_results(r);
 return res;
}

mi_frames *mi_res_frames_list(mi_h *h)
{
 mi_output *r, *res;
 mi_frames *ret=NULL, *nframe, *last=NULL;
 mi_results *c;

 r=mi_get_response_blk(h);
 res=mi_get_rrecord(r);
 if (res && res->tclass==MI_CL_DONE)
   {
    c=res->c;
    while (c)
      {
       if (strcmp(c->var,"frame")==0 && c->type==t_tuple)
         {
          nframe=mi_parse_frame(c->v.rs);
          if (nframe)
            {
             if (!last)
                ret=nframe;
             else
                last->next=nframe;
             last=nframe;
            }
         }
       c=c->next;
      }
   }
 mi_free_output(r);
 return ret;
}

int mi_get_thread_ids(mi_output *res, int **list)
{
 mi_results *vids, *lids;
 int ids=-1, i;

 *list=NULL;
 vids=mi_get_var(res,"number-of-threads");
 lids=mi_get_var(res,"thread-ids");
 if (vids && vids->type==t_const &&
     lids && lids->type==t_tuple)
   {
    ids=atoi(vids->v.cstr);
    if (ids)
      {
       int *lst;
       lst=(int *)mi_calloc(ids,sizeof(int));
       if (lst)
         {
          lids=lids->v.rs;
          i=0;
          while (lids)
            {
             if (strcmp(lids->var,"thread-id")==0 && lids->type==t_const)
                lst[i++]=atoi(lids->v.cstr);
             lids=lids->next;
            }
          *list=lst;
         }
       else
          ids=-1;
      }
   }
 return ids;
}

int mi_res_thread_ids(mi_h *h, int **list)
{
 mi_output *r, *res;
 int ids=-1;

 r=mi_get_response_blk(h);
 res=mi_get_rrecord(r);
 if (res && res->tclass==MI_CL_DONE)
    ids=mi_get_thread_ids(res,list);
 mi_free_output(r);
 return ids;
}

enum mi_gvar_lang mi_lang_str_to_enum(const char *lang)
{
 enum mi_gvar_lang lg=lg_unknown;

 if (strcmp(lang,"C")==0)
    lg=lg_c;
 else if (strcmp(lang,"C++")==0)
    lg=lg_cpp;
 else if (strcmp(lang,"Java")==0)
    lg=lg_java;

 return lg;
}

const char *mi_lang_enum_to_str(enum mi_gvar_lang lang)
{
 const char *lg;

 switch (lang)
   {
    case lg_c:
         lg="C";
         break;
    case lg_cpp:
         lg="C++";
         break;
    case lg_java:
         lg="Java";
         break;
    /*case lg_unknown:*/
    default:
         lg="unknown";
         break;
   }
 return lg;
}

enum mi_gvar_fmt mi_format_str_to_enum(const char *format)
{
 enum mi_gvar_fmt fmt=fm_natural;

 if (strcmp(format,"binary")==0)
    fmt=fm_binary;
 else if (strcmp(format,"decimal")==0)
    fmt=fm_decimal;
 else if (strcmp(format,"hexadecimal")==0)
    fmt=fm_hexadecimal;
 else if (strcmp(format,"octal")==0)
    fmt=fm_octal;

 return fmt;
}

const char *mi_format_enum_to_str(enum mi_gvar_fmt format)
{
 const char *fmt;

 switch (format)
   {
    case fm_natural:
         fmt="natural";
         break;
    case fm_binary:
         fmt="binary";
         break;
    case fm_decimal:
         fmt="decimal";
         break;
    case fm_hexadecimal:
         fmt="hexadecimal";
         break;
    case fm_octal:
         fmt="octal";
         break;
    case fm_raw:
         fmt="raw";
         break;
    default:
         fmt="unknown";
   }
 return fmt;
}

char mi_format_enum_to_char(enum mi_gvar_fmt format)
{
 char fmt;

 switch (format)
   {
    case fm_natural:
         fmt='N';
         break;
    case fm_binary:
         fmt='t';
         break;
    case fm_decimal:
         fmt='d';
         break;
    case fm_hexadecimal:
         fmt='x';
         break;
    case fm_octal:
         fmt='o';
         break;
    case fm_raw:
         fmt='r';
         break;
    default:
         fmt=' ';
   }
 return fmt;
}

mi_gvar *mi_get_gvar(mi_output *o, mi_gvar *cur, const char *expression)
{
 mi_results *r;
 mi_gvar *res=cur ? cur : mi_alloc_gvar();
 int l;

 if (!res)
    return res;
 r=o->c;
 if (expression)
    res->exp=strdup(expression);
 while (r)
   {
    if (r->type==t_const)
      {
       if (strcmp(r->var,"name")==0)
         {
          free(res->name);
          res->name=r->v.cstr;
          r->v.cstr=NULL;
         }
       else if (strcmp(r->var,"numchild")==0)
         {
          res->numchild=atoi(r->v.cstr);
         }
       else if (strcmp(r->var,"type")==0)
         {
          free(res->type);
          res->type=r->v.cstr;
          r->v.cstr=NULL;
          l=strlen(res->type);
          if (l && res->type[l-1]=='*')
             res->ispointer=1;
         }
       else if (strcmp(r->var,"lang")==0)
         {
          res->lang=mi_lang_str_to_enum(r->v.cstr);
         }
       else if (strcmp(r->var,"exp")==0)
         {
          free(res->exp);
          res->exp=r->v.cstr;
          r->v.cstr=NULL;
         }
       else if (strcmp(r->var,"format")==0)
         {
          res->format=mi_format_str_to_enum(r->v.cstr);
         }
       else if (strcmp(r->var,"attr")==0)
         { /* Note: gdb 6.1.1 have only this: */
          if (strcmp(r->v.cstr,"editable")==0)
             res->attr=MI_ATTR_EDITABLE;
          else /* noneditable */
             res->attr=MI_ATTR_NONEDITABLE;
         }
      }
    r=r->next;
   }
 return res;
}

mi_gvar *mi_res_gvar(mi_h *h, mi_gvar *cur, const char *expression)
{
 mi_output *r, *res;
 mi_gvar *gvar=NULL;

 r=mi_get_response_blk(h);
 res=mi_get_rrecord(r);
 if (res && res->tclass==MI_CL_DONE)
    gvar=mi_get_gvar(res,cur,expression);
 mi_free_output(r);
 return gvar;
}

mi_gvar_chg *mi_get_gvar_chg(mi_results *r)
{
 mi_gvar_chg *n;

 if (r->type!=t_const)
    return NULL;
 n=mi_alloc_gvar_chg();
 if (n)
   {
    while (r)
      {
       if (r->type==t_const)
         {
          if (strcmp(r->var,"name")==0)
            {
             n->name=r->v.cstr;
             r->v.cstr=NULL;
            }
          else if (strcmp(r->var,"in_scope")==0)
            {
             n->in_scope=strcmp(r->v.cstr,"true")==0;
            }
          else if (strcmp(r->var,"new_type")==0)
            {
             n->new_type=r->v.cstr;
             r->v.cstr=NULL;
            }
          else if (strcmp(r->var,"new_num_children")==0)
            {
             n->new_num_children=atoi(r->v.cstr);
            }
          // type_changed="false" is the default
         }
       r=r->next;
      }
   }
 return n;
}

int mi_res_changelist(mi_h *h, mi_gvar_chg **changed)
{
 mi_gvar_chg *last, *n;
 mi_results *res=mi_res_done_var(h,"changelist"), *r;
 int count=0;

 *changed=NULL;
 if (!res)
    return 0;
 last=NULL;
 count=1;
 n=NULL;
 r=res->v.rs;

 if (res->type==t_list)
   {// MI v2 a list of tuples
    while (r)
      {
       if (r->type==t_tuple)
         {
          n=mi_get_gvar_chg(r->v.rs);
          if (n)
            {
             if (last)
                last->next=n;
             else
                *changed=n;
             last=n;
             count++;
            }
         }
       r=r->next;
      }
   }
 else if (res->type==t_tuple)
   {// MI v1 a tuple with all together *8-P
    while (r)
      {
       if (r->type==t_const) /* Just in case. */
         {/* Get one var. */
          if (strcmp(r->var,"name")==0)
            {
             if (n)
               {/* Add to the list*/
                if (last)
                   last->next=n;
                else
                   *changed=n;
                last=n;
                count++;
               }
             n=mi_alloc_gvar_chg();
             if (!n)
               {
                mi_free_gvar_chg(*changed);
                return 0;
               }
             n->name=r->v.cstr;
             r->v.cstr=NULL;
            }
          else if (strcmp(r->var,"in_scope")==0)
            {
             n->in_scope=strcmp(r->v.cstr,"true")==0;
            }
          else if (strcmp(r->var,"new_type")==0)
            {
             n->new_type=r->v.cstr;
             r->v.cstr=NULL;
            }
          else if (strcmp(r->var,"new_num_children")==0)
            {
             n->new_num_children=atoi(r->v.cstr);
            }
          // type_changed="false" is the default
         }
       r=r->next;
      }
    if (n)
      {/* Add to the list*/
       if (last)
          last->next=n;
       else
          *changed=n;
       last=n;
       count++;
      }
   }
 mi_free_results(res);

 return count;
}

int mi_get_children(mi_results *ch, mi_gvar *v)
{
 mi_gvar *cur=NULL, *aux;
 int i=0, count=v->numchild, l;

 while (ch)
   {
    if (strcmp(ch->var,"child")==0 && ch->type==t_tuple && i<count)
      {
       mi_results *r=ch->v.rs;
       aux=mi_alloc_gvar();
       if (!aux)
          return 0;
       if (!v->child)
          v->child=aux;
       else
          cur->next=aux;
       cur=aux;
       cur->parent=v;
       cur->depth=v->depth+1;

       while (r)
         {
          if (r->type==t_const)
            {
             if (strcmp(r->var,"name")==0)
               {
                cur->name=r->v.cstr;
                r->v.cstr=NULL;
               }
             else if (strcmp(r->var,"exp")==0)
               {
                cur->exp=r->v.cstr;
                r->v.cstr=NULL;
               }
             else if (strcmp(r->var,"type")==0)
               {
                cur->type=r->v.cstr;
                r->v.cstr=NULL;
                l=strlen(cur->type);
                if (l && cur->type[l-1]=='*')
                   cur->ispointer=1;
               }
             else if (strcmp(r->var,"value")==0)
               {
                cur->value=r->v.cstr;
                r->v.cstr=NULL;
               }                     
             else if (strcmp(r->var,"numchild")==0)
               {
                cur->numchild=atoi(r->v.cstr);
               }
            }
          r=r->next;
         }
       i++;
      }
    ch=ch->next;
   }
 v->vischild=i;
 v->opened=1;
 return i==v->numchild;
}

int mi_res_children(mi_h *h, mi_gvar *v)
{
 mi_output *r, *res;
 int ok=0;

 r=mi_get_response_blk(h);
 res=mi_get_rrecord(r);
 if (res && res->tclass==MI_CL_DONE)
   {
    mi_results *num=mi_get_var(res,"numchild");
    if (num && num->type==t_const)
      {
       v->numchild=atoi(num->v.cstr);
       if (v->child)
         {
          mi_free_gvar(v->child);
          v->child=NULL;
         }
       if (v->numchild)
         {
          mi_results *ch =mi_get_var(res,"children");
          if (ch && ch->type!=t_const) /* MI v1 tuple, MI v2 list */
             ok=mi_get_children(ch->v.rs,v);
         }
       else
          ok=1;
      }
   }
 mi_free_output(r);
 return ok;
}

mi_bkpt *mi_get_bkpt(mi_results *p)
{
 mi_bkpt *res;
 char *end;

 res=mi_alloc_bkpt();
 if (!res)
    return NULL;
 while (p)
   {
    if (p->type==t_const && p->var)
      {
       if (strcmp(p->var,"number")==0)
          res->number=atoi(p->v.cstr);
       else if (strcmp(p->var,"type")==0)
         {
          if (strcmp(p->v.cstr,"breakpoint")==0)
             res->type=t_breakpoint;
          else
             res->type=t_unknown;
         }
       else if (strcmp(p->var,"disp")==0)
         {
          if (strcmp(p->v.cstr,"keep")==0)
             res->disp=d_keep;
          else if (strcmp(p->v.cstr,"del")==0)
             res->disp=d_del;
          else
             res->disp=d_unknown;
         }
       else if (strcmp(p->var,"enabled")==0)
          res->enabled=p->v.cstr[0]=='y';
       else if (strcmp(p->var,"addr")==0)
          res->addr=(void *)strtoul(p->v.cstr,&end,0);
       else if (strcmp(p->var,"func")==0)
         {
          res->func=p->v.cstr;
          p->v.cstr=NULL;
         }
       else if (strcmp(p->var,"file")==0)
         {
          res->file=p->v.cstr;
          p->v.cstr=NULL;
         }
       else if (strcmp(p->var,"line")==0)
          res->line=atoi(p->v.cstr);
       else if (strcmp(p->var,"times")==0)
          res->times=atoi(p->v.cstr);
       else if (strcmp(p->var,"ignore")==0)
          res->ignore=atoi(p->v.cstr);
       else if (strcmp(p->var,"cond")==0)
         {
          res->cond=p->v.cstr;
          p->v.cstr=NULL;
         }
      }
    p=p->next;
   }
 return res;
}

mi_bkpt *mi_res_bkpt(mi_h *h)
{
 mi_results *r=mi_res_done_var(h,"bkpt");
 mi_bkpt *b=NULL;

 if (r && r->type==t_tuple)
    b=mi_get_bkpt(r->v.rs);
 mi_free_results(r);
 return b;
}

mi_wp *mi_get_wp(mi_results *p, enum mi_wp_mode m)
{
 mi_wp *res=mi_alloc_wp();

 if (res)
   {
    res->mode=m;
    while (p)
      {
       if (p->type==t_const && p->var)
         {
          if (strcmp(p->var,"number")==0)
            {
             res->number=atoi(p->v.cstr);
             res->enabled=1;
            }
          else if (strcmp(p->var,"exp")==0)
            {
             res->exp=p->v.cstr;
             p->v.cstr=NULL;
            }
         }
       p=p->next;
      }
   }
 return res;
}

mi_wp *mi_parse_wp_res(mi_output *r)
{
 mi_results *p;
 enum mi_wp_mode m=wm_unknown;

 /* The info is in a result wpt=... */
 p=r->c;
 while (p)
   {
    if (p->var)
      {
       if (strcmp(p->var,"wpt")==0)
          m=wm_write;
       else if (strcmp(p->var,"hw-rwpt")==0)
          m=wm_read;
       else if (strcmp(p->var,"hw-awpt")==0)
          m=wm_rw;
       if (m!=wm_unknown)
          break;
      }
    p=p->next;
   }
 if (!p || p->type!=t_tuple)
    return NULL;
 /* Scan the values inside it. */
 return mi_get_wp(p->v.rs,m);
}

mi_wp *mi_res_wp(mi_h *h)
{
 mi_output *r, *res;
 mi_wp *ret=NULL;

 r=mi_get_response_blk(h);
 res=mi_get_rrecord(r);

 if (res)
    ret=mi_parse_wp_res(res);

 mi_free_output(r);
 return ret;
}

char *mi_res_value(mi_h *h)
{
 mi_results *r=mi_res_done_var(h,"value");
 char *s=NULL;

 if (r && r->type==t_const)
   {
    s=r->v.cstr;
    r->v.rs=NULL;
   }
 mi_free_results(r);
 return s;
}

mi_output *mi_get_stop_record(mi_output *r)
{
 while (r)
   {
    if (r->type==MI_T_OUT_OF_BAND && r->stype==MI_ST_ASYNC &&
        r->sstype==MI_SST_EXEC && r->tclass==MI_CL_STOPPED)
       return r;
    r=r->next;
   }
 return r;
}

static
char *reason_names[]=
{
 "breakpoint-hit",
 "watchpoint-trigger",
 "read-watchpoint-trigger",
 "access-watchpoint-trigger",
 "watchpoint-scope",
 "function-finished",
 "location-reached",
 "end-stepping-range",
 "exited-signalled",
 "exited",
 "exited-normally",
 "signal-received"
};

static
enum mi_stop_reason reason_values[]=
{
 sr_bkpt_hit,
 sr_wp_trigger, sr_read_wp_trigger, sr_access_wp_trigger, sr_wp_scope,
 sr_function_finished, sr_location_reached, sr_end_stepping_range,
 sr_exited_signalled, sr_exited, sr_exited_normally,
 sr_signal_received
};

static
char *reason_expl[]=
{
 "Hit a breakpoint",
 "Write watchpoint",
 "Read watchpoint",
 "Access watchpoint",
 "Watchpoint out of scope",
 "Function finished",
 "Location reached",
 "End of stepping",
 "Exited signalled",
 "Exited with error",
 "Exited normally",
 "Signal received"
};

enum mi_stop_reason mi_reason_str_to_enum(const char *s)
{
 int i;

 for (i=0; i<sizeof(reason_names)/sizeof(char *); i++)
     if (strcmp(reason_names[i],s)==0)
        return reason_values[i];
 return sr_unknown;
}

const char *mi_reason_enum_to_str(enum mi_stop_reason r)
{
 int i;

 if (r==sr_unknown)
    return "Unknown (temp bkp?)";
 for (i=0; i<sizeof(reason_values)/sizeof(char *); i++)
     if (reason_values[i]==r)
        return reason_expl[i];
 return NULL;
}

mi_stop *mi_get_stopped(mi_results *r)
{
 mi_stop *res=mi_alloc_stop();

 if (res)
   {
    while (r)
      {
       if (r->type==t_const)
         {
          if (strcmp(r->var,"reason")==0)
             res->reason=mi_reason_str_to_enum(r->v.cstr);
          else if (!res->have_thread_id && strcmp(r->var,"thread-id")==0)
            {
             res->have_thread_id=1;
             res->thread_id=atoi(r->v.cstr);
            }
          else if (!res->have_bkptno && strcmp(r->var,"bkptno")==0)
            {
             res->have_bkptno=1;
             res->bkptno=atoi(r->v.cstr);
            }
          else if (!res->have_bkptno && strcmp(r->var,"wpnum")==0)
            {
             res->have_wpno=1;
             res->wpno=atoi(r->v.cstr);
            }
          else if (strcmp(r->var,"gdb-result-var")==0)
            {
             res->gdb_result_var=r->v.cstr;
             r->v.cstr=NULL;
            }
          else if (strcmp(r->var,"return-value")==0)
            {
             res->return_value=r->v.cstr;
             r->v.cstr=NULL;
            }
          else if (strcmp(r->var,"signal-name")==0)
            {
             res->signal_name=r->v.cstr;
             r->v.cstr=NULL;
            }
          else if (strcmp(r->var,"signal-meaning")==0)
            {
             res->signal_meaning=r->v.cstr;
             r->v.cstr=NULL;
            }
          else if (!res->have_exit_code && strcmp(r->var,"exit-code")==0)
            {
             res->have_exit_code=1;
             res->exit_code=atoi(r->v.cstr);
            }
         }
       else // tuple or list
         {
          if (strcmp(r->var,"frame")==0)
             res->frame=mi_parse_frame(r->v.rs);
          else if (!res->wp && strcmp(r->var,"wpt")==0)
             res->wp=mi_get_wp(r->v.rs,wm_write);
          else if (!res->wp && strcmp(r->var,"hw-rwpt")==0)
             res->wp=mi_get_wp(r->v.rs,wm_read);
          else if (!res->wp && strcmp(r->var,"hw-awpt")==0)
             res->wp=mi_get_wp(r->v.rs,wm_rw);
          else if (!(res->wp_old || res->wp_val) && strcmp(r->var,"value")==0)
             {
              mi_results *p=r->v.rs;
              while (p)
                {
                 if (strcmp(p->var,"value")==0 || strcmp(p->var,"new")==0)
                   {
                    res->wp_val=p->v.cstr;
                    p->v.cstr=NULL;
                   }
                 else if (strcmp(p->var,"old")==0)
                   {
                    res->wp_old=p->v.cstr;
                    p->v.cstr=NULL;
                   }
                 p=p->next;
                }
             }
         }
       r=r->next;
      }
   }
 return res;
}

mi_stop *mi_res_stop(mi_h *h)
{
 mi_output *o=mi_retire_response(h);
 mi_stop *stop=NULL;

 if (o)
   {
    mi_output *sr=mi_get_stop_record(o);
    if (sr)
       stop=mi_get_stopped(sr->c);
   }
 mi_free_output(o);

 return stop;
}

int mi_get_read_memory(mi_h *h, unsigned char *dest, unsigned ws, int *na,
                       unsigned long *addr)
{
 char *end;
 mi_results *res=mi_res_done_var(h,"memory"), *r;
 int ok=0;

 *na=0;
 r=res;
 if (r && r->type==t_list && ws==1)
   {
    r=r->v.rs;
    if (r->type!=t_tuple)
      {
       mi_free_results(res);
       return 0;
      }
    r=r->v.rs;
    while (r)
      {
       if (r->type==t_list && strcmp(r->var,"data")==0)
         {
          mi_results *data=r->v.rs;
          ok++;
          if (data && data->type==t_const &&
              strcmp(data->v.cstr,"N/A")==0)
             *na=1;
          else
             while (data)
               {
                if (data->type==t_const)
                   *(dest++)=strtol(data->v.cstr,&end,0);
                data=data->next;
               }
         }
       else if (r->type==t_const && strcmp(r->var,"addr")==0)
         {
          ok++;
          if (addr)
             *addr=strtoul(r->v.cstr,&end,0);
         }
       r=r->next;
      }

   }
 mi_free_results(res);
 return ok==2;
}

mi_asm_insn *mi_parse_insn(mi_results *c)
{
 mi_asm_insn *res=NULL, *cur=NULL;
 mi_results *sub;
 char *end;

 while (c)
   {
    if (c->type==t_tuple)
      {
       if (!res)
          res=cur=mi_alloc_asm_insn();
       else
         {
          cur->next=mi_alloc_asm_insn();
          cur=cur->next;
         }
       if (!cur)
         {
          mi_free_asm_insn(res);
          return NULL;
         }
       sub=c->v.rs;
       while (sub)
         {
          if (sub->type==t_const)
            {
             if (strcmp(sub->var,"address")==0)
                cur->addr=(void *)strtoul(sub->v.cstr,&end,0);
             else if (strcmp(sub->var,"func-name")==0)
               {
                cur->func=sub->v.cstr;
                sub->v.cstr=NULL;
               }
             else if (strcmp(sub->var,"offset")==0)
                cur->offset=atoi(sub->v.cstr);
             else if (strcmp(sub->var,"inst")==0)
               {
                cur->inst=sub->v.cstr;
                sub->v.cstr=NULL;
               }
            }
          sub=sub->next;
         }
      }
    c=c->next;
   }
 return res;
}

mi_asm_insns *mi_parse_insns(mi_results *c)
{
 mi_asm_insns *res=NULL, *cur=NULL;
 mi_results *sub;

 while (c)
   {
    if (c->var)
      {
       if (strcmp(c->var,"src_and_asm_line")==0 && c->type==t_tuple)
         {
          if (!res)
             res=cur=mi_alloc_asm_insns();
          else
            {
             cur->next=mi_alloc_asm_insns();
             cur=cur->next;
            }
          if (!cur)
            {
             mi_free_asm_insns(res);
             return NULL;
            }
          sub=c->v.rs;
          while (sub)
            {
             if (sub->var)
               {
                if (sub->type==t_const)
                  {
                   if (strcmp(sub->var,"line")==0)
                      cur->line=atoi(sub->v.cstr);
                   else if (strcmp(sub->var,"file")==0)
                     {
                      cur->file=sub->v.cstr;
                      sub->v.cstr=NULL;
                     }
                  }
                else if (sub->type==t_list)
                  {
                   if (strcmp(sub->var,"line_asm_insn")==0)
                      cur->ins=mi_parse_insn(sub->v.rs);
                  }
               }
             sub=sub->next;
            }
         }
      }
    else
      {/* No source line, just instructions */
       res=mi_alloc_asm_insns();
       res->ins=mi_parse_insn(c);
       break;
      }
    c=c->next;
   }
 return res;
}


mi_asm_insns *mi_get_asm_insns(mi_h *h)
{
 mi_results *r=mi_res_done_var(h,"asm_insns");
 mi_asm_insns *f=NULL;

 if (r && r->type==t_list)
    f=mi_parse_insns(r->v.rs);
 mi_free_results(r);
 return f;
}

mi_chg_reg *mi_parse_list_regs(mi_results *r, int *how_many)
{
 mi_results *c=r;
 int cregs=0;
 mi_chg_reg *first=NULL, *cur=NULL;

 /* Create the list. */
 while (c)
   {
    if (c->type==t_const && !c->var)
      {
       if (first)
          cur=cur->next=mi_alloc_chg_reg();
       else
          first=cur=mi_alloc_chg_reg();
       cur->name=c->v.cstr;
       cur->reg=cregs++;
       c->v.cstr=NULL;
      }
    c=c->next;
   }
 if (how_many)
    *how_many=cregs;

 return first;
}

mi_chg_reg *mi_get_list_registers(mi_h *h, int *how_many)
{
 mi_results *r=mi_res_done_var(h,"register-names");
 mi_chg_reg *l=NULL;

 if (r && r->type==t_list)
    l=mi_parse_list_regs(r->v.rs,how_many);
 mi_free_results(r);
 return l;
}

mi_chg_reg *mi_parse_list_changed_regs(mi_results *r)
{
 mi_results *c=r;
 mi_chg_reg *first=NULL, *cur=NULL;

 /* Create the list. */
 while (c)
   {
    if (c->type==t_const && !c->var)
      {
       if (first)
          cur=cur->next=mi_alloc_chg_reg();
       else
          first=cur=mi_alloc_chg_reg();
       cur->reg=atoi(c->v.cstr);
      }
    c=c->next;
   }

 return first;
}

mi_chg_reg *mi_get_list_changed_regs(mi_h *h)
{
 mi_results *r=mi_res_done_var(h,"changed-registers");
 mi_chg_reg *changed=NULL;

 if (r && r->type==t_list)
    changed=mi_parse_list_changed_regs(r->v.rs);
 mi_free_results(r);
 return changed;
}

int mi_parse_reg_values(mi_results *r, mi_chg_reg *l)
{
 mi_results *c;

 while (r && l)
   {
    if (r->type==t_tuple && !r->var)
      {
       c=r->v.rs;
       while (c)
         {
          if (c->type==t_const && c->var)
            {
             if (strcmp(c->var,"number")==0)
               {
                if (atoi(c->v.cstr)!=l->reg)
                  {
                   mi_error=MI_PARSER;
                   return 0;
                  }
               }
             else if (strcmp(c->var,"value")==0)
               {
                l->val=c->v.cstr;
                c->v.cstr=NULL;
               }
            }
          c=c->next;
         }
      }
    r=r->next;
    l=l->next;
   }

 return !l && !r;
}

int mi_get_reg_values(mi_h *h, mi_chg_reg *l)
{
 mi_results *r=mi_res_done_var(h,"register-values");
 int ok=0;

 if (r && r->type==t_list)
    ok=mi_parse_reg_values(r->v.rs,l);
 mi_free_results(r);
 return ok;
}

int mi_parse_list_regs_l(mi_results *r, mi_chg_reg *l)
{
 while (r && l)
   {
    if (r->type==t_const && !r->var)
      {
       free(l->name);
       l->name=r->v.cstr;
       r->v.cstr=NULL;
       l=l->next;
      }
    r=r->next;
   }

 return !l && !r;
}

int mi_get_list_registers_l(mi_h *h, mi_chg_reg *l)
{
 mi_results *r=mi_res_done_var(h,"register-names");
 int ok=0;

 if (r && r->type==t_list)
    ok=mi_parse_list_regs_l(r->v.rs,l);
 mi_free_results(r);
 return ok;
}

mi_chg_reg *mi_parse_reg_values_l(mi_results *r, int *how_many)
{
 mi_results *c;
 mi_chg_reg *first=NULL, *cur=NULL;
 *how_many=0;

 while (r)
   {
    if (r->type==t_tuple && !r->var)
      {
       c=r->v.rs;
       if (first)
          cur=cur->next=mi_alloc_chg_reg();
       else
          first=cur=mi_alloc_chg_reg();
       while (c)
         {
          if (c->type==t_const && c->var)
            {
             if (strcmp(c->var,"number")==0)
               {
                cur->reg=atoi(c->v.cstr);
                (*how_many)++;
               }
             else if (strcmp(c->var,"value")==0)
               {
                cur->val=c->v.cstr;
                c->v.cstr=NULL;
               }
            }
          c=c->next;
         }
      }
    r=r->next;
   }

 return first;
}

mi_chg_reg *mi_get_reg_values_l(mi_h *h, int *how_many)
{
 mi_results *r=mi_res_done_var(h,"register-values");
 mi_chg_reg *rgs=NULL;

 if (r && r->type==t_list)
    rgs=mi_parse_reg_values_l(r->v.rs,how_many);
 mi_free_results(r);
 return rgs;
}


