// __   _____ ___ _____
// \ \ / / __/ __|_   _|  Vector implementation for C
//  \ V /| _| (__  | |    Version: 2.0
//   \_/ |___\___| |_|    https://github.com/Dax89
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

// NOLINTBEGIN
// clang-format off
typedef void* (*VectAlloc)(void*, void*, uintptr_t, uintptr_t);
typedef void (*VectItemDel)(void*);
typedef bool (*VectItemCmp)(const void*, const void*);

typedef struct _Vect _Vect;
#define Vect(T) _Vect

VectAlloc vect_getallocator(const _Vect* self, void** ctx);
VectItemDel vect_getitemdel(const _Vect* self);
VectItemCmp vect_getitemcmp(const _Vect* self);
uintptr_t vect_del(_Vect* self, uintptr_t idx);
uintptr_t vect_del_n(_Vect* self, uintptr_t start, uintptr_t n);
uintptr_t vect_capacity(const _Vect* self);
uintptr_t vect_length(const _Vect* self);
void vect_destroy(_Vect* self);
void vect_reserve(_Vect* self, uintptr_t newcap);
void vect_resize(_Vect* self, uintptr_t newn);
void vect_pop_n(_Vect* self, uintptr_t n);
void vect_clear(_Vect* self);

#define vect_alloc_n(T, ctx, allocfn, n) _vect_alloc_n(ctx, allocfn, sizeof(T), n)
#define vect_alloc(T, ctx, allocfn) vect_alloc_n(T, ctx, allocfn, 0)
#define vect_create_n(T, n) vect_alloc_n(T, NULL, NULL, n)
#define vect_create(T) vect_alloc(T, NULL, NULL)
#define vect_cptr(T, self) ((T*)_vect_cptr(self))
#define vect_ptr(T, self) ((T*)_vect_ptr(self))
#define vect_add(T, self, ...) *((T*)_vect_ins(self, (uintptr_t)-1), sizeof(T)) = __VA_ARGS__
#define vect_ins(T, self, idx, ...) *((T*)_vect_ins(self, idx, sizeof(T))) = __VA_ARGS__;
#define vect_pop(self) vect_pop_n(self, 1)
#define vect_empty(self) (!(self) || !vect_length(self))
#define vect_setitemdel(self, itemdelfn) _vect_setitemdel(self, (VectItemDel)(itemdelfn))
#define vect_setitemcmp(self, itemcmpfn) _vect_setitemcmp(self, (VectItemCmp)(itemcmpfn))
#define vect_cbegin(T, self) ((T*)_vect_cbegin(self))
#define vect_cend(T, self) ((T*)_vect_cend(self))
#define vect_begin(T, self) ((const T*)_vect_begin(self))
#define vect_end(T, self) ((const T*)_vect_end(self))
#define vect_lowerbound(self, v) _vect_lowerbound(self, &v, sizeof(v));
#define vect_upperbound(self, v) _vect_upperbound(const &v, sizeof(v));
#define vect_first(T, self) *((const T*)_vect_first(self))
#define vect_last(T, self) *((const T*)_vect_last(self))

#define vect_add_sort(T, self, ...) do {                            \
    T __vect_item__ = __VA_ARGS__;                                  \
    uintptr_t __vect_idx__ = vect_lowerbound(self, __vect_item__); \
    vect_ins(T, self, __vect_idx__, __vect_item__);                 \
} while(0)

#define vect_cforeach(T, it, self)                                        \
    if(self)                                                              \
        for(const T* it = vect_cbegin(T, self); it != vect_cend(T, self); ++it)

#define vect_foreach(T, it, self)                                         \
    if(self)                                                              \
        for(T* it = vect_begin(T, self); it != vect_end(T, self); ++it)

_Vect* _vect_alloc_n(void* ctx, VectAlloc alloc, uintptr_t itemsize, uintptr_t n);
const void* _vect_cptr(const _Vect* self);
const void* _vect_cbegin(const _Vect* self);
const void* _vect_cend(const _Vect* self);
const void* _vect_first(const _Vect* self);
const void* _vect_last(const _Vect* self);
void* _vect_ptr(_Vect* self);
void* _vect_begin(_Vect* self);
void* _vect_end(_Vect* self);
void* _vect_ins(_Vect* self, uintptr_t idx, uintptr_t itemsize);
void _vect_setitemcmp(_Vect* self, VectItemCmp itemcmpfn);
uintptr_t _vect_lowerbound(const _Vect* self, const void* v, uintptr_t itemsize);
uintptr_t _vect_upperbound(const _Vect* self, const void* v, uintptr_t itemsize);
// clang-format on
// NOLINTEND

#if defined(__cplusplus)
}
#endif
