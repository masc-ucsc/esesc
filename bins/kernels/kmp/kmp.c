/* Code from:
 *  Handbook of Algorithms and Data Structures 
 *
 *               Gaston H. Gonnet 
 *                  Informatik, ETH Zürich
 *
 *               Ricardo Baeza-Yates 
 *                  Dept. of Computer Science, Univ. of Chile
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "rndnums.h"

int  *next;

// #define SCOORE

void preprocpat(char *pat,int next[])
{
  int i, j;
  i = 0;
  j = next[0] = -1;
  do{
    if(j == (-1) || pat[i] == pat[j]) {
      i++;
      j++;
      next[i] = (pat[j] == pat[i]) ? next[j] : j;
    }else{
      j = next[j];
    }
  }while (pat[i] != 0);
}

char *search(char *pat, char *text)
{
  int j;

  if (*pat == 0)
    return (text);

  preprocpat(pat, next);

  for(j = 0; *text != 0;) {
    if(j == (-1) || pat[j] == *text) {
      text++;
      j++;
      if (pat[j] == 0)
	return (text - j);
    }else{
      j = next[j];
    }
  }

  return (NULL);
}

char *buffer;
char *pat;

int main(int argc, char **argv)
{
  int i,j;
  int lpat,lbuffer;
  MILL *rand;

#ifdef SCOORE
    lbuffer = 8*1024+17;
    lpat    = 4;
    printf("KMP with test input set\n");
#else
  if( argc < 2 ){
    printf("Usage:\n\t%s <ref|train|test>\n",argv[0]);
    exit(0);
  }

  if( strcasecmp(argv[1],"ref") == 0 ) {
    lbuffer = 512*1024+17;
    lpat    = 11;
    printf("KMP with reference input set\n");
  }else if( strcasecmp(argv[1],"train") == 0 ){
    lbuffer = 96*1024+17;
    lpat    = 5;
    printf("KMP with test input set\n");
  }else if( strcasecmp(argv[1],"test") == 0 ){
    lbuffer = 32*1024+17;
    lpat    = 4;
    printf("KMP with test input set\n");
  }else{
    printf("Invalid data set use ref or train or test\n");
    exit(-1);
  }
#endif

  buffer = (char *)malloc(lbuffer);
  if( !buffer ){
    fprintf(stderr,"Not enough memory\n");
    exit(0);
  }

  rand = init_mill(0xf621,0x3128,0x8253);

  pat  = (char *)malloc(lpat+1);
  next = (int *)malloc(sizeof(int)*lpat);
  if( !pat || !next ){
    fprintf(stderr,"Not enough memory\n");
    exit(0);
  }

  fprintf(stderr,"Benchmark begin...\n");

  for(i=0;i<lbuffer;i++){
    buffer[i] = rndnum(rand) % 7 + 'a';
  }

  buffer[lbuffer-1]=0; /* end of buffer */

  for(j=0;j<23*17;j++) {
    char *pos;
    int conta;
    
    printf("%2d Pattern:",j+1);
    for(i=0;i<lpat;i++){
      pat[i] = rndnum(rand) % 7 + 'a';
      printf("%c",pat[i]);
    }
    pat[lpat]=0;

    /* Shorten lenght of buffer */
    buffer[3*lbuffer/(j % 3+3)-1]=0;

    conta = 0;
    pos = &buffer[(3*lbuffer/(j % 3+3)-1)/4];
    while(1){
      pos = search(pat,pos);
      if( pos == 0 )
	break;
      pos++;
      conta++;
    }

    printf(": found %d times\n",conta);

    buffer[3*lbuffer/(j % 3+3)-1]= 'a';
  }
  
  printf("Benchmark finish...\n");
  fflush(stdout);

  nuke_mill(rand);

  return 0;
}
