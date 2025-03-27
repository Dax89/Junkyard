#include "map.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define map_init_capacity 256
#define map_max_loadfactor 0.75f

#define MAP_TOMB ((void*)-1)

typedef struct MapBucket {
    uintptr_t hash;
    void* key;
    void* value;
} MapBucket;

typedef struct _Map {
    MapBucket* buckets;
    uintptr_t length;
    uintptr_t tombs;
    uintptr_t capacity;
    MapHash hash;
    MapProbe probe;
    MapEquals equals;
    MapKeyDel keydel;
    MapValueDel valuedel;
    MapAlloc alloc;
    void* ctx;
} _Map;

typedef struct MapKVList {
    const _Map* map;
    char kv[];
} MapKVList;

// NOLINTBEGIN
static inline bool _mapbucket_istomb(const MapBucket* b) {
    return b->key == MAP_TOMB && !b->value;
}

static inline bool _mapbucket_isfull(const MapBucket* b) {
    return b->key && b->key != MAP_TOMB;
}

static inline bool _mapbucket_isempty(const MapBucket* b) {
    return !b->key && !b->value;
}

static inline void _mapbucket_astomb(MapBucket* b) {
    b->key = MAP_TOMB;
    b->value = NULL;
}

static void* _map_defaultalloc(void* ctx, void* ptr, uintptr_t osize,
                               uintptr_t nsize) {
    (void)ctx;

    if(nsize == 0) {
        free(ptr);
        return NULL;
    }

    void* p = calloc(1, nsize);

    if(!p) {
        fprintf(stderr, "Map allocation failed\n");
        abort();
        return NULL;
    }

    if(ptr) {
        memcpy(p, ptr, osize);
        free(ptr);
    }

    return p;
}

static uintptr_t _map_defaulthash(const void* key) { return (uintptr_t)key; }

static bool _map_defaultequals(const void* key1, const void* key2) {
    return key1 == key2;
}

static uintptr_t _map_defaultprobe(const _Map* self, uintptr_t idx) {
    return (uintptr_t)((idx + 1) % self->capacity);
}

static const MapBucket* _map_findbucket(const _Map* self, const void* key) {
    assert(self && "_map_findbucket: invalid self");
    uintptr_t h = self->hash(key);
    uintptr_t idx = h % self->capacity;

    for(;;) {
        MapBucket* b = self->buckets + idx;

        // Found the key
        if(_mapbucket_isfull(b) && h == b->hash && self->equals(key, b->key))
            return b;

        // Encounter an empty bucket; key not found
        if(_mapbucket_isempty(b)) return NULL;

        // Continue probing
        idx = self->probe(self, idx);
    }

    // Should never be reached; assert to avoid undefined behavior.
    assert(false && "_map_findbucket: unreachable code reached");
    return NULL;
}

static MapBucket* _map_acquirebucket(_Map* self, void* key) {
    assert(self && "_map_acquirebucket: invalid self");

    uintptr_t h = self->hash(key);
    uintptr_t idx = h % self->capacity;
    uintptr_t tombidx = (uintptr_t)-1;

    for(;;) {
        MapBucket* b = self->buckets + idx;

        // Case 1: Found a valid item that matches the key.
        if(_mapbucket_isfull(b) && h == b->hash && self->equals(key, b->key)) {
            if(self->keydel) self->keydel(b->key);
            if(self->valuedel) self->valuedel(b->value);
            return b;
        }

        // Case 2: Handle tombstones and empty slots.
        if(_mapbucket_istomb(b) && tombidx == (uintptr_t)-1)
            tombidx = idx; // Save the first tombstone found.

        if(_mapbucket_isempty(b)) {
            // Prefer tombstone slot if one was found.
            if(tombidx != (uintptr_t)-1) {
                idx = tombidx;
                self->tombs--; // Reusing a tombstone.
            }
            else self->length++; // Inserting a new element.

            b->hash = h;
            return b;
        }

        // Continue probing.
        idx = self->probe(self, idx);
    }

    // Should never be reached; assert to catch undefined behavior.
    assert(false && "_map_acquirebucket: unreachable code reached");
    return NULL;
}

void _map_setkeydel(_Map* self, MapKeyDel keydelfn) {
    if(self) self->keydel = keydelfn;
}

void _map_setvaluedel(_Map* self, MapValueDel valuedelfn) {
    if(self) self->valuedel = valuedelfn;
}

_Map* _map_alloc_full_n(void* ctx, MapAlloc allocfn, uintptr_t cap,
                        MapHash hashfn, MapEquals equalsfn, MapProbe probefn) {
    assert(hashfn && "_map_alloc_n: invalid 'hash' function");
    assert(equalsfn && "_map_alloc_n: invalid 'equals' function");

    if(!allocfn) allocfn = _map_defaultalloc;
    if(!probefn) probefn = _map_defaultprobe;
    if(!equalsfn) equalsfn = _map_defaultequals;
    if(!hashfn) hashfn = _map_defaulthash;

    _Map* self = (_Map*)allocfn(ctx, NULL, 0, sizeof(_Map));
    *self = (_Map){0};
    self->ctx = ctx;
    self->alloc = allocfn;
    self->length = 0;
    self->capacity = 0;
    self->hash = hashfn;
    self->probe = probefn;
    self->equals = equalsfn;
    self->keydel = NULL;
    self->valuedel = NULL;

    if(cap) map_rehash(self, cap);
    return self;
}

void _map_set(_Map* self, void* key, void* value) {
    if(!self) return;

    if(!self->buckets) map_rehash(self, map_init_capacity);
    else if(map_loadfactor(self) >= map_max_loadfactor)
        map_rehash(self, self->capacity << 1);

    MapBucket* b = _map_acquirebucket(self, key);
    b->key = key;
    b->value = value;
}

void _map_del(_Map* self, const void* key) {
    if(!self) return;
    uintptr_t h = self->hash(key);
    uintptr_t idx = h % self->capacity;

    for(;;) {
        MapBucket* b = self->buckets + idx;

        // Found a matching full bucket; delete it.
        if(_mapbucket_isfull(b) && h == b->hash && self->equals(key, b->key)) {
            if(self->keydel) self->keydel(b->key);
            if(self->valuedel) self->valuedel(b->value);
            _mapbucket_astomb(b);
            self->length--;
            self->tombs++;
            return;
        }

        // If an empty bucket is reached, the key doesn't exist.
        if(_mapbucket_isempty(b)) return;

        // Continue probing.
        idx = self->probe(self, idx);
    }

    // Should never be reached; added to prevent undefined behavior.
    assert(false && "_map_del: unreachable code reached");
}

bool _map_get_ex(const _Map* self, const void* key, const void** outkey,
                 const void** outval) {
    if(!self) return false;
    const MapBucket* b = _map_findbucket(self, key);

    if(b) {
        if(outkey) *outkey = b->key;
        if(outval) *outval = b->value;
        return true;
    }

    return false;
}

const void* _map_get(const _Map* self, const void* key) {
    const void* value = NULL;
    if(_map_get_ex(self, key, NULL, &value)) return value;
    return NULL;
}

void** _map_getkeys(const _Map* self) {
    if(self && self->buckets) {
        MapKVList* kvlist = (MapKVList*)self->alloc(
            self->ctx, NULL, 0,
            sizeof(MapKVList) + (sizeof(void*) * (self->length + 1)));

        kvlist->map = self;
        void** keys = (void**)&kvlist->kv;
        uintptr_t j = 0;

        for(uintptr_t i = 0; j < self->length && i < self->capacity; i++) {
            if(_mapbucket_isfull(self->buckets + i))
                keys[j++] = self->buckets[i].key;
        }

        keys[j] = NULL;
        return keys;
    }

    return NULL;
}

void** _map_getvalues(const _Map* self) {
    if(self && self->buckets) {
        MapKVList* kvlist = (MapKVList*)self->alloc(
            self->ctx, NULL, 0,
            sizeof(MapKVList) + (sizeof(void*) * (self->length + 1)));

        kvlist->map = self;
        void** values = (void**)&kvlist->kv;
        uintptr_t j = 0;

        for(uintptr_t i = 0; j < self->length && i < self->capacity; i++) {
            if(_mapbucket_isfull(self->buckets + i))
                values[j++] = self->buckets[i].value;
        }

        values[j] = NULL;
        return values;
    }

    return NULL;
}

// NOLINTEND

uintptr_t map_hash_str(const char* s) {
    uintptr_t h = 5381;

    if(s) {
        while(*s)
            h = ((h << 5) + h) + *s++;
    }

    return h;
}

bool map_equals_str(const char* s1, const char* s2) {
    return s1 && s2 && !strcmp(s1, s2);
}

void map_clear(_Map* self) {
    if(!self || !self->buckets) return;

    if(!self->buckets) {
        assert(self->length == 0 && "map_clear: length not 0");
        assert(self->tombs == 0 && "map_clear: tombs not 0");
        return;
    }

    if(self->keydel || self->valuedel) {
        uintptr_t clean = 0;
        for(uintptr_t i = 0; i < self->capacity && clean < self->length; i++) {
            MapBucket* b = self->buckets + i;

            if(_mapbucket_isfull(b)) {
                if(self->keydel) self->keydel(b->key);
                if(self->valuedel) self->valuedel(b->value);
                clean++;
            }
        }
        assert(clean == self->length && "map_clean: bucket clean mismatch");
    }

    memset(self->buckets, 0, sizeof(MapBucket) * self->capacity);
    self->length = 0;
    self->tombs = 0;
}

void map_destroy(_Map* self) {
    if(self) {
        map_clear(self);

        if(self->buckets) {
            self->alloc(self->ctx, self->buckets,
                        (self->capacity * sizeof(MapBucket)), 0);
        }

        self->alloc(self->ctx, self, sizeof(_Map), 0);
    }
}

MapAlloc map_getallocator(const _Map* self, void** ctx) {
    if(self) {
        if(ctx) *ctx = self->ctx;
        return self->alloc;
    }

    return NULL;
}

MapHash map_gethash(const _Map* self) { return self ? self->hash : NULL; }
MapProbe map_getprobe(const _Map* self) { return self ? self->probe : NULL; }
MapEquals map_getequals(const _Map* self) { return self ? self->equals : NULL; }
MapKeyDel map_getkeydel(const _Map* self) { return self ? self->keydel : NULL; }

MapValueDel map_getvaluedel(const _Map* self) {
    return self ? self->valuedel : NULL;
}

uintptr_t map_capacity(const _Map* self) { return self ? self->capacity : 0; }
uintptr_t map_length(const _Map* self) { return self ? self->length : 0; }

void map_rehash(_Map* self, uintptr_t ncap) {
    if(!self) return;
    if(!ncap) ncap = self->capacity; // Just rehash
    else if(self->capacity >= ncap) return;

    uintptr_t oldcap = self->capacity;
    uintptr_t oldlen = self->length;
    MapBucket* oldbuckets = self->buckets;
    self->tombs = 0;
    self->length = 0;
    self->capacity = ncap;
    self->buckets = self->alloc(self->ctx, NULL, 0, sizeof(MapBucket) * ncap);

    uintptr_t moved = 0;
    for(uintptr_t i = 0; i < oldcap && moved < oldlen; i++) {
        const MapBucket* b = oldbuckets + i;
        if(_mapbucket_isfull(b)) {
            *_map_acquirebucket(self, b->key) = *b;
            ++moved;
        }
    }
    assert(moved == oldlen && "map_rehash: bucket copy mismatch");

    if(oldbuckets)
        self->alloc(self->ctx, oldbuckets, sizeof(MapBucket) * oldcap, 0);
}

void map_reserve(_Map* self, uintptr_t n) {
    if(self) map_rehash(self, ceil((double)(n / map_max_loadfactor)));
}

float map_loadfactor(const _Map* self) {
    if(self) return (self->length + self->tombs) / (float)self->capacity;
    return 0;
}

MapIterator map_iterator(const _Map* self) {
    MapIterator it = {self, 0, NULL, NULL};
    map_next(&it);
    return it;
}

bool map_hasnext(const MapIterator* self) {
    return self && self->map && self->index < self->map->capacity;
}

void map_next(MapIterator* self) {
    if(!self || !self->map || self->index >= self->map->capacity) return;

    do {
        self->index++;
    } while(self->index < self->map->capacity &&
            !_mapbucket_isfull(&self->map->buckets[self->index]));

    if(self->index < self->map->capacity) {
        self->key = self->map->buckets[self->index].key;
        self->value = self->map->buckets[self->index].value;
    }
}

void _mapkv_destroy(void** kv) {
    MapKVList* kvlist = (MapKVList*)((char*)kv - offsetof(MapKVList, kv));
    uintptr_t len =
        sizeof(MapKVList) + (sizeof(void*) + (kvlist->map->length + 1));
    kvlist->map->alloc(kvlist->map->ctx, kvlist, len, 0);
}
