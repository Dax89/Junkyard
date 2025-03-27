#include "vect.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _Vect {
    void* items;
    uintptr_t length;
    uintptr_t capacity;
    uintptr_t itemsize;
    VectItemDel itemdel;
    VectItemCmp itemcmp;
    VectAlloc alloc;
    void* ctx;
} _Vect;

// NOLINTBEGIN
static void* _vect_defaultalloc(void* ctx, void* ptr, uintptr_t osize,
                                uintptr_t nsize) {
    (void)ctx;

    if(nsize == 0) {
        free(ptr);
        return NULL;
    }

    void* p = calloc(1, nsize);

    if(!p) {
        fprintf(stderr, "Vect allocation failed\n");
        abort();
        return NULL;
    }

    if(ptr) {
        memcpy(p, ptr, osize);
        free(ptr);
    }

    return p;
}

void* _vect_ins(_Vect* self, uintptr_t idx, uintptr_t itemsize) {
    if(!self) return NULL;
    assert(self->itemsize == itemsize && "_vect_ins: wrong item type");

    uintptr_t len = self->length;
    if(idx == (uintptr_t)-1) idx = len; // Append mode
    assert(idx <= len && "_vect_ins: index out of range");

    uintptr_t newlen = len + 1;

    // Reserve space
    if(newlen > self->capacity) {
        uintptr_t c = self->capacity ? self->capacity : 1;
        while(c < newlen)
            c <<= 1;
        vect_reserve(self, c);
    }

    // If we are appending (pos == len), skip memmove
    if(idx < len) {
        // Shift existing content after the position
        memmove((char*)self->items + ((idx + 1) * self->itemsize),
                (char*)self->items + (idx * self->itemsize),
                (len - idx) * self->itemsize);
    }

    self->length++; // Update the length
    return (char*)self->items + (idx * self->itemsize);
}

const void* _vect_cptr(const _Vect* self) {
    return self && self->items ? self->items : NULL;
}
void* _vect_ptr(_Vect* self) {
    return self && self->items ? self->items : NULL;
}

const void* _vect_cbegin(const _Vect* self) {
    return self && self->items ? self->items : NULL;
}

const void* _vect_cend(const _Vect* self) {
    return self && self->items
               ? (char*)self->items + (self->length * self->itemsize)
               : NULL;
}

void* _vect_begin(_Vect* self) {
    return self && self->items ? self->items : NULL;
}

void* _vect_end(_Vect* self) {
    return self && self->items
               ? (char*)self->items + (self->length * self->itemsize)
               : NULL;
}

const void* _vect_first(const _Vect* self) {
    assert(self && "_vect_first: self is null");
    assert(self->length && "_vect_first: vect is empty");
    return _vect_cbegin(self);
}

const void* _vect_last(const _Vect* self) {
    assert(self && "_vect_last: self is null");
    assert(self->length && "_vect_last: vect is empty");
    return (const char*)_vect_cend(self) - self->itemsize;
}

void _vect_setitemdel(_Vect* self, VectItemDel itemdelfn) {
    if(self) self->itemdel = itemdelfn;
}

void _vect_setitemcmp(_Vect* self, VectItemCmp itemcmpfn) {
    if(self) self->itemcmp = itemcmpfn;
}

// NOLINTEND

VectAlloc vect_getallocator(const _Vect* self, void** ctx) {
    if(self) {
        if(ctx) *ctx = self->ctx;
        return self->alloc;
    }

    return NULL;
}

VectItemDel vect_getitemdel(const _Vect* self) {
    return self ? self->itemdel : NULL;
}

VectItemCmp vect_getitemcmp(const _Vect* self) {
    return self ? self->itemcmp : NULL;
}

uintptr_t vect_size(void) { return sizeof(_Vect); }

_Vect* _vect_alloc_n(void* ctx, VectAlloc alloc, uintptr_t itemsize,
                     uintptr_t n) {
    if(!alloc) alloc = _vect_defaultalloc;

    _Vect* self = (_Vect*)alloc(ctx, NULL, 0, sizeof(_Vect));
    *self = (_Vect){0};
    self->ctx = ctx;
    self->alloc = alloc;
    self->itemsize = itemsize;
    self->length = n;
    if(self->length) vect_resize(self, n);
    return self;
}

void vect_destroy(_Vect* self) {
    if(!self) return;
    vect_clear(self);

    if(self->items) {
        self->alloc(self->ctx, self->items, (self->itemsize * self->capacity),
                    0);
    }

    self->alloc(self->ctx, self, sizeof(_Vect), 0);
}

uintptr_t vect_capacity(const _Vect* self) { return self ? self->capacity : 0; }
uintptr_t vect_length(const _Vect* self) { return self ? self->length : 0; }

void vect_clear(_Vect* self) {
    if(!self) return;

    if(self->itemdel) {
        char* begin = (char*)self->items;
        char* end = (char*)self->items + (self->length * self->itemsize);

        for(char* it = begin; it != end; it += self->itemsize)
            self->itemdel(it);
    }

    self->length = 0;
}

void vect_reserve(_Vect* self, uintptr_t newcap) {
    if(self->capacity >= newcap) return;

    self->items = (char*)self->alloc(self->ctx, self->items,
                                     (self->itemsize * self->capacity),
                                     (self->itemsize * newcap));
    self->capacity = newcap;
}

void vect_resize(_Vect* self, uintptr_t newn) {
    if(self) {
        vect_reserve(self, newn);
        self->length = newn;
    }
}

uintptr_t vect_del(_Vect* self, uintptr_t idx) {
    assert(idx < self->length && "vect_del: index out of range");
    if(self->itemdel)
        self->itemdel((char*)self->items + (idx * self->itemsize));

    // Shift elements left if not removing the last element
    if(idx < self->length - 1) {
        memmove((char*)self->items + (idx * self->itemsize),
                (char*)self->items + ((idx + 1) * self->itemsize),
                (self->length - idx - 1) * self->itemsize);
    }

    self->length--; // Update length
    return (idx < self->length) ? idx : 0;
}

uintptr_t vect_del_n(_Vect* self, uintptr_t start, uintptr_t n) {
    assert(start < self->length && "vect_del_n: index out of range");
    if(!n) return start;

    uintptr_t end = start + n;
    // Clamp 'end'
    if(end > self->length) end = self->length;

    if(self->itemdel) {
        for(uintptr_t idx = start; idx < end; ++idx)
            self->itemdel((char*)self->items + (idx * self->itemsize));
    }

    // Shift remaining elements left (if necessary)
    if(end < self->length) {
        memmove((char*)self->items + (start * self->itemsize),
                (char*)self->items + (end * self->itemsize),
                (self->length - end) * self->itemsize);
    }

    self->length -= (end - start);
    return (start < self->length) ? start : 0;
}

void vect_pop_n(_Vect* self, uintptr_t n) {
    if(self) {
        if(self->length > n) self->length -= n;
        else self->length = 0;
    }
}

uintptr_t _vect_lowerbound(const _Vect* self, const void* v,
                           uintptr_t itemsize) {
    assert(self->itemcmp && "_vect_lowerboud: vect is not sorted");
    assert(self->itemsize == itemsize && "_vect_lowerbound: wrong item type");
    if(!vect_length(self)) return vect_length(self);

    uintptr_t left = 0, right = self->length;

    while(left < right) {
        uintptr_t mid = left + ((right - left) / 2);
        if(self->itemcmp((char*)self->items + (mid * self->itemsize), v))
            left = mid + 1;
        else right = mid;
    }

    return left;
}

uintptr_t _vect_upperbound(const _Vect* self, const void* v,
                           uintptr_t itemsize) {
    assert(self->itemcmp && "vect_upperbound: vect is not sorted");
    assert(self->itemsize == itemsize && "_vect_upperbound: wrong item type");
    if(!self->itemcmp || !vect_length(self)) return vect_length(self);

    uintptr_t left = 0, right = self->length;

    while(left < right) {
        uintptr_t mid = left + ((right - left) / 2);
        if(!self->itemcmp(v, (char*)self->items + (mid * self->itemsize)))
            left = mid + 1;
        else right = mid;
    }

    return left;
}
