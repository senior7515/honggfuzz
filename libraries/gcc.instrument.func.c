#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

feedback_t *feedback;
static pid_t mypid;

__attribute__ ((constructor))
static void mapBB(void)
{
    mypid = getpid();
    struct stat st;
    if (fstat(1022, &st) == -1) {
        perror("stat");
        _exit(1);
    }
    if (st.st_size != sizeof(feedback_t)) {
        fprintf(stderr, "st.size != sizeof(feedback_t) (%zu != %zu)\n", (size_t) st.st_size,
                sizeof(feedback_t));
        _exit(1);
    }
    if ((feedback =
         mmap(NULL, sizeof(feedback_t), PROT_READ | PROT_WRITE, MAP_SHARED, 1022,
              0)) == MAP_FAILED) {
        perror("mmap");
        _exit(1);
    }
    feedback->pidFeedback[mypid] = 0U;
}

void __cyg_profile_func_enter(void *func, void *caller)
{
    size_t pos = (((uintptr_t) func << 12) ^ ((uintptr_t) caller & 0xFFF)) & 0xFFFFFF;
    size_t byteOff = pos / 8;
    uint8_t bitSet = (uint8_t) (1 << (pos % 8));

    register uint8_t prev = ATOMIC_POST_OR(feedback->bbMap[byteOff], bitSet);
    if (!(prev & bitSet)) {
        ATOMIC_PRE_INC(feedback->pidFeedback[mypid]);
    }
}
