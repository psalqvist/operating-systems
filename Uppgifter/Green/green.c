#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include "green.h"
#include <limits.h>
#include <stdio.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#define FALSE 0
#define TRUE 1
#define _XOPEN_SOURCE 600

#define STACK_SIZE 4096

#define PERIOD 100

static sigset_t block;
int count_timer;
void timer_handler(int);

static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, NULL, FALSE};

static green_t *running = &main_green;

static void init() __attribute__((constructor));

void init() {
  sigemptyset(&block);
  sigaddset(&block, SIGVTALRM);

  struct sigaction act = {0};
  struct timeval interval;
  struct itimerval period;

  act.sa_handler = timer_handler;
  assert(sigaction(SIGVTALRM, &act, NULL) == 0);
  interval.tv_sec = 0;
  interval.tv_usec = PERIOD;
  period.it_interval = interval;
  period.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &period, NULL);
}

// Queue implementation
typedef struct ready_queue {
  green_t *first;
  green_t *last;
}ready_queue;

static ready_queue ready = {NULL, NULL};

static void insert_ready(green_t *susp) {
  if (ready.first != NULL) {
    ready.last->next = susp;
  } else {
    ready.first = susp;
  }
  ready.last = susp;
}

static green_t *release_ready() {
  if(ready.first == NULL) {
    write(1, "deadlock\n", 8);
  }
  assert(ready.first != NULL);
  green_t *temp = ready.first;
  ready.first = temp->next;
  if(temp == NULL) {
    ready.last = NULL;
  }
  temp->next = NULL;
  return temp;
}

void green_statistics() {
  printf("number of timer interupts: %d\n", count_timer);
}


void timer_handler(int sig) {
  //write(1, "time handler\n", 13);
  green_t *susp = running;
  insert_ready(susp);
  green_t *next = release_ready();
  assert(next != NULL);
  count_timer++;
  running = next;
  swapcontext(susp->context, next->context);
}



void green_thread() {
  sigprocmask(SIG_BLOCK, &block, NULL);
  green_t *this = running;
  assert(*this->fun != NULL);
  //printf("green_thread\n");
  void *retval = (*this->fun)(this->arg);

  if (this->join != NULL) {
    insert_ready(this->join);
  }
  this->retval = retval;
  this->zombie = TRUE;
  green_t *next = release_ready();
  assert(next != NULL);
  running = next;
  printf("green_thread\n");
  setcontext(next->context);
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  //printf("green_thread\n");
}

int green_create(green_t *new, void *(*fun)(void*), void *arg) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  ucontext_t *cntx = (ucontext_t *)malloc(sizeof(ucontext_t));
  getcontext(cntx);

  // void *stack = mmap(NULL, STACK_SIZE,
  //                     PROT_READ | PROT_WRITE,
  //                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  void *stack = malloc(STACK_SIZE);

  cntx->uc_link = NULL;
  cntx->uc_stack.ss_sp = stack;
  cntx->uc_stack.ss_size = STACK_SIZE;
  makecontext(cntx, green_thread, 0);

  new->context = cntx;
  new->fun = fun;
  assert(new->fun != NULL);
  new->arg = arg;
  new->next = NULL;
  new->join = NULL;
  new->retval = NULL;
  new->zombie = FALSE;

  insert_ready(new);
  //printf("create\n");
  sigprocmask(SIG_BLOCK, &block, NULL);
  return 0;
}



int green_yield() {
  sigprocmask(SIG_BLOCK, &block, NULL);
  //printf("yield\n");
  green_t *susp = running;
  insert_ready(susp);
  green_t *next = release_ready();
  assert(next != NULL);
  running = next;
  swapcontext(susp->context, next->context);
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

int green_join(green_t *thread, void **retval) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  if(!thread->zombie) {
    green_t *susp = running;
    assert(thread->join == NULL);
    thread->join = susp;
    green_t *next = release_ready();
    assert(next != NULL);
    running = next;
    swapcontext(susp->context, next->context);
  }
  if(retval != NULL) {
    *retval = thread->retval;
    thread->retval = NULL;
  }

  //munmap(thread->context->uc_stack.ss_sp, thread->context->uc_stack.ss_size);
  free(thread->context);
  thread->context = NULL;
  sigprocmask(SIG_BLOCK, &block, NULL);
  return 0;
  //free(thread->context);
}

int green_cond_init(green_cond_t *cond) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  cond->first = NULL;
  cond->last = NULL;
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

green_t *getFirstCond(green_cond_t *cond) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  //printf("getfirst1\n");
  green_t *first = cond->first;
  if(cond->first != NULL) {
    //printf("getfirst2\n");
    cond->first = cond->first->next;
    first->next = NULL;
    //printf("getfirst3\n");
  }
  if(cond->first == NULL) {
    cond->last = NULL;
  }
  sigprocmask(SIG_BLOCK, &block, NULL);
  return first;
}

void green_cond_signal(green_cond_t *cond) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  //printf("Hello1\n");
  green_t *first_susp = getFirstCond(cond);
  //printf("Hello2\n");
  insert_ready(first_susp);
  //printf("Hello3\n");
  sigprocmask(SIG_UNBLOCK, &block, NULL);
}

void green_cond_wait(green_cond_t *cond, green_mutex_t *mutex) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  // unlock
  if(mutex != NULL) {
    mutex->have_lock = NULL;
    mutex->taken = FALSE;
    green_t *susp = mutex->first;
    if(susp != NULL) {
      mutex->first = susp->next;
      if(mutex->first == NULL) {
        mutex->last = NULL;
      }
      susp->next = NULL;
      mutex->have_lock = susp;
      mutex->taken = TRUE;
      insert_ready(susp);
    }
  }
  // wait
  if (cond->last != NULL) {
    cond->last->next = running;
  } else {
    cond->first = running;
  }
  cond->last = running;
  green_t *susp = running;
  green_t *next = release_ready();
  assert(next != NULL);
  running = next;
  swapcontext(susp->context, next->context);
  // when awake, try to take lock
  if(mutex != NULL) {
    green_t *susp = running;
    if(mutex->taken) {
      if (mutex->last != NULL) {
        mutex->last->next = susp;
      } else {
        mutex->first = susp;
      }
      mutex->last = susp;
      green_t *next = release_ready();
      assert(next != NULL);
      running = next;
      swapcontext(susp->context, next->context);
      // control that we wake up having the lock
      assert(mutex->taken == TRUE);
      assert(mutex->have_lock == susp);
    } else {
        mutex->have_lock = running;
        mutex->taken = TRUE;
    }
  }
  sigprocmask(SIG_UNBLOCK, &block, NULL);
}

int green_mutex_init(green_mutex_t *mutex) {
  mutex->taken = FALSE;
  mutex->have_lock = NULL;
  mutex->first = NULL;
  mutex->last = NULL;
  return 0;
}

int green_mutex_lock(green_mutex_t *mutex) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  green_t *susp = running;
  if(mutex->taken) {
    if (mutex->last != NULL) {
      mutex->last->next = susp;
    } else {
      mutex->first = susp;
    }
    mutex->last = susp;
    green_t *next = release_ready();
    assert(next != NULL);
    running = next;
    swapcontext(susp->context, next->context);
    // control that we wake up having the lock
    assert(mutex->taken == TRUE);
    assert(mutex->have_lock == susp);
  } else {
      mutex->have_lock = running;
      mutex->taken = TRUE;
  }
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

int green_mutex_unlock(green_mutex_t *mutex) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  green_t *temp = mutex->first;
  if(temp != NULL) {
    mutex->first = temp->next;
    if(mutex->first == NULL) {
      mutex->last = NULL;
    }
    temp->next = NULL;
    mutex->have_lock = temp;
    mutex->taken = TRUE;
    insert_ready(temp);
  } else {
    mutex->have_lock = NULL;
    mutex->taken = FALSE;
  }
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}
