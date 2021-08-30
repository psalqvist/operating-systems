#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>

#define ITERATIONS 10
#define BURST 1000

int main() {
  int descr[2];

  // Using pipe on an array arr, we will write to arr[1] and read from arr[0]
  assert(0 == pipe(descr));


  // We use fork() for the control flow
  // The parent will first write and wait, and the child will then read
  if(fork() == 0) {
    //consumer
    for(int i = 0; i < ITERATIONS; i++) {
      for(int j = 0; j < BURST; j++) {
        int buffer[10];
        read(descr[0], &buffer, 10);
      }
      sleep(1);
    }
    printf("consumer done\n");
    return 0;
  }
  // producer
  for(int i = 0; i < ITERATIONS; i++) {
    for(int j = 0; j < BURST; j++) {
      write(descr[1], "0123456789", 10);
    }
    printf("producer burst %d done\n", i);
  }
  printf("producer done\n");

  wait(NULL);
  printf("all done\n");
}
