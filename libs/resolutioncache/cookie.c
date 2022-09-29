#include <stdio.h>
#include "cookie.h"

GoLibsass_ResolverCookie* golibsass_cookie_create(
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

uintptr_t golibsass_cookie_idx(
    GoLibsass_ResolverCookie *cookie,
    uintptr_t cache_size
) {
    return cookie->idx;
}

bool golibsass_cookie_hascache(
    GoLibsass_ResolverCookie *cookie
) {
    return cookie->cache.cache_size > 0;
}
