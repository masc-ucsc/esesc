/**[txh]********************************************************************

  Copyright (c) 2004 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Data manipulation.
  Comments:
  GDB/MI commands for the "Data manipulation" section.@p

@<pre>
gdb command:                       Implemented?

-data-disassemble                  Yes
-data-evaluate-expression          Yes
-data-list-changed-registers       No
-data-list-register-names          Yes
-data-list-register-values         No
-data-read-memory                  No
-display-delete                    N.A. (delete display)
-display-disable                   N.A. (disable display)
-display-enable                    N.A. (enable display)
-display-insert                    N.A. (display)
-display-list                      N.A. (info display)
-environment-cd                    No
-environment-directory             Yes, MI v1 implementation
-environment-path                  No
@</pre>

Notes:@p

1) -display* aren't implemented. You can use CLI command display, but the
results are sent to the console. So it looks like the best is to manually
use -data-evaluate-expression to emulate it.@p

2) GDB bug mi/1770: Affects gdb<=6.2, when you ask for the names of the
registers you get it plus the name of the "pseudo-registers", but if you
try to get the value of a pseudo-register you get an error saying the
register number is invalid. I reported to gdb-patches@sources.redhat.com
on 2004/08/25 and as I didn't get any answer I filled a bug report on
2004/09/02. The patch to fix this annoying bug is:

Index: gdb/mi/mi-main.c
===================================================================
RCS file: /cvs/src/src/gdb/mi/mi-main.c,v
retrieving revision 1.64
diff -u -r1.64 mi-main.c
--- gdb/mi/mi-main.c    3 Aug 2004 00:57:27 -0000       1.64
+++ gdb/mi/mi-main.c    25 Aug 2004 14:12:50 -0000
@@ -423,7 +423,7 @@
      case, some entries of REGISTER_NAME will change depending upon
      the particular processor being debugged.

-  numregs = NUM_REGS;
+  numregs = NUM_REGS + NUM_PSEUDO_REGS;

   if (argc == 0)
     {
----

Note I had to remove an end of comment in the patch to include it here.
This bug forced me to create another set of functions. The only way is to
first get the values and then the names.
Fixed by Changelog entry:

2004-09-12  Salvador E. Tropea  <set@users.sf.net>
            Andrew Cagney  <cagney@gnu.org>

        * mi/mi-main.c (mi_cmd_data_list_changed_registers)
        (mi_cmd_data_list_register_values)
        (mi_cmd_data_write_register_values): Include the PSEUDO_REGS in
        the register number computation.

***************************************************************************/

#include "mi_gdb.h"

/* Low level versions. */

void mi_data_evaluate_expression(mi_h *h, const char *expression)
{
 mi_send(h,"-data-evaluate-expression \"%s\"\n",expression);
}

void mi_dir(mi_h *h, const char *path)
{
 if (h->version>=MI_VERSION2U(2,0,0))
   {// MI v2
    if (path)
       mi_send(h,"-environment-directory \"%s\"\n",path);
    else
       mi_send(h,"-environment-directory -r\n");
   }
 else
   {
    mi_send(h,"-environment-directory %s\n",path ? path : "");
   }
}

void mi_data_read_memory_hx(mi_h *h, const char *exp, unsigned ws,
                            unsigned c, int convAddr)
{
 if (convAddr)
    mi_send(h,"-data-read-memory \"&%s\" x %d 1 %d\n",exp,ws,c);
 else
    mi_send(h,"-data-read-memory \"%s\" x %d 1 %d\n",exp,ws,c);
}

void mi_data_disassemble_se(mi_h *h, const char *start, const char *end,
                            int mode)
{
 mi_send(h,"-data-disassemble -s \"%s\" -e \"%s\" -- %d\n",start,end,mode);
}

void mi_data_disassemble_fl(mi_h *h, const char *file, int line, int lines,
                            int mode)
{
 mi_send(h,"-data-disassemble -f \"%s\" -l %d -n %d -- %d\n",file,line,lines,
         mode);
}

void mi_data_list_register_names(mi_h *h)
{
 mi_send(h,"-data-list-register-names\n");
}

void mi_data_list_register_names_l(mi_h *h, mi_chg_reg *l)
{
 mi_send(h,"-data-list-register-names ");
 while (l)
   {
    mi_send(h,"%d ",l->reg);
    l=l->next;
   }
 mi_send(h,"\n");
}

void mi_data_list_changed_registers(mi_h *h)
{
 mi_send(h,"-data-list-changed-registers\n");
}

void mi_data_list_register_values(mi_h *h, enum mi_gvar_fmt fmt, mi_chg_reg *l)
{
 mi_send(h,"-data-list-register-values %c ",mi_format_enum_to_char(fmt));
 while (l)
   {
    mi_send(h,"%d ",l->reg);
    l=l->next;
   }
 mi_send(h,"\n");
}

/* High level versions. */

/**[txh]********************************************************************

  Description:
  Evaluate an expression. Returns a parsed tree.

  Command: -data-evaluate-expression
  Return: The resulting value (as plain text) or NULL on error.
  
***************************************************************************/

char *gmi_data_evaluate_expression(mi_h *h, const char *expression)
{
 mi_data_evaluate_expression(h,expression);
 return mi_res_value(h);
}

/**[txh]********************************************************************

  Description:
  Path for sources. You must use it to indicate where are the sources for
the program to debug. Only the MI v1 implementation is available.

  Command: -environment-directory
  Return: !=0 OK
  
***************************************************************************/

int gmi_dir(mi_h *h, const char *path)
{
 mi_dir(h,path);
 return mi_res_simple_done(h);
}

int gmi_read_memory(mi_h *h, const char *exp, unsigned size,
                    unsigned char *dest, int *na, int convAddr,
                    unsigned long *addr)
{
 mi_data_read_memory_hx(h,exp,1,size,convAddr);
 return mi_get_read_memory(h,dest,1,na,addr);
}

mi_asm_insns *gmi_data_disassemble_se(mi_h *h, const char *start,
                                      const char *end, int mode)
{
 mi_data_disassemble_se(h,start,end,mode);
 return mi_get_asm_insns(h);
}

mi_asm_insns *gmi_data_disassemble_fl(mi_h *h, const char *file, int line,
                                      int lines, int mode)
{
 mi_data_disassemble_fl(h,file,line,lines,mode);
 return mi_get_asm_insns(h);
}

// Affected by gdb bug mi/1770
mi_chg_reg *gmi_data_list_register_names(mi_h *h, int *how_many)
{
 mi_data_list_register_names(h);
 return mi_get_list_registers(h,how_many);
}

int gmi_data_list_register_names_l(mi_h *h, mi_chg_reg *l)
{
 mi_data_list_register_names_l(h,l);
 return mi_get_list_registers_l(h,l);
}

mi_chg_reg *gmi_data_list_changed_registers(mi_h *h)
{
 mi_error=MI_OK;
 mi_data_list_changed_registers(h);
 return mi_get_list_changed_regs(h);
}

int gmi_data_list_register_values(mi_h *h, enum mi_gvar_fmt fmt, mi_chg_reg *l)
{
 mi_data_list_register_values(h,fmt,l);
 return mi_get_reg_values(h,l);
}

mi_chg_reg *gmi_data_list_all_register_values(mi_h *h, enum mi_gvar_fmt fmt, int *how_many)
{
 mi_data_list_register_values(h,fmt,NULL);
 return mi_get_reg_values_l(h,how_many);
}

