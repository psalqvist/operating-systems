#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

struct head {
uint16_t bfree;
uint16_t bsize;
uint16_t free;
uint16_t size;
struct head *next;
struct head *prev;
};

#define TRUE 1
#define FALSE 0
#define HEAD  (sizeof(struct head))
#define MINSIZE 8
#define MIN(size) (((size)>(8))?(size):(8))
#define LIMIT(size) (MIN(0)+HEAD+size)
#define MAGIC(memory) ((struct head*)memory - 1)
#define HIDE(block) (void*)((struct head*)block + 1)
// Memory aligned by 8 bytes, meaning that we will store the object
// in a memory adress that is a multiple of 8
#define ALIGN 8
// setting the heap to 64 Ki bytes initially
#define ARENA (64*1024)

int free = 0;
int mergecount = 0;
int merge1 = 0;
int merge2 = 0;
int merge3 = 0;

int getfree() {
  return free;
}

int getmerge() {
  return mergecount;
}

int getmerge1() {
  return merge1;
}

int getmerge2() {
  return merge2;
}

int getmerge3() {
  return merge3;
}


struct head *after(struct head *block) {
  int size=block->size;
  return (struct head*)((char*)block+HEAD+size);
}

struct head *before(struct head *block) {
  int bsize=block->bsize;
  return (struct head*)((char*)block-bsize-HEAD);
}

struct head *split(struct head *block, int size) {
int blocksize = block->size;
int rsize = blocksize - size - HEAD;
block->size= rsize;

struct head *splt = (struct head*)((char*)block + HEAD + rsize);
splt->bsize = rsize;
splt->bfree = TRUE;
splt->size = size;
splt->free = FALSE;
struct head *aft = (struct head*)((char*)splt + HEAD + size);
aft->bsize = size;
aft->bfree = FALSE;
// Vill man ha tillbaka pekaren till HEAD eller data?
//printf("splt: %p\n",splt);
return splt;
}

struct head *arena = NULL;

struct head *new() {
  if(arena != NULL) {
    printf("one arena already allocated \n");
    return NULL;
  }

  // using mmap, could use sbrk()
  struct head *new = mmap(NULL, ARENA,
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(new == MAP_FAILED) {
    printf("mmap failed: error %d\n", errno);
    return NULL;
  }

  /*make room for dummy var*/
  unsigned size = ARENA - 2*HEAD;

  new->bfree = FALSE;
  new->bsize = 0;
  new->free = TRUE;
  new->size = size;

  struct head *sentinel = after(new);

  sentinel->bfree = TRUE;
  sentinel->bsize = size;
  sentinel->free = FALSE;
  sentinel->size = 0;

  // this is the only arena we have
  arena = (struct head*)new;
  //printf("arena allocated\n");
  return new;
}

struct head *flist = NULL;

int getlength() {
  struct head *ptr = flist;
  int i = 0;
   while (ptr != NULL)
   {
       i++;
       ptr = ptr->next;
   }
   return i;
}

void detach(struct head *block) {
  //byt ut mot assert
  if(block->next != NULL) {
    //printf("here\n");
    block->next->prev = block->prev;
  }
  if(block->prev != NULL) {
    //printf("here1\n");
    block->prev->next = block->next;
  } else {
    //printf("here2\n");
    //printf("flist1: %p\n", flist);
    flist = block->next;
    //printf("flist2: %p\n", flist);
  }
  return;
}

void insert(struct head *block) {
  block->next = flist;
  block->prev = NULL;
  if(flist != NULL){
    flist->prev = block;
  }
  flist = block;
}

int adjust(size_t request) {
  int min_size = (int)MIN(request);
  int adjusted_size = min_size % ALIGN == 0 ? min_size : min_size + (ALIGN - min_size%ALIGN);
  return adjusted_size;
}

struct head *find(int size) {
  struct head *ptr = NULL;
  while(1){
    //printf("1\n");
    ptr = flist;
    while(ptr != NULL) {
      //printf("ptr size: %d\n", ptr->size);
      if(ptr->size>=size) {
        if((ptr->size)>=LIMIT(size)) {
          //printf("flist: %p\n", flist);
          ptr = split(ptr, size);
          //printf("ptr: %p\n", ptr);
          //printf("flist: %p\n", flist);
          //insert(before(ptr));
          //printf("flist: %p\n", flist);
          //detach(ptr);
          //printf("flist: %p\n", flist);
          break;
        } else {
            detach(ptr);
            break;
        }
      }
      ptr = ptr->next;
    }
    if(ptr == NULL) {
      struct head *heap = new();
      //printf("heap: %p\n", heap);
      if(heap == NULL) {
        break;
      }
      //printf("2\n");
      insert(heap);
    } else {
      break;
    };
  }
  return ptr;
}

struct head *merge(struct head *block) {
  mergecount += 1;

  struct head *aft = after(block);

  if (block->bfree && aft->free) {
    merge1 += 1;
    //printf("1\n");
    struct head *bef = before(block);
    bef->size = block->bsize + block->size + aft->size + 2*HEAD;
    detach(aft);
    block = bef;
    after(block)->bsize = block->size;
    // we already know that bfree = TRUE
  } else if (aft->free && !(block->bfree)) {
      merge2 += 1;
      //printf("2\n");
      block->size = block->size + aft->size + HEAD;
      // aft->prev->next = block;
      block->prev = aft->prev;
      aft->next->prev = block;
      block->next = aft->next;
      detach(aft);
      after(block)->bsize = block->size;
  } else if (!(aft->free) && block->bfree) {
      merge3 += 1;
      //printf("3\n");
      struct head *bef = before(block);
      bef->size =  block->bsize + block->size + HEAD;
      block = bef;
      aft->bsize = block->size;
      aft->bfree = TRUE;
  }
  return block;
}


// This procedure could for example check that all blocks
// in the freelist have the correct previous pointer and that they are all marked
// as free. The size should also be a multiple of ALIGN bytes and not less than
// our minimum.
void sanity() {
    struct head *next = flist;
    struct head *prev = NULL;
    int i=0;
    // printf("initial next: %p\n", next);
    // printf("initial prev: %p\n", prev);
    while (next != NULL) {
      //printf("current size of block %d: %u\n",i, next->size);
      // printf("current adress of block %d: %p\n",i, next);
      assert(next->free == TRUE);
      //Lägg till efter merge implementerat
      assert(next->bfree == FALSE);
      assert(next->size >= MINSIZE);
      assert((next->size)%ALIGN==0);
      assert(after(next)->bsize == next->size);
      assert(next->prev == prev);
      struct head *aft = after(next);
      assert(aft->bfree == TRUE);
      //Lägg till efter merge implementerat
      assert(aft->free == FALSE);
      prev = next;
      next = next->next;
      //printf("next: %p\n", next);
      //printf("prev: %p\n", prev);
      i++;
    }
    int length = getlength();
    //printf("length of flist: %d\n", length);
    struct head *current = arena;

    while (current->size != 0) {
      assert(after(current)->bsize == current->size);
      assert(after(current)->bfree == current->free);
      //printf("current in arena: %p\n", current);
      current = after(current);
    }
}

void *dalloc(size_t request) {
  if(request <= 0) {
    //printf("here");
    return NULL;
  }
  int size = adjust(request);
  //printf("size: %d\n", size);
  struct head *taken = find(size);
  sanity();
  if (taken == NULL) {
    //printf("here?\n");
    return NULL;
  } else {
    taken->free=FALSE;
    after(taken)->bfree=FALSE;
    return HIDE(taken);
  }
}

void dfree(void *memory) {
  if(memory != NULL) {
    free += 1;
    struct head *block = MAGIC(memory);
    struct head *merged = merge(block);
    // If no merge
    if (merged->size == block->size) {
      struct head *aft = after(block);
      block->free = TRUE;
      aft->bfree = TRUE;
      insert(block);
    }
    // struct head *block = MAGIC(memory);
    // struct head *aft = after(block);
    // block->free = TRUE;
    // aft->bfree = TRUE;
    // insert(block);
  }
  return;
}
