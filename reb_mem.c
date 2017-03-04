// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include "reb_mem.h"
#include "reb_config.h"
#include <stdint.h>
#include <assert.h>

#ifdef REB_MEM_DEBUG
#include <stdio.h>
#endif

// Custom allocator.
// Mostly an experiment, but it also lets the application have a concept of "available memory".
// Not a *good* allocator, but it should work "well enough".
// (Really, though, this is good for testing OOM response.)

#ifdef REB_MEM_CONFIG_CUSTOMALLOCATOR

typedef struct {
    void * next; // Next reb_mem_blk_t. 0 if this is the last block.
    void * end;  // Equals next, unless next == 0. Where the next block would start.
    void * use;  // 0 if unused, otherwise, a string.
} reb_mem_blk_t;

static reb_mem_blk_t * reb_mem_heap;
#ifdef REB_MEM_DEBUG
static int reb_mem_allocs;
#endif

void reb_mem_start(char * memory, size_t amount) {
#ifdef REB_MEM_DEBUG
    reb_mem_allocs = 0;
#endif
    while (((intptr_t) memory) % REB_MEM_ALIGN > 0) {
        memory++;
        amount--;
    }
    reb_mem_heap = (reb_mem_blk_t *) memory;
    reb_mem_heap->next = 0;
    reb_mem_heap->end = memory + amount;
    reb_mem_heap->use = 0;
#ifdef REB_MEM_DEBUG
    printf("meminit:%p\n", reb_mem_heap);
#endif
}

#ifdef REB_MEM_DEBUG

void reb_mem_dump() {
    reb_mem_blk_t * curblk = reb_mem_heap;
    size_t metasize = sizeof (reb_mem_blk_t);
    while (curblk) {
        char * memstart = (((char*) curblk) + metasize);
        size_t blkamount = ((char*) curblk->end) - memstart;
        if (curblk->use)
            printf("Allocation %i bytes @ %p : %s\n", blkamount, memstart, curblk->use);
        curblk = curblk->next;
    }
}

void reb_mem_checkaddrblk(reb_mem_blk_t * blk) {
    reb_mem_blk_t * curblk = reb_mem_heap;
    while (curblk != blk) {
        assert(curblk);
        curblk = curblk->next;
    }
}
#endif

void reb_mem_byebye() {
#ifdef REB_MEM_DEBUG
    puts("reb_mem.c: The following allocations remain:");
    reb_mem_dump();
    puts("--------------------------------------------");
#endif
}

int reb_mem_defrag(reb_mem_blk_t * curblk) {
    int didanything = 0;
    while (curblk->next) {
        reb_mem_blk_t * next = ((reb_mem_blk_t*) curblk->next);
        if (next->use)
            break;
        // block is unused, cut it out
        curblk->end = next->end;
        curblk->next = next->next;
        didanything = 1;
    }
    return didanything; // did the size of the first block change?
}

void * reb_mem_alloc_i(size_t amount, char * idtag) {
    assert(amount);
    reb_mem_blk_t * curblk = reb_mem_heap;
    if ((amount % REB_MEM_ALIGN) != 0)
        amount += REB_MEM_ALIGN - (amount % REB_MEM_ALIGN);
    size_t metasize = sizeof (reb_mem_blk_t);
    while (curblk) {
        char * memstart = (((char*) curblk) + metasize);
        size_t blkamount = ((char*) curblk->end) - memstart;
        // allocate?
        if (!(curblk->use)) {
            // perform defragmentation
            if (reb_mem_defrag(curblk))
                blkamount = ((char*) curblk->end) - memstart; // update blkamount
            // split block
            if (blkamount >= (amount + metasize)) {
                reb_mem_blk_t * splitptr = (reb_mem_blk_t *) (memstart + amount);
                splitptr->next = curblk->next;
                splitptr->end = curblk->end;
                splitptr->use = 0;
                curblk->next = splitptr;
                curblk->end = splitptr;
                curblk->use = idtag;
#ifdef REB_MEM_DEBUG
                printf("Alloc %p %i\n", memstart, reb_mem_allocs);
                reb_mem_allocs++;
#endif
                return memstart;
            }
            // Is this block a perfect fit?
            if (blkamount == amount) {
                curblk->use = idtag;
#ifdef REB_MEM_DEBUG
                printf("AllocFit %p %i\n", memstart, reb_mem_allocs);
                reb_mem_allocs++;
#endif
                return memstart;
            }
        }
        // nope, couldn't
        curblk = curblk->next;
    }
    // *gulp* OOM
    return 0;
}

size_t reb_mem_available() {
    reb_mem_blk_t * curblk = reb_mem_heap;
    size_t available = 0;
    size_t metasize = sizeof (reb_mem_blk_t);
    while (curblk) {
        char * memstart = (((char*) curblk) + metasize);
        size_t blkamount = ((char*) curblk->end) - memstart;
        if (!curblk->use) {
            if (reb_mem_defrag(curblk))
                blkamount = ((char*) curblk->end) - memstart; // update blkamount
            available += blkamount;
        }
        curblk = curblk->next;
    }
    return available;
}

void reb_mem_free(void * ptr) {
    assert(ptr);
    reb_mem_blk_t * blk = (reb_mem_blk_t *) (((char*) ptr) - sizeof (reb_mem_blk_t));
    assert(blk->use);
#ifdef REB_MEM_DEBUG
    reb_mem_checkaddrblk(blk);
    reb_mem_allocs--;
    printf("Free %p %i (allocated at %s)\n", ptr, reb_mem_allocs, blk->use);
    //reb_mem_dump();
#endif
    blk->use = 0;
}

void reb_mem_downsize(void * ptr, size_t new) {
    assert(ptr);
    reb_mem_blk_t * blk = (reb_mem_blk_t *) (((char*) ptr) - sizeof (reb_mem_blk_t));
    assert(blk->use);
    assert(blk->end > (ptr + new));
#ifdef REB_MEM_DEBUG
    reb_mem_checkaddrblk(blk);
#endif
    if ((new % REB_MEM_ALIGN) != 0)
        new += REB_MEM_ALIGN - (new % REB_MEM_ALIGN);
    // Firstly, is there enough room for a new block header?
    size_t oldsize = ((char*) (blk->next)) - ((char*) ptr);
    if ((oldsize - new) < (sizeof(reb_mem_blk_t) + 1))
        return; // Nope.
    reb_mem_blk_t * blk2 = (reb_mem_blk_t *) (((char*) ptr) + new);

    blk2->next = blk->next;
    blk2->end = blk->end;
    blk2->use = 0;

    blk->next = blk2;
    blk->end = blk2;
}

#else
#include <malloc.h>

void reb_mem_start() {
}

void reb_mem_byebye() {
}

void * reb_mem_alloc(size_t amount) {
    assert(amount);
    return malloc(amount);
}

void reb_mem_downsize(void * block, size_t size) {
}

void reb_mem_free(void * ptr) {
    free(ptr);
}
#endif
