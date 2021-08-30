#include <stdio.h>
#include "green.h"
#include <unistd.h>
#include <time.h>

int flag = 0;
volatile int count = 0;
green_cond_t cond;
green_mutex_t mutex;

void *test(void *arg) {
  int id = *(int*)arg;
  int loop = 1000000;
  while(loop > 0) {
    green_mutex_lock(&mutex);
    while(flag != id) {
      //printf("thread %d: %d\n", id, loop);
      //green_mutex_unlock(&mutex);
      green_cond_wait(&cond, &mutex);
      //green_mutex_lock(&mutex);
    }
    flag = (id + 1) % 2;
    green_cond_signal(&cond);
    green_mutex_unlock(&mutex);
    loop--;
  }
}

int main() {
  green_cond_init(&cond);
  green_mutex_init(&mutex);
  green_t g0, g1;
  int a0 = 0;
  int a1 = 1;

  struct timespec t_start, t_stop;

  clock_gettime(CLOCK_MONOTONIC_COARSE, &t_start);

  green_create(&g0, test, &a0);
  green_create(&g1, test, &a1);

  green_join(&g0, NULL);
  green_join(&g1, NULL);
  //green_statistics();

  clock_gettime(CLOCK_MONOTONIC_COARSE, &t_stop);

  long wall_sec = t_stop.tv_sec - t_start.tv_sec;
  long wall_nsec = t_stop.tv_nsec - t_start.tv_nsec;
  long wall_msec = (wall_sec *1000) + (wall_nsec / 1000000);

  printf("done in %ld ms\n", wall_msec);
  return 0;
}
