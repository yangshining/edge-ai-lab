/**
 * @file   memory_pool.cpp
 * @brief  Fixed-size block memory pool: init, alloc, free, and C++ new/delete overrides.
 * @author Fir
 * @date   2024-05-04
 */

#include <cstddef>
#include "memory_pool.h"

// memory pool (32-byte aligned)
__attribute__((aligned(32))) unsigned char memBase[MEM_MAX_SIZE];
// allocation map table
unsigned short memMapBase[MEM_ALLOC_TABLE_SIZE];

// pool constants
const unsigned long memTblSize = MEM_ALLOC_TABLE_SIZE; // number of map entries
const unsigned long memBlkSize = MEM_BLOCK_SIZE;        // bytes per block
const unsigned long memSize    = MEM_MAX_SIZE;           // total pool size in bytes

// memory manager controller instance
struct _m_malloc_dev malloc_dev =
    {
        memInit,
        memPerused,
        memBase,
        memMapBase,
        0,  // not yet initialised
    };

// Copy n bytes from src to des.
void *myMemCpy(void *des, const void *src, unsigned long n) {
  unsigned char *xDes = (unsigned char *) des;
  const unsigned char *xSrc = (const unsigned char *) src;
  while (n--) *xDes++ = *xSrc++;
  return des;
}

// Fill count bytes starting at s with value c.
void myMemset(void *s, unsigned char c, unsigned long count) {
  unsigned char *xS = (unsigned char *) s;
  while (count--) *xS++ = c;
}

// Initialise the memory pool: zero the map and the pool, then mark ready.
void memInit(void) {
  myMemset(malloc_dev.memMap, 0, memTblSize * 2); // clear allocation map
  myMemset(malloc_dev.memBase, 0, memSize);        // clear pool data
  malloc_dev.memRdy = 1;
}

// Return pool utilization as a percentage (0–100).
unsigned char memPerused(void) {
  unsigned long used = 0;
  unsigned long i;
  for (i = 0; i < memTblSize; i++) {
    if (malloc_dev.memMap[i]) used++;
  }
  return (used * 100) / (memTblSize);
}

// Internal alloc: find a contiguous run of free blocks for the requested size.
// Returns the byte offset into the pool, or 0xFFFFFFFF on failure.
unsigned long memMalloc(unsigned long size) {
  signed long offset = 0;
  unsigned short nmemb;       // number of blocks needed
  unsigned short cmemb = 0;   // consecutive free block counter
  unsigned long i;
  if (!malloc_dev.memRdy) malloc_dev.init(); // auto-init if not ready
  if (size == 0) return 0XFFFFFFFF;          // zero-size request is invalid
  nmemb = size / memBlkSize;
  if (size % memBlkSize) nmemb++;

  // scan the map from the end to find a contiguous free run
  for (offset = memTblSize - 1; offset >= 0; offset--) {
    if (!malloc_dev.memMap[offset]) cmemb++; // extend free run
    else cmemb = 0;                           // break in free run; reset counter

    // found nmemb consecutive free blocks
    if (cmemb == nmemb) {
      // mark blocks as allocated
      for (i = 0; i < nmemb; i++) malloc_dev.memMap[offset + i] = nmemb;
      return (offset * memBlkSize); // return byte offset
    }
  }
  return 0XFFFFFFFF; // no suitable run found
}

// Internal free: release blocks at the given pool byte offset.
// Returns 0 on success, 1 if not initialised, 2 if offset is out of range.
unsigned char memFree(unsigned long offset) {
  int i;

  if (!malloc_dev.memRdy) {
    malloc_dev.init();
    return 1; // was not initialised
  }

  if (offset < memSize) {
    int index = offset / memBlkSize;          // block index for this offset
    int nMemB = malloc_dev.memMap[index];     // number of blocks to free

    // clear the allocation map entries
    for (i = 0; i < nMemB; i++) malloc_dev.memMap[index + i] = 0;
    return 0;
  } else return 2; // offset exceeds pool bounds
}

// Public free: compute the pool offset from the pointer and release it.
void myFree(void *ptr) {
  unsigned long offset;
  if (ptr == NULL) return; // null pointer — nothing to do
  offset = (unsigned long) ptr - (unsigned long) malloc_dev.memBase;
  memFree(offset);
}

// Public alloc: allocate size bytes and return a pointer into the pool.
void *myMalloc(unsigned long size) {
  unsigned long offset;
  offset = memMalloc(size);
  if (offset == 0XFFFFFFFF) return NULL;
  else return (void *) ((unsigned long) malloc_dev.memBase + offset);
}

// Public realloc: allocate a new block, copy old data, then free the old block.
void *myReAlloc(void *ptr, unsigned long size) {
  unsigned long offset;
  offset = memMalloc(size);
  if (offset == 0XFFFFFFFF) return NULL;
  else {
    // copy old contents to the new block
    myMemCpy((void *) ((unsigned long) malloc_dev.memBase + offset), ptr, size);
    myFree(ptr); // release old block
    return (void *) ((unsigned long) malloc_dev.memBase + offset);
  }
}

void *operator new(std::size_t size) noexcept(false) {
  void *res;
  if (size == 0)
    size = 1;
  res = myMalloc(size);
  if (res) return res;
  else return nullptr; // allocation failed
}

void operator delete(void *p) { myFree(p); }
