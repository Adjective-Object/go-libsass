package libs

// #include "./resolutioncache/resolutioncache.c"
// #include "./resolutioncache/importerhandler.c"
import "C"
import "unsafe"

var globalImports SafeMap

// ImportResolver can be used as a custom import resolver. Return an empty body to
// signal loading the import body from the URL.
type ImportResolver func(url string, prev string) (newURL string, body string, resolved bool)

// Structured version of ImportResolver that can be used for C-side caching, etc
type AdvancedImportResolver func(url string, prev string) AdvancedImportResolverResult
type AdvancedImportResolverResult struct {
	NewUrl      string
	Body        string
	Resolved    bool
	ShouldCache bool
}

type ResolverMode int

const (
	ResolverModeImporterUrl ResolverMode = iota
	ResolverModeImporterAbsPath
)

func init() {
	globalImports.init()
}

// BindImporter attaches a custom importer Go function to an import in Sass
func BindImporter(opts SassOptions, resolverMode ResolverMode, resolver ImportResolver) int {

	idx := globalImports.Set(resolver)
	ptr := unsafe.Pointer(uintptr(idx))

	var handler unsafe.Pointer
	if resolverMode == ResolverModeImporterAbsPath {
		handler = C.SassImporterAbsHandler
	} else {
		handler = C.SassImporterPathsHandler
	}

	imper := C.sass_make_importer(
		C.Sass_Importer_Fn(handler),
		C.double(0),
		ptr,
	)
	impers := C.sass_make_importer_list(1)
	C.sass_importer_set_list_entry(impers, 0, imper)

	C.sass_option_set_c_importers(
		(*C.struct_Sass_Options)(unsafe.Pointer(opts)),
		impers,
	)
	return idx
}

func RemoveImporter(idx int) error {
	globalImports.Del(idx)
	return nil
}
