
#define ASIZE 32768

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>


void worker(void *ptr )
{
	int src[ASIZE];
	int dst[ASIZE];

	int id = (int)ptr;

	int i;
	for(i=0;i<ASIZE;i++) {
		dst[i] = src[i];
	}

	printf("%d thread done\n",id);
	pthread_exit(0);
}


int main(int argc, char **argv) {

  int nthreads = 0;
  if (argc==2) {
    nthreads = atoi(argv[1]);
  }else{
    printf("Usage:\n\t%s [num_threads]\n",argv[0]);
    exit(-1);
  }
  printf("starting %d threads...\n",nthreads);

  int i;
  pthread_t thr[nthreads];

  for(i=1;i<nthreads;i++) {
	  pthread_create(&thr[i], NULL, (void *) &worker, (void *) i);
  }
  worker(0);
  for(i=1;i<nthreads;i++) {
	  pthread_join(thr[i],0);
  }

}
