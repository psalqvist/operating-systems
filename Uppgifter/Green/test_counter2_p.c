#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

volatile int count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *test(void *arg) {
  int loop = 1000000;
  while(loop > 0) {
    count++;
    loop--;
  }
}

int main() {
  pthread_mutex_init(&mutex, NULL);
  pthread_t g0, g1;

  struct timespec t_start, t_stop;

  clock_gettime(CLOCK_MONOTONIC_COARSE, &t_start);

  pthread_create(&g0, NULL, test, NULL);
  pthread_create(&g1, NULL, test, NULL);

  pthread_join(g0, NULL);
  pthread_join(g1, NULL);

  printf("count: %d\n", count);

  clock_gettime(CLOCK_MONOTONIC_COARSE, &t_stop);

  long wall_sec = t_stop.tv_sec - t_start.tv_sec;
  long wall_nsec = t_stop.tv_nsec - t_start.tv_nsec;
  long wall_msec = (wall_sec *1000) + (wall_nsec / 1000000);

  printf("done in %ld ms\n", wall_msec);

  return 0;
}
