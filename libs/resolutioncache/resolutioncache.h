#ifndef GO_LIBSASS_RESOLUTION_CACHE_H
#define GO_LIBSASS_RESOLUTION_CACHE_H
#include <stdint.h>
#include <stdlib.h>
#include "sass/functions.h"

/*
* In bulk compilation with custom resolvers, a lot of the cost
* of compilation is repeatedly crossing the barrier between
* C-land and go-land, since they have entirely different runtimes
*
* Here, we attempt to circumvent that issue by instead linking a separate thing
*
* The user should only specify that a value is cachable if the URL is meant
* to be consistently resolveable, regardless of the importer.
*/



// Entry in the resolver cache. Stores the actual cached data, as well as
// heuristics used to select members to overwrite when inserting into a full
// cache
typedef struct {
    // usage heuristic, used for LRU-ish eviction from the resolution cache
    int usage;
    // wrapped data
    Sass_Import_Entry entryData;
} GoLibsass_ResolverCacheEntryInternal;

// Cache implementing a simple hashmap of resolved imports.
//
// Entries are stored in a fixed array.
// importSpecifiers are hashed to get an integer index into the cache,
// then the getter scans forward until it finds a matching entry
//
// The cache has no individual delete function, only insert and clear.
// The expected usage is for the consumer to clear the cache at the end of
// each of their project's builds.
typedef struct {
    // Length of the entries array
    int cacheSize;
    // fixed-length list of cache entries
    GoLibsass_ResolverCacheEntryInternal *entries;
} GoLibsass_ResolverCache;

// Creates a new cache with a max cache capacity
GoLibsass_ResolverCache golibsass_resolution_cache_create(size_t cacheSize);

// Inserts an entry into the resolution cache
void golibsass_resolution_cache_insert(
    GoLibsass_ResolverCache cache,
    Sass_Import_Entry entry
);

// Gets an entry from the cache, or NULL if no entry is found
//
// When an entry is found, a clone of the entry is returned that
// the callee is responsible for deleting
Sass_Import_Entry golibsass_resolution_cache_get(
    GoLibsass_ResolverCache cache,
    const char* importSpecifier
);

// free all entires in the resolution cache
void golibsass_resolution_cache_clear(GoLibsass_ResolverCache cache);

// free all entires in the resolution cache, then free the cache's internal buffer itself
void golibsass_resolution_cache_destroy(GoLibsass_ResolverCache cache);

// Cookie associated with the sass import resolver
typedef struct {
    // index into the global function map used to manage callbacks in goland
    uintptr_t idx;
    // associated cache instance for this resolver
    GoLibsass_ResolverCache cache;
} GoLibsass_InternalCookie;

#endif