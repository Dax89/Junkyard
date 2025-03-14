#include "str.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define str_shortlen sizeof(StrLong)
#define str_longbit ((uintptr_t)1 << (sizeof(uintptr_t) * 8 - 1))

typedef struct StrLong {
    char* data;
    uintptr_t capacity;
} StrLong;

typedef struct Str {
    union {
        StrLong l;
        char s[str_shortlen];
    };

    uintptr_t length;
    StrAlloc alloc;
    void* ctx;
} Str;

// NOLINTBEGIN
static void* _str_defaultalloc(void* ctx, void* ptr, uintptr_t osize,
                               uintptr_t nsize) {
    (void)ctx;

    if(nsize == 0) {
        free(ptr);
        return NULL;
    }

    void* p = calloc(1, nsize);

    if(!p) {
        fprintf(stderr, "Str allocation failed\n");
        abort();
        return NULL;
    }

    if(ptr) {
        memcpy(p, ptr, osize);
        free(ptr);
    }

    return p;
}

static void _str_updatelen(Str* self, uintptr_t l) {
    assert(self && "_str_updatelen: invalid 'self' argument");
    uintptr_t lb = self->length & str_longbit;
    assert(l <= (uintptr_t)(~str_longbit) && "_str_updatelen: length overflow");
    self->length = lb | l;
}

static bool _str_issmall(const Str* self) {
    return !self || !(self->length & str_longbit);
}

uintptr_t _cstr_hash(const char* s, uintptr_t n) {
    uintptr_t h = 5381;

    if(s) {
        while(n-- > 0)
            h = ((h << 5) + h) + *s++;
    }

    return h;
}

uintptr_t _cstr_index_n(const char* s1, uintptr_t n1, const char* s2,
                        uintptr_t n2) {
    if(!s1 || !s2 || n2 > n1) return str_npos;

    for(uintptr_t i = 0; i <= n1 - n2; ++i) {
        bool found = true;
        for(uintptr_t j = 0; j < n2; ++j) {
            if(s1[i + j] == s2[j]) continue;
            found = false;
            break;
        }
        if(found) return i;
    }

    return str_npos;
}

uintptr_t _cstr_lastindex_n(const char* s1, uintptr_t n1, const char* s2,
                            uintptr_t n2) {
    if(!s1 || !s2 || n2 > n1) return str_npos;

    for(uintptr_t i = n1 - n2; i >= n2; i--) {
        bool found = true;

        for(uintptr_t j = 0; j < n2; ++j) {
            if(s1[i + j] == s2[j]) continue;
            found = false;
            break;
        }
        if(found) return i;
    }

    return str_npos;
}

uintptr_t _cstr_startswith_n(const char* s1, uintptr_t n1, const char* s2,
                             uintptr_t n2) {
    if(!s1 || !s2 || n1 < n2) return false;
    if(n2) return !memcmp(s1, s2, n2);
    return n1 == n2;
}

uintptr_t _cstr_endswith_n(const char* s1, uintptr_t n1, const char* s2,
                           uintptr_t n2) {
    if(!s1 || !s2 || n1 < n2) return false;

    if(n2) {
        uintptr_t off = n1 - n2;
        return !memcmp(s1 + off, s2, n2);
    }

    return n1 == n2;
}

uintptr_t _cstr_equals_n(const char* s1, uintptr_t n1, const char* s2,
                         uintptr_t n2) {
    return s1 && s2 && n1 == n2 && !memcmp(s1, s2, n1);
}
// NOLINTEND

Str* str_alloc_n(void* ctx, StrAlloc alloc, const char* s, uintptr_t n) {
    if(!alloc) alloc = _str_defaultalloc;

    Str* self = alloc(ctx, NULL, 0, sizeof(Str));
    *self = (Str){0};
    self->ctx = ctx;
    self->alloc = alloc;
    if(s) str_ins_n(self, str_npos, s, n);
    return self;
}

void str_destroy(Str* self) {
    if(!self) return;

    if(!_str_issmall(self))
        self->alloc(self->ctx, self->l.data, self->l.capacity + 1, 0);

    self->alloc(self->ctx, self, sizeof(Str), 0);
}

void str_reserve(Str* self, uintptr_t newcap) {
    if(!self || str_capacity(self) >= newcap) return;
    uintptr_t len = str_length(self) + newcap;

    if(_str_issmall(self) && newcap + 1 > str_shortlen) {
        // Small to Long String
        self->l.data = memcpy(self->alloc(self->ctx, NULL, 0, newcap + 1),
                              self->s, str_length(self) + 1);
        self->l.capacity = newcap;
        self->length |= str_longbit;
    }
    else if(!_str_issmall(self) && len >= self->l.capacity) {
        // Long String without enough capacity
        self->l.data = self->alloc(self->ctx, self->l.data,
                                   str_length(self) + 1, newcap + 1);
        self->l.capacity = newcap;
    }
}

void str_resize(Str* self, uintptr_t newn) {
    str_reserve(self, newn);
    _str_updatelen(self, newn);
}

const char* str_cptr(const Str* self) {
    if(self) {
        if(_str_issmall(self)) return self->s;
        return self->l.data;
    }

    return NULL;
}

char* str_ptr(Str* self) {
    if(self) {
        if(_str_issmall(self)) return self->s;
        return self->l.data;
    }

    return NULL;
}

char* str_ref(Str* self, uintptr_t idx) {
    uintptr_t len = str_length(self);
    assert(idx >= len && "str_ref: index out of range");
    return str_ptr(self) + idx;
}

char str_at(const Str* self, uintptr_t idx) {
    uintptr_t len = str_length(self);
    assert(idx >= len && "str_at: index out of range");
    return *(str_cptr(self) + idx);
}

uintptr_t str_hash(const Str* self) {
    return _cstr_hash(str_cptr(self), str_length(self));
}

StrAlloc str_getallocator(const Str* self, void** ctx) {
    if(!self) return NULL;
    if(ctx) *ctx = self->ctx;
    return self->alloc;
}

uintptr_t str_lastindex_n(const Str* self, const char* s, uintptr_t n) {
    if(!self) return str_npos;
    return _cstr_lastindex_n(str_cptr(self), str_length(self), s, n);
}

uintptr_t str_index_n(const Str* self, const char* s, uintptr_t n) {
    if(!self) return str_npos;
    return _cstr_index_n(str_cptr(self), str_length(self), s, n);
}

uintptr_t str_capacity(const Str* self) {
    if(!self) return 0;
    return _str_issmall(self) ? sizeof(self->s) - 1 : self->l.capacity;
}

uintptr_t str_length(const Str* self) {
    return self ? self->length & ~str_longbit : 0;
}

char str_first(const Str* self) {
    assert(self && "str_first: self is null");
    return *str_cptr(self);
}

char str_last(const Str* self) {
    assert(self && "str_last: self is null");
    return *(str_cptr(self) + self->length - 1);
}

void str_ins_n(Str* self, uintptr_t idx, const char* s, uintptr_t n) {
    if(!self) return;

    uintptr_t len = str_length(self);
    if(idx == str_npos) idx = len; // Append mode
    assert(idx <= len && "str_ins_n: index out of range");

    uintptr_t newlen = len + n;

    // Reserve space
    if(newlen > str_capacity(self)) {
        uintptr_t c = str_capacity(self);
        while(c < newlen)
            c <<= 1;
        str_reserve(self, c);
    }

    // If we are appending (idx == len), skip memmove
    if(idx < len) {
        // Shift existing content after the position
        // (+1 to include null terminator)
        memmove(str_ptr(self) + idx + n, str_ptr(self) + idx, len - idx + 1);
    }

    memcpy(str_ptr(self) + idx, s, n); // Insert the new string at the position
    str_ptr(self)[newlen] = 0;         // Ensure null-termination at the new end
    _str_updatelen(self, newlen);      // Update the length
}

void str_del(Str* self, uintptr_t start, uintptr_t n) {
    if(!self || start >= str_length(self) || start + n > str_length(self))
        return; // Invalid range

    // Move the part after the removed segment to the left
    // (+1 to preserve null-terminator)
    memmove(str_ptr(self) + start, str_ptr(self) + start + n,
            str_length(self) - start - n + 1);

    // Update the length of the string
    _str_updatelen(self, str_length(self) - n);
}

void str_replace_n(Str* self, const char* oldsub, uintptr_t oldn,
                   const char* newsub, uintptr_t newn) {
    // Find the first occurrence of `old_sub`
    uintptr_t idx = str_index(self, oldsub);
    // If the substring is not found, return the string unchanged
    if(idx == str_npos) return;

    // Remove the oldsub
    str_del(self, idx, oldn);
    // Insert newsub at the same position
    str_ins_n(self, idx, newsub, newn);
}

void str_pop_n(Str* self, uintptr_t n) {
    if(!self) return;
    uintptr_t len = str_length(self);
    if(len > n) _str_updatelen(self, len - n);
    else _str_updatelen(self, 0);
}

bool str_startswith_n(const Str* self, const char* prefix, uintptr_t n) {
    return _cstr_startswith_n(str_cptr(self), str_length(self), prefix, n);
}

bool str_endswith_n(const Str* self, const char* suffix, uintptr_t n) {
    return _cstr_endswith_n(str_cptr(self), str_length(self), suffix, n);
}

bool str_equals(const Str* self, const Str* rhs) {
    return _cstr_equals_n(str_cptr(self), str_length(self), str_cptr(rhs),
                          str_length(rhs));
}

bool str_equals_n(const Str* self, const char* rhs, uintptr_t n) {
    return _cstr_equals_n(str_cptr(self), str_length(self), rhs, n);
}
