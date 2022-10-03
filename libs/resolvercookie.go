package libs

// // Opt into windows > XP so we have access to the
// // SRWLOCK type from the windows.h stdlib
// #cgo windows CFLAGS: -D_WIN32_WINNT=0x0600
//
// #include "./resolutioncache/rwmutex.c"
// #include "./resolutioncache/resolutioncache.c"
// #include "./resolutioncache/resolvercookie.c"
import "C"
import "runtime"

// Persistent cookie object associated with an
// Imports object on the Go side.
type ResolverCookie struct {
	// handle to the resolver cookie on the go side
	cCookiePtr *C.GoLibsass_ResolverCookie
}

// Creates the persistent ResolverCookie, a wrapper for
// a corresponding cookie object passed to sass.
func CreateImportsResolverCookie(
	resolverOptions ResolverOptions,
	resolver AdvancedImportResolver,
) *ResolverCookie {
	idx := globalImports.Set(resolver)

	newCookie := &ResolverCookie{
		cCookiePtr: C.golibsass_resolvercookie_create(
			C.uintptr_t(idx),
			C.uintptr_t(resolverOptions.CacheSize),
		),
	}

	// automatically release the cookie from sass when the
	// resolver itself is destroyed
	runtime.SetFinalizer(
		newCookie,
		func(cookie *ResolverCookie) {
			C.golibsass_resolvercookie_free(
				cookie.cCookiePtr,
			)
		},
	)

	return newCookie
}

// Clears the associated C-side cache, if there is any
// meant to be called between compilations
func (resolverCookie *ResolverCookie) ClearCache() {
	C.golibsass_resolvercookie_clearcache(
		resolverCookie.cCookiePtr,
	)
}
