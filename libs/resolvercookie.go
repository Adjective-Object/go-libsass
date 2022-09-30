package libs

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
