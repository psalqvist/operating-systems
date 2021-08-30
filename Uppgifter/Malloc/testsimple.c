#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "rand.h"
#include "dlmall.h"

#define ROUNDS 1000
#define LOOP 5
#define BUFFER 100

int main() {
  int i=0;
  while(i<LOOP) {
      size_t size = (size_t)request();
      printf("size: %lu\n" ,size);
      int *memory;
      memory = (int *)(dalloc(size));
      dfree(memory);
      i++;
  }
  sanity();
  return 0;

}
