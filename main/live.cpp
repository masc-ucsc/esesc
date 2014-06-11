/*
ESESC: Enhanced Super ESCalar simulator
Copyright (C) 2009 University of California, Santa Cruz.

Contributed by Jose Renau
Gabriel Southern

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
#include <netinet/in.h>
#include <netdb.h> 

extern "C" void QEMUReader_goto_sleep(void *env);
extern "C" void QEMUReader_wakeup_from_sleep(void *env);
extern "C" int qemuesesc_main(int argc, char **argv, char **envp);
extern "C" void esesc_set_timing(uint32_t fid);

//Global variables
typedef uint32_t FlowID; // from RAWDInst.h
void *handle;
int checkpoint = 0;
int64_t inst_count = 0;
bool is_parent = true;

//Live ESESC Parameters
const int64_t nrabbit = 100000000;
const int ncheckpoints = 10;

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

typedef void (*dyn_QEMUReader_queue_inst_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, void *);
dyn_QEMUReader_queue_inst_t dyn_QEMUReader_queue_inst=0;

typedef void (*dyn_QEMUReader_syscall_t)(uint32_t, uint64_t, uint32_t);
dyn_QEMUReader_syscall_t dyn_QEMUReader_syscall=0;

typedef void (*dyn_QEMUReader_finish_t)(uint32_t);
dyn_QEMUReader_finish_t dyn_QEMUReader_finish=0;

typedef void (*dyn_QEMUReader_setFlowCmd_t)(bool*);
dyn_QEMUReader_setFlowCmd_t dyn_QEMUReader_setFlowCmd=0;


void start_qemu(int argc, char **argv) 
{
  char **qargv = (char **) malloc(argc*sizeof(char **));

  qargv = (char **)malloc(argc*sizeof(char*));
  qargv[0] = (char *) "live";
  for(int j = 1; j < argc; j++) {
    qargv[j] = strdup(argv[j]);
  }

  // Change this to a pthread_create like in QEMUReader::start
  qemuesesc_main(argc,qargv,0);
}


void load_rabbit()
{
  printf("Loading rabbit...\n");
  handle = dlopen("librabbitso.so", RTLD_NOW);
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

  checkpoint++;
  printf("rabbit setup done\n");
}


void load_esesc()
{

  static bool done = false;
  if (done) {
    printf("+");
    return;
  }
  done = true;

  printf("Loading esesc...\n");

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

  checkpoint++;

  typedef void (*dyn_start_esesc_t)();
  dyn_start_esesc_t dyn_start_esesc = (dyn_start_esesc_t)dlsym(handle, "start_esesc");
  if (dyn_start_esesc==0) {
    printf("DLOPEN no start_esesc: %s\n", dlerror());
    exit(-3);
  }
  printf("esesc setup done\n");

  (*dyn_start_esesc)(); // FIXME: start_esesc should be a separate thread (qemu thread should wait until this is finished)
}

void unload_rabbit()
{
  dlclose(handle);
}


extern "C" uint32_t QEMUReader_getFid(uint32_t last_fid)
{
  return (*dyn_QEMUReader_getFid)(last_fid);
}

extern "C" void QEMUReader_finish_thread(uint32_t fid)
{
  return (*dyn_QEMUReader_finish_thread)(fid);
}

extern "C" uint64_t QEMUReader_get_time()
{
  return (*dyn_QEMUReader_get_time)();
}

extern "C" uint32_t QEMUReader_setnoStats()
{
  return (*dyn_QEMUReader_setnoStats)();
}

extern "C" FlowID QEMUReader_resumeThreadGPU(FlowID uid) 
{
  return (*dyn_QEMUReader_resumeThreadGPU)(uid);
}

extern "C" FlowID QEMUReader_resumeThread(FlowID uid, FlowID last_fid) 
{
  return (*dyn_QEMUReader_resumeThread)(uid, last_fid);
}

extern "C" void QEMUReader_pauseThread(FlowID id) 
{
  (*dyn_QEMUReader_pauseThread)(id);
}

extern "C" void QEMUReader_queue_inst(uint32_t insn, uint32_t pc, uint32_t addr, uint32_t fid, uint32_t op, uint64_t icount, void *env) 
{
  static int nchecks = ncheckpoints;
  (*dyn_QEMUReader_queue_inst)(insn,pc,addr,fid,op,icount,env);
  if(is_parent) {
      inst_count += icount;
    if(inst_count > nrabbit) {
      nchecks--;
      if (nchecks<=0) {
        printf("Master Rabbit done\n");
        exit(0);
      }
      if (fork() != 0) {
        // parent
        is_parent = true;
        inst_count = 0;
      }else{
        // children
        is_parent = false;
        inst_count = 0; // FIXME
        unload_rabbit();
        load_esesc();
      }
    }
  }
}

extern "C" void QEMUReader_syscall(uint32_t num, uint64_t usecs, uint32_t fid)
{
  (*dyn_QEMUReader_syscall)(num,usecs,fid);
}

extern "C" void QEMUReader_finish(uint32_t fid)
{
  (*dyn_QEMUReader_finish)(fid);
}

extern "C" void QEMUReader_setFlowCmd (bool* flowStatus)
{
  (*dyn_QEMUReader_setFlowCmd)(flowStatus);
}

int main(int argc, char **argv)
{
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[256];

  //Qemu Parameters
  char **qparams;
  int qparams_argc= 7;
  qparams = (char **)malloc(sizeof(char *)*qparams_argc+1);
  qparams[0] = strdup("live");
  qparams[1] = strdup("run/launcher");
  qparams[2] = strdup("--");
  qparams[3] = strdup("stdin");
  qparams[4] = strdup("run/crafty.input");
  qparams[5] = strdup("--");
  qparams[6] = strdup("crafty");
  qparams[7] = 0;

  //Pre-loading Rabbit
  load_rabbit();
  
  //Connecting to the TCP server (Running on Node.js)
  portno = atoi("1111");
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
      perror("ERROR opening socket");
  server = gethostbyname("localhost");
  if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
       (char *)&serv_addr.sin_addr.s_addr,
       server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
      perror("ERROR connecting");

  //Registering as esesc daemon
  bzero(buffer,256);
  strncpy(buffer, "esesc_daemon_register", 22);
  n = write(sockfd,buffer,strlen(buffer));
  if (n < 0) 
         perror("ERROR writing to socket");
  bzero(buffer,256);
  n = read(sockfd,buffer,255);
  if (n < 0) 
    perror("ERROR reading from socket");
  if(strcmp(buffer,"esesc_daemon_registered"))
    perror("Daemon not registered");
  else
    printf("ESESC Daemon Registered ... \n");
  bzero(buffer,256);

  //Waiting for service
  do {
    printf("Waiting for request ... \n");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
       perror("ERROR reading from socket");
    //if(strcmp(buffer,"run"))
    //{
      printf("Starting simulation ... \n");
      start_qemu(qparams_argc, qparams);
    //}
  } while(1);

  close(sockfd);
  return 0;

  //load_rabbit();
  // new interface to say start and for how many instructions (checked by qemureader_queue_inst)
  // for (1..n checkpoints) 
  //   run first nB instruction 
  //   do fork
  //   child:
  //     set live_queue_inst = 0
  //     unload qemuin.so
  //     load esesc.so (it should not spawn qmeu again)
  //     set the QEMUReader::started = true to avoid respawning qemu
  //     set all the live_* methods
  //     BootLoader::plug(argc, argv);
  //     call Report::live (delete report file descriptor, and create a socket to nodejs)
  //     set live_queue_inst = dlsum(...
  //     BootLoader::boot();

  //esesc_set_timing(0);
  //start_qemu(2, qparams);


  // Change Report::field so that instead of file, it also can report live (to nodejs) if Report::live() was called before 
  //

}
