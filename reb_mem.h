// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#ifndef REB_MEM_H
#define REB_MEM_H

#include <stdlib.h>
#include "reb_config.h"

// REB global allocator -
// for a slightly higher chance of not needing stdlib (tm)
// Since the actual implementation of memory can be nicely hidden in the C file,
//  it is. Look for REB_MEM_CUSTOMALLOCATOR - define or not define if you want.
// The current version has REB_MEM_CONFIG_CUSTOMALLOCATOR as a suggested size,
//  for ports to read, on platforms where "all available memory" is not a viable option.
#ifdef REB_MEM_CONFIG_CUSTOMALLOCATOR
// Custom allocator-specific config
// Architectures like ARM want alignment...
#define REB_MEM_ALIGN 8
// For testing only
//#define REB_MEM_DEBUG

void * reb_mem_alloc_i(size_t amount, char * idtag);
#define reb_mem_its_(a) #a
#define reb_mem_its(a) reb_mem_its_(a)
#ifdef REB_MEM_DEBUG
#define reb_mem_alloc(amount) reb_mem_alloc_i(amount, __FILE__ ":" reb_mem_its(__LINE__))
#else
#define reb_mem_alloc(amount) reb_mem_alloc_i(amount, "")
#endif
size_t reb_mem_available();
// Used by ports to specify the available memory area.
void reb_mem_start(char * memory, size_t size);
#else
void reb_mem_start();
void * reb_mem_alloc(size_t amount);
#endif

// Notably, this function can do absolutely nothing if need be.
void reb_mem_downsize(void * block, size_t size);
void reb_mem_free(void * ptr);
void reb_mem_byebye();

#endif

