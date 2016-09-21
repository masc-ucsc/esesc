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
#include <linux/stat.h>   
#include <sys/syscall.h>
#include <atomic>

//#include "crypto++/sha.h" 
#include "Transporter.h" 
#include "LiveCache.h"

#define WARMUP_ARRAY_SIZE 262144
typedef uint32_t FlowID; // from RAWDInst.h

#define MAX_HOST_ADDR_LEN 100
#define MAX_CONF_NAME_LEN 100
#define MAX_BENCH_NAME_LEN 100
#define MAX_FIFO_BASE_PATH_LEN 256

//#define DOCKER_ID_LEN 65


struct LiveSetupState {
  char *cont_port;
  char *cont_host;

  void *handle;
  int cpid, portno;
  char host_adr[MAX_HOST_ADDR_LEN];

  char bench_id[MAX_BENCH_NAME_LEN];
  char fifo_base_path[MAX_FIFO_BASE_PATH_LEN];

  int64_t nrabbit;
  int nchecks;

  bool simulating;
  bool roi_only;

  char esesc_conf[MAX_CONF_NAME_LEN];

  LiveCache *live_warmup_cache;
  uint64_t live_warmup_addr[WARMUP_ARRAY_SIZE];
  bool live_warmup_st[WARMUP_ARRAY_SIZE];

  pthread_mutex_t fork_cp_lock;
};

// Global State required for QEMU reader functions that have fixed interface
LiveSetupState gs;

// Needs to a global because of use in rabbitso.cpp
LiveCache *live_warmup_cache;
//std::atomic<bool> global_in_roi = false;
volatile bool global_in_roi = false;

//XXX: checking to see if making inst_count a global instead of static function
//changes anything (fixes segfault).
static std::atomic_long inst_count(0);

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

void get_docker_id(char *buf) {
 
  // Shell command to read the docker ID of the running container. The
  // docker ID is 64 characters long. An additional character is needed to
  // store the `\0` at the end of the string
  static const char *cmd = "cat /proc/self/cgroup | grep docker | sed 's/^.*\\///' | head -n1";

  FILE *p = popen(cmd, "r");
  char *rv = fgets(buf, DOCKER_ID_LEN, p);
  int rc = pclose(p);

  I(rv);
  I(rc == 0);
}

typedef void (*dyn_QEMUReader_start_roi_t)(uint32_t);
dyn_QEMUReader_start_roi_t dyn_QEMUReader_start_roi=0;

// Send message to criu controller on localhost
void msg_criucont(const char *buf, size_t bytes, const char *host, const char *port) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int criusock;
  int rv;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
 
  rv =  getaddrinfo(host, port, &hints, &result);
  if (rv != 0) {
    fprintf(stderr, "Connection to criu controller failed: %s\n", gai_strerror(rv));
    exit(-1);
  }

  for(rp = result; rp != NULL; rp = rp->ai_next) {
    criusock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (criusock == -1)
      continue;
    if (connect(criusock, rp->ai_addr, rp->ai_addrlen) != -1)
      break;
    close(criusock);
  }

  if(rp == NULL) {
    fprintf(stderr, "Connection to criu controller failed\n");
    exit(-1);
  }

  freeaddrinfo(result);

  ssize_t total_bytes_written = 0;
  while(total_bytes_written != bytes) {
    I(total_bytes_written < bytes);
    ssize_t bytes_written = write(criusock, 
                                  &buf[total_bytes_written],
                                  bytes - total_bytes_written);
    if(bytes_written == -1) {
      fprintf(stderr,"Error sending message to criu controller\n");
      exit(-1);
    }
    total_bytes_written += bytes_written;
  }

  close(criusock);

}

char *wait_fifo(char *readbuf, int bufsize) {
  char fifo_file[MAX_FIFO_BASE_PATH_LEN+MAX_BENCH_NAME_LEN+16];

  sprintf(fifo_file, "%s/%s/cp%d", gs.fifo_base_path, gs.bench_id, gs.cpid);

  FILE *fp;
  fp = fopen(fifo_file,"r");
  if(fp == NULL) {
    int rc = mkfifo(fifo_file, 0666);
    if(rc) {
      perror("Could not create fifo");
      exit(-1);
    }
    else {
      fp = fopen(fifo_file,"r");
      if(fp == NULL) {
        perror("Could not open fifo");
      }
    }
    //fprintf(stderr, "Error: could not open fifo: %s", fifo_file);
    //exit(-1);
  }
  char *rv = NULL;
  int fifo_count = 0;
  do {
    rv = fgets(readbuf, bufsize, fp);
    if(rv == NULL) {
      fprintf(stderr, "WARNING reading: '%s' returned NULL %d times\t errno: '%m'\n", fifo_file, ++fifo_count);
      sleep(1);
    }
  } while(rv == NULL); 

  fprintf(stderr,"WAIT_FIFO: read '%s' from '%s'\n",readbuf,fifo_file);

  fclose(fp);
  return rv;
}

void init_QEMUReader_fns(void *handle) {
  dlerror();
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
  dyn_QEMUReader_start_roi       = (dyn_QEMUReader_start_roi_t)dlsym(handle, "QEMUReader_start_roi");

  char *err = dlerror();
  if(err) {
    fprintf(stderr, "Error loading QEMUReader functions: '%s'\n", err);
    exit(-1);
  }
}


//This function loads the master rabbit dynamic library
void load_rabbit () {

  live_warmup_cache = new LiveCache();
  gs.live_warmup_cache = live_warmup_cache;

  gs.handle = dlopen("librabbitso.so", RTLD_NOW);
  if (!gs.handle) {
    fprintf(stderr,"DLOPEN: %s\n", dlerror());
    exit(EXIT_FAILURE);
  }

  init_QEMUReader_fns(gs.handle);
}

void start_simulation() {

  fprintf(stderr,"Called start_simulation()\n"); 
  
  
  // Fill out the live warmup array?
  uint64_t live_warmup_cnt;
  
  //TODO: Need to make LiveCache work with multithreaded apps
  //For now if simulating 'roi_only' then don't traverse live cache because 
  //it crashes.
  if(!gs.roi_only) {
    fprintf(stderr,"live_warmup_cache: %p\n",gs.live_warmup_cache); // Debugging code
    live_warmup_cnt = gs.live_warmup_cache->traverse(gs.live_warmup_addr, gs.live_warmup_st);
    fprintf(stderr,"live_warmup_cnt: %d\n",live_warmup_cnt);  // Debugging code
  }

  pid_t pid = getpid();

  fprintf(stderr,"Connecting to: %s:%d\n",gs.host_adr, gs.portno);
  Transporter::connect_to_server(gs.host_adr, gs.portno);
  Transporter::send_fast("pid", "%d", pid);

   
  dlclose(gs.handle);
  gs.handle = dlopen("libesescso.so", RTLD_NOW);
  if (!gs.handle) {
    fprintf(stderr,"DLOPEN: %s\n", dlerror());
    exit(EXIT_FAILURE);
  }

  init_QEMUReader_fns(gs.handle);

  typedef void (*dyn_start_esesc_t)(char *, int, int, int, int, uint64_t *, bool *, uint64_t, int, const char *);
  dyn_start_esesc_t dyn_start_esesc = (dyn_start_esesc_t)dlsym(gs.handle, "start_esesc");
  if (dyn_start_esesc == NULL) {
    fprintf(stderr,"DLOPEN no start_esesc: %s\n", dlerror());
    exit(-3);
  }

  int force_warmup, genwarm, dlc; 
  
  //TODO: check why variable names/order below differ from that in compute_node.js
  //Transporter::receive_fast("simulate", "%d,%d,%d", &force_warmup, &genwarm, &dlc); 

  //TODO: actually receive warmup data from server
  force_warmup = 2;
  genwarm = 0;
  dlc = 1;
  
  (*dyn_start_esesc)(gs.host_adr,
                     gs.portno,
                     gs.cpid,
                     force_warmup,
                     genwarm,
                     gs.live_warmup_addr,
                     gs.live_warmup_st,
                     live_warmup_cnt,
                     dlc,
                     gs.esesc_conf
                     );

}

void fork_checkpoint(uint16_t fid) {

  pid_t tid = syscall(SYS_gettid); // DEBUGGING: remove when done
  fprintf(stderr,"Calling fork_checkpoint() fid: %d\t tid: %d\n", fid, tid);
  I(gs.cont_host);
  I(gs.cont_port);

  char msgbuf[MAX_BENCH_NAME_LEN+100];

  int len = sprintf(msgbuf,"%s:READY",gs.bench_id);

  msg_criucont(msgbuf, len, gs.cont_host, gs.cont_port);
   
  const int FIFO_SIZE = 80;
  char fifo_command[FIFO_SIZE];
  char *fifo_val = wait_fifo(fifo_command, FIFO_SIZE);
  
  if(strcmp(fifo_val,"CREATE_CP") == 0) {
    return;
  } 
  else if(strcmp(fifo_val,"SIMULATE") == 0) {
    gs.simulating = true;
    start_simulation();
  } 
  else {
    fprintf(stderr,"Received message: '%s' ", fifo_val);
    fprintf(stderr,"Exiting ESESC\n");
    exit(0);
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
  fprintf(stderr,"Livecriu: start_roi\n");
  return (*dyn_QEMUReader_start_roi)(fid);
}


extern "C" void QEMUReader_pauseThread (FlowID id) {
  (*dyn_QEMUReader_pauseThread)(id);
}

extern "C" uint64_t QEMUReader_queue_inst(uint64_t pc, uint64_t addr, uint16_t fid, uint16_t op,
    uint16_t src1, uint16_t src2, uint16_t dest, void *env) {

  // XXX: Note using the return value from the dyn_QEMUReader_queue_inst function may have caused
  // a segfault when running in debug mode.  This may have been because of stackoverflow
  // (the coredump stacktrace was unclear).  Perhaps reading the return value inhibits certain
  // compiler optimizations. In either case this is something that it maybe worth refactoring at a later time.
  (*dyn_QEMUReader_queue_inst)(pc, addr, fid, op, src1, src2, dest, env);

  if(!gs.simulating) {
    

    if(gs.roi_only && !global_in_roi) {
      return 0;
    }

    inst_count++; 
    if(inst_count > gs.nrabbit) {
      pthread_mutex_lock(&gs.fork_cp_lock); //LOCK

      // Only fork one checkpoit per inst count interval. If the other
      // thread set inst_count to 0 then it will be less than nsrabbit
      // when this thread aquires the lock. So thread should return rather
      // than forking a checkpoint
      if(inst_count < gs.nrabbit) {
        pthread_mutex_unlock(&gs.fork_cp_lock); //UNLOCK for early exit
        return 0;
      }
      
      if (gs.nchecks <= 0) {
        fprintf(stderr,"Exiting 0 checks left\n");
        pthread_mutex_unlock(&gs.fork_cp_lock); //UNLOCK for early exit
        exit(0);
      }

      inst_count = 0;
      write(2,"[\n",2);  // Debugging code to check for race
      fork_checkpoint(fid);
      write(2,"]\n",2);  // Debugging code to check for race
      gs.cpid++;
      gs.nchecks--;
      pthread_mutex_unlock(&gs.fork_cp_lock); //UNLOCK
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


int main (int argc, char **argv) {
  const int NUM_LIVE_ARGS = 12;

  fprintf(stderr,"Starting ESESC Livecriu test1\n");
  //raise(SIGSTOP);

  // TODO: Note that benchmark arguments are specified in two places.  Here on the command line
  // and also in the esesc.conf file.  Possibly see if there is a good way to eliminate this
  // duplication 
  if(argc < NUM_LIVE_ARGS + 1) { 
    printf("Usage: live <id> <controller host> <controller port> <host_adr> <server port_number> <num_cp> <cp_interval> "
        "<roi_only> <esesc_conf_file> <benchmark name> <fifo base path> <benchmark binary> [benchmark_args] \n");
    exit(-1);
  }

  // TODO: improve argument parsing
  gs.cont_host = argv[2];
  gs.cont_port = argv[3];
  strncpy(gs.host_adr, argv[4], MAX_HOST_ADDR_LEN);
  gs.portno = atoi(argv[5]);
  gs.nchecks = atoi(argv[6]);
  gs.nrabbit = atoi(argv[7]);
  gs.roi_only = static_cast<bool>(atoi(argv[8]));
  strncpy(gs.esesc_conf, argv[9], MAX_CONF_NAME_LEN);
  strncpy(gs.bench_id, argv[10], MAX_BENCH_NAME_LEN);
  strncpy(gs.fifo_base_path, argv[11], MAX_FIFO_BASE_PATH_LEN);
  gs.cpid = 0;
  gs.simulating = false;

  //inst_count = 0;
  pthread_mutex_init(&gs.fork_cp_lock, NULL);

  int qargc = argc - (NUM_LIVE_ARGS - 1);
  char **qargv = static_cast<char **>(malloc(qargc * sizeof(char *)));
  qargv[0] = strdup("live");
  for(int i = NUM_LIVE_ARGS; i < argc; i++) {
    qargv[i - (NUM_LIVE_ARGS - 1)] = strdup(argv[i]);
    fprintf(stderr,"arg[%d]: '%s'\n",i - (NUM_LIVE_ARGS-1), argv[i]);
  }
  fprintf(stderr,"Starting ESESC Livecriu test2\n");
  if(gs.roi_only) {
    fprintf(stderr,"ROI only measurement mode\n");
  }

  load_rabbit();
  qemuesesc_main(qargc,qargv,0);

}
