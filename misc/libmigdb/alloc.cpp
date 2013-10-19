/**[txh]********************************************************************

  Copyright (c) 2004 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Allocator.
  Comments:
  Most alloc/free routines are here. Free routines must accept NULL
pointers. Alloc functions must set mi_error.@p
  
***************************************************************************/

#include "mi_gdb.h"

void *mi_calloc(size_t count, size_t sz)
{
 void *res=calloc(count,sz);
 if (!res)
    mi_error=MI_OUT_OF_MEMORY;
 return res;
}

void *mi_calloc1(size_t sz)
{
 return mi_calloc(1,sz);
}

char *mi_malloc(size_t sz)
{
 char *res=(char *)malloc(sz);
 if (!res)
    mi_error=MI_OUT_OF_MEMORY;
 return res;
}

mi_results *mi_alloc_results(void)
{
 return (mi_results *)mi_calloc1(sizeof(mi_results));
}

mi_output *mi_alloc_output(void)
{
 return (mi_output *)mi_calloc1(sizeof(mi_output));
}

mi_frames *mi_alloc_frames(void)
{
 return (mi_frames *)mi_calloc1(sizeof(mi_frames));
}

mi_gvar *mi_alloc_gvar(void)
{
 return (mi_gvar *)mi_calloc1(sizeof(mi_gvar));
}

mi_gvar_chg *mi_alloc_gvar_chg(void)
{
 return (mi_gvar_chg *)mi_calloc1(sizeof(mi_gvar_chg));
}

mi_bkpt *mi_alloc_bkpt(void)
{
 mi_bkpt *b=(mi_bkpt *)mi_calloc1(sizeof(mi_bkpt));
 if (b)
   {
    b->thread=-1;
    b->ignore=-1;
   }
 return b;
}

mi_wp *mi_alloc_wp(void)
{
 return (mi_wp *)mi_calloc1(sizeof(mi_wp));
}

mi_stop *mi_alloc_stop(void)
{
 return (mi_stop *)mi_calloc1(sizeof(mi_stop));
}

mi_asm_insns *mi_alloc_asm_insns(void)
{
 return (mi_asm_insns *)mi_calloc1(sizeof(mi_asm_insns));
}

mi_asm_insn *mi_alloc_asm_insn(void)
{
 return (mi_asm_insn *)mi_calloc1(sizeof(mi_asm_insn));
}

mi_chg_reg *mi_alloc_chg_reg(void)
{
 return (mi_chg_reg *)mi_calloc1(sizeof(mi_chg_reg));
}

/*****************************************************************************
  Free functions
*****************************************************************************/

void mi_free_frames(mi_frames *f)
{
 mi_frames *aux;

 while (f)
   {
    free(f->func);
    free(f->file);
    free(f->from);
    mi_free_results(f->args);
    aux=f->next;
    free(f);
    f=aux;
   }
}

void mi_free_bkpt(mi_bkpt *b)
{
 mi_bkpt *aux;

 while (b)
   {
    free(b->func);
    free(b->file);
    free(b->file_abs);
    free(b->cond);
    aux=b->next;
    free(b);
    b=aux;
   }
}

void mi_free_gvar(mi_gvar *v)
{
 mi_gvar *aux;

 while (v)
   {
    free(v->name);
    free(v->type);
    free(v->exp);
    free(v->value);
    if (v->numchild && v->child)
       mi_free_gvar(v->child);
    aux=v->next;
    free(v);
    v=aux;
   }
}

void mi_free_gvar_chg(mi_gvar_chg *p)
{
 mi_gvar_chg *aux;

 while (p)
   {
    free(p->name);
    free(p->new_type);
    aux=p->next;
    free(p);
    p=aux;
   }
}

void mi_free_results_but(mi_results *r, mi_results *no)
{
 mi_results *aux;

 while (r)
   {
    if (r==no)
      {
       aux=r->next;
       r->next=NULL;
       r=aux;
      }
    else
      {
       free(r->var);
       switch (r->type)
         {
          case t_const:
               free(r->v.cstr);
               break;
          case t_tuple:
          case t_list:
               mi_free_results_but(r->v.rs,no);
               break;
         }
       aux=r->next;
       free(r);
       r=aux;
      }
   }
}

void mi_free_results(mi_results *r)
{
 mi_free_results_but(r,NULL);
}

void mi_free_output_but(mi_output *r, mi_output *no, mi_results *no_r)
{
 mi_output *aux;

 while (r)
   {
    if (r==no)
      {
       aux=r->next;
       r->next=NULL;
       r=aux;
      }
    else
      {
       if (r->c)
          mi_free_results_but(r->c,no_r);
       aux=r->next;
       free(r);
       r=aux;
      }
   }
}

void mi_free_output(mi_output *r)
{
 mi_free_output_but(r,NULL,NULL);
}

void mi_free_stop(mi_stop *s)
{
 if (!s)
    return;
 mi_free_frames(s->frame);
 mi_free_wp(s->wp);
 free(s->wp_old);
 free(s->wp_val);
 free(s->gdb_result_var);
 free(s->return_value);
 free(s->signal_name);
 free(s->signal_meaning);
 free(s);
}

void mi_free_wp(mi_wp *wp)
{
 mi_wp *aux;
 while (wp)
   {
    free(wp->exp);
    aux=wp->next;
    free(wp);
    wp=aux;
   }
}

void mi_free_asm_insns(mi_asm_insns *i)
{
 mi_asm_insns *aux;

 while (i)
   {
    free(i->file);
    mi_free_asm_insn(i->ins);
    aux=i->next;
    free(i);
    i=aux;
   }
}

void mi_free_asm_insn(mi_asm_insn *i)
{
 mi_asm_insn *aux;

 while (i)
   {
    free(i->func);
    free(i->inst);
    aux=i->next;
    free(i);
    i=aux;
   }
}

/*void mi_free_charp_list(char **l)
{
 char **c=l;
 while (c)
   {
    free(*c);
    c++;
   }
 free(l);
}*/

void mi_free_chg_reg(mi_chg_reg *r)
{
 mi_chg_reg *aux;
 while (r)
   {
    free(r->val);
    free(r->name);
    aux=r->next;
    free(r);
    r=aux;
   }
}

