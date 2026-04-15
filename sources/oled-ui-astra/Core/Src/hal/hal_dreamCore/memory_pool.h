/**
 * @file   memory_pool.h
 * @brief  Custom fixed-block memory pool providing myMalloc/myFree for bare-metal use.
 * @author Fir
 * @date   2024-05-04
 */

#ifndef ASTRA_CORE_SRC_HAL_HAL_DREAMCORE_MEMORY_POOL_H_
#define ASTRA_CORE_SRC_HAL_HAL_DREAMCORE_MEMORY_POOL_H_

#ifdef __cplusplus
extern "C" {
#endif

// C interface

#ifndef NULL
#define NULL 0
#endif

// Memory parameter settings. Block size may be 4–32 bytes.
// Keep this in sync with the alignment size defined in the .cpp file.
#define MEM_BLOCK_SIZE       32          ///< Size of each allocation block in bytes.
#define MEM_MAX_SIZE         (13*1023)   ///< Maximum managed memory pool size (13 × 1023 = 13299 bytes, slightly under 13 KB).
#define MEM_ALLOC_TABLE_SIZE (MEM_MAX_SIZE/MEM_BLOCK_SIZE) ///< Number of entries in the allocation table.

/** @brief Memory manager controller structure. */
struct _m_malloc_dev {
  void           (*init)();      ///< Function pointer: initialises the memory pool state and allocation table.
  unsigned char  (*perused)();   ///< Return memory usage percentage (0–100).
  unsigned char  *memBase;       ///< Pointer to the raw memory pool.
  unsigned short *memMap;        ///< Allocation state table.
  unsigned char   memRdy;        ///< Non-zero when the pool has been initialised.
};

extern struct _m_malloc_dev malloc_dev; // Defined in memory_pool.cpp.

/**
 * @brief  Fill a memory region with a constant byte value.
 * @param  s      Pointer to the memory region.
 * @param  c      Byte value to fill.
 * @param  count  Number of bytes to fill.
 */
void myMemset(void *s, unsigned char c, unsigned long count);

/**
 * @brief  Copy a block of memory from source to destination.
 * @param  des  Destination pointer.
 * @param  src  Source pointer.
 * @param  n    Number of bytes to copy.
 */
void *myMemCpy(void *des, const void *src, unsigned long n);

/**
 * @brief  Initialise the memory pool (callable from C or C++).
 */
void memInit(void);

/**
 * @brief  Allocate a block from the pool (internal use).
 * @param  size  Number of bytes requested.
 * @return Pool-relative byte offset on success; 0xFFFFFFFF on allocation failure.
 * @note   Do not pass the return value to memFree without first checking for 0xFFFFFFFF.
 */
unsigned long memMalloc(unsigned long size);

/**
 * @brief  Free a previously allocated pool block (internal use).
 * @param  offset  Byte offset returned by memMalloc.
 * @return 0 on success, non-zero on error.
 */
unsigned char memFree(unsigned long offset);

/**
 * @brief  Return the current memory usage percentage (callable from C or C++).
 * @return Usage as a value in the range 0–100.
 */
unsigned char memPerused(void);

// Public API for user code.

/**
 * @brief  Free a heap block obtained from myMalloc or myReAlloc.
 * @param  ptr  Pointer to the block to free.
 */
void myFree(void *ptr);

/**
 * @brief  Allocate a block from the pool.
 * @param  size  Number of bytes requested.
 * @return Pointer to the allocated block, or NULL on failure.
 */
void *myMalloc(unsigned long size);

/**
 * @brief  Resize a previously allocated pool block.
 * @param  ptr   Pointer to the existing block (may be NULL).
 * @param  size  New size in bytes.
 * @return Pointer to the resized block, or NULL on failure.
 */
void *myReAlloc(void *ptr, unsigned long size);

#ifdef __cplusplus
}

// C++ interface

#endif

#endif //ASTRA_CORE_SRC_HAL_HAL_DREAMCORE_MEMORY_POOL_H_
