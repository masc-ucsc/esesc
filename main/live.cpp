/*
ESESC: Enhanced Super ESCalar simulator
Copyright (C) 2009 University of California, Santa Cruz.

Contributed by Jose Renau, Sina Hassani

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include "../misc/libsuc/Transporter.h" 
#include "../misc/libsuc/LiveCache.h"

#define WARMUP_ARRAY_SIZE 262144

//Declaring global variables
typedef uint32_t FlowID; // from RAWDInst.h
void *handle;
int64_t inst_count = 0;
int thread_mode = 0;
int child_id, portno;
char host_adr[100];
int64_t nrabbit = 0;
int ncheckpoints = 0;
int nchecks = 0;
int64_t live_skip = 0;
bool done_skip = false;
int force_warmup = 0;
int genwarm = 0;
int64_t foo_inst = 0;
LiveCache * live_warmup_cache;
uint64_t live_warmup_addr[WARMUP_ARRAY_SIZE];
bool live_warmup_st[WARMUP_ARRAY_SIZE];
uint64_t live_warmup_cnt;
int dlc = 0;

//Declaring functions
extern "C" void QEMUReader_goto_sleep(void *env);
extern "C" void QEMUReader_wakeup_from_sleep(void *env);
extern "C" int64_t qemuesesc_main(int64_t argc, char **argv, char **envp);
extern "C" void esesc_set_timing(uint32_t fid);

//Defining dynamic functions
typedef uint64_t (*dyn_QEMUReader_getFid_t)(uint32_t);
dyn_QEMUReader_getFid_t dyn_QEMUReader_getFid=0;

typedef void (*dyn_QEMUReader_finish_thread_t)(uint32_t);
dyn_QEMUReader_finish_thread_t dyn_QEMUReader_finish_thread=0;

typedef uint64_t (*dyn_QEMUReader_get_time_t)();
dyn_QEMUReader_get_time_t dyn_QEMUReader_get_time=0;

typedef uint32_t (*dyn_QEMUReader_setnoStats_t)();
dyn_QEMUReader_setnoStats_t dyn_QEMUReader_setnoStats=0;

typedef FlowID (*dyn_QEMUReader_resumeThreadGPU_t)(FlowID);
dyn_QEMUReader_resumeThreadGPU_t dyn_QEMUReader_resumeThreadGPU=0;

typedef FlowID (*dyn_QEMUReader_resumeThread_t)(FlowID, FlowID);
dyn_QEMUReader_resumeThread_t dyn_QEMUReader_resumeThread=0;

typedef void (*dyn_QEMUReader_pauseThread_t)(FlowID);
dyn_QEMUReader_pauseThread_t dyn_QEMUReader_pauseThread=0;

typedef uint64_t (*dyn_QEMUReader_queue_inst_t)(uint64_t, uint64_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, void *);
dyn_QEMUReader_queue_inst_t dyn_QEMUReader_queue_inst=0;

typedef void (*dyn_QEMUReader_syscall_t)(uint32_t, uint64_t, uint32_t);
dyn_QEMUReader_syscall_t dyn_QEMUReader_syscall=0;

typedef void (*dyn_QEMUReader_finish_t)(uint32_t);
dyn_QEMUReader_finish_t dyn_QEMUReader_finish=0;

typedef void (*dyn_QEMUReader_setFlowCmd_t)(bool*);
dyn_QEMUReader_setFlowCmd_t dyn_QEMUReader_setFlowCmd=0;

//This function loads the master rabbit dynamic library
void load_rabbit () {
  //Creating LiveCache to store warmup
  live_warmup_cache = new LiveCache();

  printf("Live: Loading rabbit\n");
  handle = dlopen("librabbitso.so", RTLD_NOW);

  if (!handle) {
    printf("DLOPEN: %s\n", dlerror());
    exit(EXIT_FAILURE);
  }

  //Initializing dynamic functions
  dyn_QEMUReader_getFid          = (dyn_QEMUReader_getFid_t)dlsym(handle, "QEMUReader_getFid");
  dyn_QEMUReader_finish_thread   = (dyn_QEMUReader_finish_thread_t)dlsym(handle, "QEMUReader_finish_thread");
  dyn_QEMUReader_get_time        = (dyn_QEMUReader_get_time_t)dlsym(handle, "QEMUReader_get_time");
  dyn_QEMUReader_setnoStats      = (dyn_QEMUReader_setnoStats_t)dlsym(handle, "QEMUReader_setnoStats");
  dyn_QEMUReader_resumeThreadGPU = (dyn_QEMUReader_resumeThreadGPU_t)dlsym(handle, "QEMUReader_resumeThreadGPU");
  dyn_QEMUReader_resumeThread    = (dyn_QEMUReader_resumeThread_t)dlsym(handle, "QEMUReader_resumeThread");
  dyn_QEMUReader_pauseThread     = (dyn_QEMUReader_pauseThread_t)dlsym(handle, "QEMUReader_pauseThread");
  dyn_QEMUReader_queue_inst      = (dyn_QEMUReader_queue_inst_t)dlsym(handle, "QEMUReader_queue_inst");
  dyn_QEMUReader_syscall         = (dyn_QEMUReader_syscall_t)dlsym(handle, "QEMUReader_syscall");
  dyn_QEMUReader_finish          = (dyn_QEMUReader_finish_t)dlsym(handle, "QEMUReader_finish");
  dyn_QEMUReader_setFlowCmd      = (dyn_QEMUReader_setFlowCmd_t)dlsym(handle, "QEMUReader_setFlowCmd");

  printf("Live: Rabbit setup done\n");
}

void unload_rabbit () {
  dlclose(handle);
}

//This function is used for loading ESESC dynamic library
void load_esesc () {
  static bool done = false;
  if (done) {
    //printf("+");
    return;
  }
  done = true;

  //printf("Live: Checkpoint64_t %ld is loading ESESC\n", child_id);

  handle = dlopen("libesescso.so", RTLD_NOW);
  if (!handle) {
    printf("DLOPEN: %s\n", dlerror());
    exit(EXIT_FAILURE);
  }

  //Initializing dynamci functions
  dyn_QEMUReader_getFid          = (dyn_QEMUReader_getFid_t)dlsym(handle, "QEMUReader_getFid");
  dyn_QEMUReader_finish_thread   = (dyn_QEMUReader_finish_thread_t)dlsym(handle, "QEMUReader_finish_thread");
  dyn_QEMUReader_get_time        = (dyn_QEMUReader_get_time_t)dlsym(handle, "QEMUReader_get_time");
  dyn_QEMUReader_setnoStats      = (dyn_QEMUReader_setnoStats_t)dlsym(handle, "QEMUReader_setnoStats");
  dyn_QEMUReader_resumeThreadGPU = (dyn_QEMUReader_resumeThreadGPU_t)dlsym(handle, "QEMUReader_resumeThreadGPU");
  dyn_QEMUReader_resumeThread    = (dyn_QEMUReader_resumeThread_t)dlsym(handle, "QEMUReader_resumeThread");
  dyn_QEMUReader_pauseThread     = (dyn_QEMUReader_pauseThread_t)dlsym(handle, "QEMUReader_pauseThread");
  dyn_QEMUReader_queue_inst      = (dyn_QEMUReader_queue_inst_t)dlsym(handle, "QEMUReader_queue_inst");
  dyn_QEMUReader_syscall         = (dyn_QEMUReader_syscall_t)dlsym(handle, "QEMUReader_syscall");
  dyn_QEMUReader_finish          = (dyn_QEMUReader_finish_t)dlsym(handle, "QEMUReader_finish");
  dyn_QEMUReader_setFlowCmd      = (dyn_QEMUReader_setFlowCmd_t)dlsym(handle, "QEMUReader_setFlowCmd");


  typedef void (*dyn_start_esesc_t)(char *, int, int, int, int, uint64_t *, bool *, uint64_t, int, const char *);
  dyn_start_esesc_t dyn_start_esesc = (dyn_start_esesc_t)dlsym(handle, "start_esesc");
  if (dyn_start_esesc==0) {
    printf("DLOPEN no start_esesc: %s\n", dlerror());
    exit(-3);
  }

  (*dyn_start_esesc)(host_adr, portno, child_id, force_warmup, genwarm, live_warmup_addr, live_warmup_st, live_warmup_cnt, dlc, "esesc.conf");
}

//This function is used to initialize the emulator in order to fork checkpoints
void create_checkpoints (int64_t argc, char **argv) {
  char **qargv = (char **) malloc(argc*sizeof(char **));
  qargv = (char **)malloc(argc*sizeof(char*));
  qargv[0] = (char *) "live";
  for(int64_t j = 1; j < argc; j++) {
    qargv[j] = strdup(argv[j]);
  }

  nchecks = ncheckpoints;
  load_rabbit();
  child_id = 0;
  qemuesesc_main(argc,qargv,0);
}

//This function creates a checkpoint64_t making it ready for simulation
void fork_checkpoint() {
  //find file descriptors being used
  char name[100][1024];
  int fd[100];
  int pos[100];
  int len;
  int fcnt = 0;
  for(int i = 3; i < 100; i++) {
    char lname[100];
    sprintf(lname, "/proc/self/fd/%d", i);
    if ((len = readlink(lname, name[fcnt], 1000)) != -1) {
      string sname = name[fcnt];
      if(sname.compare(0, 6, "socket") != 0) {
        name[fcnt][len] = '\0';
        int val= fcntl(i, F_GETFL, 0);
        int accmode = val & O_ACCMODE;
        if(accmode == 0) { // RDONLY has zero mask
          fd[fcnt] = i;
          pos[fcnt] = lseek(fd[fcnt], 0, SEEK_CUR);
          fcnt ++;
        }
      }
    }
  }

  if (fork() != 0) {
    //Parent: initialize new variables and continue rabbit
    thread_mode = 0;
    inst_count = 0;
    child_id++;
    return;
  }
    
  //Children: create the checkpoint

  //Fill out the live warmup array
  live_warmup_cnt = live_warmup_cache->traverse(live_warmup_addr, live_warmup_st);

  //Send the ready command to NodeJS server
  Transporter::disconnect();
  Transporter::connect_to_server(host_adr, portno);
  Transporter::send_fast("reg_cp", "%d,%d", child_id, getpid());
  thread_mode = 1;
  inst_count = 0;
  bool wait = true;
  int k;

  //Go to waiting mode
  while(wait) {
    Transporter::receive_fast("simulate", "%d,%d,%d,%d", &k, &force_warmup, &genwarm, &dlc);
    if(k == 1)
      exit(0);
    //On simulation request, create a new process to continue simulation and return to waiting mode
    signal(SIGCHLD, SIG_IGN);
    int pid = fork();
    if(pid == 0) {
      //Set the file descriptors to correct position
      for(int i = 0; i < fcnt; i++) {
        close(fd[i]);
        fd[i] = open(name[i], O_RDONLY);
        lseek(fd[i], pos[i], SEEK_SET);
        int foo = lseek(fd[i], 0, SEEK_CUR);
        //printf("resync fd=%d seek=%d %s\n", fd[i], foo, name[i]);
      }
      Transporter::disconnect();
      wait = false;
      thread_mode = 2;
      unload_rabbit();
      load_esesc();
    }
  }
}

//Qemu/ESESC interface functions
extern "C" uint32_t QEMUReader_getFid (uint32_t last_fid) {
  return (*dyn_QEMUReader_getFid)(last_fid);
}

extern "C" void QEMUReader_finish_thread (uint32_t fid) {
  return (*dyn_QEMUReader_finish_thread)(fid);
}

extern "C" uint64_t QEMUReader_get_time () {
  return (*dyn_QEMUReader_get_time)();
}

extern "C" uint32_t QEMUReader_setnoStats () {
  return (*dyn_QEMUReader_setnoStats)();
}

extern "C" FlowID QEMUReader_resumeThreadGPU (FlowID uid) {
  return (*dyn_QEMUReader_resumeThreadGPU)(uid);
}

extern "C" FlowID QEMUReader_resumeThread (FlowID uid, FlowID last_fid) {
  return (*dyn_QEMUReader_resumeThread)(uid, last_fid);
}


extern "C" void QEMUReader_start_roi(uint32_t fid) {
  return (*dyn_QEMUReader_start_roi)(fid);
}


extern "C" void QEMUReader_pauseThread (FlowID id) {
  (*dyn_QEMUReader_pauseThread)(id);
}

extern "C" uint64_t QEMUReader_queue_inst(uint64_t pc, uint64_t addr, uint16_t fid, uint16_t op, uint16_t src1, uint16_t src2, uint16_t dest, void *env) {
  (*dyn_QEMUReader_queue_inst)(pc, addr, fid, op, src1, src2, dest, env);

  foo_inst++;
  //Check for initial skip (if any)
  if(!done_skip) {
    if(inst_count < live_skip) {
      inst_count += 1;
      return 0;
    } else {
      done_skip = true;
      inst_count = nrabbit;
      return 0;
    }
  }

  if(thread_mode == 0) {
    if (nchecks <= 0) {
      exit(0);
    }

    inst_count += 1;
    if(inst_count > nrabbit) {
      nchecks--;
      fork_checkpoint();
    }
  }

  return 0;
}

extern "C" void QEMUReader_syscall (uint32_t num, uint64_t usecs, uint32_t fid) {
  (*dyn_QEMUReader_syscall)(num,usecs,fid);
}

extern "C" void QEMUReader_finish (uint32_t fid) {
  (*dyn_QEMUReader_finish)(fid);
}

extern "C" void QEMUReader_setFlowCmd (bool* flowStatus) {
  (*dyn_QEMUReader_setFlowCmd)(flowStatus);
}

//Main function
int main (int argc, char **argv) {
  char t;
  int64_t params[11];
  int64_t i;
  //Setting Qemu parameters
  char **qparams;
  int64_t qparams_argc= argc - 3;
  qparams = (char **)malloc(sizeof(char *)*qparams_argc+1);
  qparams[0] = strdup("../main/live");
  for(i = 1; i < qparams_argc; i++) {
    qparams[i] = strdup(argv[i + 3]);
  }
  qparams[qparams_argc] = 0;

  //Connecting to the TCP server (Running on Node.js)z
  strcpy(host_adr, argv[2]);
  portno = atoi(argv[3]);
  Transporter::connect_to_server(host_adr, portno);
  Transporter::send_fast("reg_daemon", "%d", getpid());
  Transporter::receive_fast("reg_ack", "");
  printf("Registered on server\n");

  //Waiting for command
  do {
    printf("Waiting for request ... \n");
    Transporter::receive_fast("config", "%d,%ld,%ld", &ncheckpoints, &nrabbit, &live_skip);
    if(fork() == 0) {
      Transporter::disconnect();
      Transporter::connect_to_server(host_adr, portno);
      Transporter::send_fast("reg_conf", "%d", getpid());
      Transporter::receive_fast("reg_ack", "");
      create_checkpoints(qparams_argc, qparams);
    }
  } while(1);
  Transporter::disconnect();
  return 0;
}
