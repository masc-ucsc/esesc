/*
This is a standalone program that generates 
ARM instruction trace. Run this program on
ARM processor.

Contributed by Himabindu Thota.
*/

#include <stdio.h>
#include "mi_gdb.h"
#include <regex.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>

int wait_for_stop(mi_h *h)
{
 int res=1;
 mi_stop *sr;
 int ret;

 regex_t re1;

 while (!mi_get_response(h))
    usleep(1000);
 /* The end of the async. */
 sr=mi_res_stop(h);
 if (sr)
   {
    printf("stopped reason: %s \n", mi_reason_enum_to_str(sr->reason) );
    ret = regcomp(&re1, "Exited", REG_EXTENDED);
    ret = regexec(&re1, mi_reason_enum_to_str(sr->reason), 0, NULL, 0);
    mi_free_stop(sr);
    if(!ret) {
      printf("Reached End of program \n");
      return 2;
    }
   }
 else
   {
    printf("Error while waiting\n");
    res=0;
   }
 return res;
}

void cb_console(const char *str, void *data)
{
 printf("CONSOLE> %s\n",str);
}

/* Note that unlike what's documented in gdb docs it isn't usable. */
void cb_target(const char *str, void *data)
{
 printf("TARGET> %s\n",str);
}

void cb_log(const char *str, void *data)
{
 printf("LOG> %s\n",str);
}

void cb_to(const char *str, void *data)
{
 printf(">> %s",str);
}

void cb_from(const char *str, void *data)
{
 printf("<< %s\n",str);
}

void cb_async(mi_output *o, void *data)
{
 printf("ASYNC\n");
}

char **get_gdb_out_line(mi_h *h) {

   int size_of_line = 128;
   int num_lines = 32;
   int i, j=0;
   int mi_l;

   char **str = (char **)malloc(32 * sizeof(*str) );
   for(i=0;i<num_lines;i++) {
      *(str+i) = (char *)malloc(size_of_line * sizeof(char) );
      memset( *(str+i), '\0', size_of_line);
   }

   while(1) {
      mi_l = mi_getline(h);

     if (!mi_l) {
        continue;
     }

     // Got some response from gdb.
     if (h->from_gdb_echo) {
       h->from_gdb_echo(h->line,h->from_gdb_echo_data);
     }

     // gdb console response line ?
     if (strncmp(h->line,"(gdb)",5)==0)
       {/* End of response. */
       break;
     } else {
       if(strncmp(h->line, "~", 1) ==0) {
         printf("   console line %s \n", h->line);
         strcpy(str[j], h->line);
         j++;	
       }
     }
   }
   return str;
}

void handle_ldrex_strex_dmb_block(mi_h *h, char *pc) {

  printf ("handle_ldrex_strex_dmb_block: pc: %s \n", pc);	
  char **out;
  int line=0;
  int ret = 0;
  regex_t re2, re3;
  regmatch_t match[2];
  int strex = 0;
  char s_pc[32];
  char s1_pc[32];
  char n_pc[32];
  mi_bkpt *bk;

  // data-memory-barrier
  int dmb = 0;
  uint32_t start_pc = strtol(pc,NULL,16) + 4;
  uint32_t next_pc;

  ret = regcomp(&re2, "strex", REG_EXTENDED);
  ret = regcomp(&re3, "dmb", REG_EXTENDED);

  printf("start to check ldrex block \n");

  while(!dmb) {

    next_pc = start_pc + 4; 
    sprintf(s_pc, "0x%08x", start_pc);
    sprintf(n_pc, "0x%08x", next_pc);

    mi_send(h, "disas /r %s, %s \n", s_pc, n_pc);
    out = get_gdb_out_line(h);

    for(line=0; ; line++) {
      printf("line: %s \n", *(out+line));
      if(**(out+line) == '\0') {
        break;
      }
      ret = regexec(&re2, *(out+line), 1, match, 0);
      if(!ret) {
        strex = 1;
        printf("check strex for debugging purposes STREX %d \n", strex);
        break;
      }
      ret = regexec(&re3, *(out+line), 1, match, 0);
      if(!ret) {
        dmb = 1;
        printf("dmb is 1 \n");
        break;
      }
    }
    start_pc = next_pc;
  }

  sprintf(s1_pc, "*%s", s_pc);
  printf("before break s_pc %s \n", s1_pc);
  bk=gmi_break_insert_full(h,1,0,NULL,-1,-1,s1_pc);
  if (!bk)
  {
    printf("Error setting breakpoint\n");
    mi_disconnect(h);
    exit(0);
  }
  printf("Breakpoint %d @ function: %s\n",bk->number,bk->func);
  mi_free_bkpt(bk);

  if (!gmi_exec_continue(h))
  {
    printf("Error in continue!\n");
    mi_disconnect(h);
    exit(0);
  }

  // Wait to hit the breakpoint
  if(!wait_for_stop(h)) {
    mi_disconnect(h);
    printf("Error while waiting to hit the breakpoint!\n");
    exit(0);
  }
}

void logPCAndInst(FILE *fp, char **out, char *pc, char *inst, int *isSVCInst, int *isLDREXInst, int *inst_count) {

  regex_t re1, re2, re3, re5, re6;
  regmatch_t match[6];
  int ret1, ret2, ret3, ret5, ret6;
  int line=0;
  int i=0, j, k;
  char byte[4][4];
  int Thumb16Inst = 0;

  // RE for the instruction
  ret1 = regcomp(&re1, "(0x[[:alnum:]]{8})", REG_EXTENDED);
  ret2 = regcomp(&re2, "([[:alnum:]]{2})[[:space:]]([[:alnum:]]{2})[[:space:]]([[:alnum:]]{2})[[:space:]]([[:alnum:]]{2})", REG_EXTENDED);
  ret3 = regcomp(&re3, "svc", REG_EXTENDED);
  ret5 = regcomp(&re5, "ldrex", REG_EXTENDED);
  ret6 = regcomp(&re6, "0x[[:alnum:]]{8}.*:.*([[:alnum:]]{2})[[:space:]]([[:alnum:]]{2}).*", REG_EXTENDED);

  for(line=0; ; line++) {
    printf("Line:: %s \n", *(out+line));

    if( **(out+line) == '\0') {
       printf("ERROR: NO PC found. This should not happen \n");
       exit(0);
    }

    ret5 = regexec(&re5, *(out+line), 1, match, 0);
    if(!ret5) {
      *isLDREXInst = 1;
      printf("setting isLDREXInst %d \n", *isLDREXInst);
    }

    ret1 = regexec(&re1, *(out+line), 2, match, 0);

    if (!Thumb16Inst) {
      if(!ret1) {

        j = 0;
        for(i = match[1].rm_so; i < match[1].rm_eo; i++) {
          pc[j] = *(*(out + line) + i);
          j++;
        } 
        pc[j] = '\0';
        printf("** pc is %s \n", pc);

        fprintf(fp, "Instruction number: %d\n", ++*inst_count);
        fprintf(fp, "pc: %s\n", pc);
      }
    }

    ret3 = regexec(&re3, *(out+line), 1, match, 0);
    if(!ret3) {
      *isSVCInst = 1;
    } else {
      *isSVCInst = 0;
    }

    ret2 = regexec(&re2, *(out+line), 6, match, 0);
    if(!ret2) {
      if(Thumb16Inst) {
        for(k = 2; k > 0; k--) {
          j = 0;
          for(i = match[k].rm_so; i < match[k].rm_eo; i++) {
            byte[2-k][j] = *(*(out + line) + i);
            j++;
          } 
          byte[2-k][j] = '\0';
        }
        printf("thumb inst ");
      } else {
        for(k = 4; k > 0; k--) {
          j = 0;
          for(i = match[k].rm_so; i < match[k].rm_eo; i++) {
            byte[4-k][j] = *(*(out + line) + i);
            j++;
          } 
          byte[4-k][j] = '\0';
        }
      }
      printf("inst %s %s %s %s sup %d \n", byte[0], byte[1], byte[2], byte[3], *isSVCInst);
      fprintf(fp, "inst: 0x%s%s%s%s\n", byte[0], byte[1], byte[2], byte[3]);

      sprintf(inst, "%s%s%s%s", byte[0], byte[1], byte[2], byte[3]);

      break;
    }

    ret6 = regexec(&re6, *(out+line), 3, match, 0);
    printf("Line %s ret6 %d \n", *(out+line), ret6);
    if(!ret6) {
      if(Thumb16Inst) {
        for(k = 2; k > 0; k--) {
          j = 0;
          for(i = match[k].rm_so; i < match[k].rm_eo; i++) {
            if (k == 2) {
              byte[0][j] = *(*(out + line) + i);
            } else {
              byte[1][j] = *(*(out + line) + i);
            }
            j++;
          }
          if (k == 2) {
            byte[0][j] = '\0';
          } else {
            byte[1][j] = '\0';
            printf("thumb inst: %s %s %s %s \n", byte[0], byte[1], byte[2], byte[3]);
            fprintf(fp, "inst: 0x%s%s%s%s\n", byte[0], byte[1], byte[2], byte[3]);
            sprintf(inst, "%s%s%s%s", byte[0], byte[1], byte[2], byte[3]);
            return;
          }
        }
      } else {
        for(k = 2; k > 0; k--) {
          j = 0;
          for(i = match[k].rm_so; i < match[k].rm_eo; i++) {
            if (k == 2) {
              byte[2][j] = *(*(out + line) + i);
            } else {
              byte[3][j] = *(*(out + line) + i);
            }
            j++;
          }
          if (k == 2) {
            byte[2][j] = '\0';
          } else {
            byte[3][j] = '\0';
          }
          Thumb16Inst = 1;
        }
      }
    }
  }
}

void parseFPRegisters(FILE *fp, int reg, char *val) {

  regex_t re2;
  regmatch_t match[2];
  int ret;
  char reg_val[64];
  int i = 0;
  int j = 0;
  
  ret = regcomp(&re2, "u64[[:space:]]+=[[:space:]]+(0x[[:alnum:]]+),", REG_EXTENDED);

  ret = regexec(&re2, val, 2, match, 0);

  if (!ret) {
    for (i = match[1].rm_so; i < match[1].rm_eo; i++) {
      reg_val[j] = val[i];
      j++;
    }
  }
  reg_val[j] = '\0'; 
  // printf("reg %d, reg_val %s \n", reg, reg_val);
  fprintf(fp, "r%d=%s \n", reg, reg_val);

}

void logSyscall_partI(FILE *fp, mi_h *h, char *pc, char *syscall_num, char *buf_ptr) {

 int num;

 mi_chg_reg *l = mi_alloc_chg_reg();
 l->reg = 7;

 gmi_data_list_register_values(h,fm_hexadecimal,l);
 strcpy(syscall_num, l->val);
 num = strtol(syscall_num, NULL, 16);

 if(num == 983045) {
   printf("Syscall: 983045 (0xf0005 - ARM_NR_set_tls) is not handled in the emulator, so ignore for now \n");
   return;
 }

 fprintf(fp, "------------------- \n");
 fprintf(fp, "pc: %s\n", pc);
 fprintf(fp, "syscall: %d\n", num);

 if(num == 122) {
   l->reg = 0;
 } else if(num == 197) {
   l->reg = 1;
 }
 gmi_data_list_register_values(h,fm_hexadecimal,l);
 printf("buf ptr %s \n", l->val);
 strcpy(buf_ptr, l->val);
 
}

void processUnameOut(FILE *fp, char **out) {

  regex_t re1;
  regmatch_t match[2];
  int ret1, j, k, line;
  uint32_t i;
  char str[128];

  ret1 = regcomp(&re1, ":.*\"(.*)\\\"\"", REG_EXTENDED);

  for(line=0;;line++) {
    if(**(out+line) == '\0') {
      return;
    }
    for(i=0;i<strlen(*(out+line));i++) {
      // printf("%c \n", *(*(out+line)+i) );
    }
    ret1 = regexec(&re1, *(out+line), 2, match, 0);

    if(!ret1) {
      j = 0;
      for(k = match[1].rm_so; k < match[1].rm_eo; k++) {
        str[j] = *(*(out + line) + k);
        j++;
      }
      // This is a workaround to ignore the last backslash
      str[j-1] = '\0';
      // printf("str: %s \n", str);
      fprintf(fp,"%s\n",str);
    }
  }
}

void processFstat64Out(FILE *fp, char **out) {

  regex_t re1;
  regmatch_t match[9];
  int ret1, i, j, k, line;
  char str[8][128];

  ret1 = regcomp(&re1, "0x.*:.*0x([[:alnum:]]{2}).*0x([[:alnum:]]{2}).*0x([[:alnum:]]{2}).*0x([[:alnum:]]{2}).*0x([[:alnum:]]{2}).*0x([[:alnum:]]{2}).*0x([[:alnum:]]{2}).*0x([[:alnum:]]{2})", REG_EXTENDED);
  
  for(line=0;;line++) {
    printf("** Line: %s \n", *(out+line) );

    if(**(out+line) == '\0') {
      return;
    }
    ret1 = regexec(&re1, *(out+line), 9, match, 0);

    if(!ret1) {

      for(i=1; i<=8;i++) {
        j = 0;
        for(k = match[i].rm_so; k < match[i].rm_eo; k++) {
          str[i-1][j] = *(*(out + line) + k);
          j++;
        }
        str[i-1][j] = '\0';
      }

      for(i=7;i>=0;i--) {
        fprintf(fp, "%s", str[i]);  
      }
      fprintf(fp, "\n");
    }
  }
}

void logSyscall_partII(FILE *fp, mi_h *h, char *syscall_num, char *buf_ptr) {

  int i, offset = 0;
  char **gdb_out;
  int num;

  // printf("logSyscall_partII, num %s %d buf_ptr %s \n", syscall_num, strtol(syscall_num, NULL, 16), buf_ptr);
  
 num = strtol(syscall_num, NULL, 16);
 if(num == 983045) {
   printf("Syscall: 983045 (0xf0005 - ARM_NR_set_tls) is not handled in the emulator, so ignore for now \n");
   return;
 }

  if(num == 122) {
    printf("match found \n");
    for(i=0;i<5;i++) {
      offset = i*65;
      mi_send(h, "x /s %s+%d \n", buf_ptr, offset);    
      gdb_out = get_gdb_out_line(h);
      processUnameOut(fp, gdb_out);
    }
  } else if(num == 197) {
    mi_send(h, "x/144bx %s \n", buf_ptr);
    gdb_out = get_gdb_out_line(h);
    processFstat64Out(fp, gdb_out);
  }
}

int main(int argc, char *argv[])
{
 mi_pty *pty=NULL;
 mi_h *h;
 mi_bkpt *bk;

 time_t start;
 time_t end;
 double diff;

 mi_chg_reg *chg;

 int first_inst = 1;

 //FILE *f_reopen = NULL;
 FILE *inst_fp = NULL;
 FILE *syscall_fp = NULL;
 FILE *scmain_fp = NULL;

 char binary[32];
 char results_dir[128];
 char stdout_log[256];
 char stack_mem_dump[256];
 char syscall_trace[256];
 char inst_trace[256];
 char scmain_usage[256];

 char m_start[32], m_end[32];
 char mod_str[32] = "00000000";

 mi_chg_reg *l = mi_alloc_chg_reg();
 l->reg = 13;

 char *inst = (char *)malloc(32 * sizeof(char));
 char *pc = (char *)malloc(32 * sizeof(char));
 int isSVCInst = 0;
 int isLDREXInst = 0;
 char *syscall_num = (char *)malloc(32 * sizeof(char));
 char *buf_ptr = (char *)malloc(32 * sizeof(char));

 struct stat st;

 char **gdb_out;

 int inst_count = 0;

 if (argc == 3) {
   strcpy(binary, argv[1]);
   strcpy(results_dir, argv[2]);
 } else {
   printf("Usage: generateARMTrace <binary> <results_dir> \n");
   return 0;
 }


 time(&start);
 printf("start time %s \n", ctime(&start));

 if(stat(results_dir,&st) != 0) {
   if((mkdir(results_dir,00777)) == -1) {
     printf("Error in creating the directory %s \n", results_dir);
     return 0;
   }
 }

 sprintf(stdout_log, "%s/stdout_log.txt", results_dir);
 sprintf(stack_mem_dump, "%s/stack_mem_dump.bin", results_dir);
 sprintf(inst_trace, "%s/inst_trace", results_dir);
 sprintf(syscall_trace, "%s/syscall_trace", results_dir);
 sprintf(scmain_usage, "%s/scmain_cmd", results_dir);

 printf("This program will generate the following files... \n\n");
 // printf("%-30s		redirected stdout \n", stdout_log);
 printf("%-30s		stack memory dump \n", stack_mem_dump);
 printf("%-30s		instruction trace \n", inst_trace);
 printf("%-30s		system call trace \n", syscall_trace);
 printf("%-30s		command line for running scmain, ARM emulator \n", scmain_usage);
 printf("\n\n");
 printf("Please wait, the program will finish in a few mins! \n\n");

 // comment the line to see stdout going to the screen.
 // f_reopen = freopen(stdout_log, "w", stdout);
 printf("start time %s \n", ctime(&start));

 inst_fp = fopen(inst_trace, "w");
 syscall_fp = fopen(syscall_trace, "w");
 scmain_fp = fopen(scmain_usage, "w");

 /* Connect to gdb child. */
 h=mi_connect_local();
 if (!h)
   {
    printf("Connect failed\n");
    return 1;
   }
 printf("Connected to gdb!\n");

 /* Set all callbacks. */
 mi_set_console_cb(h,cb_console,NULL);
 mi_set_target_cb(h,cb_target,NULL);
 mi_set_log_cb(h,cb_log,NULL);
 mi_set_async_cb(h,cb_async,NULL);
 mi_set_to_gdb_cb(h,cb_to,NULL);
 mi_set_from_gdb_cb(h,cb_from,NULL);

 /* Look for a free pseudo terminal. */
 pty=gmi_look_for_free_pty();
 if (!pty) 
 {
  printf("Error opening pseudo terminal.\n");
  return 1;
 }

 printf("Free pty slave = %s, master on %d\n",pty->slave,pty->master);

 /* Tell gdb to attach the terminal. */
 if (!gmi_target_terminal(h,pty->slave))
 {
  printf("Error selecting target terminal\n");
  mi_disconnect(h);
  return 1;
 }

 /* Set the name of the child and the command line aguments. */
 if (!gmi_set_exec(h,binary,""))
 {
   printf("Error setting exec y args\n");
   mi_disconnect(h);
   return 1;
 }                          

 // Insert break point at *_start
 bk=gmi_break_insert_full(h,1,0,NULL,-1,-1,"*_start");

 if (!bk)
 {
   printf("Error setting breakpoint\n");
   mi_disconnect(h);
   return 1;
  }
  printf("Breakpoint %d @ function: %s\n",bk->number,bk->func);
  mi_free_bkpt(bk);

 if (!gmi_exec_run(h))
 {
   printf("Error in run!\n");
   mi_disconnect(h);
   return 1;
 }
 // Wait to hit the breakpoint
 if(!wait_for_stop(h)) {
   mi_disconnect(h);
   return 1;
 }

 printf("Now find SP and dump stack memory around SP \n");

 gmi_data_list_register_values(h,fm_hexadecimal,l);
 strcpy(m_start, l->val);
 strcpy(m_end, l->val);
 memcpy(m_start+6, mod_str, 4);
 strcpy(mod_str,"ffffffff");
 memcpy(m_end+6, mod_str, 4);
 
 printf("start %s end %s \n", m_start, m_end);

 mi_send(h, "dump binary memory %s %s %s \n", stack_mem_dump, m_start, m_end);
 if(!mi_res_simple_done(h)) {
   printf("Error while dumping stack memory!\n");
   mi_disconnect(h);
   return 1; 
 }

 fprintf(scmain_fp, "main/scmain %s %s stack_mem_dump.bin %s inst_trace syscall_trace \n", l->val, m_start, binary);

 while(1) {


   // PC and instruction are noted before stepping (si)
   // and registers after si :)

   mi_send(h, "disas /r $pc, $pc+4 \n");
   gdb_out = get_gdb_out_line(h);
   logPCAndInst(inst_fp, gdb_out, pc, inst, &isSVCInst, &isLDREXInst, &inst_count);
 
   if(isSVCInst)
     logSyscall_partI(syscall_fp, h, pc, syscall_num, buf_ptr);

   gmi_exec_step_instruction(h);
   if (wait_for_stop(h) == 2) break;

   if(isSVCInst)
     logSyscall_partII(syscall_fp, h, syscall_num, buf_ptr);

   chg = gmi_data_list_changed_registers(h);
   if (!first_inst) {
    gmi_data_list_register_values(h,fm_hexadecimal,chg);
    while (chg) {
      if (chg->reg >= 58 && chg->reg <= 73) {
        parseFPRegisters(inst_fp, chg->reg, chg->val);
      }
      else if (chg->reg < 15) {
        fprintf(inst_fp, "r%d=%s\n", (chg->reg + 1),chg->val);
      }
      chg=chg->next;
     }
   } else {
     first_inst = 0;
   }

   fprintf(inst_fp, "----------------\n");

 }
	
 /* Exit from gdb. */
 gmi_gdb_exit(h);

 time(&end);
 printf("end time %s \n", ctime(&end));
 diff = difftime(end, start);
 printf("difftime %lf \n", diff);

 /* Close the connection. */
 mi_disconnect(h);
 gmi_end_pty(pty);

 // fclose(f_reopen);

 return 0;
}
