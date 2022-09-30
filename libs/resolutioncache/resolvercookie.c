#include <stdio.h>
#include "resolvercookie.h"
#include "resolutioncache.h"

GoLibsass_ResolverCookie* golibsass_resolvercookie_create(
    uintptr_t callback_idx,
    uintptr_t cache_size
) {
    GoLibsass_ResolverCookie* cookie = calloc(sizeof(GoLibsass_ResolverCookie), 1);
    cookie->idx = callback_idx;

    printf("making a cookie with callback_idx=%lu, cache_size=%lu\n", callback_idx, cache_size);
    if (cache_size > 0) {
        GoLibsass_ResolverCache cache = golibsass_resolution_cache_create(cache_size);
        cookie->cache = cache;
    }

    return cookie;
}


void golibsass_resolvercookie_free(
    GoLibsass_ResolverCookie* cookie
) {
    golibsass_resolution_cache_destroy(cookie->cache);
}

uintptr_t golibsass_resolvercookie_idx(
    GoLibsass_ResolverCookie *cookie,
    uintptr_t cache_size
) {
    return cookie->idx;
}

void golibsass_resolvercookie_clearcache(
    GoLibsass_ResolverCookie* cookie
) {
    if (golibsass_resolvercookie_hascache(cookie)) {
        golibsass_resolution_cache_clear(cookie->cache);
    }
}


bool golibsass_resolvercookie_hascache(
    GoLibsass_ResolverCookie *cookie
) {
    return cookie->cache.cache_size > 0;
}
