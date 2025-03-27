//   _____ _    _ _______ _
//  / ____| |  | |__   __| |
// | |    | |  | |  | |  | |       Tiny C utilities
// | |    | |  | |  | |  | |       v1.0 - github.com/Dax89
// | |____| |__| |  | |  | |____
//  \_____|\____/   |_|  |______|
//
// SPDX-FileCopyrightText: 2025 Antonio Davide Trogu <contact@antoniodavide.dev>
// SPDX-License-Identifier: MIT

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_MSC_VER)
    #if defined(CUTL_EXPORTS)
        #define CUTL_API __declspec(dllexport)
    #else
        #define CUTL_API __declspec(dllimport)
    #endif
#else
    #if defined(CUTL_EXPORTS)
        #define CUTL_API __attribute__((visibility("default")))
    #else
        #define CUTL_API
    #endif
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// NOLINTBEGIN
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef ptrdiff_t isize;
typedef size_t usize;

typedef uintptr_t uptr;
typedef intptr_t iptr;
// NOLINTEND

#if defined(__GNUC__) // GCC, Clang, ICC
    #define intrinsic_trap() __builtin_trap()
    #define intrinsic_unreachable() __builtin_unreachable()
    #define intrinsic_unlikely(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER) // MSVC
    #define intrinsic_trap() __debugbreak()
    #define intrinsic_unreachable() __assume(false)
    #define intrinsic_unlikely(x) (!!(x))
#else
    #error "cutl.h: unsupported compiler"
#endif

// *** Error handling *** //
#if defined(NDEBUG) // Release
static inline void _unreachable_impl(void) { intrinsic_unreachable(); }
#else
static inline void _unreachable_impl(void) { intrinsic_trap(); }
#endif

CUTL_API void _except_impl(const char* file, int line, const char* fmt, ...);

#define assume(...)                                                            \
    do {                                                                       \
        if(intrinsic_unlikely(!(__VA_ARGS__))) {                               \
            fprintf(stderr, "[%s:%d] Assume condition failed '%s'\n",          \
                    __FILE__, __LINE__, #__VA_ARGS__);                         \
            intrinsic_trap();                                                  \
        }                                                                      \
    } while(0)

#define unreachable                                                            \
    do {                                                                       \
        fprintf(stderr, "[%s:%d] Unreachable code detected\n", __FILE__,       \
                __LINE__);                                                     \
        _unreachable_impl();                                                   \
    } while(0)

#define except(msg) _except_impl(__FILE__, __LINE__, msg)
#define exceptf(fmt, ...) _except_impl(__FILE__, __LINE__, fmt, __VA_ARGS__)

#define except_if(cond, msg)                                                   \
    do {                                                                       \
        if(cond) except(msg);                                                  \
    } while(0)

#define exceptf_if(cond, msg, ...)                                             \
    do {                                                                       \
        if(cond) exceptf(msg, __VA_ARGS__);                                    \
    } while(0)
// *** Error handling *** //

#define parent_struct(ptr, T, m) ((T*)((char*)(ptr) - offsetof(T, m)))

// *** Allocators *** //
typedef void* (*Allocator)(void* ctx, void* ptr, isize osize, isize nsize);

typedef struct LinearArena {
    char* begin;
    char* end;
} LinearArena;

CUTL_API LinearArena lineararena_create(isize n);
CUTL_API LinearArena lineararena_createfrom_n(char* p, isize n);
CUTL_API void lineararena_reset(LinearArena* self);
CUTL_API void lineararena_destroy(LinearArena* self);

#define lineararena_createfrom(p) lineararena_createfrom_n(p, sizeof(p))

CUTL_API void* std_allocator(void* ctx, void* ptr, isize osize, isize nsize);
CUTL_API void* linear_allocator(void* ctx, void* ptr, isize osize, isize nsize);

CUTL_API void* mem_alloc(isize n, Allocator a, void* ctx);
CUTL_API void* mem_alloc0(isize n, Allocator a, void* ctx);
CUTL_API void* mem_realloc(void* ptr, isize osize, isize nsize, Allocator a,
                           void* ctx);
CUTL_API void* mem_realloc0(void* ptr, isize osize, isize nsize, Allocator a,
                            void* ctx);
CUTL_API void mem_free(void* ptr, isize n, Allocator a, void* ctx);

#define mem_alloctype_n(T, n, a, ctx) (T*)mem_alloc(sizeof(T) * n, a, ctx)
#define mem_alloctype0_n(T, n, a, ctx) (T*)mem_alloc0(sizeof(T) * n, a, ctx)
#define mem_alloctype(T, a, ctx) mem_alloctype_n(T, 1, a, ctx)
#define mem_alloctype0(T, a, ctx) mem_alloctype0_n(T, 1, a, ctx)

#define mem_realloctype(T, ptr, oldn, newn, a, ctx)                            \
    (T*)mem_realloc(ptr, sizeof(T) * (oldn), sizeof(T) * (newn), a, ctx)

#define mem_realloctype0(T, ptr, oldn, newn, a, ctx)                           \
    (T*)mem_realloc0(ptr, sizeof(T) * (oldn), sizeof(T) * (newn), a, ctx)

#define mem_freetype_n(T, ptr, n, a, ctx)                                      \
    mem_free(ptr, sizeof(T) * (n), a, ctx);

#define mem_freetype(T, ptr, a, ctx) mem_freetype_n(T, ptr, 1, a, ctx)
// *** Allocators *** //

// *** Slice *** //
#define define_slice(name, T)                                                  \
    typedef struct name {                                                      \
        T* data;                                                               \
        isize length;                                                          \
        isize capacity;                                                        \
        Allocator alloc;                                                       \
        void* ctx;                                                             \
    } name

#define slice_clear(self) ((self)->length = 0)
#define slice_empty(self) ((self)->length == 0)
#define slice_reserve(self, n) _slice_reserve(self, n, sizeof(*(self)->data))
#define slice_destroy(self) _slice_destroy(self, sizeof(*(self)->data))
#define slice_pop(self) ((self)->data[--(self)->length])

#define slice_push(self, ...)                                                  \
    do {                                                                       \
        _slice_grow(self, sizeof(*(self)->data));                              \
        (self)->data[(self)->length++] = __VA_ARGS__;                          \
    } while(0)

CUTL_API void slice_init(void* self, Allocator a, void* ctx);
CUTL_API void _slice_reserve(void* self, isize n, isize dsize);
CUTL_API void _slice_grow(void* self, isize dsize);
CUTL_API void _slice_destroy(void* self, isize dsize);
// *** Slice *** //

// *** String *** //
define_slice(Str, char);

typedef struct StrSplit {
    const Str* source;
    isize pos;
    const char* sep;
    isize nsep;
} StrSplit;

#define str_init slice_init
#define str_isview(self) ((self)->alloc == NULL)
#define str_ismutable(self) ((self)->alloc != NULL)
#define str_empty slice_empty

CUTL_API Str str_create_n(const char* s, isize n, Allocator a, void* ctx);
CUTL_API Str str_view_n(const char* s, isize n);
CUTL_API Str str_sub(const Str* self, isize start, isize end);
CUTL_API Str str_dup(const Str* self);
CUTL_API Str* str_dup_to(const Str* self, Str* dest);
CUTL_API void str_clear(Str* self);
CUTL_API void str_ncat(Str* self, const char* s, isize n);
CUTL_API void str_reserve(Str* self, isize n);
CUTL_API void str_destroy(Str* self);
CUTL_API void str_deleterange(Str* self, isize start, isize end);
CUTL_API void str_tolower(Str* self);
CUTL_API void str_toupper(Str* self);
CUTL_API void str_trim(Str* self);
CUTL_API usize str_hash(const Str* self);
CUTL_API bool str_equals(const Str* self, const Str* rhs);
CUTL_API bool _str_startswith_n(const Str* self, const char* s, isize n);
CUTL_API bool _str_endswith_n(const Str* self, const char* s, isize n);
CUTL_API isize _str_indexof_n(const Str* self, const char* s, isize n);
CUTL_API isize _str_lastindexof_n(const Str* self, const char* s, isize n);
CUTL_API void _str_insert_n(Str* self, isize idx, const char* s, isize n);
CUTL_API void _str_replace_n(Str* self, const char* from, isize nfrom,
                             const char* to, isize nto);

static inline bool str_startswith(const Str* self, const char* s) {
    return _str_startswith_n(self, s, strlen(s));
}

static inline bool str_endswith(const Str* self, const char* s) {
    return _str_endswith_n(self, s, strlen(s));
}

static inline bool str_startswith_str(const Str* self, const Str* s) {
    return _str_startswith_n(self, s->data, s->length);
}

static inline bool str_endswith_str(const Str* self, const Str* s) {
    return _str_endswith_n(self, s->data, s->length);
}

static isize str_indexof(const Str* self, const char* s) {
    return _str_indexof_n(self, s, strlen(s));
}

static isize str_indexof_str(const Str* self, const Str* s) {
    return _str_indexof_n(self, s->data, s->length);
}

static isize str_lastindexof(const Str* self, const char* s) {
    return _str_lastindexof_n(self, s, strlen(s));
}

static isize str_lastindexof_str(const Str* self, const Str* s) {
    return _str_lastindexof_n(self, s->data, s->length);
}

static bool str_contains(const Str* self, const char* s) {
    return str_indexof(self, s) != -1;
}

static bool str_contains_str(const Str* self, const Str* s) {
    return str_indexof_str(self, s) != -1;
}

static inline Str str_create(const char* s, Allocator a, void* ctx) {
    return str_create_n(s, strlen(s), a, ctx);
}

static inline Str str_view(const char* s) { return str_view_n(s, strlen(s)); }

static inline void str_cat(Str* self, const char* s) {
    str_ncat(self, s, strlen(s));
}

static inline void str_push(Str* self, const Str* s) {
    str_ncat(self, s->data, s->length);
}

static inline void str_insert(Str* self, isize idx, const char* s) {
    _str_insert_n(self, idx, s, strlen(s));
}

static inline void str_insert_str(Str* self, isize idx, const Str* s) {
    _str_insert_n(self, idx, s->data, s->length);
}

static inline void str_replace(Str* self, const char* from, const char* to) {
    _str_replace_n(self, from, strlen(from), to, strlen(to));
}

static inline void str_replace_str(Str* self, const Str* from, const Str* to) {
    _str_replace_n(self, from->data, from->length, to->data, to->length);
}

CUTL_API StrSplit _strsplit_create_n(const Str* self, const char* sep, isize n);
CUTL_API Str str_split_next(StrSplit* it);

static inline StrSplit strsplit_create(Str* self, const char* sep) {
    return _strsplit_create_n(self, sep, strlen(sep));
}

static inline StrSplit strsplit_create_str(Str* self, const Str* sep) {
    return _strsplit_create_n(self, sep->data, sep->length);
}

#define strsplit_foreach(self, it, sep)                                        \
    for(StrSplit _it##it = strsplit_create((self), (sep));                     \
        _it##it.pos <= (self)->length &&                                       \
        ((it) = str_split_next(&_it##it), true);)

#define strsplit_foreach_str(self, it, sep)                                    \
    for(StrSplit _it##var = strsplit_create_str((self), (sep));                \
        _it##it.pos <= (self)->length &&                                       \
        ((it) = str_split_next(&_it##it), true);)

// *** String *** //

// *** List *** //
typedef struct ListNode {
    struct ListNode* prev;
    struct ListNode* next;
} ListNode;

typedef struct List {
    ListNode* head;
    ListNode* tail;
} List;

#define list_item(self, T, m) parent_struct(self, T, m)

#define list_first(self, T, m)                                                 \
    ((self)->head ? list_item((self)->head, T, m) : NULL)

#define list_last(self, T, m)                                                  \
    ((self)->tail ? list_item((self)->tail, T, m) : NULL)

#define list_next(self, T, m)                                                  \
    ((self)->m.next ? list_item((self)->m.next, T, m) : NULL)

#define list_foreach(it, self, T, m)                                           \
    for(it = list_first((self), T, m); !!(it); it = list_next(it, T, m))

CUTL_API void list_init(List* self);
CUTL_API void list_push_tail(List* self, ListNode* n);
CUTL_API void list_push(List* self, ListNode* n);
CUTL_API void list_del(List* self, ListNode* n);
// *** List *** //

// *** HList *** //
typedef struct HListNode {
    struct HListNode* next;
    struct HListNode** backlink;
} HListNode;

typedef struct HList {
    HListNode* first;
} HList;

#define hlist_item(self, T, m) parent_struct(self, T, m)

#define hlist_foreach(it, self, T, m)                                          \
    for(it = (self)->first ? hlist_item((self)->first, T, m) : NULL; !!(it);   \
        it = it->m.next ? hlist_item(it->m.next, T, m) : NULL)

CUTL_API void hlist_push(HList* self, HListNode* n);
CUTL_API void hlist_del(HListNode* n);
// *** HList *** //

// *** Map *** //
typedef uptr (*HMapHash)(const void*);

#define define_hmap(name, b)                                                   \
    typedef struct name {                                                      \
        HList data[1 << (b)];                                                  \
        HMapHash hash;                                                         \
    } name

#define hmap_capacity(self) (sizeof((self)->data) / sizeof(*(self)->data))
#define hmap_bits(self) _hmap_bits(hmap_capacity(self))
#define hmap_index(self, k) (_hmap_hash(self, k) & (hmap_capacity(self) - 1))
#define hmap_set(self, n, k) hlist_push(&(self)->data[hmap_index(self, k)], n);
// NOLINTBEGIN
#define _hmap_hash(self, k) ((self)->hash((const void*)((uptr)k)) * 11)
// NOLINTEND

#define hmap_init(self, hashfn)                                                \
    do {                                                                       \
        _hmap_init((self)->data, hmap_capacity(self));                         \
        (self)->hash = hashfn ? (HMapHash)hashfn : _hmap_directhash;           \
    } while(0)

#define hmap_foreach_key(it, self, T, m, k)                                    \
    hlist_foreach(it, &(self)->data[hmap_index(self, k)], T, m)

#define hmap_foreach(it, self, T, m)                                           \
    for(usize __hmap_i = 0; __hmap_i < hmap_capacity(self); ++__hmap_i)        \
    hlist_foreach(it, &(self)->data[__hmap_i], T, m)

#define hmap_get(it, self, T, m, k, matchexpr)                                 \
    hmap_foreach_key(it, self, T, m, k) {                                      \
        if(matchexpr) break;                                                   \
    }

#define hmap_del(it, self, T, m, k, matchexpr)                                 \
    hmap_foreach_key(it, self, T, m, k) {                                      \
        if(matchexpr) {                                                        \
            hlist_del(&it->m);                                                 \
            break;                                                             \
        }                                                                      \
    }

CUTL_API void _hmap_init(HList* self, usize n);
CUTL_API uptr _hmap_directhash(const void* p);
CUTL_API unsigned long _hmap_bits(unsigned long cap);
// *** Map *** //

#if defined(__cplusplus)
}
#endif
