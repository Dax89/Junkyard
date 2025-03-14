//  ___ _____ ___
// / __|_   _| _ \  String implementation for C
// \__ \ | | |   /  Version: 2.0
// |___/ |_| |_|_\  https://github.com/Dax89
//
// SPDX-FileCopyrightText: 2025 Antonio Davide Trogu <contact@antoniodavide.dev>
// SPDX-License-Identifier: MIT

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// clang-format off
// NOLINTBEGIN
typedef void* (*StrAlloc)(void* ctx, void* ptr, uintptr_t osize, uintptr_t nsize);
typedef struct Str Str;

Str* str_alloc_n(void* ctx, StrAlloc alloc, const char* s, uintptr_t n);
void str_destroy(Str* self);
void str_reserve(Str* self, uintptr_t newcap);
void str_resize(Str* self, uintptr_t newn);
void str_clear(Str* self);
void str_ins_n(Str* self, uintptr_t idx, const char* s, uintptr_t n);
void str_del(Str* self, uintptr_t start, uintptr_t n);
void str_replace_n(Str* self, const char* oldsub, uintptr_t oldn, const char* newsub, uintptr_t newn);
void str_pop_n(Str* self, uintptr_t n);
bool str_startswith_n(const Str* self, const char* prefix, uintptr_t n);
bool str_endswith_n(const Str* self, const char* suffix, uintptr_t n);
bool str_equals_n(const Str* self, const char* rhs, uintptr_t n);
bool str_equals(const Str* self, const Str* rhs);
const char* str_cptr(const Str* self);
char* str_ptr(Str* self);
char* str_ref(Str* self, uintptr_t idx);
char str_at(const Str* self, uintptr_t idx);
uintptr_t str_hash(const Str* self);
StrAlloc str_getallocator(const Str* self, void** ctx);
uintptr_t str_lastindex_n(const Str* self, const char* s, uintptr_t n);
uintptr_t str_index_n(const Str* self, const char* s, uintptr_t n);
uintptr_t str_capacity(const Str* self);
uintptr_t str_length(const Str* self);
char str_first(const Str* self);
char str_last(const Str* self);
// NOLINTEND

#define str_npos ((uintptr_t)-1)
#define str_alloc(ctx, allocfn) str_alloc_n(ctx, allocfn, 0)
#define str_alloc_lit(ctx, allocfn, s) str_alloc_n(ctx, allocfn, s, sizeof(s) - 1)
#define str_alloc_from(ctx, allocfn, s) str_alloc_n(ctx, allocfn, s, strlen(s))
#define str_alloc_from_strv(ctx, allocfn, strv) str_alloc_n(ctx, allocfn, (strv).str, (strv).length)
#define str_create() str_alloc(NULL, NULL)
#define str_create_n(n) str_alloc_n(NULL, NULL, n)
#define str_create_lit(s) str_alloc_lit(NULL, NULL, s)
#define str_create_from(s) str_alloc_from(NULL, NULL, s)
#define str_create_from_strv(strv) str_alloc_from_strv(NULL, NULL, strv)
#define str_add(self, s) str_ins_n(self, str_npos, s, strlen(s))
#define str_add_lit(self, s) str_ins_n(self, str_npos, s, sizeof(s) - 1)
#define str_add_n(self, s) str_ins_n(self, str_npos, s, strlen(s)
#define str_ins(self, idx, s) str_ins_n(self, idx, s, strlen(s))
#define str_ins_lit(self, idx, s) str_ins_n(self, idx, s, sizeof(s) - 1)
#define str_replace(self, oldsub, newsub) str_replace_n(self, oldsub, strlen(oldsub), newsub, strlen(newsub))
#define str_replace_lit(self, oldsub, newsub) str_replace_n(self, oldsub, sizeof(oldsub) - 1, newsub, sizeof(newsub) - 1)
#define str_pop(self) str_pop_n(self, 1)
#define str_empty(self) (!(self) || !str_length(self))
#define str_equals_c(self, rhs) str_equals_n(self, rhs, strlen(rhs))
#define str_equals_lit(self, rhs) str_equals_n(self, rhs, sizeof(rhs) - 1)
#define str_startswith(self, s) str_startswith_n(self, s, strlen(s))
#define str_startswith_lit(self, s) str_startswith_n(self, s, sizeof(s) - 1)
#define str_endswith(self, s) str_endswith_n(self, s, strlen(s))
#define str_endswith_lit(self, s) str_endswith_n(self, s, sizeof(s) - 1)
#define str_index(self, s) str_index_n((self), s, strlen(s))
#define str_index_lit(self, s) str_index_n((self), s, sizeof(s) - 1)
#define str_lastindex(self, s) str_lastindex_n((self), s, strlen(s))
#define str_lastindex_lit(self, s) str_lastindex_n((self), s, sizeof(s) - 1)
#define str_contains(self, s) (str_index(self, s) != str_npos)
#define str_cbegin(self) str_cptr(self)
#define str_cend(self) (str_cptr(self) + str_length(self))
#define str_begin(self) str_ptr(self)
#define str_end(self) (str_ptr(self) + str_length(self))

// NOLINTBEGIN
#define str_foreach(item, self)  \
    if(self)                     \
        for(char* item = str_begin(self); item != str_end(self); item++)

#define str_cforeach(item, self) \
    if(self)                     \
        for(const char* item = str_cbegin(self); item != str_cend(self); item++)

// Generic string functions
uintptr_t _cstr_hash(const char* s, uintptr_t n);
uintptr_t _cstr_index_n(const char* s1, uintptr_t n1, const char* s2, uintptr_t n2);
uintptr_t _cstr_lastindex_n(const char* s1, uintptr_t n1, const char* s2, uintptr_t n2);
uintptr_t _cstr_startswith_n(const char* s1, uintptr_t n1, const char* s2, uintptr_t n2);
uintptr_t _cstr_endswith_n(const char* s1, uintptr_t n1, const char* s2, uintptr_t n2);
uintptr_t _cstr_equals_n(const char* s1, uintptr_t n1, const char* s2, uintptr_t n2);
// NOLINTEND

// clang-format on

#if defined(__cplusplus)
}
#endif
