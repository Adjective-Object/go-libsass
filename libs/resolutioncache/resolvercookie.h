#ifndef GO_LIBSASS_COOKIE_H
#define GO_LIBSASS_COOKIE_H
#include <stdint.h>
#include <stdlib.h>
#include "resolutioncache.h"

// Cookie associated with the sass import resolver
typedef struct {
    // index into the go-side global function map used to
    // manage go-callbacks for resolution
    uintptr_t idx;
    // associated cache instance for this resolver
    GoLibsass_ResolverCache cache;
} GoLibsass_ResolverCookie;

// creates a new cookie
GoLibsass_ResolverCookie* golibsass_resolvercookie_create(
    uintptr_t callback_idx,
    uintptr_t cache_size
);

// releases a cookie and the associated resolver cache, if any
void golibsass_resolvercookie_free(GoLibsass_ResolverCookie* cookie);

// Gets the index off of a cookie
uintptr_t golibsass_resolvercookie_idx(
    GoLibsass_ResolverCookie *cookie,
    uintptr_t cache_size
);

// Checks if a cookie has an initialized cache
bool golibsass_resolvercookie_hascache(
    GoLibsass_ResolverCookie *cookie
);

#endif