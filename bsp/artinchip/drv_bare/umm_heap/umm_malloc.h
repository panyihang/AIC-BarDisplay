/* ----------------------------------------------------------------------------
 * umm_malloc.h - a memory allocator for embedded systems (microcontrollers)
 *
 * See copyright notice in LICENSE.TXT
 * ----------------------------------------------------------------------------
 */

#ifndef UMM_MALLOC_H
#define UMM_MALLOC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */

typedef struct umm_heap_config {
    void *pheap;
    size_t heap_size;
    uint16_t numblocks;
} umm_heap;

extern void  umm_init_heap(umm_heap *heap, void *ptr, size_t size);
extern void  umm_init(umm_heap *heap);

extern void *umm_malloc(umm_heap *heap, size_t size);
extern void *umm_calloc(umm_heap *heap, size_t num, size_t size);
extern void *umm_realloc(umm_heap *heap, void *ptr, size_t size);
extern void  umm_free(umm_heap *heap, void *ptr);
extern void *umm_malloc_align(umm_heap *heap, size_t size, size_t align);
extern void  umm_free_align(umm_heap *heap, void *ptr);

/* ------------------------------------------------------------------------ */

#ifdef __cplusplus
}
#endif

#endif /* UMM_MALLOC_H */
