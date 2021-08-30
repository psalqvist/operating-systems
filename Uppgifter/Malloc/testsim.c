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
  size_t size1 = (size_t)request();
  printf("size: %lu\n" ,size1);
  int *memory1;
  memory1 = (int *)(dalloc(size1));
  printf("memory1: %p\n" ,memory1);
  sanity();

  size_t size2 = (size_t)request();
  printf("size: %lu\n" ,size2);
  int *memory2;
  memory2 = (int *)(dalloc(size2));
  printf("memory2: %p\n" ,memory2);
  sanity();

  size_t size3 = (size_t)request();
  printf("size: %lu\n" ,size3);
  int *memory3;
  memory3 = (int *)(dalloc(size3));
  printf("memory3: %p\n" ,memory3);
  sanity();

  size_t size4 = (size_t)request();
  printf("size: %lu\n" ,size4);
  int *memory4;
  memory4 = (int *)(dalloc(size4));
  printf("memory4: %p\n" ,memory4);
  sanity();

  dfree(memory2);
  printf("en free\n");
  sanity();
  dfree(memory1);
  printf("två free\n");
  sanity();
  dfree(memory3);
  printf("tre free\n");
  sanity();
  dfree(memory4);
  printf("fyra free\n");
  sanity();
  return 0;

}
