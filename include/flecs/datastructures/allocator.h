/**
 * @file allocator.h
 * @brief Allocator that returns memory objects of any size. 
 */

#ifndef FLECS_ALLOCATOR_H
#define FLECS_ALLOCATOR_H

#include "../private/api_defines.h"

FLECS_DBG_API extern int64_t ecs_block_allocator_alloc_count;
FLECS_DBG_API extern int64_t ecs_block_allocator_free_count;
FLECS_DBG_API extern int64_t ecs_stack_allocator_alloc_count;
FLECS_DBG_API extern int64_t ecs_stack_allocator_free_count;

struct ecs_allocator_t {
#ifndef FLECS_USE_OS_ALLOC
    ecs_block_allocator_t chunks;
    struct ecs_sparse_t sizes; /* <size, block_allocator_t> */
#else
    bool dummy;
#endif
};

FLECS_API
void flecs_allocator_init(
    ecs_allocator_t *a);

FLECS_API
void flecs_allocator_fini(
    ecs_allocator_t *a);

FLECS_API
ecs_block_allocator_t* flecs_allocator_get(
    ecs_allocator_t *a, 
    ecs_size_t size);

FLECS_API
char* flecs_strdup(
    ecs_allocator_t *a, 
    const char* str);

FLECS_API
void flecs_strfree(
    ecs_allocator_t *a, 
    char* str);

FLECS_API
void* flecs_dup(
    ecs_allocator_t *a,
    ecs_size_t size,
    const void *src);

#ifndef FLECS_USE_OS_ALLOC
#define flecs_allocator(obj) (&obj->allocators.dyn)

#define flecs_alloc(a, size) flecs_balloc(flecs_allocator_get(a, size))
#define flecs_alloc_w_dbg_info(a, size, type_name) flecs_balloc_w_dbg_info(flecs_allocator_get(a, size), type_name)
#define flecs_alloc_t(a, T) flecs_alloc_w_dbg_info(a, ECS_SIZEOF(T), #T)
#define flecs_alloc_n(a, T, count) flecs_alloc_w_dbg_info(a, ECS_SIZEOF(T) * (count), #T)

#define flecs_calloc(a, size) flecs_bcalloc(flecs_allocator_get(a, size))
#define flecs_calloc_w_dbg_info(a, size, type_name) flecs_bcalloc_w_dbg_info(flecs_allocator_get(a, size), type_name)
#define flecs_calloc_t(a, T) flecs_calloc_w_dbg_info(a, ECS_SIZEOF(T), #T)
#define flecs_calloc_n(a, T, count) flecs_calloc_w_dbg_info(a, ECS_SIZEOF(T) * (count), #T)

#define flecs_free(a, size, ptr)\
    flecs_bfree((ptr) ? flecs_allocator_get(a, size) : NULL, ptr)
#define flecs_free_t(a, T, ptr)\
    flecs_bfree_w_dbg_info((ptr) ? flecs_allocator_get(a, ECS_SIZEOF(T)) : NULL, ptr, #T)
#define flecs_free_n(a, T, count, ptr)\
    flecs_bfree_w_dbg_info((ptr) ? flecs_allocator_get(a, ECS_SIZEOF(T) * (count)) : NULL\
        , ptr, #T)

#define flecs_realloc(a, size_dst, size_src, ptr)\
    flecs_brealloc(flecs_allocator_get(a, size_dst),\
    flecs_allocator_get(a, size_src),\
    ptr)
#define flecs_realloc_w_dbg_info(a, size_dst, size_src, ptr, type_name)\
    flecs_brealloc_w_dbg_info(flecs_allocator_get(a, size_dst),\
    flecs_allocator_get(a, size_src),\
    ptr,\
    type_name)
#define flecs_realloc_n(a, T, count_dst, count_src, ptr)\
    flecs_realloc(a, ECS_SIZEOF(T) * (count_dst), ECS_SIZEOF(T) * (count_src), ptr)

#define flecs_dup_n(a, T, count, ptr) flecs_dup(a, ECS_SIZEOF(T) * (count), ptr)

#else

void* flecs_alloc(
    ecs_allocator_t *a, 
    ecs_size_t size);

void* flecs_calloc(
    ecs_allocator_t *a, 
    ecs_size_t size);

void* flecs_realloc(
    ecs_allocator_t *a, 
    ecs_size_t dst_size, 
    ecs_size_t src_size, 
    void *ptr);

void flecs_free(
    ecs_allocator_t *a, 
    ecs_size_t size,
    void *ptr);

#define flecs_alloc_w_dbg_info(a, size, type_name) flecs_alloc(a, size)
#define flecs_alloc_t(a, T) flecs_alloc(a, ECS_SIZEOF(T))
#define flecs_alloc_n(a, T, count) flecs_alloc(a, ECS_SIZEOF(T) * (count))

#define flecs_calloc_w_dbg_info(a, size, type_name) flecs_calloc(a, size)
#define flecs_calloc_t(a, T) flecs_calloc(a, ECS_SIZEOF(T))
#define flecs_calloc_n(a, T, count) flecs_calloc(a, ECS_SIZEOF(T) * (count))

#define flecs_free_t(a, T, ptr) flecs_free(a, ECS_SIZEOF(T), ptr)
#define flecs_free_n(a, T, count, ptr) flecs_free(a, ECS_SIZEOF(T) * (count), ptr)

#define flecs_realloc_w_dbg_info(a, size_dst, size_src, ptr, type_name)\
    flecs_realloc(a, size_dst, size_src, ptr)

#define flecs_realloc_n(a, T, count_dst, count_src, ptr)\
    flecs_realloc(a, ECS_SIZEOF(T) * count_dst, ECS_SIZEOF(T) * count_src, ptr)

#define flecs_dup_n(a, T, count, ptr) flecs_dup(a, ECS_SIZEOF(T) * (count), ptr)

#endif

#endif
