package libs

// #include "sass/context.h"
// #include "./resolutioncache/importerhandler.h"
import "C"

// C changes will not be picked up unless you change the above cgo snipp
import "unsafe"

var globalImports SafeMap

// ImportResolver can be used as a custom import resolver. Return an empty body to
// signal loading the import body from the URL.
type ImportResolver func(url string, prev string) (newURL string, body string, resolved bool)

// Structured version of ImportResolver that can be used for C-side caching, etc
type AdvancedImportResolver func(url string, prev string) ResolverResult
type ResolverResult struct {
	NewUrl      string
	Source      string
	Resolved    bool
	ShouldCache bool
}

type ResolverOptions struct {
	// Resolver mode to use, either
	// libs.ResolverModeImporterUrl or
	// libs.ResolverModeImporterAbsPath
	ResolverMode ResolverMode
	// Size of the C-side resolution cache to use.
	// If cacheSize is zero, no cache will be constructed.
	CacheSize int
}

type ResolverMode int

const (
	ResolverModeImporterUrl ResolverMode = iota
	ResolverModeImporterAbsPath
)

func init() {
	globalImports.init()
}

// BindImporter the resolver callback function + cache stored in the
// ResolverCookie to an importer in the sass options instance
func BindImporter(
	opts SassOptions,
	resolverCookie *ResolverCookie,
	resolverOptions ResolverOptions,
) {
	var handler unsafe.Pointer
	if resolverOptions.ResolverMode == ResolverModeImporterAbsPath {
		handler = C.SassImporterAbsHandler
	} else {
		handler = C.SassImporterPathsHandler
	}

	imper := C.sass_make_importer(
		C.Sass_Importer_Fn(handler),
		C.double(0),
		unsafe.Pointer(
			resolverCookie.cCookiePtr,
		),
	)

	impers := C.sass_make_importer_list(1)
	C.sass_importer_set_list_entry(impers, 0, imper)

	C.sass_option_set_c_importers(
		(*C.struct_Sass_Options)(unsafe.Pointer(opts)),
		impers,
	)
}

func RemoveImporter(idx int) error {
	globalImports.Del(idx)
	return nil
}
