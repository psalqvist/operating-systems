#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "rand.h"
#include "dlmall.h"

#define ROUNDS 1000
#define LOOP 1000
#define BUFFER 100

int main() {
    size_t size = (size_t)request();
    struct head *ptr = find(size);
    return 0;
}
