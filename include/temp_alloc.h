#pragma once
/*
 * temp_alloc.h - C3-style temporary allocator for C and C++ (single header)
 *
 * tmalloc() works like malloc(), but memory comes from a thread-local arena
 * and is never freed individually. Instead you release everything allocated
 * since a point in time by resetting to a mark (the C3 `@pool` idea).
 *
 * Setup (stb style): in exactly ONE .c/.cpp file, before including:
 *
 *     #define TEMP_ALLOC_IMPLEMENTATION
 *     #include "temp_alloc.h"
 *
 * Every other file just includes the header.
 *
 * Usage (C or C++):
 *
 *     TEMP_POOL {                         // everything inside is freed at '}'
 *         char *s = tprintf("%s-%d", name, id);
 *         int  *v = (int *)tmalloc(n * sizeof(int));
 *         ...
 *     }
 *
 *     C++ only, RAII:  { temp_scope scope; char *s = tstrdup("x"); }
 *
 * Notes:
 *  - TEMP_POOL: leaving it with break/return/goto skips the reset (memory is
 *    then released by the next enclosing reset). In C++ prefer temp_scope.
 *  - Memory is 16-byte aligned. The arena is per thread. Pointers stay valid
 *    until the pool they were allocated in ends (the arena never moves).
 *  - Never pass tmalloc'd memory to free(). Call temp_free_all() to give
 *    all blocks back to the system (e.g. before a thread exits).
 *  - C++ needs C++11 (thread_local).
 *
 * Optional settings (define before the IMPLEMENTATION include):
 *     TEMP_BLOCK_SIZE   bytes per arena block (default 64 KiB)
 *     TEMP_MALLOC/TEMP_FREE   backing allocator (default malloc/free)
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct temp_block_ temp_block_;

typedef struct temp_mark_t {
    temp_block_ *block;
    size_t used;
} temp_mark_t;

#if defined(__GNUC__) || defined(__clang__)
#define TEMP_PRINTF_(f, a) __attribute__((format(printf, f, a)))
#else
#define TEMP_PRINTF_(f, a)
#endif

/* malloc / calloc / realloc / strdup / printf, all from the temp arena */
void *tmalloc(size_t size);
void *tcalloc(size_t count, size_t size);
void *trealloc(void *ptr, size_t old_size, size_t new_size); /* ptr may be NULL */
char *tstrdup(const char *s);
char *tprintf(const char *fmt, ...) TEMP_PRINTF_(1, 2);
char *tvprintf(const char *fmt, va_list ap);

/* Pools: remember a position, later release everything allocated after it */
temp_mark_t temp_mark(void);
void temp_reset(temp_mark_t mark);
void temp_clear(void);     /* release everything, keep blocks for reuse */
void temp_free_all(void);  /* give all blocks back to the system */

#ifdef __cplusplus
}
#endif

/* TEMP_POOL { ... }  -- runs the block once, then resets the arena to where it was */
#define TEMP_POOL                                                       \
    for (temp_mark_t temp_m_ = temp_mark(), *temp_once_ = &temp_m_;     \
         temp_once_; temp_once_ = NULL, temp_reset(temp_m_))

#ifdef __cplusplus
struct temp_scope {
    temp_mark_t mark_;
    temp_scope() : mark_(temp_mark()) {}
    ~temp_scope() { temp_reset(mark_); }
    temp_scope(const temp_scope &) = delete;
    temp_scope &operator=(const temp_scope &) = delete;
};
#endif

/* ======================================================================= */
#ifdef TEMP_ALLOC_IMPLEMENTATION

#ifndef TEMP_BLOCK_SIZE
#define TEMP_BLOCK_SIZE (64 * 1024)
#endif
#ifndef TEMP_MALLOC
#define TEMP_MALLOC malloc
#endif
#ifndef TEMP_FREE
#define TEMP_FREE free
#endif

#ifndef TEMP_TLS
#  if defined(__cplusplus)
#    define TEMP_TLS thread_local
#  elif defined(_MSC_VER)
#    define TEMP_TLS __declspec(thread)
#  elif defined(__GNUC__) || defined(__clang__)
#    define TEMP_TLS __thread
#  elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#    define TEMP_TLS _Thread_local
#  else
#    define TEMP_TLS
#  endif
#endif

#define TEMP_ALIGN 16
#define TEMP_ROUND(n) (((n) + (TEMP_ALIGN - 1)) & ~(size_t)(TEMP_ALIGN - 1))
#define TEMP_MAX_REQUEST (((size_t)-1) / 2)

#ifdef __cplusplus
extern "C" {
#endif

struct temp_block_ {
    temp_block_ *next;  /* next block in the chain (spare when not current) */
    size_t cap;         /* usable bytes after the header */
    size_t used;
};

#define TEMP_HDR TEMP_ROUND(sizeof(struct temp_block_))

static TEMP_TLS temp_block_ *temp_first_;
static TEMP_TLS temp_block_ *temp_cur_;

static unsigned char *temp_data_(temp_block_ *b) {
    return (unsigned char *)b + TEMP_HDR;
}

static temp_block_ *temp_new_block_(size_t min_cap) {
    size_t cap = TEMP_ROUND((size_t)TEMP_BLOCK_SIZE);
    temp_block_ *b;
    if (cap < min_cap) cap = min_cap;
    b = (temp_block_ *)TEMP_MALLOC(TEMP_HDR + cap);
    if (!b) return NULL;
    b->next = NULL;
    b->cap = cap;
    b->used = 0;
    return b;
}

void *tmalloc(size_t size) {
    temp_block_ *b = temp_cur_;
    void *p;

    if (size > TEMP_MAX_REQUEST) return NULL;
    size = TEMP_ROUND(size ? size : 1);

    if (!b) {
        b = temp_new_block_(size);
        if (!b) return NULL;
        temp_first_ = temp_cur_ = b;
    } else if (b->cap - b->used < size) {
        temp_block_ *n = b->next;
        if (!n || n->cap < size) {
            temp_block_ *nb = temp_new_block_(size);
            if (!nb) return NULL;
            nb->next = n;   /* keep the too-small spare chained after it */
            b->next = nb;
            n = nb;
        }
        n->used = 0;
        temp_cur_ = b = n;
    }

    p = temp_data_(b) + b->used;
    b->used += size;
    return p;
}

void *tcalloc(size_t count, size_t size) {
    void *p;
    if (size && count > TEMP_MAX_REQUEST / size) return NULL;
    p = tmalloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}

void *trealloc(void *ptr, size_t old_size, size_t new_size) {
    temp_block_ *b = temp_cur_;
    size_t o, n;
    void *np;

    if (!ptr) return tmalloc(new_size);
    if (new_size > TEMP_MAX_REQUEST) return NULL;

    o = TEMP_ROUND(old_size ? old_size : 1);
    n = TEMP_ROUND(new_size ? new_size : 1);

    /* last allocation in the current block: grow or shrink in place */
    if (b && b->used >= o && (unsigned char *)ptr == temp_data_(b) + b->used - o) {
        if (n <= o) { b->used -= o - n; return ptr; }
        if (b->cap - b->used >= n - o) { b->used += n - o; return ptr; }
    }

    np = tmalloc(new_size);
    if (np) memcpy(np, ptr, old_size < new_size ? old_size : new_size);
    return np;
}

char *tstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)tmalloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

char *tvprintf(const char *fmt, va_list ap) {
    va_list ap2;
    char buf[256];
    int n;
    char *p;

    /* common case: result fits the stack buffer, so only one formatting pass */
    va_copy(ap2, ap);
    n = vsnprintf(buf, sizeof buf, fmt, ap2);
    va_end(ap2);
    if (n < 0) return NULL;

    p = (char *)tmalloc((size_t)n + 1);
    if (!p) return NULL;
    if ((size_t)n < sizeof buf) memcpy(p, buf, (size_t)n + 1);
    else vsnprintf(p, (size_t)n + 1, fmt, ap);
    return p;
}

char *tprintf(const char *fmt, ...) {
    va_list ap;
    char *p;
    va_start(ap, fmt);
    p = tvprintf(fmt, ap);
    va_end(ap);
    return p;
}

temp_mark_t temp_mark(void) {
    temp_mark_t m;
    m.block = temp_cur_;
    m.used = temp_cur_ ? temp_cur_->used : 0;
    return m;
}

void temp_reset(temp_mark_t m) {
    if (m.block) {
        temp_cur_ = m.block;
        m.block->used = m.used;
    } else if (temp_first_) {   /* mark taken before the first allocation */
        temp_cur_ = temp_first_;
        temp_first_->used = 0;
    }
}

void temp_clear(void) {
    temp_mark_t m;
    m.block = NULL;
    m.used = 0;
    temp_reset(m);
}

void temp_free_all(void) {
    temp_block_ *b = temp_first_;
    while (b) {
        temp_block_ *next = b->next;
        TEMP_FREE(b);
        b = next;
    }
    temp_first_ = temp_cur_ = NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* TEMP_ALLOC_IMPLEMENTATION */
