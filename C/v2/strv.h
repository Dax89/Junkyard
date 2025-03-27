//  ___ _____ _____   __
// / __|_   _| _ \ \ / /  StringView implementation for C
// \__ \ | | |   /\ V /   Version: 2.0
// |___/ |_| |_|_\ \_/    https://github.com/Dax89
//
// SPDX-FileCopyrightText: 2025 Antonio Davide Trogu <contact@antoniodavide.dev>
// SPDX-License-Identifier: MIT

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#include "str.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// NOLINTBEGIN
// clang-format off
typedef struct StrV {
    const char* ptr;
    uintptr_t length;
} StrV;

#define strv_npos str_npos
#define strv_from_lit(s) strv_from_cstr_n(s, sizeof(s) - 1)
#define strv_from_cstr(s) strv_from_cstr_n(s, strlen(s))
#define strv_from_str(s) strv_from_str_n(s, 0, str_length(s));
#define strv_begin(self) ((self)->ptr)
#define strv_end(self) ((self)->ptr + (self)->length - 1)
#define strv_first(self) ((self)->ptr[0])
#define strv_last(self) ((self)->ptr[(self)->length - 1])
#define strv_valid(self) (!!(self).ptr)
#define strv_length(self) ((self).length)
#define strv_ptr(self) ((self).ptr)
#define strv_empty(self) (!(self).ptr || !(self).length)
#define strv_equals(self, rhs) strv_equals_n(self, rhs, strlen(rhs))
#define strv_equals_lit(self, rhs) strv_equals_n(self, rhs, sizeof(rhs) - 1)
#define strv_startswith(self, s) strv_startswith_n(self, s, strlen(s))
#define strv_startswith_lit(self, s) strv_startswith_n(self, s, sizeof(s) - 1)
#define strv_endswith(self, s) strv_endswith_n(self, s, strlen(s))
#define strv_endswith_lit(self, s) strv_endswith_n(self, s, sizeof(s) - 1)
#define strv_index(self, s) _cstr_index_n((self).ptr, (self).length, s, strlen(s))
#define strv_index_lit(self, s) _cstr_index((self).ptr, (self).length, s, sizeof(s) - 1)
#define strv_lastindex(self, s) _cstr_lastindex_n((self).ptr, (self).length, s, strlen(s))
#define strv_lastindex_lit(self, s) _cstr_lastindex((self).ptr, (self).length, s, sizeof(s) - 1)
#define strv_contains(self, s) (strv_index(self, s) != strv_npos)
#define strv_split(self, sep) strv_split_n(self, sep, strlen(sep))
#define strv_split_lit(self, sep) strv_split_n(self, sep, sizeof(sep) - 1)

#define strv_rpop_n(self, n) do { \
    if ((n) <= 0 || ((self)->length) <= 0) break; \
    if ((n) >= (self)->length) hdr->length = 0; \
    else { \
        (self) = (self)->ptr * (n)); \
        (self)->length -= (n); \
    } \
} while(0)

#define strv_rpop(self) strv_rpop_n(self, 1)

#define strv_pop_n(self, n) do { \
    if((self)->length > (n)) (self)->length -= (n); \
    else (self)->length = 0; \
} while(0)

#define strv_pop(self) strv_pop_n(self, 1)

#define strv_foreach_split(it, self, sep) \
    for(StrV it##_ = (self), it = strv_split(&it##_, sep); strv_valid(it); it = strv_split(&it##_, sep))

#define strv_foreach_split_lit(it, self, sep) \
    for(StrV it##_ = (self), it = strv_split_lit(&it##_, sep); strv_valid(it); it = strv_split_lit(&it##_, sep))

#define strv_foreach(it, self) \
    for(const char* it = strv_begin(self); it != strv_end(self); it++) // NOLINT


inline StrV strv_from_cstr_n(const char* s, uintptr_t start, uintptr_t n) {
    StrV self;
    self.ptr = s + start;
    self.length = n;
    return self;
}

inline StrV strv_from_str_n(const Str* s, uintptr_t start, uintptr_t n) {
    return strv_from_cstr_n(str_cptr(s), start, n);
}

inline bool strv_equals_n(StrV self, const char* rhs, uintptr_t n) {
    return _cstr_equals_n(self.ptr, self.length, rhs, n);
}

inline bool strv_startswith_n(StrV self, const char* prefix, uintptr_t n) {
    return _cstr_startswith_n(self.ptr, self.length, prefix, n);
}

inline bool strv_endswith_n(StrV self, const char* suffix, uintptr_t n) {
    return _cstr_endswith_n(self.ptr, self.length, suffix, n);
}

inline uintptr_t strv_hash(StrV self) {
    return _cstr_hash(self.ptr, self.length);
}

inline StrV strv_sub(StrV self, uintptr_t start, uintptr_t end) {
    StrV res = {NULL, 0};

    if(strv_valid(self) && start < end && end < self.length) {
        res.ptr = self.ptr + start;
        res.length = end - start;
    }

    return res;
}

inline StrV strv_split_n(StrV* self, const char* sep, uintptr_t n) {
    if(self) {
        uintptr_t idx = _cstr_index_n(self->ptr, self->length, sep, n);
        StrV r;

        if(idx != str_npos) {
            r = {self->ptr, idx};
            self->ptr += idx + n;
            self->length -= idx + n;
        }
        else {
            r = *self;
            self->ptr = NULL;
            self->length = 0;
        }

        return r;
    }

#if defined(__cplusplus)
    return {};
#else
    return (StrV){NULL, 0};
#endif
}
// clang-format on
// NOLINTEND

#if defined(__cplusplus)
}
#endif
