//  __  __   _   ___
// |  \/  | /_\ | _ \  Open-Addressing HashTable for C
// | |\/| |/ _ \|  _/  Version: 2.0
// |_|  |_/_/ \_\_|    https://github.com/Dax89
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
#include <string.h>

// NOLINTBEGIN
// clang-format off
typedef struct _Map _Map;

typedef struct MapIterator {
    const _Map* map;
    uintptr_t index;
    const void* key;
    const void* value;
} MapIterator;

typedef void* (*MapAlloc)(void* ctx, void* ptr, uintptr_t osize, uintptr_t nsize);
typedef uintptr_t (*MapHash)(const void*);
typedef uintptr_t (*MapProbe)(const _Map*, uintptr_t);
typedef bool (*MapEquals)(const void*, const void*);
typedef void (*MapKeyDel)(void*);
typedef void (*MapValueDel)(void*);

#define Map(K, V) _Map

MapAlloc map_getallocator(const _Map* self, void** ctx);
MapHash map_gethash(const _Map* self);
MapProbe map_getprobe(const _Map* self);
MapEquals map_getequals(const _Map* self);
MapKeyDel map_getkeydel(const _Map* self);
MapValueDel map_getvaluedel(const _Map* self);
uintptr_t map_capacity(const _Map* self);
uintptr_t map_length(const _Map* self);
float map_loadfactor(const _Map* self);
void map_rehash(_Map* self, uintptr_t ncap);
void map_reserve(_Map* self, uintptr_t n);
void map_clear(_Map* self);
void map_destroy(_Map* self);
MapIterator map_iterator(const _Map* self);
bool map_hasnext(const MapIterator* self);
void map_next(MapIterator* self);

// Hashing Helpers
uintptr_t map_hash_str(const char* arg);
bool map_equals_str(const char* lhs, const char* rhs);
// Hashing Helpers

#define map_alloc_full_n(ctx, allocfn, n, hashfn, equalsfn, probefn) \
    _map_alloc_full_n(ctx, allocfn, n, (MapHash)hashfn, (MapEquals)equalsfn, (MapProbe)probefn) 
#define map_alloc_n(ctx, allocfn, n) map_alloc_full_n(ctx, allocfn, n, NULL, NULL, NULL) 
#define map_alloc_full(ctx, allocfn, hashfn, equalsfn, probefn) map_alloc_full_n(ctx, allocfn, n, allocfn, hashfn, equalsfn, probefn) 
#define map_alloc(ctx, allocfn) map_alloc_n(ctx, allocfn, 0) 
#define map_create() map_alloc(NULL, NULL) 
#define map_create_n(n) map_alloc_n(NULL, NULL, n) 
#define map_create_full_n(n, hashfn, equalsfn, probefn) map_alloc_full_n(NULL, NULL, n, hashfn, equalsfn, probefn) 
#define map_create_full(hashfn, equalsfn, probefn) map_create_full_n(0, hashfn, equalsfn, probefn);
#define map_del(self, k) _map_del(self, (void*)((uintptr_t)(k)))
#define map_set(self, k, v) _map_set(self, (void*)((uintptr_t)(k)), (void*)((uintptr_t)(v)))
#define map_get(K, self, k) (const K*)((uintptr_t)_map_get(self, (const void*)((uintptr_t)(k))))
#define map_get_ex(self, k, outkey, outval) \
    _map_get_ex(self, (const void*)((uintptr_t)(k)), (const void**)((uintptr_t)(outkey)), (const void**)((uintptr_t)(outval)))
#define map_empty(self) (!(self) || !map_length(self))
#define map_setkeydel(self, keydelfn) _map_setkeydel(self, (MapKeyDel)keydelfn)
#define map_setvaluedel(self, valuedelfn) _map_setvaluedel(self, (MapValueDel)valuedelfn)
#define map_contains(self, k) map_get_ex(self, k, NULL, NULL)
#define map_getkeys(K, self) ((K**)_map_getkeys(self))
#define map_getvalues(K, self) ((K**)_map_getvalues(self))
#define mapkv_destroy(kv) _mapkv_destroy((void**)kv)

#define mapiterator_key(T, it) ((const T*)it.key)
#define mapiterator_value(T, it) ((const T*)it.value)

#define map_foreach(item, self) \
    for(MapIterator item = map_iterator(self); map_hasnext(&item); map_next(&it))

_Map* _map_alloc_full_n(void* ctx, MapAlloc allocfn, uintptr_t cap, MapHash hashfn, MapEquals equalsfn, MapProbe probefn);
void _map_setkeydel(_Map* self, MapKeyDel keydelfn);
void _map_setvaluedel(_Map* self, MapValueDel valuedelfn);
void _map_set(_Map* self, void* key, void* value);
void _map_del(_Map* self, const void* key);
bool _map_get_ex(const _Map* self, const void* key, const void** outkey, const void** outval);
const void* _map_get(const _Map* self, const void* key);
void** _map_getkeys(const _Map* self);
void** _map_getvalues(const _Map* self);
void _mapkv_destroy(void** kv);

// clang-format off
// NOLINTEND

#if defined(__cplusplus)
}
#endif
