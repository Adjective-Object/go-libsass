#include "./resolutioncache.h"
#include "sass/functions.h"
#include <limits.h>
#include <string.h>

// Cost deducted from a cache entry each time it is stepped over.
// Entries with lower usage heuristics are prioritized for overwriting
#define CACHE_USAGE_STEP_OVER_COST 1
// Credit added to a cache entry's heuristic each time it is explicitly read
#define CACHE_USAGE_READ_CREDIT 5
// arbitrary char limit on the length of an import specifier because we
// don't want to permit unbounded string comparisons
#define CACHE_MAX_SPECIFIER_LEN 8192
// In order to satisfy compiler warnings, put an arbitrary cap on
// maximum filepath len of 1MB
#define CACHE_MAX_FILEPATH_LEN 1048576
// In order to satisfy compiler warnings, put an arbitrary cap on
// maximum sass file body size of 1GB
#define CACHE_MAX_BODY_OR_SRCMAP_LEN 1073741824 

// utility to clone a Sass_Import_Entry.
//
// Does not clone the abs_path if any, since this is meant to be
// used on imports that have not yet returned to sass and had their
// abspaths cached.
Sass_Import_Entry clone_entry(Sass_Import_Entry original){
    const char *orig_imp_path = sass_import_get_imp_path(original);
    const char *orig_abs_path = sass_import_get_abs_path(original);
    const char *orig_source = sass_import_get_source(original);
    const char *orig_srcmap = sass_import_get_srcmap(original);

    char *new_imp_path = malloc(sizeof(char) * (strnlen(orig_imp_path, CACHE_MAX_SPECIFIER_LEN) + 1));
    char *new_abs_path = malloc(sizeof(char) * (strnlen(orig_abs_path, CACHE_MAX_FILEPATH_LEN) + 1));
    char *new_source = malloc(sizeof(char) * (strnlen(orig_source, CACHE_MAX_BODY_OR_SRCMAP_LEN) + 1));
    char *new_srcmap = malloc(sizeof(char) * (strnlen(orig_srcmap, CACHE_MAX_BODY_OR_SRCMAP_LEN) + 1));

    strcpy(new_imp_path, orig_imp_path);
    strcpy(new_source, orig_source);
    strcpy(new_srcmap, orig_srcmap);

    return sass_make_import(
        new_imp_path,
        new_abs_path,
        new_source,
        new_srcmap
    );
}

// not-cryptographically secure hash into the resolution hashmap
int hash_string(const char* hashString) {
    int hash = 0;
    for (int i=0; i<hashString[i] != '\0'; i++) {
        hash = (hash << 1) ^ hashString[i];
    }
}

// Gets the initial index into the cache that should be checked for the
// given key. Note that this may not be the position of the entry, as
// multiple entries may conflict.
// This should be used as the starting point when stepping through
// the hashmap's internal buffer to find the appropriate entry
int golibsass_resolution_cache_raw_index(
    GoLibsass_ResolverCache cache,
    const char* key
) {
    return hash_string(key) % (cache.cacheSize);
}

// Creates a new instance of the resolution cache
GoLibsass_ResolverCache golibsass_resolution_cache_create(size_t cacheSize) {
    GoLibsass_ResolverCache cache = {
        cacheSize: cacheSize,
        entries:  calloc(cacheSize, sizeof(GoLibsass_ResolverCacheEntryInternal)),
    };

    return cache;
}

// Adds a new resolved import to the resolution cache.
// Clones the passed-in entry and stores a copy in the map.
void golibsass_resolution_cache_insert(
    // The cache to insert into.
    //
    // Note that the cache object itself is immutable, and the actual
    // mutable contents are indirected through the Entries pointer, so
    // passing the struct rather than a pointer to the struct saves
    // a layer of indirection
    GoLibsass_ResolverCache cache,
    // The entry to insert.
    //
    // The cache will take ownership of the buffers in the cache entry, which
    // will be evicted when the cache is cleared or destroyed with
    // golibsass_resolution_cache_clear or golibsass_resolution_cache_destroy
    Sass_Import_Entry entry
) {
    if (entry == NULL) {
        return;
    }
    const char* entryImporterPath = sass_import_get_imp_path(entry);
    if (entryImporterPath == NULL) {
        // entryData.imp_path = NULL is a special value used to
        // represent an empty entry
        //
        // If the consumer tries to insert a NULL import, this could lead to leaks.
        // protect against that.
        return;
    }

    int startingIndex = golibsass_resolution_cache_raw_index(cache, entryImporterPath);
    GoLibsass_ResolverCacheEntryInternal* leastUsed = (cache.entries) + startingIndex;

    for (int i=0; i<cache.cacheSize; i++) {
        int realIndex = (startingIndex + i) % cache.cacheSize;
        GoLibsass_ResolverCacheEntryInternal* cur = cache.entries + realIndex;
        const char* curImporterPath = sass_import_get_imp_path(cur->entryData);

        if (curImporterPath == NULL) {
            // found an empty entry, overwrite it
            (*cur).entryData = clone_entry(entry);
            (*cur).usage = 0;
            return;
        }

        if (cur->usage < leastUsed->usage) {
            leastUsed = cur;
        }
    }

    // found no empty entries, overwrite the least-used entry
    // according to the usage heuristic
    sass_delete_import(leastUsed->entryData);
    leastUsed->entryData = clone_entry(entry);
    leastUsed->usage = 0;
}


Sass_Import_Entry golibsass_resolution_cache_get(
    GoLibsass_ResolverCache cache,
    const char* importSpecifier
) {
    int startingIndex = golibsass_resolution_cache_raw_index(cache, importSpecifier);

    for (int i=0; i<cache.cacheSize; i++) {
        int realIndex = (startingIndex + i) % cache.cacheSize;
        GoLibsass_ResolverCacheEntryInternal* cur = cache.entries + realIndex;

        const char* curImporterPath = sass_import_get_imp_path(cur->entryData);

        if (curImporterPath == NULL) {
            // Found an empty entry - that means there is no entry available
            return NULL;
        } else if (strncmp(importSpecifier, curImporterPath, CACHE_MAX_SPECIFIER_LEN) == 0) {
            // Found the entry, return it
            cur->usage += CACHE_USAGE_READ_CREDIT;
            // return the clone since we want the cache to maintain
            // ownership of the cache ddata.
            return clone_entry(cur->entryData);
        } else {
            // we are stepping over an unrelated entry that had a hash collision.
            // Deduct a step-over cost from it to make it more likely to be
            // overwritten.
            cur->usage -= CACHE_USAGE_STEP_OVER_COST;
        }
    }

    // no matches
    return NULL;
}

void golibsass_resolution_cache_destory(
    // The cache to destroy
    //
    // Note that the cache object itself is immutable, and the actual
    // mutable contents are indirected through the Entries pointer, so
    // passing the struct rather than a pointer to the struct saves
    // a layer of indirection
    GoLibsass_ResolverCache cache
) {
    golibsass_resolution_cache_clear(cache);
    free(cache.entries);
}

void golibsass_resolution_cache_clear(
    // The cache to clear.
    //
    // Note that the cache object itself is immutable, and the actual
    // mutable contents are indirected through the Entries pointer, so
    // passing the struct rather than a pointer to the struct saves
    // a layer of indirection
    GoLibsass_ResolverCache cache
) {
    for (int i=0;i<cache.cacheSize;i++) {
        GoLibsass_ResolverCacheEntryInternal entry = cache.entries[i];
        sass_delete_import(entry.entryData);
    }

    // zero the cache entries 
    memset(cache.entries, sizeof(GoLibsass_ResolverCacheEntryInternal) * cache.cacheSize, 0);
}
