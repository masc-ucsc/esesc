/* The Computer Language Benchmarks Game
 * http://benchmarksgame.alioth.debian.org/

* contributed by Premysl Hruby
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>

#define MAX_THREADS (503)
#define ASIZE  (32768)


struct stack {
   char x[PTHREAD_STACK_MIN];
};


/* staticaly initialize mutex[0] mutex */
static pthread_mutex_t mutex[MAX_THREADS]; 
static int data[MAX_THREADS];
static struct stack stacks[MAX_THREADS];
/* stacks must be defined staticaly, or my i386 box run of virtual memory for this
 * process while creating thread +- #400 */


int shared_rd[ASIZE];
int shared_rdwr[ASIZE];

int num_threads;

static void* thread(void *num)
{
   int l = (int)num;
   int r = (l+1) % num_threads;
   int token;

   while(1) {
      pthread_mutex_lock(mutex + l);
      token = data[l];
      if (token) {
         int i;
         fprintf(stderr,"start s%d...\n",l);
         for(i=0;i<ASIZE;i++) {
		 shared_rdwr[i] += shared_rd[i] + token;
         }
         data[r] = token - 1;
         fprintf(stderr,"...s%d done\n",l);
         pthread_mutex_unlock(mutex + r);
      } else {
         printf("%i\n", l+1);
         exit(0);
      }
   }
}

int main(int argc, char **argv)
{
   int i;
   pthread_t cthread;
   pthread_attr_t stack_attr;

   if (argc != 3) {
      printf("Usage:\n%s [num_threads] [pings]\n",argv[0]);
      exit(255);
   }
   num_threads = atoi(argv[1]);
   data[0] = atoi(argv[2]);

   for(i=0;i<ASIZE;i++) {
      shared_rdwr[i] = i;
      shared_rd[i] = i;
   }

   pthread_attr_init(&stack_attr);
      
   for (i = 0; i < num_threads; i++) {
      pthread_mutex_init(mutex + i, NULL);
      pthread_mutex_lock(mutex + i);

      pthread_attr_setstack(&stack_attr, &stacks[i], sizeof(struct stack)); 
      pthread_create(&cthread, &stack_attr, thread, (void*)i);
   }

   pthread_mutex_unlock(mutex + 0);
   pthread_join(cthread, NULL);   
}
