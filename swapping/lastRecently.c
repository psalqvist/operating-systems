// fungerar ej men kan fungera som förklaring kring simulate

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define HIGH 20
#define FREQ 80
#define PAGES 100
#define SAMPLES 20

typedef struct pte {
  int id;
  int present;
  struct pte *next;
  struct pte *prev;
} pte;

void init (int *sequence, int refs, int pages) {

  int high = (int)(pages*((float)HIGH/100));

  for(int i = 0; i<refs; i++) {
    if(rand()%100 < FREQ) {
      // the frequent case
      sequence[i] = rand()%high;
    } else {
      // the less frequent case
      sequence[i] = high + rand()%(pages - high);
    }
  }
}

int simulate(int *seq, pte *table, int refs, int frms, int pgs) {

  int hits = 0;
  int allocated = 0;

  pte *first = NULL;
  pte *last = NULL;

  for(int i = 0; i < refs; i++) {
    int next = seq[i];
    pte *entry = &table[next];

    if(entry->present == 1) {
      //printf("here\n");
      hits++;
      if(entry->next==NULL && entry->prev==NULL) {

      } if(entry->next==NULL) {

      } else if(entry->prev == NULL) {
        entry->next->prev = NULL;
        first = entry->next;
        entry->prev = last;
        entry->next = NULL;
        last = entry;
      } else {
      entry->prev->next = entry->next;
      entry->next->prev = entry->prev;
      entry->next = NULL;
      entry->prev = last;
      last = entry;
    }
    } else {
      if(allocated < frms) {
        allocated++;
        entry->present = 1;
        entry->prev = last;
        last->next = entry;
        entry->next = NULL;
        last = entry;
      } else {
        pte *evict;

        evict = first;
        first = evict->next;

        evict->present = 0;

        entry->present = 1;
        entry->prev = last;
        last->next = entry;
        entry->next = NULL;
        last = entry;
        }
      }
    }
    return hits;
  }


void clear_page_table(pte *page_table, int pages) {
  for (int i=0; i < pages; i++) {
    page_table[i].present = 0;
  }
}

int main(int argc, char *argv[]) {
  // could be command line arguments
  int refs = 100000;
  int pages = 100;

  pte table[PAGES];
  //pte *table = (pte *)malloc(pages*sizeof(pte));

  int *sequence = (int*)malloc(refs*sizeof(int));

  init(sequence, refs, pages);

  // a small experiment to show that it works
  for(int i = 1; i < refs; i++) {
    printf(", %d", sequence[i]);
  }
  printf("\n");

  printf("# This is a benchmark of random replacement\n");
  printf("# %d page references\n", refs);
  printf("# %d pages \n", pages);
  printf("#\n#\n#frames\tratio\n");

  /*frames is the size of the memory in frames*/
  int frames;

  int incr = pages/SAMPLES;

  for(frames = incr; frames <= pages; frames += incr) {
    /* clear page table entries */
    clear_page_table(table, pages);

    int hits = simulate(sequence, table, refs, frames, pages);
    //printf("hits: %d\n",hits);
    //printf("refs: %d\n",refs);
    float ratio = (float)hits/refs;

    printf("%d\t%.2f\n", frames, ratio);
  }

  return 0;
}
