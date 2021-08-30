#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "rand.h"
#include "dlmall.h"
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>

#define ROUNDS 1
#define LOOP 1000000
#define BUFFER 100

int main() {
  double time;

  void *buffer[BUFFER];
  for(int i = 0; i<BUFFER; i++) {
    buffer[i] = NULL;
  }


  for(int j = 0; j < ROUNDS; j++) {
    clock_t t;
    t = clock();
    for(int i=0; i < LOOP ; i++) {
      int index = rand() % BUFFER;
      if(buffer[index] != NULL) {
        //printf("free\n");
        dfree(buffer[index]);
        buffer[index] = NULL;
      }
      size_t size = (size_t)request();
      int *memory;
      memory = (int *)(dalloc(size));

      if(memory==NULL) {
        fprintf(stderr, "memory allocation failed\n");
        return(1);
      }
      buffer[index] = memory;
      /* writing to the memory so we know it exists */
      *memory = 123;
      //printf("i: %d\n", i);
    }
    t=clock() - t;
    time = ((double)t)/CLOCKS_PER_SEC;
    //kill();
    //printf("time: %f\n", time);


  }
  int merge1 = getmerge1();
  printf("merge1 \t%d\n", merge1);
  int merge2 = getmerge2();
  printf("merge2 \t%d\n", merge2);
  int merge3 = getmerge3();
  printf("merge3 \t%d\n", merge3);
  //sanity();
  return 0;
}
